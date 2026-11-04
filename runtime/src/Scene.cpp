#include "pixie/runtime/Scene.hpp"
#include "SceneSave.hpp"
#include "PresentationJson.hpp"

#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/Lighting2D.hpp"
#include "pixie/runtime/Name.hpp"
#include "pixie/runtime/RigidBody2D.hpp"
#include "pixie/runtime/World.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <format>
#include <fstream>
#include <optional>
#include <random>
#include <set>
#include <span>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace pixie {
namespace detail {

// What a loaded entity had that this build does not know, kept as JSON text to be written back
// unchanged: {"entity": {key: value}, "components": {type: value}, "fields": {type: {field: value}}}.
struct SceneExtras {
    std::string json;
};

// Root and scope extras, plus the version that an ordinary save must retain.
struct SceneDocumentExtras {
    std::string json;
    int version = 1;
    std::unordered_map<std::uint64_t, std::string> scopes;
};

// The World's scene entities by scene ID, kept in the registry's context: built the first time a
// read needs it, and kept current from then on by hooks on SceneId, so a read does not scan the
// World. When two entities hold one ID (only when a game adds it by hand), one of them is found, as
// a scan finds one, and the index is built again after either goes.
struct SceneIndex {
    std::unordered_map<std::uint64_t, entt::entity> entities;
    // Some ID is held by more than one entity.
    bool repeated = false;
    // Built again before its next use: a SceneId was replaced, or a repeated one removed.
    bool stale = false;
};

} // namespace detail

namespace {

// Ordered, so a component's fields are written in declaration order and a loaded file's keys keep
// theirs.
using Json = nlohmann::ordered_json;

constexpr std::uint32_t kNoEntity = 0xFFFFFFFFu;

std::string HexId(std::uint64_t id) {
    return std::format("{:016x}", id);
}

bool ParseHexId(const Json& value, std::uint64_t& id) {
    if (!value.is_string()) {
        return false;
    }
    const std::string& text = value.get_ref<const std::string&>();
    if (text.empty() || text.size() > 16) {
        return false;
    }
    const char* end = text.data() + text.size();
    const auto [last, error] = std::from_chars(text.data(), end, id, 16);
    return error == std::errc() && last == end && id != 0;
}

std::string PathText(const std::filesystem::path& path) {
    const std::u8string text = path.u8string();
    return std::string(reinterpret_cast<const char*>(text.data()), text.size());
}

Json ParseKept(const std::string& text) {
    Json kept = Json::parse(text, nullptr, false);
    return kept.is_object() ? kept : Json::object();
}

// Adds every key of `from` that `to` does not have yet.
void Merge(Json& to, const Json& from) {
    if (!from.is_object()) {
        return;
    }
    for (auto it = from.begin(); it != from.end(); ++it) {
        if (!to.contains(it.key())) {
            to[it.key()] = it.value();
        }
    }
}

// A float as the shortest decimal that reads back as the same float, so 0.6f is saved as 0.6 and
// not as the double nearest to it.
Json FloatNumber(double value) {
    const float single = static_cast<float>(value);
    char buffer[32];
    const auto [end, error] = std::to_chars(buffer, buffer + sizeof buffer, single);
    double shortest = single;
    if (error == std::errc()) {
        std::from_chars(buffer, end, shortest);
    }
    return shortest;
}

bool Numbers(const Json& value, std::size_t count, double* out) {
    if (!value.is_array() || value.size() != count) {
        return false;
    }
    for (std::size_t i = 0; i < count; ++i) {
        if (!value[i].is_number()) {
            return false;
        }
        out[i] = value[i].get<double>();
    }
    return true;
}

Json Encode(const FieldDescriptor& field, const FieldValue& v, entt::registry& registry, std::string_view where,
            std::vector<std::string>* warnings) {
    switch (field.kind) {
    case FieldKind::Bool:
        return v.x != 0.0;
    case FieldKind::Int:
        return static_cast<std::int64_t>(std::llround(v.x));
    case FieldKind::Float:
        return FloatNumber(v.x);
    case FieldKind::Double:
        return v.x;
    case FieldKind::Enum: {
        const long long index = std::llround(v.x);
        if (index >= 0 && static_cast<std::size_t>(index) < field.enumNames.size()) {
            return field.enumNames[static_cast<std::size_t>(index)];
        }
        return static_cast<std::int64_t>(index);
    }
    case FieldKind::Point:
    case FieldKind::Vector:
        return Json::array({v.x, v.y});
    case FieldKind::Rect:
        return Json::array({v.x, v.y, v.z, v.w});
    case FieldKind::Color:
        return Json::array({std::llround(v.x), std::llround(v.y), std::llround(v.z), std::llround(v.w)});
    case FieldKind::Entity: {
        const auto number = static_cast<std::uint32_t>(v.x);
        if (number == kNoEntity) {
            return nullptr;
        }
        const auto target = static_cast<entt::entity>(number);
        if (registry.valid(target)) {
            if (const SceneId* id = registry.try_get<SceneId>(target)) {
                return HexId(id->value);
            }
        }
        if (warnings != nullptr) {
            warnings->push_back(std::format("{}.{} refers to an entity that is not in the scene; saved as none",
                                            where, field.name));
        }
        return nullptr;
    }
    case FieldKind::Text:
    case FieldKind::PointList:
    case FieldKind::RecordList:
        // Written through readText or readPoints; never a FieldValue.
        return nullptr;
    }
    return nullptr;
}

// The entities an Entity field of a list may name, by scene ID: the list's own, under the IDs it was
// written with, so a reference inside the list names the new entity; then the World's.
struct SceneLookup {
    const std::unordered_map<std::uint64_t, entt::entity>& listed;
    const std::unordered_map<std::uint64_t, entt::entity>& world;

