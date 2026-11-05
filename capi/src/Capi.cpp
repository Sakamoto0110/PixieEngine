#include "pixie/runtime/ParticleStep.hpp"
// The C API over a ProjectSession. Every exported function converts its arguments, runs inside
// Guard (so no exception crosses into the caller), and reports failures through a result code and
// the thread's last error.

#include "pixie_capi.h"
#include "GameModules.hpp"
#include "Session.hpp"
#include "SdkIdentity.hpp"

#include "pixie/render/gl/GlLoader.hpp"
#include "pixie/platform/HostSurface.hpp"
#include "pixie/runtime/Name.hpp"
#include "pixie/runtime/Path.hpp"
#include "pixie/runtime/PathDrawing.hpp"
#include "pixie/runtime/Picking.hpp"
#include "pixie/runtime/Scene.hpp"
#include "pixie/runtime/Shape.hpp"
#include "pixie/runtime/Sprite.hpp"
#include "pixie/runtime/SpriteDrawing.hpp"
#include "pixie/runtime/Text.hpp"
#include "pixie/runtime/TextDrawing.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/ProjectSession.hpp"
#include "pixie/runtime/SceneMovement2D.hpp"
#include "pixie/runtime/RigidBody2D.hpp"
#include "pixie/runtime/Lighting2D.hpp"
#include "pixie/runtime/ColliderEditing2D.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <exception>
#include <filesystem>
#include <format>
#include <memory>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

static_assert(sizeof(PixieFieldInfo) == 72, "PixieFieldInfo's layout is part of the C API");
static_assert(sizeof(PixieSceneEditReceipt) == 24, "PixieSceneEditReceipt's layout is part of the C API");
static_assert(sizeof(PixieSceneEditToken) == 24, "PixieSceneEditToken's layout is part of the C API");
static_assert(sizeof(PixieBodyMass2D) == 40, "PixieBodyMass2D's layout is part of the C API");
static_assert(sizeof(PixieColliderHandles2D) == 72, "PixieColliderHandles2D's layout is part of the C API");
static_assert(static_cast<int>(pixie::FieldEditor::Arc) == PIXIE_EDITOR_ARC);
static_assert(static_cast<int>(pixie::FieldEditor::LayerMask) == PIXIE_EDITOR_LAYER_MASK);
static_assert(static_cast<int>(pixie::FieldEditor::LayerNames) == PIXIE_EDITOR_LAYER_NAMES);
static_assert(static_cast<int>(pixie::FieldKind::Entity) == PIXIE_FIELD_ENTITY);
static_assert(static_cast<int>(pixie::FieldKind::Text) == PIXIE_FIELD_TEXT);
static_assert(static_cast<int>(pixie::FieldKind::PointList) == PIXIE_FIELD_POINT_LIST);
static_assert(static_cast<int>(pixie::FieldKind::RecordList) == PIXIE_FIELD_RECORD_LIST);
static_assert(static_cast<int>(pixie::SessionMode::Edit) == PIXIE_MODE_EDIT);
static_assert(static_cast<int>(pixie::EnvironmentSelectionKind2D::LegacyWorld) == PIXIE_ENVIRONMENT_LEGACY_WORLD);
static_assert(static_cast<int>(pixie::EnvironmentSelectionKind2D::Scene) == PIXIE_ENVIRONMENT_SCENE);
static_assert(static_cast<int>(pixie::EnvironmentResolutionKind2D::LegacyWorld) == PIXIE_ENVIRONMENT_RESOLVED_WORLD);
static_assert(static_cast<int>(pixie::EnvironmentResolutionKind2D::SceneInheritedWorld) == PIXIE_ENVIRONMENT_SCENE_INHERITS_WORLD);
static_assert(static_cast<int>(pixie::EnvironmentResolutionKind2D::RemovedSceneFallback) == PIXIE_ENVIRONMENT_REMOVED_SCENE_FALLBACK);
static_assert(static_cast<int>(pixie::LightingState2D::Absent) == PIXIE_ENVIRONMENT_LIGHTING_ABSENT);
static_assert(static_cast<int>(pixie::LightingState2D::Disabled) == PIXIE_ENVIRONMENT_LIGHTING_DISABLED);
static_assert(static_cast<int>(pixie::LightingState2D::Configured) == PIXIE_ENVIRONMENT_LIGHTING_CONFIGURED);

namespace {

constexpr std::size_t kMaxProjectSnapshotBytes = 128u * 1024u * 1024u;

#ifdef PIXIE_DEVELOPMENT
std::shared_ptr<PixieGameModule> loadedModule;
#endif

thread_local std::string lastError;

PixieResult Fail(PixieResult result, std::string message) {
    lastError = std::move(message);
    return result;
}

// Runs `body`, turning an escaping exception into PIXIE_ERROR_INTERNAL.
template<typename Body>
PixieResult Guard(Body&& body) {
    try {
        return body();
    } catch (const std::exception& e) {
        return Fail(PIXIE_ERROR_INTERNAL, std::format("exception: {}", e.what()));
    } catch (...) {
        return Fail(PIXIE_ERROR_INTERNAL, "unknown exception");
    }
}

PixieResult Copy(std::string_view text, char* buffer, int32_t capacity) {
    if (buffer == nullptr || capacity <= 0) {
        return Fail(PIXIE_ERROR_ARGUMENT, "null or empty buffer");
    }
    const auto room = static_cast<std::size_t>(capacity - 1);
    const std::size_t length = std::min(text.size(), room);
    std::memcpy(buffer, text.data(), length);
    buffer[length] = '\0';
    return text.size() > room ? Fail(PIXIE_ERROR_BUFFER, "buffer too small; string truncated")
                              : PIXIE_OK;
}

entt::entity ToEntity(uint32_t handle) {
    return static_cast<entt::entity>(handle);
}

const pixie::ComponentDescriptor* TypeAt(PixieSession* s, int32_t type) {
    const auto& types = s->session.Types().All();
    return type >= 0 && type < static_cast<int32_t>(types.size()) ? &types[static_cast<std::size_t>(type)]
                                                                  : nullptr;
}

const pixie::FieldDescriptor* FieldAt(PixieSession* s, int32_t type, int32_t field) {
    const pixie::ComponentDescriptor* descriptor = TypeAt(s, type);
    if (descriptor == nullptr || field < 0 || field >= static_cast<int32_t>(descriptor->fields.size())) {
        return nullptr;
    }
    return &descriptor->fields[static_cast<std::size_t>(field)];
}

// An enum field takes only the values its names cover; anything else would land in the member's
// underlying byte as a value no name describes.
PixieResult CheckEnumValue(const pixie::FieldDescriptor& field, double value) {
    const auto count = static_cast<double>(field.enumNames.size());
    if (field.kind != pixie::FieldKind::Enum || field.enumNames.empty() ||
        (value >= 0.0 && value < count && value == std::floor(value))) {
        return PIXIE_OK;
    }
    return Fail(PIXIE_ERROR_ARGUMENT, std::format("{} takes a whole number from 0 to {}, not {}", field.name,
                                                  field.enumNames.size() - 1, value));
}

// The component instance of `type` on `entity`, or null after recording why.
void* ComponentOf(PixieSession* s, uint32_t entity, int32_t type, PixieResult& result) {
    const pixie::ComponentDescriptor* descriptor = TypeAt(s, type);
    if (descriptor == nullptr) {
        result = Fail(PIXIE_ERROR_ARGUMENT, std::format("no component type {}", type));
        return nullptr;
    }
    void* component = descriptor->find(s->session.GetWorld().Registry(), ToEntity(entity));
    if (component == nullptr) {
        result = Fail(PIXIE_ERROR_NOT_FOUND,
                      std::format("entity {} is gone or has no {}", entity, descriptor->name));
    }
    return component;
}

std::filesystem::path FromUtf8(const char* text) {
    return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(text)));
}

// Whether the entity belongs to the scene, so changing it is an edit to save.
bool InScene(PixieSession* s, uint32_t entity) {
    const entt::registry& registry = s->session.GetWorld().Registry();
    return registry.valid(ToEntity(entity)) && registry.all_of<pixie::SceneId>(ToEntity(entity));
}

std::string ComponentAdditionError(PixieSession* s, uint32_t entity, int32_t type,
                                   const pixie::ComponentDescriptor& descriptor) {
    if (s->session.Mode() == pixie::SessionMode::Edit && InScene(s, entity)
        && pixie::SceneFormatVersion(s->session.GetWorld()) == 2 && !s->session.Types().AllowsV2Import(type))
        return std::format("version 2 authored components require a declarative contract: {}", descriptor.name);
    if (s->session.GetWorld().Presentation() && descriptor.name == "LightingSettings2D")
        return "canonical presentation owns lighting; legacy component addition is refused";
    return descriptor.AdditionError(s->session.GetWorld(), ToEntity(entity));
}

// Where the last render drew the entity: between the previous step and this one.
pixie::Transform2D DrawnTransform(PixieSession* s, entt::entity id, const pixie::Transform2D& transform) {
    const auto* previous = s->session.GetWorld().Registry().try_get<pixie::PreviousTransform2D>(id);
    return previous != nullptr ? pixie::Interpolate(previous->value, transform, s->session.Alpha()) : transform;
}

// Creates a session for the game, with the project in `folder` open when it is not null.
PixieResult Open(int32_t width, int32_t height, uint64_t seed, const char* folder, PixieSession** session) {
    if (session == nullptr || width <= 0 || height <= 0) {
        return Fail(PIXIE_ERROR_ARGUMENT, "null output or empty view");
    }
    *session = nullptr;
    pixie::HostSurface surface({width, height});
    surface.MakeCurrent();
    if (!pixie::LoadGl()) {
        return Fail(PIXIE_ERROR_GL, "OpenGL 4.5 core functions could not be loaded for the current context");
    }
    pixie::SessionSettings settings;
    settings.seed = seed;
#ifdef PIXIE_DEVELOPMENT
    const auto moduleLease = loadedModule;
#endif
    std::unique_ptr<pixie::Project> project;
    pixie::ProjectSession::ProjectFactory candidateFactory;
    bool candidateAuthoring = false;
#ifdef PIXIE_CAPI_FACTORY
    project = pixie::CreateProject();
#endif
#ifdef PIXIE_DEVELOPMENT
    if (moduleLease) {
        project.reset(moduleLease->create());
        if (!project) return Fail(PIXIE_ERROR_INTERNAL, "game module factory returned no project");
        if (moduleLease->candidateContract == 1) {
            candidateFactory = [moduleLease] { return std::unique_ptr<pixie::Project>(moduleLease->create()); };
            candidateAuthoring = true;
        }
    }
#endif
    if (!project) {
        project = std::make_unique<pixie::SceneProject>();
#ifndef PIXIE_CAPI_FACTORY
        candidateAuthoring = true;
#endif
    }
    auto created = std::make_unique<PixieSession>(std::move(project), settings, candidateAuthoring, std::move(candidateFactory));
#ifdef PIXIE_DEVELOPMENT
    created->module = moduleLease;
#endif
    if (folder != nullptr && !created->session.OpenProject(FromUtf8(folder))) {
        return Fail(PIXIE_ERROR_IO, created->session.Error());
    }
    created->surface.Resize(surface.GetFramebufferSize());
    if (!created->session.Start(created->surface.GetFramebufferSize())) {
        created->session.Stop();
        return Fail(created->session.Error().empty() ? PIXIE_ERROR_GL : PIXIE_ERROR_IO,
                    created->session.Error().empty() ? "the renderers could not be created" : created->session.Error());
    }
    *session = created.release();
    return PIXIE_OK;
}

// A point in view pixels, in world units.
pxPoint ToWorld(PixieSession* s, double x, double y) {
    return s->session.ResolvedCamera().camera.ScreenToWorld(pxPoint(x, y));
}

// A length in view pixels, in world units.
double ToWorldLength(PixieSession* s, double pixels) {
    return pixels / s->session.ResolvedCamera().camera.Zoom();
}

#define PIXIE_REQUIRE_SESSION(s)                                                                   \
    if ((s) == nullptr) {                                                                          \
        return Fail(PIXIE_ERROR_ARGUMENT, "null session");                                         \
    }

