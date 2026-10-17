#pragma once

#include "pixie/runtime/Entity.hpp"
#include "pixie/runtime/World.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace pixie {

class ComponentTypes;

// Makes an entity part of the scene, under an ID that stays the same through saving and loading.
// Only entities with one are saved; what project code generates while building (boids from a
// count) has none, and is built again instead. 0 is never an ID. The World finds its scene entities
// by ID, so an ID is changed through Entity::Add, never in place.
struct SceneId {
    std::uint64_t value = 0;
};

// The scene file's format name and the newest version this engine reads and writes.
inline constexpr std::string_view kSceneFormat = "pixie-scene";
inline constexpr int kSceneVersion = 1;
// The format name of a set of entities written apart from their scene (WriteEntities), with the
// scene's version.
inline constexpr std::string_view kEntitiesFormat = "pixie-entities";

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

// Some of the World's scene entities, written apart from their scene: a document of the scene's form,
// with format kEntitiesFormat and only the entities' list, in the order given. An Entity field holds
// its target's scene ID, inside the set or not. An entity that is gone or not in the scene is left
// out, with a warning. For snapshots a tool takes to bring entities back, or to copy them.
std::string WriteEntities(World& world, const ComponentTypes& types, std::span<const Entity> entities,
                          std::vector<std::string>* warnings = nullptr);
// The same, with `meta`, a JSON object as text, written at the top of the document: what a game keeps
// about a saved set, such as a template's name. Meta that is not a JSON object is left out, with a
// warning.
std::string WriteEntities(World& world, const ComponentTypes& types, std::span<const Entity> entities,
                          std::string_view meta, std::vector<std::string>* warnings = nullptr);

// How ReadEntities gives the entities it adds their scene IDs.
enum class SceneIds {
    // The IDs written: none may be in the World, as when bringing back entities that were destroyed.
    Keep,
    // New IDs: a copy. An Entity field that named an entity of the set names its copy; one that named
    // another entity keeps naming it, if it is in the World.
    New,
};

// What ReadEntities found, and the entities it added, in the document's order.
struct EntitiesReport : SceneReport {
    std::vector<Entity> entities;
    // The document's meta object, as compact JSON text; empty when it has none.
    std::string meta;
};

// Adds a set of entities written by WriteEntities to the World. Nothing is added when the document
// cannot be read, or, with SceneIds::Keep, when one of its IDs is already in the World. Unknown
// components and fields are kept as a scene keeps them. Its cost is the document's, not the World's
// size: the World keeps its scene entities by ID from the first read on. Not during a step.
EntitiesReport ReadEntities(World& world, const ComponentTypes& types, std::string_view text, SceneIds ids);

// What ReadFragmentMeta found: a set of entities' meta object, as compact JSON text (empty when it
// has none), and how many entities the set holds.
struct FragmentMeta : SceneReport {
    std::string meta;
    std::size_t entityCount = 0;
};

// Reads the meta of a set of entities written by WriteEntities without building anything, so a game
// can list its saved sets (its templates) by name and size quickly. Fails as ReadEntities does when
// the document is not a set of entities this engine reads.
FragmentMeta ReadFragmentMeta(std::string_view text);

// WriteScene to a file, through SaveTextFile.
SceneReport SaveSceneFile(World& world, const ComponentTypes& types, const std::filesystem::path& path);
// ReadScene from a file.
SceneReport LoadSceneFile(World& world, const ComponentTypes& types, const std::filesystem::path& path);

} // namespace pixie