    const entt::entity* Find(std::uint64_t id) const {
        if (const auto found = listed.find(id); found != listed.end()) {
            return &found->second;
        }
        if (const auto found = world.find(id); found != world.end()) {
            return &found->second;
        }
        return nullptr;
    }
};

// Reads a field's value from the file into `out`; returns what is wrong with it, or nothing.
std::string Decode(const FieldDescriptor& field, const Json& value, const SceneLookup& ids, FieldValue& out) {
    switch (field.kind) {
    case FieldKind::Bool:
        if (!value.is_boolean()) {
            return "expected true or false";
        }
        out.x = value.get<bool>() ? 1.0 : 0.0;
        return {};
    case FieldKind::Int:
    case FieldKind::Float:
    case FieldKind::Double:
        if (!value.is_number()) {
            return "expected a number";
        }
        out.x = value.get<double>();
        return {};
    case FieldKind::Enum:
        if (value.is_string()) {
            const auto& names = field.enumNames;
            const auto found = std::find(names.begin(), names.end(), value.get_ref<const std::string&>());
            if (found == names.end()) {
                return std::format("{} is not one of its values", value.dump());
            }
            out.x = static_cast<double>(found - names.begin());
            return {};
        }
        if (!value.is_number_integer()) {
            return "expected the name of a value";
        }
        out.x = value.get<double>();
        return {};
    case FieldKind::Point:
    case FieldKind::Vector:
        return Numbers(value, 2, &out.x) ? std::string() : "expected [x, y]";
    case FieldKind::Rect:
        return Numbers(value, 4, &out.x) ? std::string() : "expected [x, y, width, height]";
    case FieldKind::Color:
        return Numbers(value, 4, &out.x) ? std::string() : "expected [red, green, blue, alpha]";
    case FieldKind::Entity: {
        if (value.is_null()) {
            out.x = kNoEntity;
            return {};
        }
        std::uint64_t id = 0;
        if (!ParseHexId(value, id)) {
            return "expected an entity's id or null";
        }
        const entt::entity* found = ids.Find(id);
        if (found == nullptr) {
            return std::format("no entity in the scene has the id {}", value.dump());
        }
        out.x = static_cast<double>(static_cast<std::uint32_t>(*found));
        return {};
    }
    case FieldKind::Text:
        return "expected text";
    case FieldKind::PointList:
        return "expected [[x, y], ...]";
    case FieldKind::RecordList:
        return "expected a record list";
    }
    return "unknown field kind";
}

const ComponentDescriptor& RecordType(const ComponentTypes& types, const FieldDescriptor& field) {
    for (const auto& type : types.All()) if (type.type == field.recordType) return type;
    throw std::invalid_argument("record item type is not registered");
}

Json EncodeField(const FieldDescriptor& field, void* component, const ComponentTypes& types,
                 entt::registry& registry, const std::string& where, std::vector<std::string>* warnings, int depth = 0) {
    if (depth > 8) throw std::invalid_argument("record nesting exceeds 8 levels");
    if (field.kind == FieldKind::Text) return field.readText(component);
    if (field.kind == FieldKind::PointList) {
        Json points = Json::array();
        for (const auto& p : field.readPoints(component)) points.push_back(Json::array({p.x, p.y}));
        return points;
    }
    if (field.kind == FieldKind::RecordList) {
        const auto& itemType = RecordType(types, field);
        if (field.recordCount(component) > FieldDescriptor::maxRecords) throw std::invalid_argument("record list exceeds 256 items");
        Json records = Json::array();
        for (std::size_t i = 0; i < field.recordCount(component); ++i) {
            Json item = Json::object();
            item["v"] = itemType.version;
            for (const auto& child : itemType.fields)
                item[child.name] = EncodeField(child, field.recordAt(component, i), types, registry,
                    std::format("{}.{}[{}]", where, field.name, i), warnings, depth + 1);
            const auto kept = ParseKept(field.recordExtras(component, i));
            if (kept.contains("v") && kept["v"].is_number_integer() && kept["v"].get<std::int64_t>() > itemType.version)
                item["v"] = kept["v"];
            Merge(item, kept);
            records.push_back(std::move(item));
        }
        return records;
    }
    return Encode(field, field.read(component), registry, where, warnings);
}

std::string DecodeField(const FieldDescriptor& field, const Json& saved, void* component,
                        const ComponentTypes& types, const SceneLookup& ids, int depth = 0) {
    if (depth > 8) return "record nesting exceeds 8 levels";
    if (field.kind == FieldKind::Text) {
        if (!saved.is_string()) return "expected text";
        field.writeText(component, saved.get_ref<const std::string&>());
        return {};
    }
    if (field.kind == FieldKind::PointList) {
        if (!saved.is_array()) return "expected [[x, y], ...]";
        std::vector<pxPoint> points;
        for (const auto& point : saved) {
            double xy[2];
            if (!Numbers(point, 2, xy)) return "expected [[x, y], ...]";
            points.emplace_back(xy[0], xy[1]);
        }
        field.writePoints(component, points);
        return {};
    }
    if (field.kind == FieldKind::RecordList) {
        if (!saved.is_array() || saved.size() > FieldDescriptor::maxRecords) return "expected at most 256 records";
        try {
            const auto& itemType = RecordType(types, field);
            field.replaceRecords(component, saved.size(), [&](void* item, std::string& extras, std::size_t i) {
                if (!saved[i].is_object()) throw std::invalid_argument("record must be an object");
                Json unknown = Json::object();
                for (auto child = saved[i].begin(); child != saved[i].end(); ++child) {
                    if (child.key() == "v") {
                        if (child.value().is_number_integer() && child.value().get<std::int64_t>() > itemType.version)
                            unknown["v"] = child.value();
                        continue;
                    }
                    const auto known = std::find_if(itemType.fields.begin(), itemType.fields.end(),
                        [&](const FieldDescriptor& f) { return f.name == child.key(); });
                    if (known == itemType.fields.end()) { unknown[child.key()] = child.value(); continue; }
                    if (auto error = DecodeField(*known, child.value(), item, types, ids, depth + 1); !error.empty())
                        throw std::invalid_argument(std::format("record {}.{}: {}", i, child.key(), error));
                }
                extras = unknown.empty() ? "" : unknown.dump();
            });
            return {};
        } catch (const std::invalid_argument& error) { return error.what(); }
    }
    FieldValue value;
    auto error = Decode(field, saved, ids, value);
    if (error.empty()) field.write(component, value);
    return error;
}

std::string Scalar(const Json& value) {
    return value.dump(-1, ' ', false, Json::error_handler_t::replace);
}

// Two-space indentation, with arrays of plain values on one line, so [x, y] stays readable.
void Pretty(const Json& value, std::string& out, int indent) {
    if (value.is_object() && !value.empty()) {
        out += "{\n";
        bool first = true;
        for (auto it = value.begin(); it != value.end(); ++it) {
            if (!first) {
                out += ",\n";
            }
            first = false;
            out.append(static_cast<std::size_t>(indent + 2), ' ');
            out += Scalar(Json(it.key()));
            out += ": ";
            Pretty(it.value(), out, indent + 2);
        }
        out += '\n';
        out.append(static_cast<std::size_t>(indent), ' ');
        out += '}';
    } else if (value.is_array() && !value.empty()) {
        const bool flat = std::none_of(value.begin(), value.end(), [](const Json& v) { return v.is_structured(); });
        if (flat) {
            out += '[';
            for (std::size_t i = 0; i < value.size(); ++i) {
                if (i > 0) {
                    out += ", ";
                }
                out += Scalar(value[i]);
            }
            out += ']';
            return;
        }
        out += "[\n";
        for (std::size_t i = 0; i < value.size(); ++i) {
            if (i > 0) {
                out += ",\n";
            }
            out.append(static_cast<std::size_t>(indent + 2), ' ');
            Pretty(value[i], out, indent + 2);
        }
        out += '\n';
        out.append(static_cast<std::size_t>(indent), ' ');
        out += ']';
    } else {
        out += Scalar(value);
    }
}

SceneReport Failed(std::string error) {
    SceneReport report;
    report.ok = false;
    report.error = std::move(error);
    return report;
}

} // namespace

std::uint64_t NewSceneId() {
    thread_local std::mt19937_64 generator = [] {
        std::random_device device;
        const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
        std::seed_seq seeds{device(), device(), device(), device(), static_cast<std::uint32_t>(now),
                            static_cast<std::uint32_t>(static_cast<std::uint64_t>(now) >> 32)};
        return std::mt19937_64(seeds);
    }();
    std::uint64_t id = 0;
    while (id == 0) {
        id = generator();
    }
    return id;
}

std::uint64_t AddToScene(Entity entity) {
    if (!entity.IsAlive()) {
        return 0;
    }
    if (const SceneId* id = entity.TryGet<SceneId>()) {
        return id->value;
    }
    return entity.Add<SceneId>(SceneId{NewSceneId()}).value;
}