struct RecordAccess {
    void* owner = nullptr;
    void* item = nullptr;
    const pixie::FieldDescriptor* list = nullptr;
    const pixie::FieldDescriptor* field = nullptr;
    int32_t itemType = -1;
};
PixieResult RecordOf(PixieSession* s, uint32_t entity, int32_t type, int32_t listIndex,
                    int32_t index, int32_t child, RecordAccess& out) {
    out.list = FieldAt(s, type, listIndex);
    if (!out.list || out.list->kind != pixie::FieldKind::RecordList)
        return Fail(PIXIE_ERROR_ARGUMENT, "field is not a record list");
    PixieResult result = PIXIE_OK;
    out.owner = ComponentOf(s, entity, type, result);
    if (!out.owner) return result;
    const auto& types = s->session.Types().All();
    for (std::size_t i = 0; i < types.size(); ++i)
        if (types[i].type == out.list->recordType) out.itemType = static_cast<int32_t>(i);
    if (out.itemType < 0) return Fail(PIXIE_ERROR_ARGUMENT, "record item type is not registered");
    if (index < 0) return PIXIE_OK; // list-only access
    if (static_cast<std::size_t>(index) >= out.list->recordCount(out.owner))
        return Fail(PIXIE_ERROR_ARGUMENT, "record index is out of range");
    out.item = out.list->recordAt(out.owner, static_cast<std::size_t>(index));
    out.field = FieldAt(s, out.itemType, child);
    return out.field ? PIXIE_OK : Fail(PIXIE_ERROR_ARGUMENT, "record field is out of range");
}
void RecordChanged(PixieSession* s, uint32_t entity) {
    if (InScene(s, entity)) s->session.MarkChanged();
}

} // namespace

#ifdef PIXIE_DEV_CHANNEL
namespace pixie::dev {
PixieResult SetNativeError(PixieResult result, const std::string& message) { return Fail(result, message); }
}
#endif

