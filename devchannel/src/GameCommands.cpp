#include "pixie/dev/GameCommands.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace pixie::dev {
namespace {

bool NameAllowed(std::string_view name) {
    constexpr std::string_view player = "game.player.", developer = "game.dev.";
    if (name.starts_with(player)) name.remove_prefix(player.size());
    else if (name.starts_with(developer)) name.remove_prefix(developer.size());
    else return false;
    return !name.empty() && name.size() <= 48 && name.front() >= 'a' && name.front() <= 'z' &&
           std::ranges::all_of(name, [](char ch) {
               return (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_';
           });
}

void Validate(const CommandJson& schema, const CommandJson& value, const std::string& field) {
    const auto type = schema.at("type").get<std::string>();
    const bool matches = (type == "string" && value.is_string()) || (type == "boolean" && value.is_boolean()) ||
        (type == "object" && value.is_object()) || (type == "array" && value.is_array()) ||
        (type == "integer" && value.is_number_integer()) || (type == "number" && value.is_number());
    if (!matches) throw std::invalid_argument(field + " must be " + type);
    if (const auto options = schema.find("enum"); options != schema.end() &&
        std::ranges::find(*options, value) == options->end())
        throw std::invalid_argument(field + " is not an available option");
    if (value.is_number()) {
        const double number = value.get<double>();
        if (!std::isfinite(number) || (schema.contains("minimum") && number < schema.at("minimum").get<double>()) ||
            (schema.contains("maximum") && number > schema.at("maximum").get<double>()))
            throw std::invalid_argument(field + " is outside the supported range");
    }
    if (value.is_object()) {
        if (const auto required = schema.find("required"); required != schema.end())
            for (const auto& key : *required)
                if (!value.contains(key.get<std::string>())) throw std::invalid_argument(field + " is missing " + key.get<std::string>());
        const auto properties = schema.find("properties");
        for (const auto& [name, item] : value.items()) {
            if (properties != schema.end() && properties->contains(name)) Validate(properties->at(name), item, name);
            else if (!schema.value("additionalProperties", true)) throw std::invalid_argument("unknown parameter: " + name);
        }
    }
    if (value.is_array() && schema.contains("items"))
        for (const auto& item : value) Validate(schema.at("items"), item, field);
}

} // namespace

void GameCommands::Add(std::string name, std::string subsystem, std::string description,
                       CommandJson parameters, CommandJson result, Handler handler) {
    if (!NameAllowed(name) || name == "game.dev.describe" || subsystem.empty() || description.empty() ||
        !parameters.is_object() || parameters.value("type", "") != "object" ||
        !result.is_object() || result.value("type", "") != "object" || !handler)
        throw std::invalid_argument("invalid game command registration: " + name);
    if (commands_.size() >= 128 || commands_.contains(name))
        throw std::invalid_argument("duplicate game command or registry limit exceeded: " + name);
    parameters["additionalProperties"] = false;
    CommandJson metadata{{"name", name}, {"subsystem", std::move(subsystem)}, {"description", std::move(description)},
                         {"inputSchema", std::move(parameters)}, {"outputSchema", std::move(result)}};
    commands_.emplace(std::move(name), Entry{std::move(metadata), std::move(handler)});
}

CommandJson GameCommands::Describe(const CommandJson& filter) const {
    const CommandJson schema = {{"type", "object"}, {"additionalProperties", false}, {"properties", {
        {"command", {{"type", "string"}}}, {"subsystem", {{"type", "string"}}}}}};
    Validate(schema, filter, "parameters");
    if (filter.size() > 1) throw std::invalid_argument("select either command or subsystem");
    auto catalog = CommandJson::array();
    const auto include = [&](const CommandJson& metadata) {
        if ((filter.contains("command") && metadata.at("name") != filter.at("command")) ||
            (filter.contains("subsystem") && metadata.at("subsystem") != filter.at("subsystem"))) return;
        catalog.push_back(metadata);
    };
    if (!Empty()) include({{"name", "game.dev.describe"}, {"subsystem", "discovery"},
        {"description", "Describe the game commands available in this session."}, {"inputSchema", schema},
        {"outputSchema", {{"type", "object"}, {"properties", {{"commands", {{"type", "array"}}}}}, {"required", {"commands"}}}}});
    for (const auto& [name, entry] : commands_) include(entry.metadata);
    return catalog;
}

const GameCommands::Handler* GameCommands::Find(std::string_view name, const CommandJson& parameters) const {
    const auto found = commands_.find(name);
    if (found == commands_.end()) return nullptr;
    Validate(found->second.metadata.at("inputSchema"), parameters, "parameters");
    return &found->second.handler;
}

} // namespace pixie::dev