namespace {

// One scene entity as the scene format writes it.
Json WriteEntity(entt::registry& registry, const ComponentTypes& types, entt::entity id,
                 std::vector<std::string>* warnings) {
    const std::string hex = HexId(registry.get<SceneId>(id).value);
    Json entity = Json::object();
    entity["id"] = hex;
    if (const Name* name = registry.try_get<Name>(id); name != nullptr && !name->value.empty()) {
        entity["name"] = name->value;
    }
    Json extras = Json::object();
    if (const auto* kept = registry.try_get<detail::SceneExtras>(id)) {
        extras = ParseKept(kept->json);
    }
    if (extras.contains("entity")) {
        Json keys = extras["entity"];
        if (keys.is_object()) {
            keys.erase("components");
        }
        Merge(entity, keys);
    }

    Json components = Json::object();
    for (const ComponentDescriptor& type : types.All()) {
        if (type.valueType) continue;
        void* component = type.find(registry, id);
        if (component == nullptr) {
            continue;
        }
        Json fields = Json::object();
        fields["v"] = type.version;
        const std::string where = std::format("{} of entity {}", type.name, hex);
        for (const FieldDescriptor& field : type.fields) {
            fields[field.name] = EncodeField(field, component, types, registry, where, warnings);
        }
        if (extras.contains("fields") && extras["fields"].is_object() && extras["fields"].contains(type.name)) {
            Merge(fields, extras["fields"][type.name]);
        }
        components[type.name] = std::move(fields);
    }
    if (extras.contains("components")) {
        Merge(components, extras["components"]);
    }
    entity["components"] = std::move(components);
    return entity;
}

std::string Finish(const Json& document) {
    std::string text;
    Pretty(document, text, 0);
    text += '\n';
    return text;
}

// Parses a document of `format` (a scene or a fragment, named by `what`), checks its version and its
// list of entities, and returns it; on failure, sets `error`.
Json ParseDocument(std::string_view text, std::string_view format, std::string_view what, std::string& error,
                   int maxVersion = kSceneVersion) {
    Json document;
    try {
        document = Json::parse(text.begin(), text.end());
    } catch (const Json::parse_error& e) {
        error = std::format("the {} is not valid JSON: {}", what, e.what());
        return {};
    }
    if (!document.is_object()) {
        error = std::format("the {} is not a JSON object", what);
    } else if (!document.contains("format") || document["format"] != std::string(format)) {
        error = std::format("not a {}: its format is not \"{}\"", what, format);
    } else if (!document.contains("version") || !document["version"].is_number_integer()) {
        error = std::format("the {} has no version", what);
    } else if (const auto version = document["version"].get<std::int64_t>(); version < 1) {
        error = std::format("the {}'s version, {}, is not valid", what, version);
    } else if (version > maxVersion) {
        error = std::format("the {} was written by a newer engine (version {}; this one reads up to {})", what,
                            version, maxVersion);
    } else if (!document.contains("entities") || !document["entities"].is_array()) {
        error = std::format("the {} has no list of entities", what);
    }
    return document;
}

// A document's meta object as compact JSON text; empty when it has none, and when it is not an
// object (with a warning).
std::string MetaText(const Json& document, std::vector<std::string>& warnings) {
    const auto meta = document.find("meta");
    if (meta == document.end()) {
        return {};
    }
    if (!meta->is_object()) {
        warnings.push_back("the meta is not a JSON object; ignored");
        return {};
    }
    return meta->dump(-1, ' ', false, Json::error_handler_t::replace);
}

void BuildSceneIndex(entt::registry& registry, detail::SceneIndex& index) {
    index.entities.clear();
    index.entities.reserve(registry.storage<SceneId>().size());
    index.repeated = false;
    index.stale = false;
    for (const auto [id, sceneId] : registry.view<const SceneId>().each()) {
        if (!index.entities.try_emplace(sceneId.value, id).second) {
            index.repeated = true;
        }
    }
}

void OnSceneIdAdded(entt::registry& registry, entt::entity id) {
    detail::SceneIndex& index = registry.ctx().get<detail::SceneIndex>();
    const auto [found, added] = index.entities.try_emplace(registry.get<SceneId>(id).value, id);
    if (!added && found->second != id) {
        index.repeated = true;
    }
}

// The ID it replaced is not known here.
void OnSceneIdReplaced(entt::registry& registry, entt::entity) {
    registry.ctx().get<detail::SceneIndex>().stale = true;
}

// Called while the entity still has its SceneId.
void OnSceneIdRemoved(entt::registry& registry, entt::entity id) {
    detail::SceneIndex& index = registry.ctx().get<detail::SceneIndex>();
    if (index.stale) {
        return;
    }
    if (const auto found = index.entities.find(registry.get<SceneId>(id).value);
        found != index.entities.end() && found->second == id) {
        index.entities.erase(found);
    }
    // Another entity may hold the ID it held.
    index.stale = index.repeated;
}

// The World's scene entities by scene ID (detail::SceneIndex), kept with the World; it changes as
// SceneIds are added, so an iterator into it is not kept across adding one.
const std::unordered_map<std::uint64_t, entt::entity>& SceneEntities(entt::registry& registry) {
    detail::SceneIndex* index = registry.ctx().find<detail::SceneIndex>();
    if (index == nullptr) {
        index = &registry.ctx().emplace<detail::SceneIndex>();
        registry.on_construct<SceneId>().connect<&OnSceneIdAdded>();
        registry.on_update<SceneId>().connect<&OnSceneIdReplaced>();
        registry.on_destroy<SceneId>().connect<&OnSceneIdRemoved>();
        index->stale = true;
    }
    if (index->stale) {
        BuildSceneIndex(registry, *index);
    }
    return index->entities;
}

// Checks a list of entities before anything is added: each an object with a valid id, used once in
// the list, and, when `againstWorld`, not already in the World; components, if any, an object.
// Returns what is wrong, or nothing, with the ids in `listed`.
std::string CheckEntities(const Json& list, const std::unordered_map<std::uint64_t, entt::entity>& world,
                          bool againstWorld, std::vector<std::uint64_t>& listed) {
    std::unordered_set<std::uint64_t> seen;
    listed.reserve(list.size());
    for (std::size_t i = 0; i < list.size(); ++i) {
        const Json& entry = list[i];
        if (!entry.is_object()) {
            return std::format("entity {} of the list is not an object", i);
        }
        std::uint64_t id = 0;
        if (!entry.contains("id") || !ParseHexId(entry["id"], id)) {
            return std::format("entity {} of the list has no valid id", i);
        }
        if (!seen.insert(id).second || (againstWorld && world.contains(id))) {
            return std::format("the id {} is used twice, or is already in the World", HexId(id));
        }
        if (entry.contains("components") && !entry["components"].is_object()) {
            return std::format("the components of entity {} are not an object", HexId(id));
        }
        listed.push_back(id);
    }
    return {};
}

// Reject registered project types before even installing the scene index or drawing new IDs.
// Unregistered component names remain opaque scene data and still round-trip.
std::string V2AdmissionError(const std::set<std::string>& unsupported) {
    if (unsupported.empty()) return {};
    std::string error = "version 2 import requires a declarative contract for registered components: ";
    for (const std::string& name : unsupported) {
        if (error.back() != ' ') error += ", ";
        error += name;
    }
    return error;
}

std::string CheckV2ComponentAdmission(const Json& list, const ComponentTypes& types) {
    std::set<std::string> unsupported;
    for (const Json& entity : list) {
        if (!entity.is_object() || !entity.contains("components") || !entity["components"].is_object()) continue;
        for (auto component = entity["components"].begin(); component != entity["components"].end(); ++component) {
            const int index = types.IndexOf(component.key());
            if (index >= 0 && !types.AllowsV2Import(index)) unsupported.insert(component.key());
        }
    }
    if (auto problem = V2AdmissionError(unsupported); !problem.empty()) return problem;
    for (const auto& entity : list) {
        if (!entity.is_object() || !entity.contains("components") || !entity["components"].is_object() || !entity["components"].contains("Camera2D")) continue;
        const auto& camera = entity["components"]["Camera2D"];
        if (!camera.is_object()) return "invalid Camera2D component";
        if (camera.contains("zoom") && (!camera["zoom"].is_number() || !std::isfinite(camera["zoom"].get<double>()) ||
            camera["zoom"].get<double>() < 0.0001 || camera["zoom"].get<double>() > 10000)) return "invalid authored camera zoom";
        if (camera.contains("enabled") && !camera["enabled"].is_boolean()) return "invalid authored camera enabled flag";
    }
    return {};
}

std::string CheckV2WorldAdmission(const World& world, const ComponentTypes& types) {
    std::set<std::string> unsupported;
    const entt::registry& registry = world.Registry();
    if (world.Presentation() && !registry.view<const LightingSettings2D>().empty())
        return "canonical presentation conflicts with legacy LightingSettings2D";
    for (const entt::entity entity : registry.view<SceneId>()) {
        for (std::size_t index = 0; index < types.All().size(); ++index) {
            if (types.AllowsV2Import(static_cast<int>(index))) continue;
            const ComponentDescriptor& type = types.All()[index];
            const auto* storage = registry.storage(type.type);
            if (storage != nullptr && storage->contains(entity)) unsupported.insert(type.name);
        }
    }
    return V2AdmissionError(unsupported);
}

// Legacy whole-scene reads retain their behavior. New fragment mutations must not add to, or
// operate through, an ambiguous settings authority.
std::string CheckFragmentSettings(const World& world, const Json& entities,
                                 const std::unordered_set<entt::entity>& excluded = {}) {
    if (world.Presentation()) {
        for (const auto& entity : entities) {
            if (entity.contains("components") && entity["components"].contains("LightingSettings2D"))
                return "legacy lighting conflicts with canonical presentation";
        }
    }
    const auto count = [&](std::string_view name, auto view) {
        std::size_t found = 0;
        for (const auto id : view) { if (!excluded.contains(id)) ++found; }
        for (const Json& entity : entities) {
            if (entity.contains("components") && entity["components"].contains(std::string(name)) &&
                entity["components"][std::string(name)].is_object()) {
                ++found;
            }
        }
        return found;
    };
    const entt::registry& registry = world.Registry();
    if (count("PhysicsSettings2D", registry.view<const PhysicsSettings2D>()) > 1) {
        return "a fragment would leave duplicate PhysicsSettings2D authorities";
    }
    if (count("LightingSettings2D", registry.view<const LightingSettings2D>()) > 1) {
        return "a fragment would leave duplicate LightingSettings2D authorities";
    }
    return {};
}

// Resolve the complete v2 catalog and every membership before touching the live World.
std::string ParseScopeGravity(const Json& entry, std::optional<SceneGravity2D>& gravity) {
    if (!entry.contains("gravity")) return {};
    const Json& saved = entry["gravity"];
    if (!saved.is_object() || !saved.contains("mode") || !saved["mode"].is_string()) {
        return "scene gravity requires an object and a mode";
    }
    SceneGravity2D parsed;
    const std::string mode = saved["mode"].get<std::string>();
    if (mode == "inherit") parsed.mode = SceneGravityMode2D::Inherit;
    else if (mode == "multiply") parsed.mode = SceneGravityMode2D::Multiply;
    else if (mode == "override") parsed.mode = SceneGravityMode2D::Override;
    else return "unknown scene gravity mode";
    if (saved.contains("value")) {
        const Json& value = saved["value"];
        if (!value.is_array() || value.size() != 2 || !value[0].is_number() || !value[1].is_number()) {
            return "scene gravity value must be a two-number vector";
        }
        parsed.value = {value[0].get<double>(), value[1].get<double>()};
    }
    if (saved.contains("multiplier")) {
        if (!saved["multiplier"].is_number()) return "scene gravity multiplier must be a number";
        parsed.multiplier = saved["multiplier"].get<double>();
    }
    if (!std::isfinite(parsed.value.x) || !std::isfinite(parsed.value.y) ||
        !std::isfinite(parsed.multiplier) || parsed.multiplier < 0) return "invalid scene gravity values";
    gravity = parsed;
    return {};
}

// Check gravity against the settings the import would produce, before changing the live World.
std::string CheckIncomingGravity(const Json& document, const World& world,
                                 const std::vector<SceneScope>& incoming, bool fragment,
                                 const std::unordered_set<entt::entity>& excluded = {}) {
    PhysicsSettings2D settings;
    for (auto [id, authored] : world.Registry().view<const PhysicsSettings2D>().each()) {
        if (excluded.contains(id)) continue;
        settings = authored;
    }
    for (const Json& entity : document["entities"]) {
        if (!entity.is_object() || !entity.contains("components") || !entity["components"].is_object() ||
            !entity["components"].contains("PhysicsSettings2D")) continue;
        const Json& component = entity["components"]["PhysicsSettings2D"];
        if (!component.is_object()) return "invalid PhysicsSettings2D definition";
        if (component.contains("gravity")) {
            const Json& gravity = component["gravity"];
            if (!gravity.is_array() || gravity.size() != 2 || !gravity[0].is_number() || !gravity[1].is_number())
                return "invalid PhysicsSettings2D gravity";
            settings.gravity = {gravity[0].get<double>(), gravity[1].get<double>()};
        }
        if (component.contains("unitsPerMeter")) {
            if (!component["unitsPerMeter"].is_number()) return "invalid PhysicsSettings2D unitsPerMeter";
            settings.unitsPerMeter = component["unitsPerMeter"].get<double>();
        }
        if (component.contains("substeps")) {
            if (!component["substeps"].is_number_integer()) return "invalid PhysicsSettings2D substeps";
            settings.substeps = component["substeps"].get<int>();
        }
    }
    if (!std::isfinite(settings.gravity.x) || !std::isfinite(settings.gravity.y) ||
        !std::isfinite(settings.unitsPerMeter) || settings.unitsPerMeter < 0.001 ||
        settings.unitsPerMeter > 1000000 || settings.substeps < 1 || settings.substeps > 32 ||
        std::abs(settings.gravity.x) > 1000000 * settings.unitsPerMeter ||
        std::abs(settings.gravity.y) > 1000000 * settings.unitsPerMeter) {
        return "invalid PhysicsSettings2D gravity, scale or substep count";
    }
    const auto check = [&](const SceneScope& scope) -> std::string {
        if (!scope.gravity) return {};
        const auto& gravity = *scope.gravity;
        const double limit = 1000000 * settings.unitsPerMeter;
        if (std::abs(gravity.value.x) > limit || std::abs(gravity.value.y) > limit)
            return "scene gravity override exceeds the physical coordinate limit";
        if (gravity.mode == SceneGravityMode2D::Multiply) {
            const pxVec2 resolved = settings.gravity * gravity.multiplier;
            if (!std::isfinite(resolved.x) || !std::isfinite(resolved.y) ||
                std::abs(resolved.x) > limit || std::abs(resolved.y) > limit)
                return "scene gravity multiplication exceeds the physical coordinate limit";
        }
        return {};
    };
    if (fragment) {
        for (const SceneScope& scope : world.Scopes()) {
            if (std::string error = check(scope); !error.empty()) return error;
        }
    }
    for (const SceneScope& scope : incoming) {
        if (std::string error = check(scope); !error.empty()) return error;
    }
    return {};
}

void WriteScopeGravity(Json& entry, const SceneScope& scope) {
    if (!scope.gravity) { entry.erase("gravity"); return; }
    Json saved = entry.contains("gravity") && entry["gravity"].is_object()
        ? entry["gravity"] : Json::object();
    const SceneGravity2D& gravity = *scope.gravity;
    switch (gravity.mode) {
    case SceneGravityMode2D::Inherit: saved["mode"] = "inherit"; break;
    case SceneGravityMode2D::Multiply: saved["mode"] = "multiply"; break;
    case SceneGravityMode2D::Override: saved["mode"] = "override"; break;
    }
    saved["value"] = Json::array({gravity.value.x, gravity.value.y});
    saved["multiplier"] = gravity.multiplier;
    entry["gravity"] = std::move(saved);
}

std::string CheckScopes(const Json& document, const World& world, std::vector<SceneScope>& scopes,
                        std::vector<ScopeId>& memberships,
                        std::unordered_map<std::uint64_t, std::string>& keptScopes) {
    if (!document.contains("sceneScopes") || !document["sceneScopes"].is_array()) {
        return "the version 2 scene has no sceneScopes array";
    }
    std::unordered_set<std::uint64_t> known;
    for (const SceneScope& existing : world.Scopes()) known.insert(existing.id.value);
    const Json& catalog = document["sceneScopes"];
    for (std::size_t i = 0; i < catalog.size(); ++i) {
        const Json& entry = catalog[i];
        std::uint64_t id = 0;
        if (!entry.is_object() || !entry.contains("id") || !ParseHexId(entry["id"], id)) {
            return std::format("scene scope {} has no valid id", i);
        }
        if (!entry.contains("name") || !entry["name"].is_string()) {
            return std::format("scene scope {} has no valid name", HexId(id));
        }
        if (!known.insert(id).second) {
            return std::format("scene scope id {} is used twice or already exists", HexId(id));
        }
        std::optional<SceneGravity2D> gravity;
        if (std::string error = ParseScopeGravity(entry, gravity); !error.empty()) {
            return std::format("scene scope {}: {}", HexId(id), error);
        }
        SceneScope scope{ScopeId{id}, entry["name"].get<std::string>(), gravity};
        try { detail::ReadScopePresentation(entry, scope); } catch (const std::exception& error) { return error.what(); }
        scopes.push_back(std::move(scope));
        keptScopes.emplace(id, entry.dump());
    }
    const Json& entities = document["entities"];
    memberships.reserve(entities.size());
    for (std::size_t i = 0; i < entities.size(); ++i) {
        const Json& entry = entities[i];
        ScopeId membership;
        if (entry.contains("sceneScope") && !entry["sceneScope"].is_null()) {
            if (!ParseHexId(entry["sceneScope"], membership.value) ||
                std::none_of(scopes.begin(), scopes.end(), [&](const SceneScope& scope) {
                    return scope.id == membership;
                })) {
                return std::format("entity {} refers to an invalid or unknown scene scope", i);
            }
        }
        memberships.push_back(membership);
    }
    return {};
}

Json ScopeDefinition(const SceneScope& scope, const detail::SceneDocumentExtras* kept) {
    Json entry = kept != nullptr && kept->scopes.contains(scope.id.value)
        ? ParseKept(kept->scopes.at(scope.id.value)) : Json::object();
    entry["id"] = HexId(scope.id.value);
    entry["name"] = scope.name;
    WriteScopeGravity(entry, scope);
    detail::WriteScopePresentation(scope, entry);
    return entry;
}

std::string CheckFragmentScopes(const Json& document, const World& world,
                                std::vector<SceneScope>& toAdd, std::vector<ScopeId>& memberships,
                                std::unordered_map<std::uint64_t, std::string>& keptScopes) {
    if (!document.contains("sceneScopes") || !document["sceneScopes"].is_array()) {
        return "the version 2 fragment has no sceneScopes array";
    }
    std::unordered_map<std::uint64_t, SceneScope> existing;
    for (const SceneScope& scope : world.Scopes()) existing.emplace(scope.id.value, scope);
    const auto* kept = world.Registry().ctx().find<detail::SceneDocumentExtras>();
    std::unordered_set<std::uint64_t> listedScopes;
    for (const Json& entry : document["sceneScopes"]) {
        std::uint64_t id = 0;
        if (!entry.is_object() || !entry.contains("id") || !ParseHexId(entry["id"], id) ||
            !entry.contains("name") || !entry["name"].is_string()) {
            return "the fragment has an invalid scene scope definition";
        }
        if (!listedScopes.insert(id).second) return std::format("scene scope {} is listed twice", HexId(id));
        std::optional<SceneGravity2D> gravity;
        if (std::string error = ParseScopeGravity(entry, gravity); !error.empty()) {
            return std::format("scene scope {}: {}", HexId(id), error);
        }
        Json normalized = entry;
        normalized["id"] = HexId(id);
        SceneScope incoming{ScopeId{id}, entry["name"].get<std::string>(), gravity};
        try { detail::ReadScopePresentation(entry, incoming); } catch (const std::exception& error) { return error.what(); }
        WriteScopeGravity(normalized, incoming);
        detail::WriteScopePresentation(incoming, normalized);
        if (const auto found = existing.find(id); found != existing.end()) {
            if (nlohmann::json::parse(ScopeDefinition(found->second, kept).dump()) !=
                nlohmann::json::parse(normalized.dump())) {
                return std::format("scene scope {} conflicts with an existing definition", HexId(id));
            }
        } else {
            toAdd.push_back(incoming);
            keptScopes.emplace(id, normalized.dump());
        }
    }
    std::unordered_set<std::uint64_t> referencedScopes;
    for (const Json& entity : document["entities"]) {
        ScopeId membership;
        if (entity.contains("sceneScope") && !entity["sceneScope"].is_null()) {
            if (!ParseHexId(entity["sceneScope"], membership.value) ||
                !listedScopes.contains(membership.value)) {
                return "an entity refers to an invalid or missing scene scope definition";
            }
            referencedScopes.insert(membership.value);
        }
        memberships.push_back(membership);
    }
    if (referencedScopes.size() != listedScopes.size()) {
        return "the fragment includes an unreferenced scene scope definition";
    }
    return {};
}

struct PreparedEntities {
    Json document;
    std::vector<std::uint64_t> listed;
    std::vector<SceneScope> scopesToAdd;
    std::vector<ScopeId> memberships;
    std::unordered_map<std::uint64_t, std::string> keptScopes;
};

// Pure preparation: no SceneIndex installation, ID generation, component callbacks or writes.
std::string PrepareEntities(const World& world, const ComponentTypes& types, std::string_view text,
                            SceneIds mode, std::span<const std::uint64_t> replacing, PreparedEntities& prepared) {
    if (world.IsStepping()) return "entities cannot be read during a step";
    if (mode != SceneIds::Keep && mode != SceneIds::New) return "unknown entity ID mode";
    std::string error;
    prepared.document = ParseDocument(text, kEntitiesFormat, "set of entities", error);
    if (!error.empty()) return error;
    const Json& document = prepared.document;
    const Json& list = document["entities"];
    const int version = document["version"].get<int>();
    if (version == 2) {
        if (std::string problem = CheckV2ComponentAdmission(list, types); !problem.empty()) return problem;
    }
    std::unordered_set<std::uint64_t> replacingIds;
    for (const auto id : replacing) {
        if (mode != SceneIds::Keep || id == 0 || !replacingIds.insert(id).second)
            return "replacement IDs must be nonzero, unique and used with Keep IDs";
    }
    std::unordered_map<std::uint64_t, entt::entity> remaining;
    std::unordered_set<entt::entity> excluded;
    const auto* index = world.Registry().ctx().find<detail::SceneIndex>();
    const auto* currentIds = &remaining;
    if (replacing.empty() && index != nullptr && !index->stale) {
        currentIds = &index->entities;
    } else {
        for (auto [entity, id] : world.Registry().view<const SceneId>().each()) {
            if (replacingIds.contains(id.value)) excluded.insert(entity);
            else remaining.emplace(id.value, entity);
        }
    }
    if (std::string problem = CheckEntities(list, *currentIds, mode == SceneIds::Keep, prepared.listed); !problem.empty())
        return problem;
    if (std::string problem = CheckFragmentSettings(world, list, excluded); !problem.empty()) return problem;
    const auto* destination = world.Registry().ctx().find<detail::SceneDocumentExtras>();
    if (version == 1 && destination != nullptr && destination->version == 2) {
        for (const Json& entity : list) {
            if (entity.contains("sceneScope")) return "version 1 fragment sceneScope data conflicts with version 2 membership";
        }
    }
    if (version == 2) {
        if (destination == nullptr || destination->version != 2) return "a version 2 fragment requires a version 2 World";
        if (std::string problem = CheckFragmentScopes(document, world, prepared.scopesToAdd,
                prepared.memberships, prepared.keptScopes); !problem.empty()) return problem;
        if (std::string problem = CheckIncomingGravity(document, world, prepared.scopesToAdd, true, excluded);
            !problem.empty()) return problem;
    }
    return {};
}

// Rolls back newly added entities and the captured catalog, membership, RNG and document metadata
// if a v2 callback throws. It cannot restore pre-existing component storage or entities changed by
// a callback, nor effects outside this World. This is not a general callback transaction.
class SceneMutationSnapshot {
public:
    explicit SceneMutationSnapshot(World& world) : world_(world), random_(world.GetRandom()) {
        entt::registry& registry = world_.Registry();
        for (const entt::entity id : registry.view<entt::entity>()) {
            entities_.insert(id);
            memberships_.emplace(id, world_.ScopeOf(world_.Wrap(id)).value_or(ScopeId{}));
        }
        scopes_ = world_.Scopes();
        presentation_ = world_.Presentation();
        if (const auto* kept = registry.ctx().find<detail::SceneDocumentExtras>()) document_ = *kept;
    }

