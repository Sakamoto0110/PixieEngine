#pragma once

#include "pixie/dev/Dispatcher.hpp"

#include <nlohmann/json.hpp>

#include <functional>
#include <map>
#include <stdexcept>
#include <string>

namespace pixie {
class World;

namespace dev {

using CommandJson = nlohmann::json;

/// Use only to reject an invalid argument before any game state changes.
struct CommandArgumentError : std::invalid_argument { using std::invalid_argument::invalid_argument; };

/// Command metadata and handlers belong to one private player session.
class GameCommands {
public:
    using Handler = std::function<CommandJson(ProjectSession&, const CommandJson&)>;

    /// Parameters use an object schema with named properties. The dispatcher checks its
    /// required keys, types, enums and numeric bounds before calling the handler.
    PIXIE_DEV_API void Add(std::string name, std::string subsystem, std::string description,
                           CommandJson parameters, CommandJson result, Handler handler);

    CommandJson Describe(const CommandJson& filter = CommandJson::object()) const;
    const Handler* Find(std::string_view name, const CommandJson& parameters) const;
    bool Empty() const { return commands_.empty(); }

private:
    struct Entry {
        CommandJson metadata;
        Handler handler;
    };
    std::map<std::string, Entry, std::less<>> commands_;
};

/// Optional private-profile mixin. A game still implements the ordinary Project interface.
class GameCommandProvider {
public:
    virtual ~GameCommandProvider() = default;
    virtual void RegisterGameCommands(GameCommands& commands) = 0;
    virtual void ReleaseGameControls(World&) noexcept {}
};

} // namespace dev
} // namespace pixie
