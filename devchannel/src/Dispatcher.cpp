#include "pixie/dev/Dispatcher.hpp"
#include "Session.hpp"
#include "DevError.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <atomic>
#include <charconv>
#include <cmath>
#include <format>
#include <limits>
#include <set>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <vector>

namespace pixie::dev {
namespace {
using Json = nlohmann::json;
constexpr int kMaxItems = 65536;
constexpr int kMaxText = 1024 * 1024;
struct Failure : std::runtime_error {
    PixieResult result;
    bool possiblyApplied = false;
    explicit Failure(PixieResult code, std::string message, bool applied = false)
        : std::runtime_error(std::move(message)), result(code), possiblyApplied(applied) {}
};
[[noreturn]] void Reject(std::string message, PixieResult result = PIXIE_ERROR_ARGUMENT) {
    SetNativeError(result, message);
    throw Failure(result, std::move(message));
}
void Check(PixieResult result) {
    if (result == PIXIE_OK) return;
    const int size = pixie_last_error(nullptr, 0);
    std::vector<char> text(static_cast<std::size_t>(size) + 1);
    pixie_last_error(text.data(), static_cast<int>(text.size()));
    throw Failure(result, text.data());
}
std::string String(const Json& params, const char* name) {
    if (!params.contains(name) || !params[name].is_string()) Reject(std::format("{} must be a string", name));
    auto value = params[name].get<std::string>();
    if (value.find('\0') != std::string::npos) Reject("embedded NUL is not allowed");
    return value;
}
double Number(const Json& value) {
    if (!value.is_number()) Reject("expected a finite number");
    const double result = value.get<double>();
    if (!std::isfinite(result)) Reject("expected a finite number");
    return result;
}
double Number(const Json& p, const char* name) {
    if (!p.contains(name)) Reject(std::format("missing {}", name));
    return Number(p[name]);
}
int Integer(const Json& value, int minimum, int maximum) {
    const double number = Number(value);
    if (number != std::floor(number) || number < minimum || number > maximum) Reject("integer outside command bounds");
    return static_cast<int>(number);
}
bool Boolean(const Json& p, const char* name) {
    if (!p.contains(name) || !p[name].is_boolean()) Reject(std::format("{} must be boolean", name));
    return p[name].get<bool>();
}
uint64_t ParseId(const Json& value) {
    if (!value.is_string()) Reject("entity id must be a 16 digit hexadecimal scene ID");
    const auto text = value.get<std::string>();
    uint64_t id = 0;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), id, 16);
    if (text.size() != 16 || parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size() || id == 0)
        Reject("entity id must be a nonzero 16 digit hexadecimal scene ID");
    return id;
}
std::string IdText(uint64_t id) { return std::format("{:016x}", id); }
template<class Read> std::string Text(Read&& read) {
    // Metadata/name APIs do not measure; fail explicitly rather than truncate.
    std::vector<char> buffer(kMaxText);
    Check(read(buffer.data(), static_cast<int>(buffer.size())));
    return buffer.data();
}
Json Error(const Json& id, int rpc, const Failure& error, uint64_t sequence, uint64_t generation) {
    return {{"jsonrpc", "2.0"}, {"id", id}, {"error", {{"code", rpc}, {"message", error.what()},
        {"data", {{"pixie_result", error.result}, {"pixie_last_error", error.what()},
            {"possibly_applied", error.possiblyApplied}, {"sequence", sequence}, {"generation", generation}}}}}};
}
} // namespace