    bool Restore() noexcept {
        bool complete = true;
        try {
            entt::registry& registry = world_.Registry();
            std::vector<entt::entity> added;
            for (const entt::entity id : registry.view<entt::entity>()) {
                if (!entities_.contains(id)) added.push_back(id);
            }
            for (const entt::entity id : added) {
                if (registry.valid(id)) world_.Destroy(world_.Wrap(id));
            }
            for (const entt::entity id : entities_) {
                if (!registry.valid(id)) { complete = false; continue; }
                world_.SetScope(world_.Wrap(id), ScopeId{});
            }
            for (const SceneScope& scope : world_.Scopes()) {
                if (!world_.RemoveScope(scope.id)) complete = false;
            }
            world_.SetPresentation(presentation_);
            for (const SceneScope& scope : scopes_) {
                if (!world_.CreateScope(scope.id, scope.name, scope.gravity) ||
                    !world_.SetScopeEnvironment(scope.id, scope.environment) || !world_.SetScopeCamera(scope.id, scope.camera)) complete = false;
            }
            for (const auto& [id, scope] : memberships_) {
                if (scope.value != 0 && registry.valid(id) && !world_.SetScope(world_.Wrap(id), scope)) {
                    complete = false;
                }
            }
            if (document_) registry.ctx().insert_or_assign(*document_);
            else registry.ctx().erase<detail::SceneDocumentExtras>();
        } catch (...) {
            complete = false;
        }
        world_.GetRandom() = random_;
        return complete;
    }

private:
    World& world_;
    Random random_;
    std::unordered_set<entt::entity> entities_;
    std::unordered_map<entt::entity, ScopeId> memberships_;
    std::vector<SceneScope> scopes_;
    std::optional<WorldPresentation2D> presentation_;
    std::optional<detail::SceneDocumentExtras> document_;
};

// Offers the numeric fields of `typeName` that `kept` holds to the type's upgrade, then removes from
// `kept` those it consumed, so a converted value is not also written back as an unknown field.
void UpgradeComponent(const ComponentDescriptor& type, void* component, int savedVersion, Json& kept,
                      const std::string& typeName) {
    LegacyFields legacy;
    if (kept.contains(typeName)) {
        for (auto field = kept[typeName].begin(); field != kept[typeName].end(); ++field) {
            const Json& saved = field.value();
            FieldValue value;
            if (saved.is_number()) {
                value.x = saved.get<double>();
            } else if (saved.is_array() && !saved.empty() && saved.size() <= 4
                       && std::all_of(saved.begin(), saved.end(), [](const Json& n) { return n.is_number(); })) {
                double* parts[] = {&value.x, &value.y, &value.z, &value.w};
                for (std::size_t i = 0; i < saved.size(); ++i) {
                    *parts[i] = saved[i].get<double>();
                }
            } else {
                continue;
            }
            legacy.emplace(field.key(), value);
        }
    }
    std::vector<std::string> offered;
    for (const auto& entry : legacy) {
        offered.push_back(entry.first);
    }
    type.upgrade(component, savedVersion, legacy);
    if (!kept.contains(typeName)) {
        return;
    }
    for (const std::string& name : offered) {
        if (!legacy.contains(name)) {
            kept[typeName].erase(name);
        }
    }
    if (kept[typeName].empty()) {
        kept.erase(typeName);
    }
}

// Adds the listed entities under the scene IDs `assigned` and fills their components. An Entity
// field names an entity of the list by the ID it was written with (`listed`), so a reference inside
// the list finds the new entity, and one outside it the World's (`sceneEntities`). Returns the
// entities, in the list's order.
std::vector<Entity> AddEntities(World& world, const ComponentTypes& types, const Json& list,
                                const std::vector<std::uint64_t>& listed, const std::vector<std::uint64_t>& assigned,
                                const std::unordered_map<std::uint64_t, entt::entity>& sceneEntities,
                                SceneReport& report) {
    std::vector<Entity> created;
    created.reserve(list.size());
    std::unordered_map<std::uint64_t, entt::entity> listedEntities;
    listedEntities.reserve(list.size());
    for (std::size_t i = 0; i < list.size(); ++i) {
        const Json& entry = list[i];
        std::string name;
        if (entry.contains("name") && entry["name"].is_string()) {
            name = entry["name"].get<std::string>();
        }
        Entity entity = world.Create(name);
        entity.Add<SceneId>(SceneId{assigned[i]});
        listedEntities.insert_or_assign(listed[i], entity.Id());
        created.push_back(entity);
    }
    const SceneLookup ids{listedEntities, sceneEntities};

    for (std::size_t i = 0; i < list.size(); ++i) {
        const Json& entry = list[i];
        const Entity entity = created[i];
        const std::string hex = HexId(assigned[i]);
        Json keptEntity = Json::object();
        Json keptComponents = Json::object();
        Json keptFields = Json::object();
        for (auto it = entry.begin(); it != entry.end(); ++it) {
            const std::string& key = it.key();
            if (key == "id" || key == "components" || (key == "name" && it.value().is_string())) {
                continue;
            }
            keptEntity[key] = it.value();
        }

        const Json components = entry.contains("components") ? entry["components"] : Json::object();
        for (auto it = components.begin(); it != components.end(); ++it) {
            const std::string& typeName = it.key();
            const Json& saved = it.value();
            const int index = types.IndexOf(typeName);
            if (index < 0) {
                keptComponents[typeName] = saved;
                continue;
            }
            const ComponentDescriptor& type = types.All()[static_cast<std::size_t>(index)];
            if (!saved.is_object()) {
                report.warnings.push_back(
                    std::format("{} of entity {} is not an object; kept as it was", typeName, hex));
                keptComponents[typeName] = saved;
                continue;
            }
            void* component = type.add != nullptr ? type.add(world, entity.Id()) : nullptr;
            if (component == nullptr) {
                report.warnings.push_back(
                    std::format("{} of entity {} cannot be added (no default value); kept as it was", typeName, hex));
                keptComponents[typeName] = saved;
                continue;
            }
            if (saved.contains("v") && saved["v"].is_number_integer() && saved["v"].get<std::int64_t>() > type.version) {
                report.warnings.push_back(std::format("{} of entity {} was saved at version {}; this build knows {}",
                                                      typeName, hex, saved["v"].get<std::int64_t>(), type.version));
            }
            for (auto field = saved.begin(); field != saved.end(); ++field) {
                if (field.key() == "v") {
                    continue;
                }
                const auto known = std::find_if(type.fields.begin(), type.fields.end(),
                                                [&](const FieldDescriptor& f) { return f.name == field.key(); });
                if (known == type.fields.end()) {
                    keptFields[typeName][field.key()] = field.value();
                    continue;
                }
                const auto problem = DecodeField(*known, field.value(), component, types, ids);
                if (!problem.empty()) {
                    report.warnings.push_back(std::format("{}.{} of entity {}: {}; left at its default", typeName,
                                                          field.key(), hex, problem));
                }
            }
            const std::int64_t savedVersion =
                saved.contains("v") && saved["v"].is_number_integer() ? saved["v"].get<std::int64_t>() : 1;
            if (type.upgrade != nullptr && savedVersion < type.version) {
                UpgradeComponent(type, component, static_cast<int>(std::max<std::int64_t>(savedVersion, 0)),
                                 keptFields, typeName);
            }
        }

        if (!keptEntity.empty() || !keptComponents.empty() || !keptFields.empty()) {
            Json kept = Json::object();
            if (!keptEntity.empty()) {
                kept["entity"] = std::move(keptEntity);
            }
            if (!keptComponents.empty()) {
                kept["components"] = std::move(keptComponents);
            }
            if (!keptFields.empty()) {
                kept["fields"] = std::move(keptFields);
            }
            entity.Add<detail::SceneExtras>(detail::SceneExtras{kept.dump()});
        }
    }
    return created;
}

} // namespace

