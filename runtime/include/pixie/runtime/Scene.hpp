#pragma once

#include "pixie/runtime/Entity.hpp"
#include "pixie/runtime/World.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace pixie {

class ComponentTypes;

// Makes an entity part of the scene, under an ID that stays the same through saving and loading.
// Only entities with one are saved; what project code generates while building (boids from a
// count) has none, and is built again instead. 0 is never an ID.
struct SceneId {
    std::uint64_t value = 0;
};

// The scene file's format name and the newest version this engine reads and writes.
inline constexpr std::string_view kSceneFormat = "pixie-scene";
inline constexpr int kSceneVersion = 1;

// A new random scene ID, never 0. IDs come from their own generator, not from a World's Random, so
// making one does not change a simulation.
std::uint64_t NewSceneId();

// Gives the entity a SceneId if it has none, and returns its ID; 0 for a dead entity. Not during a
// step.
std::uint64_t AddToScene(Entity entity);

// An entity with a T: what a project finds of its scene while it builds. When there is none,
// as when no scene was loaded, a new scene entity named `name` with `value`, so a save keeps it. Not
// during a step.
template<typename T>
Entity SceneEntityWith(World& world, std::string_view name, T value = {}) {
    if (auto view = world.Registry().view<T>(); view.begin() != view.end()) {
        return world.Wrap(*view.begin());
    }
    Entity created = world.Create(name);
    created.Add<T>(std::move(value));
    AddToScene(created);
    return created;
}

// What reading or writing a scene found: whether it worked, why not, and what it had to leave out.
struct SceneReport {
    bool ok = true;
    std::string error;
    std::vector<std::string> warnings;
};

// The World's scene entities as a scene document: JSON text, oldest entity first. Components are
// written through their descriptors in `types`; what a loaded file had that `types` does not know
// is written back as it was read. Warnings go to `warnings` when given.
std::string WriteScene(World& world, const ComponentTypes& types, std::vector<std::string>* warnings = nullptr);

// Adds a scene document's entities to `world` and reseeds the World's Random with the scene's seed.
// Nothing is added when the document cannot be read, or when one of its IDs is repeated or already
// in the World. Not during a step.
SceneReport ReadScene(World& world, const ComponentTypes& types, std::string_view text);

// Writes `text` to a file through a temporary file beside it (`<file>.saving`), which then replaces
// the file, so a failed write leaves the old one.
SceneReport SaveTextFile(std::string_view text, const std::filesystem::path& path);

// WriteScene to a file, through SaveTextFile.
SceneReport SaveSceneFile(World& world, const ComponentTypes& types, const std::filesystem::path& path);
// ReadScene from a file.
SceneReport LoadSceneFile(World& world, const ComponentTypes& types, const std::filesystem::path& path);

} // namespace pixie