struct WorldDispatcher::Impl {
    // This view never calls Stop or pixie_session_destroy. It owns no World/module.
    PixieSession view;
    std::thread::id owner = std::this_thread::get_id();
    bool hello = false;
    bool mutationStarted = false;
    uint64_t sequence = 0;
    uint64_t generation = 0;
    uint64_t nextToken = 0;
    std::string nonce;
    struct Snapshot {
        std::string text;
        std::vector<uint64_t> ids;
        std::set<uint32_t> handles;
        uint64_t revision = 0;
    };
    std::unordered_map<std::string, Snapshot> snapshots;
    explicit Impl(ProjectSession& session) : view(session) {
        static std::atomic<uint64_t> nextSession{0};
        nonce = std::format("session-{}", ++nextSession);
    }
    PixieSession* S() { return &view; }
    uint32_t Find(uint64_t id) {
        uint32_t entity = 0;
        Check(pixie_entity_find(S(), id, &entity));
        return entity;
    }
    uint32_t Entity(const Json& p) {
        if (!p.contains("id")) Reject("missing scene id");
        return Find(ParseId(p["id"]));
    }
    Json SceneId(uint32_t entity) {
        uint64_t id = 0;
        const auto result = pixie_entity_scene_id(S(), entity, &id);
        if (result == PIXIE_ERROR_NOT_FOUND) return nullptr;
        Check(result);
        return IdText(id);
    }
    int Type(const std::string& name) {
        int type = 0;
        Check(pixie_type_find(S(), name.c_str(), &type));
        return type;
    }
    std::string TypeName(int type) {
        return Text([&](char* b, int n) { return pixie_type_name(S(), type, b, n); });
    }
    std::string FieldName(int type, int field) {
        return Text([&](char* b, int n) { return pixie_field_name(S(), type, field, b, n); });
    }
    int Field(int type, const std::string& name) {
        for (int i = 0; i < pixie_field_count(S(), type); ++i) if (FieldName(type, i) == name) return i;
        Reject("unknown named field: " + name, PIXIE_ERROR_NOT_FOUND);
    }
    PixieFieldInfo Info(int type, int field) {
        PixieFieldInfo info{};
        Check(pixie_field_info(S(), type, field, &info));
        return info;
    }
    Json Schema(int type) {
        Json fields = Json::array();
        for (int f = 0; f < pixie_field_count(S(), type); ++f) {
            const auto info = Info(type, f);
            auto text = [&](int which) { return Text([&](char* b, int n) { return pixie_field_text(S(), type, f, which, b, n); }); };
            Json enums = Json::array();
            for (int e = 0; e < info.enumCount; ++e)
                enums.push_back(Text([&](char* b, int n) { return pixie_field_enum_name(S(), type, f, e, b, n); }));
            fields.push_back({{"name", FieldName(type, f)}, {"kind", info.kind}, {"editor", info.editor},
                {"read_only", (info.flags & PIXIE_FIELD_READ_ONLY) != 0}, {"label", text(PIXIE_TEXT_LABEL)},
                {"tooltip", text(PIXIE_TEXT_TOOLTIP)}, {"help", text(PIXIE_TEXT_HELP)}, {"order", info.order},
                {"range", (info.flags & PIXIE_FIELD_HAS_RANGE) ? Json{{"min", info.rangeMin}, {"max", info.rangeMax}, {"step", info.rangeStep}} : Json(nullptr)},
                {"unit", (info.flags & PIXIE_FIELD_HAS_UNIT) ? Json{{"factor", info.unitFactor}, {"offset", info.unitOffset}, {"suffix", text(PIXIE_TEXT_UNIT_SUFFIX)}} : Json(nullptr)},
                {"enums", enums}, {"asset", text(PIXIE_TEXT_ASSET)}, {"references", text(PIXIE_TEXT_REFERENCES)}});
        }
        return {{"name", TypeName(type)}, {"flags", pixie_type_flags(S(), type)}, {"fields", fields}};
    }
    struct Address { uint32_t entity; int type; int field; int list = -1; int index = -1; int metaType; };
    Address Resolve(const Json& p) {
        const auto name = String(p, "field");
        const auto dot = name.find('.');
        if (dot == std::string::npos || dot == 0 || dot + 1 == name.size()) Reject("field must be Type.member");
        const int type = Type(name.substr(0, dot));
        Address a{Entity(p), type, Field(type, name.substr(dot + 1)), -1, -1, type};
        if (p.contains("index") || p.contains("member")) {
            if (!p.contains("index")) Reject("record member requires item index");
            int itemType = -1, count = 0;
            Check(pixie_record_info(S(), a.entity, a.type, a.field, &itemType, &count));
            a.index = Integer(p["index"], 0, count - 1);
            a.list = a.field;
            a.metaType = itemType;
            a.field = Field(itemType, String(p, "member"));
        }
        return a;
    }
    Json Decode(const PixieValue& value, int kind) {
        switch (kind) {
        case PIXIE_FIELD_BOOL: return value.x != 0;
        case PIXIE_FIELD_ENTITY: return value.x == 4294967295.0 ? Json(nullptr) : SceneId(static_cast<uint32_t>(value.x));
        case PIXIE_FIELD_POINT: case PIXIE_FIELD_VECTOR: return Json::array({value.x, value.y});
        case PIXIE_FIELD_RECT: case PIXIE_FIELD_COLOR: return Json::array({value.x, value.y, value.z, value.w});
        default: return value.x;
        }
    }
    PixieValue Encode(const Json& value, const PixieFieldInfo& info) {
        PixieValue v{};
        switch (info.kind) {
        case PIXIE_FIELD_BOOL:
            if (!value.is_boolean()) Reject("boolean field requires boolean");
            v.x = value.get<bool>() ? 1 : 0; break;
        case PIXIE_FIELD_ENTITY: v.x = value.is_null() ? 4294967295.0 : Find(ParseId(value)); break;
        case PIXIE_FIELD_POINT: case PIXIE_FIELD_VECTOR: case PIXIE_FIELD_RECT: case PIXIE_FIELD_COLOR: {
            const int count = info.kind == PIXIE_FIELD_POINT || info.kind == PIXIE_FIELD_VECTOR ? 2 : 4;
            if (!value.is_array() || value.size() != static_cast<std::size_t>(count)) Reject("wrong vector arity");
            double* parts[] = {&v.x, &v.y, &v.z, &v.w};
            for (int i = 0; i < count; ++i) {
                *parts[i] = Number(value[i]);
                if (info.kind == PIXIE_FIELD_COLOR && (*parts[i] < 0 || *parts[i] > 255 || std::floor(*parts[i]) != *parts[i]))
                    Reject("color channels must be integers from 0 to 255");
            }
            break;
        }
        default:
            v.x = Number(value);
            if (info.kind == PIXIE_FIELD_FLOAT && std::abs(v.x) > std::numeric_limits<float>::max()) Reject("float overflow");
            // Native INT metadata does not expose its signedness/width. Use the safe signed32
            // wire domain, except the explicit native unsigned LayerMask editor contract.
            if (info.kind == PIXIE_FIELD_INT || info.kind == PIXIE_FIELD_ENUM) {
                const double min = info.editor == PIXIE_EDITOR_LAYER_MASK ? 0 : std::numeric_limits<int32_t>::min();
                const double max = info.editor == PIXIE_EDITOR_LAYER_MASK ? 4294967295.0 : std::numeric_limits<int32_t>::max();
                if (v.x < min || v.x > max || std::floor(v.x) != v.x) Reject("integer outside safe wire domain");
                if (info.kind == PIXIE_FIELD_ENUM && (v.x < 0 || v.x >= info.enumCount)) Reject("unknown enum value");
            }
        }
        return v;
    }
    Json Read(Address a) {
        const auto info = Info(a.metaType, a.field);
        if (info.kind == PIXIE_FIELD_TEXT) {
            int length = 0;
            auto get = [&](char* b, int n) { return a.list < 0
                ? pixie_field_get_text(S(), a.entity, a.type, a.field, b, n, &length)
                : pixie_record_get_text(S(), a.entity, a.type, a.list, a.index, a.field, b, n, &length); };
            Check(get(nullptr, 0));
            if (length < 0 || length >= kMaxText) Reject("text exceeds read bound", PIXIE_ERROR_BUFFER);
            std::vector<char> text(static_cast<std::size_t>(length) + 1);
            Check(get(text.data(), static_cast<int>(text.size())));
            return std::string(text.data());
        }
        if (info.kind == PIXIE_FIELD_POINT_LIST) {
            int count = 0;
            auto get = [&](double* b, int n) { return a.list < 0
                ? pixie_field_get_points(S(), a.entity, a.type, a.field, b, n, &count)
                : pixie_record_get_points(S(), a.entity, a.type, a.list, a.index, a.field, b, n, &count); };
            Check(get(nullptr, 0));
            if (count < 0 || count > kMaxItems) Reject("point list exceeds read bound", PIXIE_ERROR_BUFFER);
            std::vector<double> xy(static_cast<std::size_t>(count) * 2);
            Check(get(xy.data(), count));
            Json points = Json::array();
            for (int i = 0; i < count; ++i) points.push_back({xy[2 * i], xy[2 * i + 1]});
            return points;
        }
        if (info.kind == PIXIE_FIELD_RECORD_LIST) {
            if (a.list >= 0) Reject("nested records are not supported");
            int type = -1, count = 0;
            Check(pixie_record_info(S(), a.entity, a.type, a.field, &type, &count));
            Json items = Json::array();
            for (int i = 0; i < count; ++i) {
                Json item = Json::object();
                for (int f = 0; f < pixie_field_count(S(), type); ++f)
                    item[FieldName(type, f)] = Read({a.entity, a.type, f, a.field, i, type});
                items.push_back(item);
            }
            return items;
        }
        PixieValue value{};
        Check(a.list < 0 ? pixie_field_get(S(), a.entity, a.type, a.field, &value)
            : pixie_record_get(S(), a.entity, a.type, a.list, a.index, a.field, &value));
        return Decode(value, info.kind);
    }
    void Write(Address a, const Json& value) {
        const auto info = Info(a.metaType, a.field);
        if ((info.flags & PIXIE_FIELD_READ_ONLY) || (a.list >= 0 && (Info(a.type, a.list).flags & PIXIE_FIELD_READ_ONLY))) Reject("field is read only");
        if (info.kind == PIXIE_FIELD_TEXT) {
            Json p{{"value", value}};
            const auto text = String(p, "value");
            mutationStarted = true;
            Check(a.list < 0 ? pixie_field_set_text(S(), a.entity, a.type, a.field, text.c_str())
                : pixie_record_set_text(S(), a.entity, a.type, a.list, a.index, a.field, text.c_str()));
        } else if (info.kind == PIXIE_FIELD_POINT_LIST) {
            if (!value.is_array() || value.size() > kMaxItems) Reject("invalid point list");
            std::vector<double> xy;
            for (const auto& p : value) {
                if (!p.is_array() || p.size() != 2) Reject("point requires two numbers");
                xy.push_back(Number(p[0])); xy.push_back(Number(p[1]));
            }
            mutationStarted = true;
            Check(a.list < 0 ? pixie_field_set_points(S(), a.entity, a.type, a.field, xy.data(), static_cast<int>(value.size()))
                : pixie_record_set_points(S(), a.entity, a.type, a.list, a.index, a.field, xy.data(), static_cast<int>(value.size())));
        } else {
            if (info.kind == PIXIE_FIELD_RECORD_LIST) Reject("use record_insert/record_remove and named member set");
            const auto v = Encode(value, info);
            mutationStarted = true;
            Check(a.list < 0 ? pixie_field_set(S(), a.entity, a.type, a.field, &v)
                : pixie_record_set(S(), a.entity, a.type, a.list, a.index, a.field, &v));
        }
    }
    std::vector<uint32_t> Entities() {
        int count = 0;
        Check(pixie_entities(S(), nullptr, 0, &count));
        if (count < 0 || count > kMaxItems) Reject("entity enumeration exceeds bound", PIXIE_ERROR_BUFFER);
        std::vector<uint32_t> entities(static_cast<std::size_t>(count));
        Check(pixie_entities(S(), entities.data(), count, &count));
        return entities;
    }
    Json Describe(uint32_t entity, bool components) {
        Json result{{"id", SceneId(entity)}, {"addressable", false},
            {"name", Text([&](char* b, int n) { return pixie_entity_name(S(), entity, b, n); })}};
        result["addressable"] = !result["id"].is_null();
        if (components) {
            Json data = Json::object();
            for (int t = 0; t < pixie_type_count(S()); ++t) if (pixie_entity_has(S(), entity, t)) {
                Json fields = Json::object();
                for (int f = 0; f < pixie_field_count(S(), t); ++f) fields[FieldName(t, f)] = Read({entity, t, f, -1, -1, t});
                data[TypeName(t)] = fields;
            }
            result["components"] = data;
        }
        return result;
    }
    Json Stats() {
        PixieStats stats{};
        Check(pixie_session_stats(S(), &stats));
        return {{"steps", stats.steps}, {"elapsed", stats.elapsed}, {"paused", stats.paused != 0},
            {"time_scale", stats.timeScale}, {"entities", stats.entities},
            {"mode", pixie_session_mode(S()) == PIXIE_MODE_EDIT ? "edit" : "play"},
            {"unsaved", pixie_project_unsaved(S()) != 0}};
    }
    std::string TakeSnapshot(const std::vector<uint32_t>& entities) {
        int length = 0;
        Check(pixie_entities_snapshot(S(), entities.data(), static_cast<int>(entities.size()), nullptr, 0, &length));
        if (length < 0 || length >= kMaxText) Reject("snapshot exceeds 1 MiB bound", PIXIE_ERROR_BUFFER);
        std::vector<char> buffer(static_cast<std::size_t>(length) + 1);
        Check(pixie_entities_snapshot(S(), entities.data(), static_cast<int>(entities.size()), buffer.data(), static_cast<int>(buffer.size()), &length));
        return buffer.data();
    }
    bool ReferencesAny(Address a, const std::set<uint32_t>& targets) {
        const auto info = Info(a.metaType, a.field);
        if (info.kind == PIXIE_FIELD_ENTITY) {
            PixieValue value{};
            Check(a.list < 0 ? pixie_field_get(S(), a.entity, a.type, a.field, &value)
                : pixie_record_get(S(), a.entity, a.type, a.list, a.index, a.field, &value));
            return targets.contains(static_cast<uint32_t>(value.x));
        }
        if (info.kind == PIXIE_FIELD_RECORD_LIST) {
            if (a.list >= 0) Reject("restore cannot preflight external references in nested records");
            int type = -1, count = 0;
            Check(pixie_record_info(S(), a.entity, a.type, a.field, &type, &count));
            for (int i = 0; i < count; ++i)
                for (int f = 0; f < pixie_field_count(S(), type); ++f)
                    if (ReferencesAny({a.entity, a.type, f, a.field, i, type}, targets)) return true;
        }
        return false;
    }
    Json Restore(const std::string& token) {
        const auto found = snapshots.find(token);
        if (found == snapshots.end()) Reject("unknown snapshot token for this dispatcher", PIXIE_ERROR_NOT_FOUND);
        auto& snapshot = found->second;
        if (snapshot.revision != view.session.WorldRevision()) Reject("snapshot expired after World rebuild; take a new snapshot");
        std::vector<uint32_t> alive;
        for (auto id : snapshot.ids) {
            uint32_t entity = 0;
            const auto status = pixie_entity_find(S(), id, &entity);
            if (status == PIXIE_OK) alive.push_back(entity);
            else if (status != PIXIE_ERROR_NOT_FOUND) Check(status);
        }
        // Replacing B changes its runtime handle. References inside the snapshot are remapped
        // by native restore; reject incoming references from outside it before touching B.
        const std::set<uint32_t> replaced(alive.begin(), alive.end());
        std::set<uint32_t> referencedHandles = snapshot.handles;
        referencedHandles.insert(alive.begin(), alive.end());
        if (referencedHandles.size() + snapshot.ids.size() > kMaxItems) Reject("snapshot handle history exhausted; take a fresh token", PIXIE_ERROR_BUFFER);
        for (auto entity : Entities()) if (!replaced.contains(entity))
            for (int t = 0; t < pixie_type_count(S()); ++t) if (pixie_entity_has(S(), entity, t))
                for (int f = 0; f < pixie_field_count(S(), t); ++f)
                    if (ReferencesAny({entity, t, f, -1, -1, t}, referencedHandles))
                        Reject("restore would invalidate an incoming Entity reference; include its owner in the snapshot");
        const auto rollback = TakeSnapshot(alive); // All validation/journaling precedes destruction.
        mutationStarted = true;
        ++generation;
        for (auto entity : alive) Check(pixie_entity_destroy(S(), entity));
        int count = 0;
        std::vector<uint32_t> restored(snapshot.ids.size());
        snapshot.handles = std::move(referencedHandles);
        const auto status = pixie_entities_restore(S(), snapshot.text.c_str(), PIXIE_RESTORE_SAME_IDS, restored.data(), static_cast<int>(restored.size()), &count);
        if (status != PIXIE_OK) {
            std::string reason;
            try { Check(status); } catch (const Failure& e) { reason = e.what(); }
            // Descriptor constructors can throw after native restore has created entities.
            // Remove only selected scene IDs from this failed attempt before rollback.
            for (auto id : snapshot.ids) {
                uint32_t partial = 0;
                if (pixie_entity_find(S(), id, &partial) == PIXIE_OK) {
                    snapshot.handles.insert(partial);
                    const auto cleanup = pixie_entity_destroy(S(), partial);
                    if (cleanup != PIXIE_OK) {
                        try { Check(cleanup); } catch (const Failure& e) { reason += std::string("; partial cleanup failed: ") + e.what(); }
                    }
                }
            }
            const auto rollbackStatus = pixie_entities_restore(S(), rollback.c_str(), PIXIE_RESTORE_SAME_IDS, restored.data(), static_cast<int>(restored.size()), &count);
            if (rollbackStatus == PIXIE_OK) snapshot.handles.insert(restored.begin(), restored.begin() + count);
            if (rollbackStatus != PIXIE_OK) {
                try { Check(rollbackStatus); } catch (const Failure& e) { reason += std::string("; rollback failed: ") + e.what(); }
            } else reason += "; prior live entities restored (runtime handles changed)";
            SetNativeError(status, reason);
            throw Failure(status, reason, true);
        }
        snapshot.handles.insert(restored.begin(), restored.begin() + count);
        return {{"restored", count}, {"token", token}, {"stats", Stats()}};
    }
    Json Execute(const std::string& method, const Json& p) {
        if (method == "world.hello") {
            hello = true;
            return {{"protocol_version", 1}, {"pixie_api_version", pixie_api_version()}, {"session", nonce},
                {"project", Text([&](char* b, int n) { return pixie_project_name(S(), b, n); })}, {"stats", Stats()},
                {"capabilities", {{"world", true}, {"inspector", false}, {"capture", false}, {"transport", false}, {"save", false}}}};
        }
        if (!hello) Reject("world.hello must be the first call");
        if (method == "world.stats") return Stats();
        if (method == "world.types") {
            Json types = Json::array();
            for (int t = 0; t < pixie_type_count(S()); ++t) types.push_back(TypeName(t));
            return types;
        }
        if (method == "world.type") return Schema(Type(String(p, "name")));
        if (method == "world.entities") {
            Json entities = Json::array();
            int unaddressable = 0;
            for (auto e : Entities()) {
                auto item = Describe(e, false);
                if (item["id"].is_null()) ++unaddressable;
                else entities.push_back(std::move(item));
            }
            return {{"entities", entities}, {"unaddressable_count", unaddressable}};
        }
        if (method == "world.entity") return Describe(Entity(p), true);
        if (method == "world.get") return {{"value", Read(Resolve(p))}};
        if (method == "world.set") {
            const auto a = Resolve(p);
            if (!p.contains("value")) Reject("missing field value");
            Write(a, p["value"]);
            return {{"value", Read(a)}, {"stats", Stats()}};
        }
        if (method == "world.snapshot") {
            if (!p.contains("ids") || !p["ids"].is_array() || p["ids"].empty() || p["ids"].size() > 1024) Reject("snapshot needs 1..1024 scene IDs");
            if (snapshots.size() >= 16) Reject("snapshot token capacity reached; release a token first", PIXIE_ERROR_BUFFER);
            Snapshot snapshot;
            snapshot.revision = view.session.WorldRevision();
            std::vector<uint32_t> entities;
            std::set<uint64_t> unique;
            for (const auto& value : p["ids"]) {
                const auto id = ParseId(value);
                if (!unique.insert(id).second) Reject("duplicate snapshot id");
                entities.push_back(Find(id)); snapshot.ids.push_back(id);
            }
            snapshot.text = TakeSnapshot(entities);
            snapshot.handles.insert(entities.begin(), entities.end());
            std::size_t total = snapshot.text.size();
            for (const auto& entry : snapshots) total += entry.second.text.size();
            if (total > 4 * 1024 * 1024) Reject("snapshot store exceeds 4 MiB", PIXIE_ERROR_BUFFER);
            const auto token = std::format("{}-snapshot-{}", nonce, ++nextToken);
            snapshots.emplace(token, std::move(snapshot));
            return {{"token", token}};
        }
        if (method == "world.restore") return Restore(String(p, "token"));
        if (method == "world.release_snapshot") return {{"released", snapshots.erase(String(p, "token")) != 0}};
        if (method == "world.create") {
            const auto name = p.contains("name") ? String(p, "name") : std::string{};
            uint32_t entity = 0;
            mutationStarted = true;
            Check(pixie_entity_create(S(), name.c_str(), &entity)); ++generation;
            return Describe(entity, true);
        }
        if (method == "world.destroy" || method == "world.rename" || method == "world.add_component" || method == "world.remove_component") {
            const auto entity = Entity(p);
            const auto name = method == "world.rename" ? String(p, "name") : std::string{};
            const int type = method == "world.add_component" || method == "world.remove_component" ? Type(String(p, "type")) : -1;
            mutationStarted = true;
            if (method == "world.destroy") Check(pixie_entity_destroy(S(), entity));
            else if (method == "world.rename") Check(pixie_entity_set_name(S(), entity, name.c_str()));
            else if (method == "world.add_component") Check(pixie_component_add(S(), entity, type));
            else Check(pixie_component_remove(S(), entity, type));
            ++generation;
            return {{"stats", Stats()}};
        }
        if (method == "world.record_insert" || method == "world.record_remove") {
            const auto a = Resolve(p);
            if (a.list >= 0) Reject("record editing needs a list field, without member/index");
            if (Info(a.type, a.field).flags & PIXIE_FIELD_READ_ONLY) Reject("record list is read only");
            if (!p.contains("item")) Reject("missing item index");
            const int index = Integer(p["item"], 0, 256);
            mutationStarted = true;
            Check(pixie_record_edit(S(), a.entity, a.type, a.field, index, method == "world.record_remove" ? 1 : 0));
            ++generation;
            return {{"value", Read(a)}};
        }
        if (method == "world.key" || method == "world.mouse_button" || method == "world.cursor" || method == "world.scroll") {
            if (method == "world.key" || method == "world.mouse_button") {
                const bool key = method == "world.key";
                const auto code = key ? "key" : "button";
                if (!p.contains(code)) Reject("missing input code");
                const int value = Integer(p[code], key ? 32 : 0, key ? 348 : 7);
                const bool down = Boolean(p, "down");
                mutationStarted = true;
                Check(key ? pixie_input_key(S(), value, down ? 1 : 0) : pixie_input_mouse_button(S(), value, down ? 1 : 0));
            } else {
                const double x = Number(p, method == "world.scroll" ? "dx" : "x");
                const double y = Number(p, method == "world.scroll" ? "dy" : "y");
                mutationStarted = true;
                Check(method == "world.scroll" ? pixie_input_scroll(S(), x, y) : pixie_input_cursor(S(), x, y));
            }
            return {{"queued_for_step", true}};
        }
        if (method == "world.view_to_world" || method == "world.world_to_view") {
            double x = 0, y = 0;
            const auto px = Number(p, "x"), py = Number(p, "y");
            Check(method == "world.view_to_world" ? pixie_view_to_world(S(), px, py, &x, &y) : pixie_world_to_view(S(), px, py, &x, &y));
            return {{"x", x}, {"y", y}};
        }
        if (method == "world.pick") {
            uint32_t entity = 0;
            const double tolerance = p.contains("tolerance") ? Number(p, "tolerance") : 0;
            if (tolerance < 0) Reject("negative pick tolerance");
            const auto result = pixie_pick(S(), Number(p, "x"), Number(p, "y"), tolerance, &entity);
            if (result == PIXIE_ERROR_NOT_FOUND) return {{"id", nullptr}};
            Check(result);
            return Describe(entity, false);
        }
        if (method == "world.pick_box") {
            const double x0 = Number(p, "x0"), y0 = Number(p, "y0"), x1 = Number(p, "x1"), y1 = Number(p, "y1");
            int count = 0;
            Check(pixie_pick_box(S(), x0, y0, x1, y1, nullptr, 0, &count));
            if (count < 0 || count > kMaxItems) Reject("pick result exceeds bound", PIXIE_ERROR_BUFFER);
            std::vector<uint32_t> entities(static_cast<std::size_t>(count));
            Check(pixie_pick_box(S(), x0, y0, x1, y1, entities.data(), count, &count));
            Json result = Json::array();
            for (auto entity : entities) result.push_back(Describe(entity, false));
            return result;
        }
        if (method == "world.step" || method == "world.advance" || method == "world.time_scale" ||
            method == "world.play" || method == "world.stop" || method == "world.pause" || method == "world.resume" || method == "world.reset") {
            const auto before = Stats();
            if (method == "world.step") {
                const int count = p.contains("count") ? Integer(p["count"], 0, 10000) : 1;
                mutationStarted = true;
                for (int i = 0; i < count; ++i) Check(pixie_session_step(S()));
            } else if (method == "world.advance") {
                const double seconds = Number(p, "seconds");
                if (seconds < 0 || seconds > 10) Reject("advance seconds outside 0..10");
                mutationStarted = true;
                // FixedStepper clamps scaled frame time at .25s. Chunk before scaling to
                // preserve requested duration at every accepted native time scale.
                const double chunk = 0.125 / before["time_scale"].get<double>();
                double remaining = seconds;
                while (remaining > 0) {
                    const double delta = std::min(chunk, remaining);
                    Check(pixie_session_advance(S(), delta)); remaining -= delta;
                }
            } else if (method == "world.time_scale") {
                const double scale = Number(p, "value");
                if (scale < ProjectSession::kMinTimeScale || scale > ProjectSession::kMaxTimeScale) Reject("time scale outside native range .01..8");
                mutationStarted = true;
                Check(pixie_session_set_time_scale(S(), scale));
            } else {
                mutationStarted = true;
                if (method == "world.pause" || method == "world.resume") Check(pixie_session_set_paused(S(), method == "world.pause" ? 1 : 0));
                else {
                    ++generation;
                    if (method == "world.play") Check(pixie_session_play(S()));
                    else if (method == "world.stop") Check(pixie_session_stop(S()));
                    else Check(pixie_session_reset(S()));
                }
            }
            const auto after = Stats();
            return {{"stats", after}, {"steps_run", after["steps"].get<uint64_t>() >= before["steps"].get<uint64_t>() ? after["steps"].get<uint64_t>() - before["steps"].get<uint64_t>() : 0}};
        }
        Reject("unsupported World method (save, filesystem and host capture are disabled): " + method, PIXIE_ERROR_NOT_FOUND);
    }
};