static std::string WriteSceneVersion(World& world, const ComponentTypes& types,
                                     std::vector<std::string>* warnings, int version) {
    entt::registry& registry = world.Registry();
    const auto* kept = registry.ctx().find<detail::SceneDocumentExtras>();
    const std::vector<SceneScope> scopes = world.Scopes();
    if (version == 1 && (world.Presentation() || !scopes.empty())) {
        throw std::runtime_error("scene scopes require an explicit version 2 conversion");
    }
    // A v2 document must remain importable under this registry. Check storage
    // before invoking any component getter or constructing the output document.
    if (version == 2) {
        if (std::string problem = CheckV2WorldAdmission(world, types); !problem.empty()) {
            throw std::runtime_error(problem);
        }
    }
    Json document = Json::object();
    document["format"] = std::string(kSceneFormat);
    document["version"] = version;
    document["seed"] = world.GetRandom().Seed();
    if (kept != nullptr) {
        Json extras = ParseKept(kept->json);
        extras.erase("entities");
        if (version == 2) extras.erase("sceneScopes");
        Merge(document, extras);
    }

    if (version == 2) {
        if (world.Presentation()) document["presentation"] = detail::WritePresentation(*world.Presentation(), document.value("presentation", Json::object()));
        Json catalog = Json::array();
        for (const SceneScope& scope : scopes) {
            catalog.push_back(ScopeDefinition(scope, kept));
        }
        document["sceneScopes"] = std::move(catalog);
    }

    // EnTT lists live entities newest first; the scene lists them oldest first.
    std::vector<entt::entity> members;
    for (const entt::entity id : registry.view<entt::entity>()) {
        if (registry.all_of<SceneId>(id)) {
            members.push_back(id);
        }
    }
    std::reverse(members.begin(), members.end());

    Json entities = Json::array();
    for (const entt::entity id : members) {
        Json entry = WriteEntity(registry, types, id, warnings);
        if (version == 2) {
            const ScopeId membership = world.ScopeOf(world.Wrap(id)).value_or(ScopeId{});
            if (membership.value == 0) entry.erase("sceneScope");
            else entry["sceneScope"] = HexId(membership.value);
        }
        entities.push_back(std::move(entry));
    }
    document["entities"] = std::move(entities);
    return Finish(document);
}

