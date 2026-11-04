#pragma once

#include "pixie/runtime/Export.hpp"

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

// The scene file's format name and the newest version this engine can read and write.
// A normal write retains the version of a loaded scene; new Worlds begin with version 1.
inline constexpr std::string_view kSceneFormat = "pixie-scene";
inline constexpr int kSceneVersion = 2;
// The format name of a set of entities written apart from their scene (WriteEntities), with the
// scene's version.
inline constexpr std::string_view kEntitiesFormat = "pixie-entities";

// A new random scene ID, never 0. IDs come from their own generator, not from a World's Random, so
// making one does not change a simulation.
PIXIE_RUNTIME_API std::uint64_t NewSceneId();

// Gives the entity a SceneId if it has none, and returns its ID; 0 for a dead entity. Not during a
// step.
PIXIE_RUNTIME_API std::uint64_t AddToScene(Entity entity);

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
PIXIE_RUNTIME_API std::string WriteScene(World& world, const ComponentTypes& types, std::vector<std::string>* warnings = nullptr);

// Read-only admission check before writing a v2 World snapshot. Reports registered component
// types that cannot enter v2 and conflicting settings authorities, without invoking descriptor
// callbacks. A v1 World returns empty so legacy duplicate reads remain available.
// Other serialization failures remain the responsibility of WriteScene.
PIXIE_RUNTIME_API std::string ValidateV2SceneSnapshot(const World& world, const ComponentTypes& types);
// New candidate edits require a v2 envelope and an unambiguous existing settings authority.
// ReadScene still performs complete input admission on the isolated candidate.
PIXIE_RUNTIME_API std::string ValidateSceneEditInput(const World& world, std::string_view text);
// Pure admission before candidate factory/Describe/World construction. v1 retains its behavior.
PIXIE_RUNTIME_API std::string SceneImportAdmissionError(const ComponentTypes& types, std::string_view text);

// Adds a scene document's entities to `world` and reseeds the World's Random with the scene's seed.
// Nothing is added when the document cannot be read, or when one of its IDs is repeated or already
// in the World. Not during a step.
PIXIE_RUNTIME_API SceneReport ReadScene(World& world, const ComponentTypes& types, std::string_view text);

// Writes `text` to a file through a temporary file beside it (`<file>.saving`), which then replaces
// the file, so a failed write leaves the old one.
PIXIE_RUNTIME_API SceneReport SaveTextFile(std::string_view text, const std::filesystem::path& path);

// Some of the World's scene entities, written apart from their scene: a document with format
// kEntitiesFormat and the entities' list in the order given. A v2 fragment also carries the
// definitions of the scopes referenced by those entities. An Entity field holds
// its target's scene ID, inside the set or not. An entity that is gone or not in the scene is left
// out, with a warning. For snapshots a tool takes to bring entities back, or to copy them.
PIXIE_RUNTIME_API std::string WriteEntities(World& world, const ComponentTypes& types, std::span<const Entity> entities,
                          std::vector<std::string>* warnings = nullptr);
// The same, with `meta`, a JSON object as text, written at the top of the document: what a game keeps
// about a saved set, such as a template's name. Meta that is not a JSON object is left out, with a
// warning.
PIXIE_RUNTIME_API std::string WriteEntities(World& world, const ComponentTypes& types, std::span<const Entity> entities,
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
// cannot be read, or, with SceneIds::Keep, when one of its IDs is already in the World. A v2
// fragment requires a v2 World and rejects conflicting scope definitions before mutation. Unknown
// components and fields are kept as a scene keeps them. Its cost is the document's, not the World's
// size: the World keeps its scene entities by ID from the first read on. Not during a step.
PIXIE_RUNTIME_API EntitiesReport ReadEntities(World& world, const ComponentTypes& types, std::string_view text, SceneIds ids);

// Structural preflight only: no World mutation, ID generation or component callbacks.
// Replacement IDs logically exclude their current owners (Keep mode only). A passing
// result does not guarantee that later constructors, writers or external hooks succeed.
PIXIE_RUNTIME_API SceneReport ValidateEntities(const World& world, const ComponentTypes& types,
    std::string_view text, SceneIds ids, std::span<const std::uint64_t> replacing = {});

// Prepare a complete v2 candidate document without touching the published World.
// The JSON result contains `document` and ordered hexadecimal `ids`. Empty fragment
// text means deletion of replacing IDs. Only qualified declarative types are used
// in an unbuilt scratch World; the caller must publish through its project factory.
struct SceneFragmentPlan : SceneReport { std::string text; };
PIXIE_RUNTIME_API SceneFragmentPlan PrepareSceneFragment(World& world, const ComponentTypes& types,
    std::string_view fragment, SceneIds ids, std::span<const std::uint64_t> replacing = {},
    double offsetX = 0, double offsetY = 0);

// What ReadFragmentMeta found: a set of entities' meta object, as compact JSON text (empty when it
// has none), and how many entities the set holds.
struct FragmentMeta : SceneReport {
    std::string meta;
    std::size_t entityCount = 0;
};

// Reads the meta of a set of entities written by WriteEntities without building anything, so a game
// can list its saved sets (its templates) by name and size quickly. Fails as ReadEntities does when
// the document is not a set of entities this engine reads.
PIXIE_RUNTIME_API FragmentMeta ReadFragmentMeta(std::string_view text);

// Writes the World's scene through SaveTextFile. A v2 document cannot replace an existing v1
// file: use explicit ConvertSceneFileV1ToV2 first. The destination envelope is inspected without
// component callbacks; this is not a concurrent-writer compare-and-swap guarantee.
PIXIE_RUNTIME_API SceneReport SaveSceneFile(World& world, const ComponentTypes& types, const std::filesystem::path& path);
// Explicitly converts an existing v1 scene file from the current authored World. Validates a staged
// v2 document, preserves the original at `<file>.v1-backup`, then replaces the file. Never called by
// ordinary open, play, rebuild or save. A failed conversion leaves the World and original file intact.
PIXIE_RUNTIME_API SceneReport ConvertSceneFileV1ToV2(World& world, const ComponentTypes& types,
                                                     const std::filesystem::path& path);
// Successful conversion also hands the committed text to a session without allocating after replacement.
PIXIE_RUNTIME_API SceneReport ConvertSceneFileV1ToV2(World& world, const ComponentTypes& types,
                                                    const std::filesystem::path& path, std::string* converted);
PIXIE_RUNTIME_API int SceneFormatVersion(const World& world);
// ReadScene from a file.
PIXIE_RUNTIME_API SceneReport LoadSceneFile(World& world, const ComponentTypes& types, const std::filesystem::path& path);

} // namespace pixie
