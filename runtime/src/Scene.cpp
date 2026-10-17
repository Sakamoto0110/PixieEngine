#include "pixie/runtime/Scene.hpp"

#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/Name.hpp"
#include "pixie/runtime/World.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <format>
#include <fstream>
#include <random>
#include <span>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace pixie {
namespace detail {

// What a loaded entity had that this build does not know, kept as JSON text to be written back
// unchanged: {"entity": {key: value}, "components": {type: value}, "fields": {type: {field: value}}}.
struct SceneExtras {
    std::string json;
};

// The same for the document's own keys, kept in the registry's context.
struct SceneDocumentExtras {
    std::string json;
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
    }
    return "unknown field kind";
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
        void* component = type.find(registry, id);
        if (component == nullptr) {
            continue;
        }
        Json fields = Json::object();
        fields["v"] = type.version;
        const std::string where = std::format("{} of entity {}", type.name, hex);
        for (const FieldDescriptor& field : type.fields) {
            if (field.kind == FieldKind::Text) {
                fields[field.name] = field.readText(component);
            } else if (field.kind == FieldKind::PointList) {
                Json points = Json::array();
                for (const pxPoint& p : field.readPoints(component)) {
                    points.push_back(Json::array({p.x, p.y}));
                }
                fields[field.name] = std::move(points);
            } else {
                fields[field.name] = Encode(field, field.read(component), registry, where, warnings);
            }
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
Json ParseDocument(std::string_view text, std::string_view format, std::string_view what, std::string& error) {
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
    } else if (version > kSceneVersion) {
        error = std::format("the {} was written by a newer engine (version {}; this one reads up to {})", what,
                            version, kSceneVersion);
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
                std::string problem;
                if (known->kind == FieldKind::Text) {
                    if (field.value().is_string()) {
                        known->writeText(component, field.value().get_ref<const std::string&>());
                    } else {
                        problem = "expected text";
                    }
                } else if (known->kind == FieldKind::PointList) {
                    std::vector<pxPoint> points;
                    if (field.value().is_array()) {
                        for (const Json& item : field.value()) {
                            double xy[2];
                            if (!Numbers(item, 2, xy)) {
                                problem = "expected [[x, y], ...]";
                                break;
                            }
                            points.emplace_back(xy[0], xy[1]);
                        }
                    } else {
                        problem = "expected [[x, y], ...]";
                    }
                    if (problem.empty()) {
                        known->writePoints(component, points);
                    }
                } else {
                    FieldValue value;
                    problem = Decode(*known, field.value(), ids, value);
                    if (problem.empty()) {
                        known->write(component, value);
                    }
                }
                if (!problem.empty()) {
                    report.warnings.push_back(std::format("{}.{} of entity {}: {}; left at its default", typeName,
                                                          field.key(), hex, problem));
                }
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

std::string WriteScene(World& world, const ComponentTypes& types, std::vector<std::string>* warnings) {
    entt::registry& registry = world.Registry();
    Json document = Json::object();
    document["format"] = std::string(kSceneFormat);
    document["version"] = kSceneVersion;
    document["seed"] = world.GetRandom().Seed();
    if (const auto* kept = registry.ctx().find<detail::SceneDocumentExtras>()) {
        Json extras = ParseKept(kept->json);
        extras.erase("entities");
        Merge(document, extras);
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
        entities.push_back(WriteEntity(registry, types, id, warnings));
    }
    document["entities"] = std::move(entities);
    return Finish(document);
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

    // Every ID is checked before anything is added, so a bad file leaves the World as it was.
    entt::registry& registry = world.Registry();
    const std::unordered_map<std::uint64_t, entt::entity>& ids = SceneEntities(registry);
    std::vector<std::uint64_t> listed;
    if (std::string problem = CheckEntities(list, ids, true, listed); !problem.empty()) {
        return Failed(std::move(problem));
    }

    SceneReport report;
    if (document.contains("seed") && document["seed"].is_number_unsigned()) {
        world.GetRandom().Reseed(document["seed"].get<std::uint64_t>());
    } else {
        report.warnings.push_back("the scene has no valid seed; the World keeps its own");
    }
    AddEntities(world, types, list, listed, listed, ids, report);

    Json keptDocument = Json::object();
    for (auto it = document.begin(); it != document.end(); ++it) {
        const std::string& key = it.key();
        if (key != "format" && key != "version" && key != "seed" && key != "entities") {
            keptDocument[key] = it.value();
        }
    }
    if (keptDocument.empty()) {
        registry.ctx().erase<detail::SceneDocumentExtras>();
    } else {
        registry.ctx().insert_or_assign(detail::SceneDocumentExtras{keptDocument.dump()});
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
    Json document = Json::object();
    document["format"] = std::string(kEntitiesFormat);
    document["version"] = kSceneVersion;
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
    for (const Entity entity : entities) {
        if (!registry.valid(entity.Id()) || !registry.all_of<SceneId>(entity.Id())) {
            if (warnings != nullptr) {
                warnings->push_back(std::format("entity {} is gone or not in the scene; left out", entity.Number()));
            }
            continue;
        }
        if (written.insert(entity.Id()).second) {
            list.push_back(WriteEntity(registry, types, entity.Id(), warnings));
        }
    }
    document["entities"] = std::move(list);
    return Finish(document);
}

EntitiesReport ReadEntities(World& world, const ComponentTypes& types, std::string_view text, SceneIds sceneIds) {
    EntitiesReport report;
    const auto fail = [&](std::string error) {
        report.ok = false;
        report.error = std::move(error);
        return report;
    };
    if (world.IsStepping()) {
        return fail("entities cannot be read during a step");
    }
    std::string error;
    const Json document = ParseDocument(text, kEntitiesFormat, "set of entities", error);
    if (!error.empty()) {
        return fail(std::move(error));
    }
    const Json& list = document["entities"];
    const std::unordered_map<std::uint64_t, entt::entity>& ids = SceneEntities(world.Registry());
    std::vector<std::uint64_t> listed;
    if (std::string problem = CheckEntities(list, ids, sceneIds == SceneIds::Keep, listed); !problem.empty()) {
        return fail(std::move(problem));
    }

    std::vector<std::uint64_t> assigned = listed;
    if (sceneIds == SceneIds::New) {
        for (std::uint64_t& id : assigned) {
            do {
                id = NewSceneId();
            } while (ids.contains(id));
        }
    }
    report.entities = AddEntities(world, types, list, listed, assigned, ids, report);
    report.meta = MetaText(document, report.warnings);
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

SceneReport SaveSceneFile(World& world, const ComponentTypes& types, const std::filesystem::path& path) {
    std::vector<std::string> warnings;
    SceneReport report = SaveTextFile(WriteScene(world, types, &warnings), path);
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
