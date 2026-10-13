// The C API over a ProjectSession. Every exported function converts its arguments, runs inside
// Guard (so no exception crosses into the caller), and reports failures through a result code and
// the thread's last error.

#include "pixie_capi.h"

#include "pixie/GlLoader.hpp"
#include "pixie/runtime/Name.hpp"
#include "pixie/runtime/ProjectSession.hpp"

#include <algorithm>
#include <cstring>
#include <exception>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

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
    return Guard([&] {
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
        if (!created->session.Start({width, height})) {
            created->session.Stop();
            return Fail(PIXIE_ERROR_GL, "the renderers could not be created");
        }
        *session = created.release();
        return PIXIE_OK;
    });
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
    return Guard([&] { return Copy(s->session.GetProject().Name(), buffer, capacity); });
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
        return PIXIE_OK;
    });
}

} // extern "C"