WorldDispatcher::WorldDispatcher(ProjectSession& session) : impl_(std::make_unique<Impl>(session)) {}
WorldDispatcher::~WorldDispatcher() = default;

std::string WorldDispatcher::Dispatch(std::string_view request) {
    // Rejection here must not race with any owner-thread dispatcher state.
    if (std::this_thread::get_id() != impl_->owner) {
        const Failure e(PIXIE_ERROR_ARGUMENT, "development dispatch requires the owner thread");
        SetNativeError(e.result, e.what());
        return Error(nullptr, -32600, e, 0, 0).dump();
    }
    Json id = nullptr;
    bool notification = false;
    int rpc = -32600;
    impl_->mutationStarted = false;
    try {
        if (request.size() > kMaxRequestBytes) Reject("development request exceeds 64 KiB");
        // Bound nesting before the JSON parser recurses. Count only outside strings.
        int depth = 0;
        bool inString = false, escape = false;
        for (const char c : request) {
            if (escape) { escape = false; continue; }
            if (inString && c == '\\') { escape = true; continue; }
            if (c == '"') inString = !inString;
            if (!inString && (c == '{' || c == '[') && ++depth > 32) Reject("JSON nesting exceeds 32");
            if (!inString && (c == '}' || c == ']')) --depth;
        }
        rpc = -32700;
        const auto input = Json::parse(request, nullptr, false);
        if (input.is_discarded()) Reject("invalid JSON");
        rpc = -32600;
        if (!input.is_object() || input.value("jsonrpc", Json(nullptr)) != "2.0" || !input.contains("method") || !input["method"].is_string()) Reject("invalid JSON-RPC 2.0 request");
        if (input.contains("id")) {
            if (!input["id"].is_string() && !input["id"].is_number_integer() && !input["id"].is_null()) Reject("invalid JSON-RPC id");
            id = input["id"];
        } else notification = true;
        ++impl_->sequence;
        rpc = -32602;
        const auto params = input.value("params", Json::object());
        if (!params.is_object()) Reject("params must be an object");
        if (!impl_->view.session.GetTelemetry()) Reject("session has not started");
        const auto method = String(input, "method");
        rpc = -32000;
        if (!method.starts_with("world.")) { rpc = -32601; Reject("player dispatcher supports only world.*", PIXIE_ERROR_NOT_FOUND); }
        auto value = impl_->Execute(method, params);
        Json response{{"jsonrpc", "2.0"}, {"id", id}, {"result", {{"value", value}, {"session", impl_->nonce},
            {"sequence", impl_->sequence}, {"generation", impl_->generation}}}};
        auto output = response.dump();
        if (output.size() >= static_cast<std::size_t>(kResponseCapacity))
            throw Failure(PIXIE_ERROR_BUFFER, "response exceeds 4 MiB; do not retry mutations", impl_->mutationStarted);
        return notification ? std::string{} : output;
    } catch (const Failure& failure) {
        auto e = failure;
        e.possiblyApplied = e.possiblyApplied || impl_->mutationStarted;
        SetNativeError(e.result, e.what());
        return notification ? std::string{} : Error(id, rpc, e, impl_->sequence, impl_->generation).dump();
    } catch (const std::exception& exception) {
        const Failure e(PIXIE_ERROR_INTERNAL, exception.what(), impl_->mutationStarted);
        SetNativeError(e.result, e.what());
        return notification ? std::string{} : Error(id, -32603, e, impl_->sequence, impl_->generation).dump();
    }
}
} // namespace pixie::dev
