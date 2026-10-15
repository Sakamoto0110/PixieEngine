// The C API over a ProjectSession. Every exported function converts its arguments, runs inside
// Guard (so no exception crosses into the caller), and reports failures through a result code and
// the thread's last error.

#include "pixie_capi.h"

#include "pixie/GlLoader.hpp"
#include "pixie/runtime/Name.hpp"
#include "pixie/runtime/Picking.hpp"
#include "pixie/runtime/Scene.hpp"
#include "pixie/runtime/Shape.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/ProjectSession.hpp"

#include <algorithm>
#include <cstring>
#include <exception>
#include <filesystem>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

static_assert(sizeof(PixieFieldInfo) == 72, "PixieFieldInfo's layout is part of the C API");
static_assert(static_cast<int>(pixie::FieldEditor::Arc) == PIXIE_EDITOR_ARC);
static_assert(static_cast<int>(pixie::FieldKind::Entity) == PIXIE_FIELD_ENTITY);
static_assert(static_cast<int>(pixie::SessionMode::Edit) == PIXIE_MODE_EDIT);

struct PixieSession {
    pixie::ProjectSession session;
    explicit PixieSession(std::unique_ptr<pixie::Project> project, pixie::SessionSettings settings)
        : session(std::move(project), settings) {}
};

namespace {

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

// Creates a session for the game, with the project in `folder` open when it is not null.
PixieResult Open(int32_t width, int32_t height, uint64_t seed, const char* folder, PixieSession** session) {
    if (session == nullptr || width <= 0 || height <= 0) {
        return Fail(PIXIE_ERROR_ARGUMENT, "null output or empty view");
    }
    *session = nullptr;
    if (!pixie::LoadGl()) {
        return Fail(PIXIE_ERROR_GL, "OpenGL 4.5 core functions could not be loaded for the current context");
    }
    pixie::SessionSettings settings;
    settings.seed = seed;
    auto created = std::make_unique<PixieSession>(pixie::CreateProject(), settings);
    if (folder != nullptr && !created->session.OpenProject(FromUtf8(folder))) {
        return Fail(PIXIE_ERROR_IO, created->session.Error());
    }
    if (!created->session.Start({width, height})) {
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

} // namespace

extern "C" {

int32_t pixie_api_version(void) {
    return PIXIE_CAPI_VERSION;
}

int32_t pixie_last_error(char* buffer, int32_t capacity) {
    if (buffer != nullptr && capacity > 0) {
        const auto length = std::min(lastError.size(), static_cast<std::size_t>(capacity - 1));
        std::memcpy(buffer, lastError.data(), length);
        buffer[length] = '\0';
    }
    return static_cast<int32_t>(lastError.size());
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
        s->session.Resize({width, height});
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
        const entt::entity picked =
            pixie::PickAt(s->session.GetWorld(), ToWorld(s, x, y), ToWorldLength(s, tolerance));
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
        const std::vector<entt::entity> picked = pixie::PickIn(s->session.GetWorld(), box);
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
        if (transform == nullptr || shape == nullptr) {
            return Fail(PIXIE_ERROR_NOT_FOUND, std::format("entity {} is gone or has no Transform2D and Shape", entity));
        }
        // Where the last render drew it: between the previous step and this one.
        pxPoint position = transform->position;
        if (const auto* previous = registry.try_get<pixie::PreviousTransform2D>(id)) {
            position = pxPoint::Lerp(previous->value.position, transform->position, s->session.Alpha());
        }
        const pixie::Camera2D& camera = s->session.GetCamera();
        const pxPoint screen = camera.WorldToScreen(position);
        *x = screen.x;
        *y = screen.y;
        *radius = pixie::PickRadius(*shape, *transform) * camera.Zoom();
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
    return descriptor != nullptr && descriptor->scenario ? PIXIE_TYPE_SCENARIO : 0u;
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

PixieResult pixie_component_add(PixieSession* s, uint32_t entity, int32_t type) {
    PIXIE_REQUIRE_SESSION(s);
    const pixie::ComponentDescriptor* descriptor = TypeAt(s, type);
    if (descriptor == nullptr) {
        return Fail(PIXIE_ERROR_ARGUMENT, std::format("no component type {}", type));
    }
    if (descriptor->add == nullptr) {
        return Fail(PIXIE_ERROR_ARGUMENT, std::format("{} has no default value to add", descriptor->name));
    }
    return Guard([&] {
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

} // extern "C"