std::string WriteScene(World& world, const ComponentTypes& types, std::vector<std::string>* warnings) {
    const auto* kept = world.Registry().ctx().find<detail::SceneDocumentExtras>();
    return WriteSceneVersion(world, types, warnings, kept != nullptr ? kept->version : 1);
}

std::string ValidateV2SceneSnapshot(const World& world, const ComponentTypes& types) {
    const auto* kept = world.Registry().ctx().find<detail::SceneDocumentExtras>();
    if (kept == nullptr || kept->version != 2) return {};
    if (auto problem = CheckV2WorldAdmission(world, types); !problem.empty()) return problem;
    return CheckFragmentSettings(world, Json::array());
}

std::string ValidateSceneEditInput(const World& world, std::string_view text) {
    if (world.IsStepping()) return "candidate edits require a World between steps";
    if (std::string problem = CheckFragmentSettings(world, Json::array()); !problem.empty()) return problem;
    std::string error;
    const Json document = ParseDocument(text, kSceneFormat, "scene", error);
    if (!error.empty()) return error;
    if (document["version"].get<int>() != 2) return "candidate edits require an explicit version 2 document";
    return {};
}

std::string SceneImportAdmissionError(const ComponentTypes& types, std::string_view text) {
    std::string error;
    const Json document = ParseDocument(text, kSceneFormat, "scene", error);
    if (!error.empty()) return error;
    return document["version"].get<int>() == 2 ? CheckV2ComponentAdmission(document["entities"], types) : std::string{};
}

SceneReport ReadScene(World& world, const ComponentTypes& types, std::string_view text) {
    if (world.IsStepping()) {
        return Failed("a scene cannot be read during a step");
    }
    std::string error;
    const Json document = ParseDocument(text, kSceneFormat, "scene", error);
    if (!error.empty()) {
        return Failed(std::move(error));
    }
    const Json& list = document["entities"];
    const int version = document["version"].get<int>();
    if (version == 2) {
        if (std::string problem = CheckV2ComponentAdmission(list, types); !problem.empty()) {
            return Failed(std::move(problem));
        }
    }

    // Every ID is checked before anything is added, so a bad file leaves the World as it was.
    entt::registry& registry = world.Registry();
    const std::unordered_map<std::uint64_t, entt::entity>& ids = SceneEntities(registry);
    std::vector<std::uint64_t> listed;
    if (std::string problem = CheckEntities(list, ids, true, listed); !problem.empty()) {
        return Failed(std::move(problem));
    }

    std::vector<SceneScope> scopes;
    std::vector<ScopeId> memberships;
    std::unordered_map<std::uint64_t, std::string> keptScopes;
    if (version == 2) {
        if (std::string problem = CheckFragmentSettings(world, list); !problem.empty()) {
            return Failed(std::move(problem));
        }
        if (std::string problem = CheckScopes(document, world, scopes, memberships, keptScopes); !problem.empty()) {
            return Failed(std::move(problem));
        }
        if (std::string problem = CheckIncomingGravity(document, world, scopes, false); !problem.empty()) {
            return Failed(std::move(problem));
        }
    }

    std::optional<WorldPresentation2D> presentation;
    if (version == 2 && document.contains("presentation")) {
        try { presentation = detail::ReadPresentation(document["presentation"]); }
        catch (const std::exception& failure) { return Failed(failure.what()); }
        if (world.Presentation() || !registry.view<const LightingSettings2D>().empty())
            return Failed("presentation settings require one canonical authority in an empty destination");
        for (const auto& entry : list) {
            if (entry.contains("components") && entry["components"].contains("LightingSettings2D"))
                return Failed("canonical presentation conflicts with legacy LightingSettings2D");
        }
    }
    SceneReport report;
    const auto apply = [&] {
        if (document.contains("seed") && document["seed"].is_number_unsigned()) {
            world.GetRandom().Reseed(document["seed"].get<std::uint64_t>());
        } else {
            report.warnings.push_back("the scene has no valid seed; the World keeps its own");
        }
        for (const SceneScope& scope : scopes) {
            if (!world.CreateScope(scope.id, scope.name)) {
                throw std::runtime_error("could not create validated scene scope");
            }
        }
        const std::vector<Entity> created = AddEntities(world, types, list, listed, listed, ids, report);
        if (presentation && !world.SetPresentation(presentation)) throw std::runtime_error("could not apply canonical presentation");
        for (const SceneScope& scope : scopes) {
            if (!world.SetScopeEnvironment(scope.id, scope.environment) || !world.SetScopeCamera(scope.id, scope.camera)) throw std::runtime_error("could not apply Scene presentation");
            if (scope.gravity && !world.SetScopeGravity(scope.id, *scope.gravity)) {
                throw std::runtime_error("could not apply validated scene gravity");
            }
        }
        for (std::size_t i = 0; i < memberships.size(); ++i) {
            if (memberships[i].value != 0) world.SetScope(created[i], memberships[i]);
        }
        Json keptDocument = Json::object();
        for (auto it = document.begin(); it != document.end(); ++it) {
            const std::string& key = it.key();
            if (key != "format" && key != "version" && key != "seed" && key != "entities" &&
                (version != 2 || key != "sceneScopes")) {
                keptDocument[key] = it.value();
            }
        }
        registry.ctx().insert_or_assign(detail::SceneDocumentExtras{keptDocument.dump(), version, std::move(keptScopes)});
    };
    if (version == 1) {
        apply();
    } else {
        SceneMutationSnapshot before(world);
        try { apply(); }
        catch (const std::exception& failure) {
            const std::string reason = failure.what();
            return Failed(std::format("version 2 scene application failed: {}{}", reason,
                                      before.Restore() ? "" : "; rollback failed"));
        } catch (...) {
            return Failed(std::format("version 2 scene application failed: unknown exception{}",
                                      before.Restore() ? "" : "; rollback failed"));
        }
    }
    return report;
}

std::string WriteEntities(World& world, const ComponentTypes& types, std::span<const Entity> entities,
                          std::vector<std::string>* warnings) {
    return WriteEntities(world, types, entities, std::string_view(), warnings);
}

