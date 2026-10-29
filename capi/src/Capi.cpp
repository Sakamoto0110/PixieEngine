#include "pixie/runtime/ParticleStep.hpp"
// The C API over a ProjectSession. Every exported function converts its arguments, runs inside
// Guard (so no exception crosses into the caller), and reports failures through a result code and
// the thread's last error.

#include "pixie_capi.h"
#include "GameModules.hpp"
#include "Session.hpp"

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
#include "pixie/runtime/ColliderEditing2D.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <exception>
#include <filesystem>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

static_assert(sizeof(PixieFieldInfo) == 72, "PixieFieldInfo's layout is part of the C API");
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

namespace {

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
    std::unique_ptr<pixie::Project> project;
#ifdef PIXIE_CAPI_FACTORY
    project = pixie::CreateProject();
#endif
#ifdef PIXIE_DEVELOPMENT
    if (loadedModule) {
        project.reset(loadedModule->create());
        if (!project) return Fail(PIXIE_ERROR_INTERNAL, "game module factory returned no project");
    }
#endif
    if (!project) project = std::make_unique<pixie::SceneProject>();
    auto created = std::make_unique<PixieSession>(std::move(project), settings);
#ifdef PIXIE_DEVELOPMENT
    created->module = loadedModule;
#endif
    if (folder != nullptr && !created->session.OpenProject(FromUtf8(folder))) {
        return Fail(PIXIE_ERROR_IO, created->session.Error());
    }
    created->surface.Resize(surface.GetFramebufferSize());
    if (!created->session.Start(created->surface.GetFramebufferSize())) {
        created->session.Stop();
        return Fail(PIXIE_ERROR_GL, "the renderers could not be created");
    }
    *session = created.release();
    return PIXIE_OK;
}

// A point in view pixels, in world units.
pxPoint ToWorld(PixieSession* s, double x, double y) {
    return s->session.GetCamera().ScreenToWorld(pxPoint(x, y));
}

// A length in view pixels, in world units.
double ToWorldLength(PixieSession* s, double pixels) {
    return pixels / s->session.GetCamera().Zoom();
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
            return Fail(PIXIE_ERROR_ARGUMENT, "game module ABI version or factory is incompatible");
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
        return PIXIE_OK;
    });
}

PixieResult pixie_session_render(PixieSession* s, uint32_t* texture) {
    PIXIE_REQUIRE_SESSION(s);
    if (texture == nullptr) {
        return Fail(PIXIE_ERROR_ARGUMENT, "null output");
    }
    return Guard([&] {
        s->surface.MakeCurrent();
        *texture = s->session.RenderToTarget();
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
    const pxPoint view = s->session.GetCamera().WorldToScreen(pxPoint(worldX, worldY));
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
        const pixie::Camera2D& camera = s->session.GetCamera();
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
                    const auto view = s->session.GetCamera().WorldToScreen(box[i]);
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
            const pxPoint view = s->session.GetCamera().WorldToScreen(world[i]);
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
            const pxPoint view = s->session.GetCamera().WorldToScreen(line[static_cast<std::size_t>(i)]);
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
        const auto reason = descriptor->AdditionError(world, ToEntity(entity));
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
        if (auto reason = descriptor->AdditionError(world, ToEntity(entity)); !reason.empty())
            return Fail(PIXIE_ERROR_ARGUMENT, std::move(reason));
        if (descriptor->find(world.Registry(), ToEntity(entity))) return PIXIE_OK;
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
        PixieResult result = PIXIE_OK;
        void* component = ComponentOf(s, entity, type, result);
        if (component == nullptr) {
            return result;
        }
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