extern "C" {

int32_t pixie_api_version(void) {
    return PIXIE_CAPI_VERSION;
}

int32_t pixie_sdk_identity(char* buffer, int32_t capacity) {
    const auto identity = pixie::sdk::RuntimeIdentity();
    if (capacity < 0 || (!buffer && capacity > 0) || identity.size() > PIXIE_SDK_IDENTITY_MAX_LENGTH) {
        Fail(PIXIE_ERROR_ARGUMENT, "invalid SDK identity buffer/capacity or oversized runtime identity");
        return -1;
    }
    if (buffer && capacity > 0) {
        const auto length = std::min(identity.size(), static_cast<std::size_t>(capacity - 1));
        std::memcpy(buffer, identity.data(), length);
        buffer[length] = '\0';
    }
    return static_cast<int32_t>(identity.size());
}

uint64_t pixie_capabilities(void) {
    return pixie::sdk::Capabilities();
}

#ifdef PIXIE_DEVELOPMENT
PixieResult pixie_module_load(const char* path) {
    return Guard([&]() -> PixieResult {
        if (!path) { loadedModule.reset(); return PIXIE_OK; }
        auto module = std::make_shared<PixieGameModule>();
#ifdef _WIN32
        const auto absolute = std::filesystem::absolute(FromUtf8(path));
        module->handle = LoadLibraryExW(absolute.c_str(), nullptr,
            LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
#else
        module->handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
#endif
        if (!module->handle) return Fail(PIXIE_ERROR_IO, "game module could not be loaded");
        const auto version = reinterpret_cast<int (*)()>(module->Symbol("pixie_game_api_version"));
        module->create = reinterpret_cast<pixie::Project* (*)()>(module->Symbol("pixie_game_create"));
        if (!version || version() != PIXIE_CAPI_VERSION || !module->create)
            return Fail(PIXIE_ERROR_ARGUMENT, "game module ABI version or factory is incompatible; rebuild the project with this engine before opening it");
        // OS loading may already execute image initializers. No Project callback is
        // invoked before this independent C-only SDK/C++ tuple admission succeeds.
        const auto identity = reinterpret_cast<const char* (*)()>(module->Symbol("pixie_game_sdk_identity"));
        if (!identity) return Fail(PIXIE_ERROR_ARGUMENT, "game module has no SDK identity; rebuild with the selected SDK");
        std::string identityDiagnostic;
        if (!pixie::sdk::Compatible(identity(), identityDiagnostic))
            return Fail(PIXIE_ERROR_ARGUMENT, std::move(identityDiagnostic));
        const auto contract = reinterpret_cast<int (*)()>(module->Symbol("pixie_game_candidate_contract"));
        module->candidateContract = contract ? contract() : 0;
        loadedModule = std::move(module);
        return PIXIE_OK;
    });
}
#endif

int32_t pixie_last_error(char* buffer, int32_t capacity) {
    if (buffer != nullptr && capacity > 0) {
        const auto length = std::min(lastError.size(), static_cast<std::size_t>(capacity - 1));
        std::memcpy(buffer, lastError.data(), length);
        buffer[length] = '\0';
    }
    return static_cast<int32_t>(lastError.size());
}

#ifdef PIXIE_DEV_CHANNEL
PixieResult pixie_dev_dispatch(PixieSession* s, const char* request, char* buffer, int32_t capacity) {
    PIXIE_REQUIRE_SESSION(s);
    if (!request || !buffer) return Fail(PIXIE_ERROR_ARGUMENT, "null development request or response");
    if (capacity < pixie::dev::kResponseCapacity)
        return Fail(PIXIE_ERROR_BUFFER, "development response needs 4 MiB before command execution");
    if (s->ownerThread != std::this_thread::get_id())
        return Fail(PIXIE_ERROR_ARGUMENT, "development dispatch requires the session owner thread");
    std::size_t length = 0;
    while (length <= pixie::dev::kMaxRequestBytes && request[length] != '\0') ++length;
    if (length > pixie::dev::kMaxRequestBytes) return Fail(PIXIE_ERROR_ARGUMENT, "development request exceeds 64 KiB");
    return Guard([&] {
        if (!s->dispatcher) s->dispatcher = std::make_unique<pixie::dev::WorldDispatcher>(s->session);
        return Copy(s->dispatcher->Dispatch(std::string_view(request, length)), buffer, capacity);
    });
}
#endif

PixieResult pixie_physics_debug_get(PixieSession* s, uint32_t* flags) {
    PIXIE_REQUIRE_SESSION(s);
    if (!flags) return Fail(PIXIE_ERROR_ARGUMENT, "null physics debug output");
    *flags = s->session.PhysicsDebug();
    return PIXIE_OK;
}
PixieResult pixie_physics_debug_set(PixieSession* s, uint32_t flags) {
    PIXIE_REQUIRE_SESSION(s);
    if ((flags & ~PIXIE_PHYSICS_DEBUG_ALL) != 0) return Fail(PIXIE_ERROR_ARGUMENT, "unknown physics debug bits");
    s->session.SetPhysicsDebug(flags);
    return PIXIE_OK;
}
PixieResult pixie_lighting_debug_get(PixieSession* s, uint32_t* flags) {
    PIXIE_REQUIRE_SESSION(s);
    if (!flags) return Fail(PIXIE_ERROR_ARGUMENT, "null lighting debug output");
    *flags = s->session.LightingDebug();
    return PIXIE_OK;
}
PixieResult pixie_lighting_debug_set(PixieSession* s, uint32_t flags) {
    PIXIE_REQUIRE_SESSION(s);
    if ((flags & ~PIXIE_LIGHTING_DEBUG_ALL) != 0) return Fail(PIXIE_ERROR_ARGUMENT, "unknown lighting debug bits");
    s->session.SetLightingDebug(flags);
    return PIXIE_OK;
}

PixieResult pixie_movement_status(PixieSession* s, char* buffer, int32_t capacity, int32_t* required) {
    PIXIE_REQUIRE_SESSION(s);
    if (required == nullptr || capacity < 0 || (buffer == nullptr && capacity > 0))
        return Fail(PIXIE_ERROR_ARGUMENT, "invalid movement status output");
    return Guard([&] {
        auto errors = pixie::SceneMovementErrors2D(s->session.GetWorld());
        const auto particleErrors = pixie::ParticlePhysicsErrors2D(s->session.GetWorld());
        errors.insert(errors.end(), particleErrors.begin(), particleErrors.end());
        std::string summary;
        const std::size_t shown = std::min(errors.size(), std::size_t{8});
        for (std::size_t i = 0; i < shown; ++i) summary += errors[i] + "\n";
        if (shown < errors.size()) summary += std::format("{} more movement errors\n", errors.size() - shown);
        *required = static_cast<int32_t>(summary.size() + 1);
        return capacity == 0 ? PIXIE_OK : Copy(summary, buffer, capacity);
    });
}

PixieResult pixie_collider_handles_2d(PixieSession* s, uint32_t entity, int32_t shape, PixieColliderHandles2D* output) {
    PIXIE_REQUIRE_SESSION(s);
    if (!output || shape < -1 || shape > 255) return Fail(PIXIE_ERROR_ARGUMENT, "invalid collider handle query");
    return Guard([&] {
        if (s->session.Mode() != pixie::SessionMode::Edit) return Fail(PIXIE_ERROR_ARGUMENT, "collider tools require Edit mode");
        const auto h = pixie::DescribeColliderHandles2D(s->session.GetWorld(), ToEntity(entity), shape);
        if (!h) return Fail(PIXIE_ERROR_NOT_FOUND, "collider geometry has no supported handles");
        *output = {h->center.x, h->center.y, h->resize.x, h->resize.y, h->rotate.x, h->rotate.y,
            h->height.x, h->height.y, h->canRotate ? 1u : 0u, h->canHeight ? 1u : 0u};
        return PIXIE_OK;
    });
}
PixieResult pixie_collider_drag_2d(PixieSession* s, uint32_t entity, int32_t shape, int32_t kind,
                                 double x, double y, int32_t snap) {
    PIXIE_REQUIRE_SESSION(s);
    if (shape < -1 || shape > 255 || kind < 0 || kind > 2 || snap < 0 || snap > 1)
        return Fail(PIXIE_ERROR_ARGUMENT, "invalid collider drag arguments");
    return Guard([&] {
        if (s->session.Mode() != pixie::SessionMode::Edit) return Fail(PIXIE_ERROR_ARGUMENT, "collider tools require Edit mode");
        if (!pixie::DescribeColliderHandles2D(s->session.GetWorld(), ToEntity(entity), shape))
            return Fail(PIXIE_ERROR_NOT_FOUND, "collider geometry has no supported handles");
        if (InScene(s, entity)) s->session.BeginAuthoredMutation();
        if (!pixie::DragCollider2D(s->session.GetWorld(), ToEntity(entity), shape,
                                 static_cast<pixie::ColliderDrag2D>(kind), {x, y}, snap != 0))
            return Fail(PIXIE_ERROR_ARGUMENT, "collider drag rejected; keep finite, nondegenerate geometry within supported dimensions");
        RecordChanged(s, entity);
        return PIXIE_OK;
    });
}

PixieResult pixie_body_mass_2d(PixieSession* s, uint32_t entity, PixieBodyMass2D* output) {
    PIXIE_REQUIRE_SESSION(s);
    if (!output) return Fail(PIXIE_ERROR_ARGUMENT, "null body mass output");
    return Guard([&] {
        auto& world = s->session.GetWorld();
        const auto id = ToEntity(entity);
        if (!world.Registry().valid(id) || !world.Registry().all_of<pixie::RigidBody2D>(id))
            return Fail(PIXIE_ERROR_NOT_FOUND, "entity is gone or has no RigidBody2D");
        if (s->session.Mode() == pixie::SessionMode::Edit) pixie::SyncSceneMovement2D(world);
        const auto* physics = pixie::SceneCollisionWorld2D(world);
        const auto body = pixie::SceneBody2D(world, id);
        if (!physics || !physics->ContainsBody(body))
            return Fail(PIXIE_ERROR_NOT_FOUND, "body unavailable; enable valid geometry and check movement diagnostics");
        const auto mass = physics->BodyMass(body);
        *output = {mass.mass, mass.area, mass.localCenter.x, mass.localCenter.y, mass.inertia};
        return PIXIE_OK;
    });
}

PixieResult pixie_collision_layer_name(PixieSession* s, int32_t slot, char* buffer, int32_t capacity, int32_t* required) {
    PIXIE_REQUIRE_SESSION(s);
    if (slot < 1 || slot > 31 || !required || capacity < 0 || (!buffer && capacity > 0))
        return Fail(PIXIE_ERROR_ARGUMENT, "invalid collision layer name query (slots are 1..31)");
    return Guard([&] {
        const auto names = pixie::DescribeCollisionLayers2D(s->session.GetWorld());
        const auto& name = names.names[slot - 1];
        *required = static_cast<int32_t>(name.size() + 1);
        return capacity == 0 ? PIXIE_OK : Copy(name, buffer, capacity);
    });
}

PixieResult pixie_light_layer_name(PixieSession* s, int32_t slot, char* buffer, int32_t capacity, int32_t* required) {
    PIXIE_REQUIRE_SESSION(s);
    if (slot < 1 || slot > 8 || !required || capacity < 0 || (!buffer && capacity > 0))
        return Fail(PIXIE_ERROR_ARGUMENT, "invalid light layer name query (slots are 1..8)");
    return Guard([&] {
        const auto names = pixie::LightLayerNames(s->session.GetWorld());
        const auto& name = names[static_cast<std::size_t>(slot - 1)];
        *required = static_cast<int32_t>(name.size() + 1);
        return capacity == 0 ? PIXIE_OK : Copy(name, buffer, capacity);
    });
}

PixieResult pixie_session_create(int32_t width, int32_t height, uint64_t seed, PixieSession** session) {
    return Guard([&] { return Open(width, height, seed, nullptr, session); });
}

PixieResult pixie_project_create(const char* folder) {
    if (folder == nullptr || folder[0] == '\0') {
        return Fail(PIXIE_ERROR_ARGUMENT, "null or empty folder");
    }
    return Guard([&] {
        const pixie::OpenedProject created = pixie::CreateProjectFolder(FromUtf8(folder));
        return created.ok ? PIXIE_OK : Fail(PIXIE_ERROR_IO, created.error);
    });
}

PixieResult pixie_session_open_project(int32_t width, int32_t height, const char* folder, PixieSession** session) {
    if (folder == nullptr || folder[0] == '\0') {
        return Fail(PIXIE_ERROR_ARGUMENT, "null or empty folder");
    }
    return Guard([&] { return Open(width, height, 1, folder, session); });
}

PixieResult pixie_project_save(PixieSession* s) {
    PIXIE_REQUIRE_SESSION(s);
    return Guard([&] {
        if (!s->session.HasProject()) {
            return Fail(PIXIE_ERROR_ARGUMENT, "no project is open");
        }
        return s->session.Save() ? PIXIE_OK : Fail(PIXIE_ERROR_IO, s->session.Error());
    });
}

int32_t pixie_project_unsaved(PixieSession* s) {
    return s != nullptr && s->session.HasUnsavedChanges() ? 1 : 0;
}

PixieResult pixie_project_snapshot(PixieSession* s, char* buffer, int32_t capacity, int32_t* length) {
    PIXIE_REQUIRE_SESSION(s);
    if (length == nullptr || capacity < 0 || (buffer == nullptr && capacity > 0)) {
        return Fail(PIXIE_ERROR_ARGUMENT, "null length or buffer, or negative capacity");
    }
    return Guard([&] {
        if (!s->session.HasProject() || s->session.Mode() != pixie::SessionMode::Edit) {
            return Fail(PIXIE_ERROR_ARGUMENT, "an open project in Edit mode is required to snapshot its authored scene");
        }
        if (const std::string problem = pixie::ValidateV2SceneSnapshot(s->session.GetWorld(), s->session.Types());
            !problem.empty()) {
            return Fail(PIXIE_ERROR_ARGUMENT, problem);
        }
        const std::string snapshot = pixie::WriteScene(s->session.GetWorld(), s->session.Types());
        if (snapshot.size() > kMaxProjectSnapshotBytes) {
            return Fail(PIXIE_ERROR_ARGUMENT, "authored scene snapshot exceeds 128 MiB");
        }
        *length = static_cast<int32_t>(snapshot.size());
        return capacity == 0 ? PIXIE_OK : Copy(snapshot, buffer, capacity);
    });
}

PixieResult pixie_project_restore(PixieSession* s, const char* snapshot) {
    PIXIE_REQUIRE_SESSION(s);
    if (snapshot == nullptr) return Fail(PIXIE_ERROR_ARGUMENT, "null authored scene snapshot");
    return Guard([&] {
        if (!s->session.HasProject() || s->session.Mode() != pixie::SessionMode::Edit) {
            return Fail(PIXIE_ERROR_ARGUMENT, "an open project in Edit mode is required to restore an authored scene");
        }
        std::size_t length = 0;
        while (length <= kMaxProjectSnapshotBytes && snapshot[length] != '\0') ++length;
        if (length > kMaxProjectSnapshotBytes) {
            return Fail(PIXIE_ERROR_ARGUMENT, "authored scene snapshot exceeds 128 MiB");
        }
        if (length == 0) return Fail(PIXIE_ERROR_ARGUMENT, "empty authored scene snapshot");
        return s->session.RestoreAuthoredScene(std::string_view(snapshot, length))
                   ? PIXIE_OK : Fail(PIXIE_ERROR_ARGUMENT, s->session.Error());
    });
}

PixieResult pixie_project_revision(PixieSession* s, PixieSceneEditToken* token) {
    PIXIE_REQUIRE_SESSION(s);
    if (!token) return Fail(PIXIE_ERROR_ARGUMENT, "null project token output");
    if (s->ownerThread != std::this_thread::get_id()) return Fail(PIXIE_ERROR_ARGUMENT, "project token requires owner thread");
    *token = {s->session.Identity(), s->session.WorldRevision(), s->session.AuthoredRevision()};
    return PIXIE_OK;
}

PixieResult pixie_project_scene_version(PixieSession* s, int32_t* version) {
    PIXIE_REQUIRE_SESSION(s);
    if (!version || !s->session.HasProject()) return Fail(PIXIE_ERROR_ARGUMENT, "an open project and output are required");
    if (s->ownerThread != std::this_thread::get_id()) return Fail(PIXIE_ERROR_ARGUMENT, "format query requires owner thread");
    *version = pixie::SceneFormatVersion(s->session.GetWorld());
    return PIXIE_OK;
}

PixieResult pixie_entity_scene_scope(PixieSession* s, uint32_t entity, uint64_t* scope) {
    PIXIE_REQUIRE_SESSION(s);
    if (!scope) return Fail(PIXIE_ERROR_ARGUMENT, "null membership output");
    if (s->ownerThread != std::this_thread::get_id()) return Fail(PIXIE_ERROR_ARGUMENT, "membership query requires owner thread");
    return Guard([&] {
        auto& world = s->session.GetWorld();
        const auto found = world.ScopeOf(world.Wrap(static_cast<entt::entity>(entity)));
        if (!found) return Fail(PIXIE_ERROR_ARGUMENT, "invalid entity for membership query");
        *scope = found->value;
        return PIXIE_OK;
    });
}

PixieResult pixie_project_convert_scene_v2(PixieSession* s, const PixieSceneEditToken* expected) {
    PIXIE_REQUIRE_SESSION(s);
    if (!expected) return Fail(PIXIE_ERROR_ARGUMENT, "null conversion token");
    return Guard([&] {
        uint64_t revision = 0;
        const auto admitted = pixie_project_edit_status(s, &revision);
        if (admitted != PIXIE_OK) return admitted;
        if (expected->session_identity != s->session.Identity() || expected->world_revision != revision ||
            expected->authored_revision != s->session.AuthoredRevision())
            return Fail(PIXIE_ERROR_ARGUMENT, "stale conversion token");
        return s->session.ConvertStartSceneToV2() ? PIXIE_OK : Fail(PIXIE_ERROR_IO, s->session.Error());
    });
}

PixieResult pixie_project_edit_status(PixieSession* s, uint64_t* revision) {
    PIXIE_REQUIRE_SESSION(s);
    if (revision == nullptr) return Fail(PIXIE_ERROR_ARGUMENT, "null World revision output");
    if (s->ownerThread != std::this_thread::get_id()) return Fail(PIXIE_ERROR_ARGUMENT, "candidate editing requires the session owner thread");
    if (!s->candidateAuthoring || !s->session.HasProject() || s->session.Mode() != pixie::SessionMode::Edit) {
        return Fail(PIXIE_ERROR_ARGUMENT, "candidate editing requires an owned fallback or contracted module project in Edit");
    }
    *revision = s->session.WorldRevision();
    return PIXIE_OK;
}

PixieResult pixie_project_edit_scene(PixieSession* s, const PixieSceneEditToken* expected,
                                    const char* snapshot, PixieSceneEditReceipt* receipt) {
    PIXIE_REQUIRE_SESSION(s);
    if (snapshot == nullptr || receipt == nullptr || expected == nullptr) return Fail(PIXIE_ERROR_ARGUMENT, "null candidate document or receipt");
    *receipt = {};
    return Guard([&] {
        uint64_t revision = 0;
        const PixieResult admitted = pixie_project_edit_status(s, &revision);
        if (admitted != PIXIE_OK) return admitted;
        if (expected->session_identity != s->session.Identity() || expected->world_revision != revision ||
            expected->authored_revision != s->session.AuthoredRevision())
            return Fail(PIXIE_ERROR_ARGUMENT, "stale candidate session, World or authored revision");
        std::size_t length = 0;
        while (length <= kMaxProjectSnapshotBytes && snapshot[length] != '\0') ++length;
        if (length == 0 || length > kMaxProjectSnapshotBytes) return Fail(PIXIE_ERROR_ARGUMENT, "candidate document must be 1..128 MiB");
        const std::string_view text(snapshot, length);
        if (const std::string problem = pixie::ValidateSceneEditInput(s->session.GetWorld(), text); !problem.empty()) {
            return Fail(PIXIE_ERROR_ARGUMENT, problem);
        }
        pixie::SceneBuildTimings timings;
        if (!s->session.RestoreAuthoredScene(text, &timings, true, true)) return Fail(PIXIE_ERROR_ARGUMENT, s->session.Error());
        // Everything after publication is a trivial write; never format/log/call the host here.
        *receipt = {s->session.WorldRevision(), timings.preparationMilliseconds, timings.publicationMilliseconds};
        return PIXIE_OK;
    });
}

PixieResult pixie_presentation_snapshot(PixieSession* s, char* buffer, int32_t capacity, int32_t* length) {
    PIXIE_REQUIRE_SESSION(s);
    if (!length || capacity < 0 || (!buffer && capacity > 0)) return Fail(PIXIE_ERROR_ARGUMENT, "invalid presentation buffer");
    return Guard([&] {
        const std::string text = pixie::PresentationSnapshot2D(s->session.GetWorld());
        if (text.size() > 16 * 1024 * 1024) return Fail(PIXIE_ERROR_ARGUMENT, "presentation catalog exceeds16MiB");
        *length = static_cast<int32_t>(text.size());
        return capacity == 0 ? PIXIE_OK : Copy(text, buffer, capacity);
    });
}
PixieResult pixie_viewport_camera_set(PixieSession* s, int32_t kind, uint64_t entity, uint64_t scene) {
    PIXIE_REQUIRE_SESSION(s);
    if (kind < 0 || kind > 2 || (kind == 2 ? entity == 0 : entity != 0) || (kind == 0 && scene != 0))
        return Fail(PIXIE_ERROR_ARGUMENT, "invalid viewport camera choice");
    s->session.SetViewportCamera({static_cast<pixie::CameraSelectionKind2D>(kind), entity, pixie::ScopeId{scene}});
    return PIXIE_OK;
}
PixieResult pixie_viewport_camera_get(PixieSession* s, int32_t* kind, uint64_t* entity, uint64_t* scene) {
    PIXIE_REQUIRE_SESSION(s);
    if (!kind || !entity || !scene) return Fail(PIXIE_ERROR_ARGUMENT, "null camera choice output");
    const auto choice = s->session.ViewportCameraSelection();
    *kind = static_cast<int32_t>(choice.kind); *entity = choice.entity; *scene = choice.scene.value;
    return PIXIE_OK;
}
PixieResult pixie_viewport_camera_resolve(PixieSession* s, double* x, double* y, double* zoom,
    int32_t* source, char* diagnostic, int32_t capacity, int32_t* length) {
    PIXIE_REQUIRE_SESSION(s);
    if (!x || !y || !zoom || !source || !length || capacity < 0 || (!diagnostic && capacity > 0))
        return Fail(PIXIE_ERROR_ARGUMENT, "invalid camera resolution output");
    return Guard([&] {
        const auto result = s->session.ResolvedCamera();
        *x = result.camera.Center().x; *y = result.camera.Center().y; *zoom = result.camera.Zoom();
        *source = static_cast<int32_t>(result.kind); *length = static_cast<int32_t>(result.diagnostic.size());
        return capacity == 0 ? PIXIE_OK : Copy(result.diagnostic, diagnostic, capacity);
    });
}
PixieResult pixie_viewport_free_camera_get(PixieSession* s, double* x, double* y, double* zoom) {
    PIXIE_REQUIRE_SESSION(s);
    if (!x || !y || !zoom) return Fail(PIXIE_ERROR_ARGUMENT, "null free camera output");
    const auto& camera = s->session.GetCamera(); *x = camera.Center().x; *y = camera.Center().y; *zoom = camera.Zoom(); return PIXIE_OK;
}
PixieResult pixie_viewport_free_camera_set(PixieSession* s, double x, double y, double zoom) {
    PIXIE_REQUIRE_SESSION(s);
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(zoom) || zoom < pixie::Camera2D::kMinZoom || zoom > pixie::Camera2D::kMaxZoom)
        return Fail(PIXIE_ERROR_ARGUMENT, "invalid free camera values");
    s->session.GetCamera().SetCenter({x, y}); s->session.GetCamera().SetZoom(zoom); return PIXIE_OK;
}

PixieResult pixie_viewport_environment_set(PixieSession* s, int32_t kind, uint64_t scene_id) {
    PIXIE_REQUIRE_SESSION(s);
    if ((kind != PIXIE_ENVIRONMENT_LEGACY_WORLD && kind != PIXIE_ENVIRONMENT_SCENE) ||
        (kind == PIXIE_ENVIRONMENT_LEGACY_WORLD && scene_id != 0) ||
        (kind == PIXIE_ENVIRONMENT_SCENE && scene_id == 0)) {
        return Fail(PIXIE_ERROR_ARGUMENT, "select World with ID 0 or a Scene with a nonzero ID");
    }
    return Guard([&] {
        s->session.SetViewportEnvironment({static_cast<pixie::EnvironmentSelectionKind2D>(kind), pixie::ScopeId{scene_id}});
        return PIXIE_OK;
    });
}

PixieResult pixie_viewport_environment_get(PixieSession* s, int32_t* kind, uint64_t* scene_id) {
    PIXIE_REQUIRE_SESSION(s);
    if (kind == nullptr || scene_id == nullptr) return Fail(PIXIE_ERROR_ARGUMENT, "null Environment selection output");
    const auto selection = s->session.ViewportEnvironmentSelection();
    *kind = static_cast<int32_t>(selection.kind);
    *scene_id = selection.scene.value;
    return PIXIE_OK;
}

PixieResult pixie_viewport_environment_resolve(PixieSession* s, int32_t* resolution,
                                                int32_t* lighting_state, char* diagnostic,
                                                int32_t capacity, int32_t* length) {
    PIXIE_REQUIRE_SESSION(s);
    if (resolution == nullptr || lighting_state == nullptr || length == nullptr || capacity < 0 ||
        (diagnostic == nullptr && capacity > 0)) {
        return Fail(PIXIE_ERROR_ARGUMENT, "null Environment resolution output or negative capacity");
    }
    return Guard([&] {
        const auto resolved = s->session.ViewportEnvironment();
        if (!resolved) return Fail(PIXIE_ERROR_ARGUMENT, "the session has no World to resolve Environment");
        *resolution = static_cast<int32_t>(resolved->kind);
        *lighting_state = static_cast<int32_t>(resolved->lighting.state);
        *length = static_cast<int32_t>(resolved->diagnostic.size());
        return capacity == 0 ? PIXIE_OK : Copy(resolved->diagnostic, diagnostic, capacity);
    });
}

PixieResult pixie_scene_scope_count(PixieSession* s, int32_t* count) {
    PIXIE_REQUIRE_SESSION(s);
    if (count == nullptr) return Fail(PIXIE_ERROR_ARGUMENT, "null Scene scope count output");
    return Guard([&] {
        *count = static_cast<int32_t>(s->session.GetWorld().Scopes().size());
        return PIXIE_OK;
    });
}

PixieResult pixie_scene_scope_at(PixieSession* s, int32_t index, uint64_t* id,
                                  char* name, int32_t capacity, int32_t* length) {
    PIXIE_REQUIRE_SESSION(s);
    if (index < 0 || id == nullptr || length == nullptr || capacity < 0 || (name == nullptr && capacity > 0)) {
        return Fail(PIXIE_ERROR_ARGUMENT, "invalid Scene scope index or output");
    }
    return Guard([&] {
        const auto scopes = s->session.GetWorld().Scopes();
        if (static_cast<std::size_t>(index) >= scopes.size()) return Fail(PIXIE_ERROR_ARGUMENT, "Scene scope index is out of range");
        const auto& scope = scopes[static_cast<std::size_t>(index)];
        *id = scope.id.value;
        *length = static_cast<int32_t>(scope.name.size());
        return capacity == 0 ? PIXIE_OK : Copy(scope.name, name, capacity);
    });
}

int32_t pixie_session_mode(PixieSession* s) {
    return s != nullptr ? static_cast<int32_t>(s->session.Mode()) : PIXIE_MODE_PLAY;
}

PixieResult pixie_session_play(PixieSession* s) {
    PIXIE_REQUIRE_SESSION(s);
    return Guard([&] { return s->session.Play() ? PIXIE_OK : Fail(PIXIE_ERROR_ARGUMENT, s->session.Error()); });
}

PixieResult pixie_session_stop(PixieSession* s) {
    PIXIE_REQUIRE_SESSION(s);
    return Guard(
        [&] { return s->session.StopPlaying() ? PIXIE_OK : Fail(PIXIE_ERROR_ARGUMENT, s->session.Error()); });
}

void pixie_session_destroy(PixieSession* session) {
    if (session == nullptr) {
        return;
    }
    Guard([&] {
        session->session.Stop();
        delete session;
        return PIXIE_OK;
    });
}

PixieResult pixie_project_name(PixieSession* s, char* buffer, int32_t capacity) {
    PIXIE_REQUIRE_SESSION(s);
    return Guard([&] { return Copy(s->session.ProjectName(), buffer, capacity); });
}

PixieResult pixie_session_resize(PixieSession* s, int32_t width, int32_t height) {
    PIXIE_REQUIRE_SESSION(s);
    if (width <= 0 || height <= 0) {
        return Fail(PIXIE_ERROR_ARGUMENT, "empty view");
    }
    return Guard([&] {
        s->surface.Resize({width, height});
        s->session.Resize(s->surface.GetFramebufferSize());
        return PIXIE_OK;
    });
}

PixieResult pixie_session_advance(PixieSession* s, double seconds) {
    PIXIE_REQUIRE_SESSION(s);
    if (!(seconds >= 0.0)) {
        return Fail(PIXIE_ERROR_ARGUMENT, "negative or NaN time");
    }
    return Guard([&] {
        s->session.Advance(seconds);
        return PIXIE_OK;
    });
}

PixieResult pixie_session_step(PixieSession* s) {
    PIXIE_REQUIRE_SESSION(s);
    return Guard([&] {
        s->session.StepOnce();
        return PIXIE_OK;
    });
}

PixieResult pixie_session_set_paused(PixieSession* s, int32_t paused) {
    PIXIE_REQUIRE_SESSION(s);
    return Guard([&] {
        s->session.SetPaused(paused != 0);
        return PIXIE_OK;
    });
}

PixieResult pixie_session_set_time_scale(PixieSession* s, double scale) {
    PIXIE_REQUIRE_SESSION(s);
    if (!(scale > 0.0)) {
        return Fail(PIXIE_ERROR_ARGUMENT, "the time scale must be positive");
    }
    return Guard([&] {
        s->session.SetTimeScale(scale);
        return PIXIE_OK;
    });
}

PixieResult pixie_session_reset(PixieSession* s) {
    PIXIE_REQUIRE_SESSION(s);
    return Guard([&] {
        s->session.Reset();
        return s->session.Error().empty() ? PIXIE_OK : Fail(PIXIE_ERROR_ARGUMENT, s->session.Error());
    });
}

PixieResult pixie_session_render(PixieSession* s, uint32_t* texture) {
    PIXIE_REQUIRE_SESSION(s);
    if (texture == nullptr) {
        return Fail(PIXIE_ERROR_ARGUMENT, "null output");
    }
    return Guard([&] {
        s->surface.MakeCurrent();
#ifdef PIXIE_DEV_CHANNEL
        const auto alpha = s->dispatcher ? s->dispatcher->ConsumeRenderAlpha() : std::nullopt;
        *texture = alpha ? s->session.RenderToTarget(*alpha) : s->session.RenderToTarget();
#else
        *texture = s->session.RenderToTarget();
#endif
        return *texture != 0 ? PIXIE_OK : Fail(PIXIE_ERROR_GL, "the render target could not be made");
    });
}

PixieResult pixie_session_stats(PixieSession* s, PixieStats* stats) {
    PIXIE_REQUIRE_SESSION(s);
    if (stats == nullptr) {
        return Fail(PIXIE_ERROR_ARGUMENT, "null output");
    }
    return Guard([&] {
        pixie::World& world = s->session.GetWorld();
        stats->steps = world.Steps();
        stats->elapsed = world.Elapsed();
        stats->lastStepMilliseconds = s->session.LastStepMilliseconds();
        stats->entities = static_cast<int32_t>(world.EntityCount());
        stats->paused = s->session.IsPaused() ? 1 : 0;
        stats->timeScale = s->session.TimeScale();
        return PIXIE_OK;
    });
}

PixieResult pixie_input_event(PixieSession* s, const PixieInputEvent* event) {
    PIXIE_REQUIRE_SESSION(s);
    if (!event || event->kind < 0 || event->kind > 4 || (event->modifiers & ~63u) != 0)
        return Fail(PIXIE_ERROR_ARGUMENT, "invalid input event kind or modifiers");
    if (event->kind == 0 && (event->code < 0 || event->code >= pixie::Input::kKeyCount || event->action < 0 || event->action > 2))
        return Fail(PIXIE_ERROR_ARGUMENT, "invalid key event");
    if (event->kind == 1 && (event->code < 0 || event->code >= pixie::Input::kButtonCount || event->action < 0 || event->action > 1))
        return Fail(PIXIE_ERROR_ARGUMENT, "invalid mouse button event");
    if ((event->kind == 2 || event->kind == 3) && (!std::isfinite(event->x) || !std::isfinite(event->y)))
        return Fail(PIXIE_ERROR_ARGUMENT, "input coordinates must be finite");
    return Guard([&] {
        auto& input = s->session.GetInput();
        const auto action = static_cast<pixie::InputAction>(event->action);
        const auto modifiers = static_cast<pixie::InputModifiers>(event->modifiers);
        switch (event->kind) {
        case 0: input.SetKey(event->code, action, modifiers, event->scan_code); break;
        case 1: input.SetMouseButton(event->code, action, modifiers); break;
        case 2: input.SetCursor(pxPoint(event->x, event->y), modifiers); break;
        case 3: input.AddScroll(pxVec2(event->x, event->y), modifiers); break;
        case 4: input.ReleaseAll(); break;
        }
        return PIXIE_OK;
    });
}

PixieResult pixie_input_cursor(PixieSession* s, double x, double y) {
    PIXIE_REQUIRE_SESSION(s);
    s->session.GetInput().SetCursor(pxPoint(x, y));
    return PIXIE_OK;
}

PixieResult pixie_input_mouse_button(PixieSession* s, int32_t button, int32_t down) {
    PIXIE_REQUIRE_SESSION(s);
    s->session.GetInput().SetMouseButton(button, down != 0);
    return PIXIE_OK;
}

PixieResult pixie_input_key(PixieSession* s, int32_t key, int32_t down) {
    PIXIE_REQUIRE_SESSION(s);
    s->session.GetInput().SetKey(key, down != 0);
    return PIXIE_OK;
}

PixieResult pixie_input_scroll(PixieSession* s, double dx, double dy) {
    PIXIE_REQUIRE_SESSION(s);
    s->session.GetInput().AddScroll(pxVec2(dx, dy));
    return PIXIE_OK;
}

PixieResult pixie_entities(PixieSession* s, uint32_t* entities, int32_t capacity, int32_t* count) {
    PIXIE_REQUIRE_SESSION(s);
    if (count == nullptr || capacity < 0 || (entities == nullptr && capacity > 0)) {
        return Fail(PIXIE_ERROR_ARGUMENT, "null output or negative capacity");
    }
    return Guard([&] {
        // EnTT lists live entities newest first; the API lists them oldest first.
        std::vector<entt::entity> live;
        for (const entt::entity id : s->session.GetWorld().Registry().view<entt::entity>()) {
            live.push_back(id);
        }
        const auto n = static_cast<int32_t>(live.size());
        for (int32_t i = 0; i < n && i < capacity; ++i) {
            entities[i] = static_cast<uint32_t>(live[static_cast<std::size_t>(n - 1 - i)]);
        }
        *count = n;
        return PIXIE_OK;
    });
}

PixieResult pixie_entity_destroy(PixieSession* s, uint32_t entity) {
    PIXIE_REQUIRE_SESSION(s);
    return Guard([&] {
        pixie::World& world = s->session.GetWorld();
        if (!world.Registry().valid(ToEntity(entity))) {
            return Fail(PIXIE_ERROR_NOT_FOUND, std::format("entity {} is gone", entity));
        }
        if (InScene(s, entity)) {
            s->session.MarkChanged();
        }
        if (InScene(s, entity)) s->session.BeginAuthoredMutation();
        world.Destroy(world.Wrap(ToEntity(entity)));
        return PIXIE_OK;
    });
}

PixieResult pixie_view_to_world(PixieSession* s, double x, double y, double* worldX, double* worldY) {
    PIXIE_REQUIRE_SESSION(s);
    if (worldX == nullptr || worldY == nullptr) {
        return Fail(PIXIE_ERROR_ARGUMENT, "null output");
    }
    const pxPoint world = ToWorld(s, x, y);
    *worldX = world.x;
    *worldY = world.y;
    return PIXIE_OK;
}

PixieResult pixie_world_to_view(PixieSession* s, double worldX, double worldY, double* x, double* y) {
    PIXIE_REQUIRE_SESSION(s);
    if (x == nullptr || y == nullptr) {
        return Fail(PIXIE_ERROR_ARGUMENT, "null output");
    }
    const pxPoint view = s->session.ResolvedCamera().camera.WorldToScreen(pxPoint(worldX, worldY));
    *x = view.x;
    *y = view.y;
    return PIXIE_OK;
}

PixieResult pixie_pick(PixieSession* s, double x, double y, double tolerance, uint32_t* entity) {
    PIXIE_REQUIRE_SESSION(s);
    if (entity == nullptr || tolerance < 0.0) {
        return Fail(PIXIE_ERROR_ARGUMENT, "null output or negative tolerance");
    }
    return Guard([&] {
        entt::entity picked =
            pixie::PickAt(s->session.GetWorld(), ToWorld(s, x, y), ToWorldLength(s, tolerance),
                          &s->session.Images(), &s->session.GetFonts());
        if (picked == entt::null && s->session.Mode() == pixie::SessionMode::Edit)
            picked = pixie::PickMovementGizmo2D(s->session.GetWorld(), ToWorld(s, x, y), ToWorldLength(s, tolerance));
        if (picked == entt::null) {
            return Fail(PIXIE_ERROR_NOT_FOUND, std::format("nothing to pick at ({}, {})", x, y));
        }
        *entity = static_cast<uint32_t>(picked);
        return PIXIE_OK;
    });
}

PixieResult pixie_pick_box(PixieSession* s, double x0, double y0, double x1, double y1, uint32_t* entities,
                           int32_t capacity, int32_t* count) {
    PIXIE_REQUIRE_SESSION(s);
    if (count == nullptr || capacity < 0 || (entities == nullptr && capacity > 0)) {
        return Fail(PIXIE_ERROR_ARGUMENT, "null output or negative capacity");
    }
    return Guard([&] {
        const pxPoint a = ToWorld(s, x0, y0);
        const pxPoint b = ToWorld(s, x1, y1);
        const pxRect box(std::min(a.x, b.x), std::min(a.y, b.y), std::abs(a.x - b.x), std::abs(a.y - b.y));
        std::vector<entt::entity> picked = pixie::PickIn(s->session.GetWorld(), box);
        if (s->session.Mode() == pixie::SessionMode::Edit) {
            for (const auto id : pixie::PickMovementGizmosIn2D(s->session.GetWorld(), box))
                if (std::find(picked.begin(), picked.end(), id) == picked.end()) picked.push_back(id);
        }
        const auto n = static_cast<int32_t>(picked.size());
        for (int32_t i = 0; i < n && i < capacity; ++i) {
            entities[i] = static_cast<uint32_t>(picked[static_cast<std::size_t>(i)]);
        }
        *count = n;
        return PIXIE_OK;
    });
}

PixieResult pixie_entity_view_bounds(PixieSession* s, uint32_t entity, double* x, double* y, double* radius) {
    PIXIE_REQUIRE_SESSION(s);
    if (x == nullptr || y == nullptr || radius == nullptr) {
        return Fail(PIXIE_ERROR_ARGUMENT, "null output");
    }
    return Guard([&] {
        entt::registry& registry = s->session.GetWorld().Registry();
        const entt::entity id = ToEntity(entity);
        const auto* transform = registry.valid(id) ? registry.try_get<pixie::Transform2D>(id) : nullptr;
        const auto* shape = registry.valid(id) ? registry.try_get<pixie::Shape>(id) : nullptr;
        const auto* sprite = registry.valid(id) ? registry.try_get<pixie::Sprite>(id) : nullptr;
        const auto* path = registry.valid(id) ? registry.try_get<pixie::Path>(id) : nullptr;
        const auto* text = registry.valid(id) ? registry.try_get<pixie::Text>(id) : nullptr;
        if (transform == nullptr || (shape == nullptr && sprite == nullptr && path == nullptr && text == nullptr)) {
            return Fail(PIXIE_ERROR_NOT_FOUND,
                        std::format("entity {} is gone or has no Transform2D and Shape, Sprite, Text, or Path", entity));
        }
        const pixie::Transform2D drawn = DrawnTransform(s, id, *transform);
        const pixie::Camera2D camera = s->session.ResolvedCamera().camera;
        if (shape != nullptr) {
            const pxPoint screen = camera.WorldToScreen(drawn.position);
            *x = screen.x;
            *y = screen.y;
            *radius = pixie::PickRadius(*shape, *transform) * camera.Zoom();
            return PIXIE_OK;
        }
        if (sprite == nullptr && text != nullptr) {
            const auto corners =
                pixie::TextCorners(*text, drawn, pixie::TextBlock(*text, s->session.GetFonts()));
            const pxPoint screen = camera.WorldToScreen(pxPoint::Lerp(corners[0], corners[2], 0.5));
            *x = screen.x;
            *y = screen.y;
            *radius = std::hypot(corners[2].x - corners[0].x, corners[2].y - corners[0].y) * 0.5 * camera.Zoom();
            return PIXIE_OK;
        }
        if (sprite == nullptr) {
            const std::vector<pxPoint> line = pixie::PathLine(*path, drawn);
            pxPoint low = line.empty() ? drawn.position : line.front();
            pxPoint high = low;
            for (const pxPoint& p : line) {
                low = pxPoint(std::min(low.x, p.x), std::min(low.y, p.y));
                high = pxPoint(std::max(high.x, p.x), std::max(high.y, p.y));
            }
            const pxPoint screen = camera.WorldToScreen(pxPoint::Lerp(low, high, 0.5));
            *x = screen.x;
            *y = screen.y;
            *radius = std::hypot(high.x - low.x, high.y - low.y) * 0.5 * camera.Zoom();
            return PIXIE_OK;
        }
        const auto corners =
            pixie::SpriteCorners(*sprite, drawn, pixie::SpritePixelSize(*sprite, s->session.Images()));
        const pxPoint screen = camera.WorldToScreen(pxPoint::Lerp(corners[0], corners[2], 0.5));
        *x = screen.x;
        *y = screen.y;
        *radius = std::hypot(corners[2].x - corners[0].x, corners[2].y - corners[0].y) * 0.5 * camera.Zoom();
        return PIXIE_OK;
    });
}

PixieResult pixie_entity_view_quad(PixieSession* s, uint32_t entity, double* corners) {
    PIXIE_REQUIRE_SESSION(s);
    if (corners == nullptr) {
        return Fail(PIXIE_ERROR_ARGUMENT, "null output");
    }
    return Guard([&] {
        entt::registry& registry = s->session.GetWorld().Registry();
        const entt::entity id = ToEntity(entity);
        const auto* transform = registry.valid(id) ? registry.try_get<pixie::Transform2D>(id) : nullptr;
        const auto* sprite = registry.valid(id) ? registry.try_get<pixie::Sprite>(id) : nullptr;
        const auto* text = registry.valid(id) ? registry.try_get<pixie::Text>(id) : nullptr;
        if (transform != nullptr && sprite == nullptr && text == nullptr && s->session.Mode() == pixie::SessionMode::Edit) {
            std::vector<pxPoint> outline;
            for (const auto& part : pixie::MovementOutlines2D(s->session.GetWorld(), id))
                outline.insert(outline.end(), part.begin(), part.end());
            if (!outline.empty()) {
                pxPoint low = outline.front(), high = low;
                for (auto point : outline) {
                    low = {std::min(low.x, point.x), std::min(low.y, point.y)};
                    high = {std::max(high.x, point.x), std::max(high.y, point.y)};
                }
                const pxPoint box[] = {low, {high.x, low.y}, high, {low.x, high.y}};
                for (std::size_t i = 0; i < 4; ++i) {
                    const auto view = s->session.ResolvedCamera().camera.WorldToScreen(box[i]);
                    corners[i * 2] = view.x; corners[i * 2 + 1] = view.y;
                }
                return PIXIE_OK;
            }
        }
        if (transform == nullptr || (sprite == nullptr && text == nullptr)) {
            return Fail(PIXIE_ERROR_NOT_FOUND,
                        std::format("entity {} is gone or has no Transform2D and Sprite or Text", entity));
        }
        const pixie::Transform2D drawn = DrawnTransform(s, id, *transform);
        const auto world =
            sprite != nullptr
                ? pixie::SpriteCorners(*sprite, drawn, pixie::SpritePixelSize(*sprite, s->session.Images()))
                : pixie::TextCorners(*text, drawn, pixie::TextBlock(*text, s->session.GetFonts()));
        for (std::size_t i = 0; i < world.size(); ++i) {
            const pxPoint view = s->session.ResolvedCamera().camera.WorldToScreen(world[i]);
            corners[i * 2] = view.x;
            corners[i * 2 + 1] = view.y;
        }
        return PIXIE_OK;
    });
}

PixieResult pixie_entity_view_path(PixieSession* s, uint32_t entity, double* xy, int32_t capacity, int32_t* count) {
    PIXIE_REQUIRE_SESSION(s);
    if (count == nullptr || capacity < 0 || (xy == nullptr && capacity > 0)) {
        return Fail(PIXIE_ERROR_ARGUMENT, "null output or negative capacity");
    }
    return Guard([&] {
        entt::registry& registry = s->session.GetWorld().Registry();
        const entt::entity id = ToEntity(entity);
        const auto* transform = registry.valid(id) ? registry.try_get<pixie::Transform2D>(id) : nullptr;
        const auto* path = registry.valid(id) ? registry.try_get<pixie::Path>(id) : nullptr;
        if (transform == nullptr || path == nullptr) {
            return Fail(PIXIE_ERROR_NOT_FOUND, std::format("entity {} is gone or has no Transform2D and Path", entity));
        }
        const std::vector<pxPoint> line = pixie::PathLine(*path, DrawnTransform(s, id, *transform));
        const auto n = static_cast<int32_t>(line.size());
        for (int32_t i = 0; i < n && i < capacity; ++i) {
            const pxPoint view = s->session.ResolvedCamera().camera.WorldToScreen(line[static_cast<std::size_t>(i)]);
            xy[i * 2] = view.x;
            xy[i * 2 + 1] = view.y;
        }
        *count = n;
        return PIXIE_OK;
    });
}

PixieResult pixie_particle_remove_at(PixieSession* s, double x, double y, double radius) {
    PIXIE_REQUIRE_SESSION(s);
    if (radius < 0.0) {
        return Fail(PIXIE_ERROR_ARGUMENT, "negative radius");
    }
    return Guard([&] {
        if (!pixie::RemoveParticleAt(s->session.GetWorld(), ToWorld(s, x, y), ToWorldLength(s, radius))) {
            return Fail(PIXIE_ERROR_NOT_FOUND, std::format("no particle near ({}, {})", x, y));
        }
        return PIXIE_OK;
    });
}

PixieResult pixie_spawn_at(PixieSession* s, double x, double y, int32_t* created) {
    PIXIE_REQUIRE_SESSION(s);
    return Guard([&] {
        const bool made = s->session.GetProject().SpawnAt(s->session.GetWorld(), ToWorld(s, x, y));
        if (created != nullptr) {
            *created = made ? 1 : 0;
        }
        return PIXIE_OK;
    });
}

PixieResult pixie_entity_name(PixieSession* s, uint32_t entity, char* buffer, int32_t capacity) {
    PIXIE_REQUIRE_SESSION(s);
    return Guard([&] {
        entt::registry& registry = s->session.GetWorld().Registry();
        if (!registry.valid(ToEntity(entity))) {
            return Fail(PIXIE_ERROR_NOT_FOUND, std::format("entity {} is gone", entity));
        }
        const auto* name = registry.try_get<pixie::Name>(ToEntity(entity));
        return Copy(name != nullptr ? std::string_view(name->value) : std::string_view(), buffer, capacity);
    });
}

PixieResult pixie_entity_create(PixieSession* s, const char* name, uint32_t* entity) {
    PIXIE_REQUIRE_SESSION(s);
    if (entity == nullptr) {
        return Fail(PIXIE_ERROR_ARGUMENT, "null output");
    }
    return Guard([&] {
        pixie::World& world = s->session.GetWorld();
        s->session.BeginAuthoredMutation();
        const pixie::Entity created = world.Create(name != nullptr ? std::string_view(name) : std::string_view());
        pixie::AddToScene(created);
        s->session.MarkChanged();
        *entity = created.Number();
        return PIXIE_OK;
    });
}

PixieResult pixie_entity_set_name(PixieSession* s, uint32_t entity, const char* name) {
    PIXIE_REQUIRE_SESSION(s);
    return Guard([&] {
        pixie::World& world = s->session.GetWorld();
        if (!world.Registry().valid(ToEntity(entity))) {
            return Fail(PIXIE_ERROR_NOT_FOUND, std::format("entity {} is gone", entity));
        }
        if (InScene(s, entity)) s->session.BeginAuthoredMutation();
        const pixie::Entity handle = world.Wrap(ToEntity(entity));
        if (name == nullptr || name[0] == '\0') {
            handle.Remove<pixie::Name>();
        } else {
            handle.Add<pixie::Name>(pixie::Name{name});
        }
        if (InScene(s, entity)) {
            s->session.MarkChanged();
        }
        return PIXIE_OK;
    });
}

PixieResult pixie_entity_scene_id(PixieSession* s, uint32_t entity, uint64_t* id) {
    PIXIE_REQUIRE_SESSION(s);
    if (id == nullptr) {
        return Fail(PIXIE_ERROR_ARGUMENT, "null output");
    }
    if (!InScene(s, entity)) {
        return Fail(PIXIE_ERROR_NOT_FOUND, std::format("entity {} is gone or not in the scene", entity));
    }
    *id = s->session.GetWorld().Registry().get<pixie::SceneId>(ToEntity(entity)).value;
    return PIXIE_OK;
}

PixieResult pixie_entity_find(PixieSession* s, uint64_t id, uint32_t* entity) {
    PIXIE_REQUIRE_SESSION(s);
    if (entity == nullptr) {
        return Fail(PIXIE_ERROR_ARGUMENT, "null output");
    }
    return Guard([&] {
        for (const auto [found, sceneId] : s->session.GetWorld().Registry().view<const pixie::SceneId>().each()) {
            if (sceneId.value == id) {
                *entity = static_cast<uint32_t>(found);
                return PIXIE_OK;
            }
        }
        return Fail(PIXIE_ERROR_NOT_FOUND, std::format("no entity has the scene id {:016x}", id));
    });
}

PixieResult pixie_entities_validate(PixieSession* s, const char* snapshot, int32_t ids,
                                   const uint64_t* replacing, int32_t count) {
    PIXIE_REQUIRE_SESSION(s);
    if (snapshot == nullptr || count < 0 || (count > 0 && replacing == nullptr) ||
        (ids != PIXIE_RESTORE_SAME_IDS && ids != PIXIE_RESTORE_NEW_IDS))
        return Fail(PIXIE_ERROR_ARGUMENT, "invalid snapshot, replacement list or ID mode");
    return Guard([&] {
        const auto report = pixie::ValidateEntities(s->session.GetWorld(), s->session.Types(), snapshot,
            ids == PIXIE_RESTORE_SAME_IDS ? pixie::SceneIds::Keep : pixie::SceneIds::New,
            std::span<const uint64_t>(replacing, static_cast<std::size_t>(count)));
        return report.ok ? PIXIE_OK : Fail(PIXIE_ERROR_ARGUMENT, report.error);
    });
}

PixieResult pixie_entities_snapshot(PixieSession* s, const uint32_t* entities, int32_t count, char* buffer,
                                    int32_t capacity, int32_t* length) {
    PIXIE_REQUIRE_SESSION(s);
    if (length == nullptr || count < 0 || (entities == nullptr && count > 0) || capacity < 0 ||
        (buffer == nullptr && capacity > 0)) {
        return Fail(PIXIE_ERROR_ARGUMENT, "null output, or a negative count or capacity");
    }
    return Guard([&] {
        pixie::World& world = s->session.GetWorld();
        std::vector<pixie::Entity> chosen;
        chosen.reserve(static_cast<std::size_t>(count));
        for (int32_t i = 0; i < count; ++i) {
            chosen.push_back(world.Wrap(ToEntity(entities[i])));
        }
        const std::string text = pixie::WriteEntities(world, s->session.Types(), chosen);
        *length = static_cast<int32_t>(text.size());
        return capacity == 0 ? PIXIE_OK : Copy(text, buffer, capacity);
    });
}

PixieResult pixie_entities_restore(PixieSession* s, const char* snapshot, int32_t ids, uint32_t* entities,
                                   int32_t capacity, int32_t* count) {
    PIXIE_REQUIRE_SESSION(s);
    if (snapshot == nullptr || count == nullptr || capacity < 0 || (entities == nullptr && capacity > 0) ||
        (ids != PIXIE_RESTORE_SAME_IDS && ids != PIXIE_RESTORE_NEW_IDS)) {
        return Fail(PIXIE_ERROR_ARGUMENT, "null snapshot or output, a negative capacity, or an unknown way to restore");
    }
    return Guard([&] {
        if (pixie::SceneFormatVersion(s->session.GetWorld()) == 2)
            return Fail(PIXIE_ERROR_ARGUMENT, "version 2 fragments require prepared candidate publication; live restoration is refused");
        const auto policy = ids == PIXIE_RESTORE_SAME_IDS ? pixie::SceneIds::Keep : pixie::SceneIds::New;
        if (const auto check = pixie::ValidateEntities(s->session.GetWorld(), s->session.Types(), snapshot, policy); !check.ok)
            return Fail(PIXIE_ERROR_ARGUMENT, check.error);
        s->session.BeginAuthoredMutation();
        const pixie::EntitiesReport report =
            pixie::ReadEntities(s->session.GetWorld(), s->session.Types(), snapshot,
                                ids == PIXIE_RESTORE_SAME_IDS ? pixie::SceneIds::Keep : pixie::SceneIds::New);
        if (!report.ok) {
            return Fail(PIXIE_ERROR_ARGUMENT, report.error);
        }
        const auto n = static_cast<int32_t>(report.entities.size());
        for (int32_t i = 0; i < n && i < capacity; ++i) {
            entities[i] = report.entities[static_cast<std::size_t>(i)].Number();
        }
        *count = n;
        if (n > 0) {
            s->session.MarkChanged();
        }
        return PIXIE_OK;
    });
}

PixieResult pixie_project_prepare_fragment(PixieSession* s, const PixieSceneEditToken* expected,
    const char* snapshot, int32_t ids, const uint64_t* replacing, int32_t replacing_count,
    double offset_x, double offset_y, char* buffer, int32_t capacity, int32_t* length) {
    PIXIE_REQUIRE_SESSION(s);
    if (!expected || !snapshot || !length || replacing_count < 0 || (replacing_count && !replacing) ||
        capacity < 0 || (capacity > 0 && !buffer) ||
        (ids != PIXIE_RESTORE_SAME_IDS && ids != PIXIE_RESTORE_NEW_IDS))
        return Fail(PIXIE_ERROR_ARGUMENT, "invalid fragment preparation arguments");
    return Guard([&] {
        uint64_t revision = 0;
        const auto admitted = pixie_project_edit_status(s, &revision);
        if (admitted != PIXIE_OK) return admitted;
        if (expected->session_identity != s->session.Identity() || expected->world_revision != revision ||
            expected->authored_revision != s->session.AuthoredRevision())
            return Fail(PIXIE_ERROR_ARGUMENT, "stale fragment preparation token");
        std::size_t size = 0;
        while (size <= kMaxProjectSnapshotBytes && snapshot[size] != '\0') ++size;
        if (size > kMaxProjectSnapshotBytes) return Fail(PIXIE_ERROR_ARGUMENT, "fragment exceeds128 MiB");
        const auto proposal = pixie::PrepareSceneFragment(s->session.GetWorld(), s->session.Types(),
            std::string_view(snapshot, size), ids == PIXIE_RESTORE_SAME_IDS ? pixie::SceneIds::Keep : pixie::SceneIds::New,
            std::span<const uint64_t>(replacing, static_cast<std::size_t>(replacing_count)), offset_x, offset_y);
        if (!proposal.ok) return Fail(PIXIE_ERROR_ARGUMENT, proposal.error);
        if (proposal.text.size() > kMaxProjectSnapshotBytes)
            return Fail(PIXIE_ERROR_ARGUMENT, "fragment proposal exceeds128 MiB");
        *length = static_cast<int32_t>(proposal.text.size());
        return capacity == 0 ? PIXIE_OK : Copy(proposal.text, buffer, capacity);
    });
}

int32_t pixie_entity_alive(PixieSession* s, uint32_t entity) {
    return s != nullptr && s->session.GetWorld().Registry().valid(ToEntity(entity)) ? 1 : 0;
}

int32_t pixie_type_count(PixieSession* s) {
    return s != nullptr ? static_cast<int32_t>(s->session.Types().All().size()) : 0;
}

PixieResult pixie_type_name(PixieSession* s, int32_t type, char* buffer, int32_t capacity) {
    PIXIE_REQUIRE_SESSION(s);
    const pixie::ComponentDescriptor* descriptor = TypeAt(s, type);
    if (descriptor == nullptr) {
        return Fail(PIXIE_ERROR_ARGUMENT, std::format("no component type {}", type));
    }
    return Copy(descriptor->name, buffer, capacity);
}

PixieResult pixie_type_find(PixieSession* s, const char* name, int32_t* type) {
    PIXIE_REQUIRE_SESSION(s);
    if (name == nullptr || type == nullptr) {
        return Fail(PIXIE_ERROR_ARGUMENT, "null name or output");
    }
    const int index = s->session.Types().IndexOf(name);
    if (index < 0) {
        return Fail(PIXIE_ERROR_NOT_FOUND, std::format("no component type named {}", name));
    }
    *type = index;
    return PIXIE_OK;
}

PixieResult pixie_type_source(PixieSession* s, int32_t type, char* buffer, int32_t capacity, int32_t* length) {
    PIXIE_REQUIRE_SESSION(s);
    const auto* descriptor = TypeAt(s, type);
    if (!descriptor || !length) return Fail(PIXIE_ERROR_ARGUMENT, "invalid component type or source length output");
    *length = static_cast<int32_t>(descriptor->source.size()) + 1;
    return capacity == 0 ? PIXIE_OK : Copy(descriptor->source, buffer, capacity);
}

uint32_t pixie_type_flags(PixieSession* s, int32_t type) {
    const pixie::ComponentDescriptor* descriptor = s != nullptr ? TypeAt(s, type) : nullptr;
    return descriptor ? (descriptor->scenario ? PIXIE_TYPE_SCENARIO : 0u)
                      | (descriptor->singleton ? PIXIE_TYPE_SINGLETON : 0u)
                      | (descriptor->valueType ? PIXIE_TYPE_VALUE : 0u) : 0u;
}

int32_t pixie_entity_has(PixieSession* s, uint32_t entity, int32_t type) {
    if (s == nullptr) {
        return 0;
    }
    const pixie::ComponentDescriptor* descriptor = TypeAt(s, type);
    return descriptor != nullptr &&
                   descriptor->find(s->session.GetWorld().Registry(), ToEntity(entity)) != nullptr
               ? 1
               : 0;
}

PixieResult pixie_component_add_status(PixieSession* s, uint32_t entity, int32_t type,
                                     char* buffer, int32_t capacity, int32_t* required) {
    PIXIE_REQUIRE_SESSION(s);
    const auto* descriptor = TypeAt(s, type);
    if (!descriptor || !required || capacity < 0 || (!buffer && capacity > 0))
        return Fail(PIXIE_ERROR_ARGUMENT, "invalid component admission query");
    return Guard([&] {
        auto& world = s->session.GetWorld();
        if (!world.Registry().valid(ToEntity(entity)))
            return Fail(PIXIE_ERROR_NOT_FOUND, "entity no longer exists");
        const auto reason = ComponentAdditionError(s, entity, type, *descriptor);
        *required = static_cast<int32_t>(reason.size() + 1);
        return capacity == 0 ? PIXIE_OK : Copy(reason, buffer, capacity);
    });
}

PixieResult pixie_component_add(PixieSession* s, uint32_t entity, int32_t type) {
    PIXIE_REQUIRE_SESSION(s);
    const pixie::ComponentDescriptor* descriptor = TypeAt(s, type);
    if (descriptor == nullptr) {
        return Fail(PIXIE_ERROR_ARGUMENT, std::format("no component type {}", type));
    }
    return Guard([&] {
        auto& world = s->session.GetWorld();
        if (!world.Registry().valid(ToEntity(entity)))
            return Fail(PIXIE_ERROR_NOT_FOUND, std::format("entity {} is gone", entity));
        if (auto reason = ComponentAdditionError(s, entity, type, *descriptor); !reason.empty())
            return Fail(PIXIE_ERROR_ARGUMENT, std::move(reason));
        if (descriptor->find(world.Registry(), ToEntity(entity))) return PIXIE_OK;
        if (InScene(s, entity)) s->session.BeginAuthoredMutation();
        if (descriptor->add(s->session.GetWorld(), ToEntity(entity)) == nullptr) {
            return Fail(PIXIE_ERROR_NOT_FOUND, std::format("entity {} is gone", entity));
        }
        if (InScene(s, entity)) {
            s->session.MarkChanged();
        }
        return PIXIE_OK;
    });
}

PixieResult pixie_component_remove(PixieSession* s, uint32_t entity, int32_t type) {
    PIXIE_REQUIRE_SESSION(s);
    const pixie::ComponentDescriptor* descriptor = TypeAt(s, type);
    if (descriptor == nullptr) {
        return Fail(PIXIE_ERROR_ARGUMENT, std::format("no component type {}", type));
    }
    return Guard([&] {
        if (descriptor->find(s->session.GetWorld().Registry(), ToEntity(entity)) == nullptr) {
            return Fail(PIXIE_ERROR_NOT_FOUND, std::format("entity {} is gone or has no {}", entity, descriptor->name));
        }
        if (InScene(s, entity)) {
            s->session.MarkChanged();
        }
        if (InScene(s, entity)) s->session.BeginAuthoredMutation();
        descriptor->remove(s->session.GetWorld(), ToEntity(entity));
        return PIXIE_OK;
    });
}

int32_t pixie_field_count(PixieSession* s, int32_t type) {
    const pixie::ComponentDescriptor* descriptor = s != nullptr ? TypeAt(s, type) : nullptr;
    return descriptor != nullptr ? static_cast<int32_t>(descriptor->fields.size()) : 0;
}

PixieResult pixie_field_name(PixieSession* s, int32_t type, int32_t field, char* buffer, int32_t capacity) {
    PIXIE_REQUIRE_SESSION(s);
    const pixie::FieldDescriptor* descriptor = FieldAt(s, type, field);
    if (descriptor == nullptr) {
        return Fail(PIXIE_ERROR_ARGUMENT, std::format("no field {} in type {}", field, type));
    }
    return Copy(descriptor->name, buffer, capacity);
}

PixieResult pixie_field_kind(PixieSession* s, int32_t type, int32_t field, int32_t* kind) {
    PIXIE_REQUIRE_SESSION(s);
    const pixie::FieldDescriptor* descriptor = FieldAt(s, type, field);
    if (descriptor == nullptr || kind == nullptr) {
        return Fail(PIXIE_ERROR_ARGUMENT, std::format("no field {} in type {}, or null output", field, type));
    }
    *kind = static_cast<int32_t>(descriptor->kind);
    return PIXIE_OK;
}

PixieResult pixie_field_info(PixieSession* s, int32_t type, int32_t field, PixieFieldInfo* info) {
    PIXIE_REQUIRE_SESSION(s);
    const pixie::FieldDescriptor* descriptor = FieldAt(s, type, field);
    if (descriptor == nullptr || info == nullptr) {
        return Fail(PIXIE_ERROR_ARGUMENT, std::format("no field {} in type {}, or null output", field, type));
    }
    const pixie::FieldMetadata& m = descriptor->metadata;
    *info = PixieFieldInfo{};
    info->kind = static_cast<int32_t>(descriptor->kind);
    info->editor = static_cast<int32_t>(m.editor);
    info->flags = (m.readOnly ? PIXIE_FIELD_READ_ONLY : 0u) | (m.hasRange ? PIXIE_FIELD_HAS_RANGE : 0u) |
                  (m.hasUnit ? PIXIE_FIELD_HAS_UNIT : 0u);
    info->order = m.order;
    info->scrubAxis = static_cast<int32_t>(m.scrubAxis);
    info->enumCount = static_cast<int32_t>(descriptor->enumNames.size());
    info->rangeMin = m.rangeMin;
    info->rangeMax = m.rangeMax;
    info->rangeStep = m.rangeStep;
    info->scrub = m.scrub;
    info->unitFactor = m.unitFactor;
    info->unitOffset = m.unitOffset;
    return PIXIE_OK;
}

PixieResult pixie_field_text(PixieSession* s, int32_t type, int32_t field, int32_t which, char* buffer,
                             int32_t capacity) {
    PIXIE_REQUIRE_SESSION(s);
    const pixie::FieldDescriptor* descriptor = FieldAt(s, type, field);
    if (descriptor == nullptr) {
        return Fail(PIXIE_ERROR_ARGUMENT, std::format("no field {} in type {}", field, type));
    }
    const pixie::FieldMetadata& m = descriptor->metadata;
    switch (which) {
    case PIXIE_TEXT_LABEL:
        return Copy(m.label.empty() ? descriptor->name : m.label, buffer, capacity);
    case PIXIE_TEXT_TOOLTIP:
        return Copy(m.tooltip, buffer, capacity);
    case PIXIE_TEXT_HELP:
        return Copy(m.help, buffer, capacity);
    case PIXIE_TEXT_UNIT_SUFFIX:
        return Copy(m.unitSuffix, buffer, capacity);
    case PIXIE_TEXT_ARC_SPREAD:
        return Copy(m.arcSpread, buffer, capacity);
    case PIXIE_TEXT_REFERENCES:
        return Copy(m.references, buffer, capacity);
    case PIXIE_TEXT_ASSET:
        return Copy(m.asset, buffer, capacity);
    default:
        return Fail(PIXIE_ERROR_ARGUMENT, std::format("no field text {}", which));
    }
}

int32_t pixie_field_enum_count(PixieSession* s, int32_t type, int32_t field) {
    const pixie::FieldDescriptor* descriptor = s != nullptr ? FieldAt(s, type, field) : nullptr;
    return descriptor != nullptr ? static_cast<int32_t>(descriptor->enumNames.size()) : 0;
}

PixieResult pixie_field_enum_name(PixieSession* s, int32_t type, int32_t field, int32_t value,
                                  char* buffer, int32_t capacity) {
    PIXIE_REQUIRE_SESSION(s);
    const pixie::FieldDescriptor* descriptor = FieldAt(s, type, field);
    if (descriptor == nullptr || value < 0 || value >= static_cast<int32_t>(descriptor->enumNames.size())) {
        return Fail(PIXIE_ERROR_ARGUMENT, std::format("no enum value {} for field {} of type {}", value, field, type));
    }
    return Copy(descriptor->enumNames[static_cast<std::size_t>(value)], buffer, capacity);
}

PixieResult pixie_field_get(PixieSession* s, uint32_t entity, int32_t type, int32_t field, PixieValue* value) {
    PIXIE_REQUIRE_SESSION(s);
    const pixie::FieldDescriptor* descriptor = FieldAt(s, type, field);
    if (descriptor == nullptr || value == nullptr) {
        return Fail(PIXIE_ERROR_ARGUMENT, std::format("no field {} in type {}, or null output", field, type));
    }
    return Guard([&] {
        if (descriptor->read == nullptr) {
            return Fail(PIXIE_ERROR_ARGUMENT,
                        std::format("{} is not a value; use pixie_field_get_text, pixie_field_get_points or pixie_record_* for its kind",
                                    descriptor->name));
        }
        PixieResult result = PIXIE_OK;
        void* component = ComponentOf(s, entity, type, result);
        if (component == nullptr) {
            return result;
        }
        const pixie::FieldValue read = descriptor->read(component);
        *value = PixieValue{read.x, read.y, read.z, read.w};
        return PIXIE_OK;
    });
}

PixieResult pixie_field_set(PixieSession* s, uint32_t entity, int32_t type, int32_t field,
                            const PixieValue* value) {
    PIXIE_REQUIRE_SESSION(s);
    const pixie::FieldDescriptor* descriptor = FieldAt(s, type, field);
    if (descriptor == nullptr || value == nullptr) {
        return Fail(PIXIE_ERROR_ARGUMENT, std::format("no field {} in type {}, or null value", field, type));
    }
    return Guard([&] {
        if (descriptor->read == nullptr) {
            return Fail(PIXIE_ERROR_ARGUMENT,
                        std::format("{} is not a value; use pixie_field_set_text, pixie_field_set_points or pixie_record_* for its kind",
                                    descriptor->name));
        }
        PixieResult result = CheckEnumValue(*descriptor, value->x);
        if (result != PIXIE_OK) {
            return result;
        }
        void* component = ComponentOf(s, entity, type, result);
        if (component == nullptr) {
            return result;
        }
        const auto nativeType = TypeAt(s, type)->type;
        auto& world = s->session.GetWorld();
        if (nativeType == entt::type_hash<pixie::CameraComponent2D>::value() &&
            (!std::isfinite(value->x) || (descriptor->name == "zoom" &&
             (value->x < pixie::Camera2D::kMinZoom || value->x > pixie::Camera2D::kMaxZoom))))
            return Fail(PIXIE_ERROR_ARGUMENT, "camera values must be finite and zoom within native limits");
        const bool physicsSettings = nativeType == entt::type_hash<pixie::PhysicsSettings2D>::value();
        const bool lightingSettings = nativeType == entt::type_hash<pixie::LightingSettings2D>::value();
        if (lightingSettings && world.Presentation()) return Fail(PIXIE_ERROR_ARGUMENT, "canonical presentation is the lighting authority");
        if (physicsSettings || lightingSettings) {
            const auto count = physicsSettings ? world.Registry().view<const pixie::PhysicsSettings2D>().size()
                : world.Registry().view<const pixie::LightingSettings2D>().size();
            if (count != 1) return Fail(PIXIE_ERROR_ARGUMENT, "duplicate World settings authorities; resolve the conflict before editing");
            if (!std::isfinite(value->x) || !std::isfinite(value->y) ||
                !std::isfinite(value->z) || !std::isfinite(value->w)) {
                return Fail(PIXIE_ERROR_ARGUMENT, "World settings require finite field values");
            }
            if (descriptor->kind == pixie::FieldKind::Int &&
                (value->x < std::numeric_limits<int>::min() || value->x > std::numeric_limits<int>::max() ||
                 std::trunc(value->x) != value->x)) {
                return Fail(PIXIE_ERROR_ARGUMENT, "World settings require a representable integer value");
            }
        }
        if (physicsSettings) {
            // Runtime settings have only numeric/vector fields. Admit their prospective value
            // before touching live storage or changing the backend's units/state.
            if (descriptor->kind == pixie::FieldKind::Int &&
                (value->x < 1 || value->x > 32 || std::trunc(value->x) != value->x)) {
                return Fail(PIXIE_ERROR_ARGUMENT, "PhysicsSettings2D substeps must be an integer from 1 to 32");
            }
            auto candidate = *static_cast<pixie::PhysicsSettings2D*>(component);
            descriptor->write(&candidate, pixie::FieldValue{value->x, value->y, value->z, value->w});
            if (const auto error = pixie::ScenePhysicsSettingsEditError2D(world, candidate); !error.empty()) {
                return Fail(PIXIE_ERROR_ARGUMENT, error);
            }
        }
        if (InScene(s, entity)) s->session.BeginAuthoredMutation();
        descriptor->write(component, pixie::FieldValue{value->x, value->y, value->z, value->w});
        if (InScene(s, entity)) {
            s->session.MarkChanged();
        }
        return PIXIE_OK;
    });
}

PixieResult pixie_field_get_text(PixieSession* s, uint32_t entity, int32_t type, int32_t field, char* buffer,
                                 int32_t capacity, int32_t* length) {
    PIXIE_REQUIRE_SESSION(s);
    const pixie::FieldDescriptor* descriptor = FieldAt(s, type, field);
    if (descriptor == nullptr || length == nullptr || capacity < 0 || (buffer == nullptr && capacity > 0)) {
        return Fail(PIXIE_ERROR_ARGUMENT,
                    std::format("no field {} in type {}, a null output, or a negative capacity", field, type));
    }
    return Guard([&] {
        if (descriptor->kind != pixie::FieldKind::Text) {
            return Fail(PIXIE_ERROR_ARGUMENT,
                        std::format("{} is not text; read it with pixie_field_get", descriptor->name));
        }
        PixieResult result = PIXIE_OK;
        void* component = ComponentOf(s, entity, type, result);
        if (component == nullptr) {
            return result;
        }
        const std::string text = descriptor->readText(component);
        *length = static_cast<int32_t>(text.size());
        return capacity == 0 ? PIXIE_OK : Copy(text, buffer, capacity);
    });
}

PixieResult pixie_field_set_text(PixieSession* s, uint32_t entity, int32_t type, int32_t field, const char* text) {
    PIXIE_REQUIRE_SESSION(s);
    const pixie::FieldDescriptor* descriptor = FieldAt(s, type, field);
    if (descriptor == nullptr || text == nullptr) {
        return Fail(PIXIE_ERROR_ARGUMENT, std::format("no field {} in type {}, or null text", field, type));
    }
    return Guard([&] {
        if (descriptor->kind != pixie::FieldKind::Text) {
            return Fail(PIXIE_ERROR_ARGUMENT,
                        std::format("{} is not text; write it with pixie_field_set", descriptor->name));
        }
        PixieResult result = PIXIE_OK;
        void* component = ComponentOf(s, entity, type, result);
        if (component == nullptr) {
            return result;
        }
        if (InScene(s, entity)) s->session.BeginAuthoredMutation();
        descriptor->writeText(component, text);
        if (InScene(s, entity)) {
            s->session.MarkChanged();
        }
        return PIXIE_OK;
    });
}

PixieResult pixie_field_get_points(PixieSession* s, uint32_t entity, int32_t type, int32_t field, double* xy,
                                   int32_t capacity, int32_t* count) {
    PIXIE_REQUIRE_SESSION(s);
    const pixie::FieldDescriptor* descriptor = FieldAt(s, type, field);
    if (descriptor == nullptr || count == nullptr || capacity < 0 || (xy == nullptr && capacity > 0)) {
        return Fail(PIXIE_ERROR_ARGUMENT,
                    std::format("no field {} in type {}, a null output, or a negative capacity", field, type));
    }
    return Guard([&] {
        if (descriptor->kind != pixie::FieldKind::PointList) {
            return Fail(PIXIE_ERROR_ARGUMENT, std::format("{} is not a list of points", descriptor->name));
        }
        PixieResult result = PIXIE_OK;
        void* component = ComponentOf(s, entity, type, result);
        if (component == nullptr) {
            return result;
        }
        const std::vector<pxPoint> points = descriptor->readPoints(component);
        const auto n = static_cast<int32_t>(points.size());
        for (int32_t i = 0; i < n && i < capacity; ++i) {
            xy[i * 2] = points[static_cast<std::size_t>(i)].x;
            xy[i * 2 + 1] = points[static_cast<std::size_t>(i)].y;
        }
        *count = n;
        return PIXIE_OK;
    });
}

PixieResult pixie_field_set_points(PixieSession* s, uint32_t entity, int32_t type, int32_t field, const double* xy,
                                   int32_t count) {
    PIXIE_REQUIRE_SESSION(s);
    const pixie::FieldDescriptor* descriptor = FieldAt(s, type, field);
    if (descriptor == nullptr || count < 0 || (xy == nullptr && count > 0)) {
        return Fail(PIXIE_ERROR_ARGUMENT, std::format("no field {} in type {}, null points, or a negative count", field, type));
    }
    return Guard([&] {
        if (descriptor->kind != pixie::FieldKind::PointList) {
            return Fail(PIXIE_ERROR_ARGUMENT, std::format("{} is not a list of points", descriptor->name));
        }
        PixieResult result = PIXIE_OK;
        void* component = ComponentOf(s, entity, type, result);
        if (component == nullptr) {
            return result;
        }
        std::vector<pxPoint> points;
        points.reserve(static_cast<std::size_t>(count));
        for (int32_t i = 0; i < count; ++i) {
            points.emplace_back(xy[i * 2], xy[i * 2 + 1]);
        }
        if (InScene(s, entity)) s->session.BeginAuthoredMutation();
        descriptor->writePoints(component, points);
        if (InScene(s, entity)) {
            s->session.MarkChanged();
        }
        return PIXIE_OK;
    });
}

PixieResult pixie_record_info(PixieSession* s, uint32_t entity, int32_t type, int32_t list,
                             int32_t* itemType, int32_t* count) {
    PIXIE_REQUIRE_SESSION(s);
    if (!itemType || !count) return Fail(PIXIE_ERROR_ARGUMENT, "null record info output");
    return Guard([&] {
        RecordAccess access;
        const auto result = RecordOf(s, entity, type, list, -1, -1, access);
        if (result != PIXIE_OK) return result;
        *itemType = access.itemType;
        *count = static_cast<int32_t>(access.list->recordCount(access.owner));
        return PIXIE_OK;
    });
}
PixieResult pixie_record_edit(PixieSession* s, uint32_t entity, int32_t type, int32_t list,
                             int32_t index, int32_t remove) {
    PIXIE_REQUIRE_SESSION(s);
    return Guard([&] {
        RecordAccess access;
        const auto result = RecordOf(s, entity, type, list, -1, -1, access);
        if (result != PIXIE_OK) return result;
        const auto count = access.list->recordCount(access.owner);
        if (index < 0 || (remove != 0 && remove != 1) || static_cast<std::size_t>(index) > count
            || (remove && static_cast<std::size_t>(index) == count) || (!remove && count >= 256))
            return Fail(PIXIE_ERROR_ARGUMENT, "record edit index or count is out of range");
        if (access.list->metadata.readOnly) return Fail(PIXIE_ERROR_ARGUMENT, "record list is read-only");
        if (InScene(s, entity)) s->session.BeginAuthoredMutation();
        if (remove) access.list->eraseRecord(access.owner, static_cast<std::size_t>(index));
        else access.list->insertRecord(access.owner, static_cast<std::size_t>(index));
        RecordChanged(s, entity);
        return PIXIE_OK;
    });
}
PixieResult pixie_record_get(PixieSession* s, uint32_t entity, int32_t type, int32_t list,
                            int32_t index, int32_t field, PixieValue* value) {
    PIXIE_REQUIRE_SESSION(s);
    if (!value || index < 0) return Fail(PIXIE_ERROR_ARGUMENT, "null value or negative record index");
    return Guard([&] {
        RecordAccess access;
        const auto result = RecordOf(s, entity, type, list, index, field, access);
        if (result != PIXIE_OK) return result;
        if (!access.field->read) return Fail(PIXIE_ERROR_ARGUMENT, "record field is not numeric");
        const auto v = access.field->read(access.item);
        *value = {v.x, v.y, v.z, v.w};
        return PIXIE_OK;
    });
}
PixieResult pixie_record_set(PixieSession* s, uint32_t entity, int32_t type, int32_t list,
                            int32_t index, int32_t field, const PixieValue* value) {
    PIXIE_REQUIRE_SESSION(s);
    if (!value || index < 0) return Fail(PIXIE_ERROR_ARGUMENT, "null value or negative record index");
    return Guard([&] {
        RecordAccess access;
        const auto result = RecordOf(s, entity, type, list, index, field, access);
        if (result != PIXIE_OK) return result;
        if (!access.field->write || access.field->metadata.readOnly || access.list->metadata.readOnly)
            return Fail(PIXIE_ERROR_ARGUMENT, "record field is not writable numeric data");
        if (const auto checked = CheckEnumValue(*access.field, value->x); checked != PIXIE_OK) return checked;
        if (InScene(s, entity)) s->session.BeginAuthoredMutation();
        access.field->write(access.item, {value->x, value->y, value->z, value->w});
        RecordChanged(s, entity);
        return PIXIE_OK;
    });
}
PixieResult pixie_record_get_points(PixieSession* s, uint32_t entity, int32_t type, int32_t list,
                                   int32_t index, int32_t field, double* xy, int32_t capacity, int32_t* count) {
    PIXIE_REQUIRE_SESSION(s);
    if (!count || capacity < 0 || (!xy && capacity > 0) || index < 0) return Fail(PIXIE_ERROR_ARGUMENT, "invalid record points output");
    return Guard([&] {
        RecordAccess access;
        const auto result = RecordOf(s, entity, type, list, index, field, access);
        if (result != PIXIE_OK) return result;
        if (!access.field->readPoints) return Fail(PIXIE_ERROR_ARGUMENT, "record field is not a point list");
        const auto points = access.field->readPoints(access.item);
        *count = static_cast<int32_t>(points.size());
        for (int32_t i = 0; i < *count && i < capacity; ++i) { xy[2*i] = points[i].x; xy[2*i+1] = points[i].y; }
        return PIXIE_OK;
    });
}
PixieResult pixie_record_set_points(PixieSession* s, uint32_t entity, int32_t type, int32_t list,
                                   int32_t index, int32_t field, const double* xy, int32_t count) {
    PIXIE_REQUIRE_SESSION(s);
    if (count < 0 || count > 65536 || (!xy && count > 0) || index < 0) return Fail(PIXIE_ERROR_ARGUMENT, "invalid record points input");
    return Guard([&] {
        RecordAccess access;
        const auto result = RecordOf(s, entity, type, list, index, field, access);
        if (result != PIXIE_OK) return result;
        if (!access.field->writePoints || access.field->metadata.readOnly || access.list->metadata.readOnly)
            return Fail(PIXIE_ERROR_ARGUMENT, "record points are not writable");
        std::vector<pxPoint> points;
        for (int32_t i = 0; i < count; ++i) points.emplace_back(xy[2*i], xy[2*i+1]);
        if (InScene(s, entity)) s->session.BeginAuthoredMutation();
        access.field->writePoints(access.item, points);
        RecordChanged(s, entity);
        return PIXIE_OK;
    });
}
PixieResult pixie_record_get_text(PixieSession* s, uint32_t entity, int32_t type, int32_t list,
                                 int32_t index, int32_t field, char* text, int32_t capacity, int32_t* required) {
    PIXIE_REQUIRE_SESSION(s);
    if (!required || capacity < 0 || (!text && capacity > 0) || index < 0) return Fail(PIXIE_ERROR_ARGUMENT, "invalid record text output");
    return Guard([&] {
        RecordAccess access;
        const auto result = RecordOf(s, entity, type, list, index, field, access);
        if (result != PIXIE_OK) return result;
        if (!access.field->readText) return Fail(PIXIE_ERROR_ARGUMENT, "record field is not text");
        const auto value = access.field->readText(access.item);
        *required = static_cast<int32_t>(value.size() + 1);
        return capacity == 0 ? PIXIE_OK : Copy(value, text, capacity);
    });
}
PixieResult pixie_record_set_text(PixieSession* s, uint32_t entity, int32_t type, int32_t list,
                                 int32_t index, int32_t field, const char* text) {
    PIXIE_REQUIRE_SESSION(s);
    if (!text || index < 0) return Fail(PIXIE_ERROR_ARGUMENT, "null text or negative record index");
    return Guard([&] {
        RecordAccess access;
        const auto result = RecordOf(s, entity, type, list, index, field, access);
        if (result != PIXIE_OK) return result;
        if (!access.field->writeText || access.field->metadata.readOnly || access.list->metadata.readOnly)
            return Fail(PIXIE_ERROR_ARGUMENT, "record text is not writable");
        if (InScene(s, entity)) s->session.BeginAuthoredMutation();
        access.field->writeText(access.item, text);
        RecordChanged(s, entity);
        return PIXIE_OK;
    });
}

PixieResult pixie_image(PixieSession* s, const char* path, uint32_t* texture, int32_t* width, int32_t* height) {
    PIXIE_REQUIRE_SESSION(s);
    if (path == nullptr || texture == nullptr || width == nullptr || height == nullptr) {
        return Fail(PIXIE_ERROR_ARGUMENT, "null path or output");
    }
    return Guard([&] {
        pixie::SpriteImages& images = s->session.Images();
        const auto size = images.Size(path);
        if (!size) {
            return Fail(PIXIE_ERROR_IO, std::format("{}: {}", path, images.Problem(path)));
        }
        *texture = images.TextureOf(path).Id();
        *width = static_cast<int32_t>(size->x);
        *height = static_cast<int32_t>(size->y);
        return PIXIE_OK;
    });
}

} // extern "C"