std::string WriteEntities(World& world, const ComponentTypes& types, std::span<const Entity> entities,
                          std::string_view meta, std::vector<std::string>* warnings) {
    entt::registry& registry = world.Registry();
    const auto* kept = registry.ctx().find<detail::SceneDocumentExtras>();
    const int version = kept != nullptr ? kept->version : 1;
    if (version == 2) {
        std::set<std::string> unsupported;
        for (const Entity entity : entities) {
            if (!registry.valid(entity.Id()) || !registry.all_of<SceneId>(entity.Id())) continue;
            for (std::size_t index = 0; index < types.All().size(); ++index) {
                if (types.AllowsV2Import(static_cast<int>(index))) continue;
                const ComponentDescriptor& type = types.All()[index];
                const auto* storage = registry.storage(type.type);
                if (storage != nullptr && storage->contains(entity.Id())) unsupported.insert(type.name);
            }
        }
        if (std::string problem = V2AdmissionError(unsupported); !problem.empty()) {
            throw std::runtime_error(problem);
        }
    }
    Json document = Json::object();
    document["format"] = std::string(kEntitiesFormat);
    document["version"] = version;
    if (!meta.empty()) {
        Json parsed = Json::parse(meta.begin(), meta.end(), nullptr, false);
        if (parsed.is_object()) {
            document["meta"] = std::move(parsed);
        } else if (warnings != nullptr) {
            warnings->push_back("the meta is not a JSON object; left out");
        }
    }
    Json list = Json::array();
    std::unordered_set<entt::entity> written;
    std::unordered_set<std::uint64_t> referencedScopes;
    for (const Entity entity : entities) {
        if (!registry.valid(entity.Id()) || !registry.all_of<SceneId>(entity.Id())) {
            if (warnings != nullptr) {
                warnings->push_back(std::format("entity {} is gone or not in the scene; left out", entity.Number()));
            }
            continue;
        }
        if (written.insert(entity.Id()).second) {
            Json entry = WriteEntity(registry, types, entity.Id(), warnings);
            if (version == 2) {
                const ScopeId membership = world.ScopeOf(entity).value_or(ScopeId{});
                if (membership.value == 0) entry.erase("sceneScope");
                else {
                    entry["sceneScope"] = HexId(membership.value);
                    referencedScopes.insert(membership.value);
                }
            }
            list.push_back(std::move(entry));
        }
    }
    if (version == 2) {
        Json catalog = Json::array();
        for (const SceneScope& scope : world.Scopes()) {
            if (referencedScopes.contains(scope.id.value)) catalog.push_back(ScopeDefinition(scope, kept));
        }
        document["sceneScopes"] = std::move(catalog);
    }
    document["entities"] = std::move(list);
    return Finish(document);
}

SceneReport ValidateEntities(const World& world, const ComponentTypes& types, std::string_view text,
                            SceneIds ids, std::span<const std::uint64_t> replacing) {
    PreparedEntities prepared;
    if (std::string error = PrepareEntities(world, types, text, ids, replacing, prepared); !error.empty())
        return Failed(std::move(error));
    return {};
}

SceneFragmentPlan PrepareSceneFragment(World& world, const ComponentTypes& types, std::string_view fragment,
    SceneIds ids, std::span<const std::uint64_t> replacing, double offsetX, double offsetY) {
    SceneFragmentPlan plan;
    const auto fail = [&](std::string error) { plan.ok = false; plan.error = std::move(error); return plan; };
    if (world.IsStepping() || SceneFormatVersion(world) != 2)
        return fail("fragment candidates require a version 2 World between steps");
    if (!std::isfinite(offsetX) || !std::isfinite(offsetY)) return fail("fragment offset must be finite");
    std::unordered_set<std::uint64_t> removed;
    for (const auto id : replacing) {
        if (ids != SceneIds::Keep || id == 0 || !removed.insert(id).second)
            return fail("replacement IDs must be nonzero, unique and used with Keep IDs");
    }
    if (!fragment.empty()) {
        const auto admitted = ValidateEntities(world, types, fragment, ids, replacing);
        if (!admitted.ok) return fail(admitted.error);
        std::string error;
        const auto incoming = ParseDocument(fragment, kEntitiesFormat, "set of entities", error);
        if (!error.empty()) return fail(error);
        if (const auto problem = CheckV2ComponentAdmission(incoming["entities"], types); !problem.empty())
            return fail(problem); // A v1 fragment never bypasses v2 custom-type admission.
    } else if (ids != SceneIds::Keep) return fail("deletion requires Keep IDs");
    if (const auto error = ValidateV2SceneSnapshot(world, types); !error.empty()) return fail(error);

    // Keep the original document as the merge base. Serializing the entire scratch
    // World after replacement would lose outside references to retired native handles.
    Json document = Json::parse(WriteScene(world, types));
    std::unordered_map<std::uint64_t, std::size_t> originalOrder;
    for (const auto& entity : document["entities"]) {
        std::uint64_t id = 0;
        if (ParseHexId(entity["id"], id)) originalOrder.emplace(id, originalOrder.size());
    }
    World scratch;
    const auto read = ReadScene(scratch, types, document.dump());
    if (!read.ok) return fail(read.error);
    std::vector<Entity> retired;
    for (const auto [entity, id] : scratch.Registry().view<const SceneId>().each())
        if (removed.contains(id.value)) retired.push_back(scratch.Wrap(entity));
    for (const auto entity : retired) entity.Destroy();
    auto& entities = document["entities"];
    std::erase_if(entities.get_ref<Json::array_t&>(), [&](const Json& entity) {
        std::uint64_t id = 0; return ParseHexId(entity["id"], id) && removed.contains(id);
    });
    Json assigned = Json::array();
    if (!fragment.empty()) {
        const auto imported = ReadEntities(scratch, types, fragment, ids);
        if (!imported.ok) return fail(imported.error);
        Json normalized = Json::parse(WriteEntities(scratch, types, imported.entities));
        for (Json& entity : normalized["entities"]) {
            assigned.push_back(entity["id"]);
            if (entity.contains("components") && entity["components"].contains("Transform2D")) {
                auto& transform = entity["components"]["Transform2D"];
                if (transform.contains("position")) {
                    auto& position = transform["position"];
                    const double x = position[0].get<double>() + offsetX;
                    const double y = position[1].get<double>() + offsetY;
                    if (!std::isfinite(x) || !std::isfinite(y)) return fail("fragment offset overflow");
                    position = Json::array({x, y});
                }
            }
            entities.push_back(std::move(entity));
        }
        if (normalized.contains("sceneScopes")) {
            for (const Json& scope : normalized["sceneScopes"]) {
                const bool found = std::any_of(document["sceneScopes"].begin(), document["sceneScopes"].end(),
                    [&](const Json& current) { return current["id"] == scope["id"]; });
                if (!found) document["sceneScopes"].push_back(scope);
            }
        }
    }
    const auto rank = [&](const Json& entity) {
        std::uint64_t id = 0;
        if (ParseHexId(entity["id"], id)) {
            const auto found = originalOrder.find(id);
            if (found != originalOrder.end()) return found->second;
        }
        return originalOrder.size();
    };
    std::stable_sort(entities.begin(), entities.end(), [&](const Json& a, const Json& b) { return rank(a) < rank(b); });
    plan.text = Json{{"document", std::move(document)}, {"ids", std::move(assigned)}}.dump();
    return plan;
}

EntitiesReport ReadEntities(World& world, const ComponentTypes& types, std::string_view text, SceneIds sceneIds) {
    EntitiesReport report;
    const auto fail = [&](std::string error) {
        report.ok = false;
        report.error = std::move(error);
        return report;
    };
    PreparedEntities prepared;
    if (std::string error = PrepareEntities(world, types, text, sceneIds, {}, prepared); !error.empty()) {
        return fail(std::move(error));
    }
    const Json& document = prepared.document;
    const Json& list = document["entities"];
    const int version = document["version"].get<int>();
    const auto& listed = prepared.listed;
    const auto& scopesToAdd = prepared.scopesToAdd;
    const auto& memberships = prepared.memberships;
    const auto& keptScopes = prepared.keptScopes;
    const auto& ids = SceneEntities(world.Registry());

    std::vector<std::uint64_t> assigned = listed;
    if (sceneIds == SceneIds::New) {
        std::unordered_set<std::uint64_t> newIds;
        for (std::uint64_t& id : assigned) {
            do {
                id = NewSceneId();
            } while (ids.contains(id) || !newIds.insert(id).second);
        }
    }
    const auto apply = [&] {
        for (const SceneScope& scope : scopesToAdd) {
            if (!world.CreateScope(scope.id, scope.name)) {
                throw std::runtime_error("could not create validated fragment scope");
            }
        }
        report.entities = AddEntities(world, types, list, listed, assigned, ids, report);
        for (const SceneScope& scope : scopesToAdd) {
            if (!world.SetScopeEnvironment(scope.id, scope.environment) || !world.SetScopeCamera(scope.id, scope.camera)) throw std::runtime_error("could not apply imported Scene presentation");
            if (scope.gravity && !world.SetScopeGravity(scope.id, *scope.gravity)) {
                throw std::runtime_error("could not apply validated fragment gravity");
            }
        }
        for (std::size_t i = 0; i < memberships.size(); ++i) {
            if (memberships[i].value != 0) world.SetScope(report.entities[i], memberships[i]);
        }
        if (version == 2) {
            auto* kept = world.Registry().ctx().find<detail::SceneDocumentExtras>();
            kept->scopes.insert(keptScopes.begin(), keptScopes.end());
        }
        report.meta = MetaText(document, report.warnings);
    };
    if (version == 1) {
        apply();
    } else {
        SceneMutationSnapshot before(world);
        try { apply(); }
        catch (const std::exception& failure) {
            const std::string reason = failure.what();
            report.entities.clear();
            return fail(std::format("version 2 fragment application failed: {}{}", reason,
                                    before.Restore() ? "" : "; rollback failed"));
        } catch (...) {
            report.entities.clear();
            return fail(std::format("version 2 fragment application failed: unknown exception{}",
                                    before.Restore() ? "" : "; rollback failed"));
        }
    }
    return report;
}

FragmentMeta ReadFragmentMeta(std::string_view text) {
    FragmentMeta found;
    std::string error;
    const Json document = ParseDocument(text, kEntitiesFormat, "set of entities", error);
    if (!error.empty()) {
        found.ok = false;
        found.error = std::move(error);
        return found;
    }
    if (document["version"] == 2) {
        World empty;
        std::vector<SceneScope> scopes;
        std::vector<ScopeId> memberships;
        std::unordered_map<std::uint64_t, std::string> keptScopes;
        if (std::string problem = CheckFragmentScopes(document, empty, scopes, memberships, keptScopes);
            !problem.empty()) {
            found.ok = false;
            found.error = std::move(problem);
            return found;
        }
        if (std::string problem = CheckIncomingGravity(document, empty, scopes, true); !problem.empty()) {
            found.ok = false;
            found.error = std::move(problem);
            return found;
        }
    }
    found.meta = MetaText(document, found.warnings);
    found.entityCount = document["entities"].size();
    return found;
}

SceneReport SaveTextFile(std::string_view text, const std::filesystem::path& path) {
    SceneReport report;
    std::filesystem::path temporary = path;
    temporary += ".saving";
    {
        std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
        out.write(text.data(), static_cast<std::streamsize>(text.size()));
        out.close();
        if (!out) {
            std::error_code ignored;
            std::filesystem::remove(temporary, ignored);
            report.ok = false;
            report.error = std::format("could not write {}", PathText(temporary));
            return report;
        }
    }
    std::error_code error;
    std::filesystem::rename(temporary, path, error);
    if (error) {
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        report.ok = false;
        report.error = std::format("could not replace {}: {}", PathText(path), error.message());
    }
    return report;
}

SceneReport detail::SaveSceneText(std::string_view text, const std::filesystem::path& path) {
    std::string error;
    const Json candidate = ParseDocument(text, kSceneFormat, "scene", error);
    if (!error.empty()) return Failed(std::move(error));
    if (candidate["version"] == 2) {
        std::error_code fileError;
        const bool exists = std::filesystem::exists(path, fileError);
        if (fileError) return Failed(std::format("could not inspect {}: {}", PathText(path), fileError.message()));
        if (exists) {
            std::ifstream in(path, std::ios::binary);
            if (!in) return Failed(std::format("could not open {}", PathText(path)));
            std::ostringstream source;
            source << in.rdbuf();
            if (in.bad()) return Failed(std::format("could not read {}", PathText(path)));
            const Json previous = ParseDocument(source.str(), kSceneFormat, "saved scene", error);
            if (!error.empty()) return Failed(std::move(error));
            if (previous["version"] == 1) {
                return Failed("saving a version 2 snapshot over a version 1 scene requires explicit "
                              "ConvertSceneFileV1ToV2 conversion with a recoverable backup");
            }
        }
    }
    return SaveTextFile(text, path);
}

SceneReport SaveSceneFile(World& world, const ComponentTypes& types, const std::filesystem::path& path) {
    std::vector<std::string> warnings;
    std::string text;
    try {
        text = WriteScene(world, types, &warnings);
    } catch (const std::runtime_error& error) {
        return Failed(error.what());
    }
    SceneReport report = detail::SaveSceneText(text, path);
    report.warnings = std::move(warnings);
    return report;
}

SceneReport ConvertSceneFileV1ToV2(World& world, const ComponentTypes& types,
                                    const std::filesystem::path& path) {
    return ConvertSceneFileV1ToV2(world, types, path, nullptr);
}

int SceneFormatVersion(const World& world) {
    const auto* extras = world.Registry().ctx().find<detail::SceneDocumentExtras>();
    return extras != nullptr ? extras->version : 1;
}

SceneReport ConvertSceneFileV1ToV2(World& world, const ComponentTypes& types,
                                    const std::filesystem::path& path, std::string* converted) {
    if (world.IsStepping()) return Failed("a scene cannot be converted during a step");
    auto* extras = world.Registry().ctx().find<detail::SceneDocumentExtras>();
    if (extras == nullptr || extras->version != 1) {
        return Failed("the authored World must have loaded a version 1 scene before conversion");
    }
    std::ifstream in(path, std::ios::binary);
    if (!in) return Failed(std::format("could not open {}", PathText(path)));
    std::ostringstream sourceStream;
    sourceStream << in.rdbuf();
    if (in.bad()) return Failed(std::format("could not read {}", PathText(path)));
    in.close();
    const std::string source = sourceStream.str();
    std::string error;
    const Json original = ParseDocument(source, kSceneFormat, "scene", error, 1);
    if (!error.empty()) return Failed(std::move(error));
    if (original["version"] != 1) return Failed("conversion requires a version 1 source file");
    if (std::string problem = CheckV2ComponentAdmission(original["entities"], types); !problem.empty()) {
        return Failed(std::format("the version 2 candidate is invalid: {}", problem));
    }
    if (std::string problem = CheckV2WorldAdmission(world, types); !problem.empty()) {
        return Failed(std::format("the version 2 candidate is invalid: {}", problem));
    }
    if (original.contains("sceneScopes") || ParseKept(extras->json).contains("sceneScopes")) {
        return Failed("version 1 sceneScopes data conflicts with the version 2 catalog");
    }
    if (original.contains("presentation") || ParseKept(extras->json).contains("presentation")) {
        return Failed("version 1 opaque presentation data conflicts with the version 2 authority; resolve it explicitly before conversion");
    }
    for (const Json& entity : original["entities"]) {
        if (entity.is_object() && entity.contains("sceneScope")) {
            return Failed("version 1 entity sceneScope data conflicts with version 2 membership");
        }
    }
    entt::registry& registry = world.Registry();
    for (const entt::entity id : registry.view<SceneId>()) {
        if (const auto* kept = registry.try_get<detail::SceneExtras>(id)) {
            const Json data = ParseKept(kept->json);
            if (data.contains("entity") && data["entity"].is_object() &&
                data["entity"].contains("sceneScope")) {
                return Failed("authored version 1 entity sceneScope data conflicts with version 2 membership");
            }
        }
    }
    World sourceCheck;
    if (const SceneReport checked = ReadScene(sourceCheck, types, source); !checked.ok) {
        return Failed(std::format("the version 1 source is invalid: {}", checked.error));
    }

    std::vector<std::string> warnings;
    std::string candidate = WriteSceneVersion(world, types, &warnings, 2);
    World candidateCheck;
    if (const SceneReport checked = ReadScene(candidateCheck, types, candidate); !checked.ok) {
        return Failed(std::format("the version 2 candidate is invalid: {}", checked.error));
    }
    if (nlohmann::json::parse(WriteScene(candidateCheck, types)) != nlohmann::json::parse(candidate)) {
        return Failed("the version 2 candidate does not round-trip without changing authored data");
    }

    std::filesystem::path staging = path;
    staging += ".converting";
    std::filesystem::path backup = path;
    backup += ".v1-backup";
    std::error_code fileError;
    if (std::filesystem::exists(staging, fileError) || fileError) {
        return Failed(std::format("conversion staging path is unavailable: {}", PathText(staging)));
    }
    if (std::filesystem::exists(backup, fileError) || fileError) {
        return Failed(std::format("version 1 backup path is unavailable: {}", PathText(backup)));
    }
    {
        std::ofstream out(staging, std::ios::binary | std::ios::trunc);
        out.write(candidate.data(), static_cast<std::streamsize>(candidate.size()));
        out.close();
        if (!out) {
            std::filesystem::remove(staging, fileError);
            return Failed(std::format("could not write conversion staging file {}", PathText(staging)));
        }
    }
    // Refuse a source changed while the candidate was being prepared.
    std::ifstream current(path, std::ios::binary);
    std::ostringstream currentStream;
    currentStream << current.rdbuf();
    const bool sourceChanged = !current || currentStream.str() != source;
    current.close();
    if (sourceChanged) {
        std::filesystem::remove(staging, fileError);
        return Failed("the source scene changed during conversion");
    }
    if (!std::filesystem::copy_file(path, backup, std::filesystem::copy_options::none, fileError)) {
        std::filesystem::remove(staging, fileError);
        return Failed(std::format("could not preserve the version 1 backup at {}", PathText(backup)));
    }
    std::ifstream backupCheck(backup, std::ios::binary);
    std::ostringstream backupStream;
    backupStream << backupCheck.rdbuf();
    const bool backupMatches = backupCheck && backupStream.str() == source;
    backupCheck.close();
    if (!backupMatches) {
        std::filesystem::remove(staging, fileError);
        return Failed("the version 1 backup differs from the validated source; the original file remains intact");
    }
    std::ifstream finalCheck(path, std::ios::binary);
    std::ostringstream finalStream;
    finalStream << finalCheck.rdbuf();
    const bool sourceStillMatches = finalCheck && finalStream.str() == source;
    finalCheck.close();
    if (!sourceStillMatches) {
        std::filesystem::remove(staging, fileError);
        return Failed("the source scene changed before replacement; the version 1 backup remains intact");
    }
    std::filesystem::rename(staging, path, fileError);
    if (fileError) {
        std::error_code ignored;
        std::filesystem::remove(staging, ignored);
        return Failed(std::format("could not replace {}: {}; the version 1 backup remains at {}",
                                  PathText(path), fileError.message(), PathText(backup)));
    }
    extras->version = 2;
    if (converted != nullptr) converted->swap(candidate);
    SceneReport report;
    report.warnings = std::move(warnings);
    return report;
}

SceneReport LoadSceneFile(World& world, const ComponentTypes& types, const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return Failed(std::format("could not open {}", PathText(path)));
    }
    std::ostringstream text;
    text << in.rdbuf();
    return ReadScene(world, types, text.str());
}

} // namespace pixie
