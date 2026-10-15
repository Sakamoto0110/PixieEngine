# Projects Created in the Editor, with Scene Serialization

**Document ID:** GLOBAL-PROPOSAL-PROJECTS

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** current

**Subject:** game projects created and opened from the editor, portable across machines, together with the scene serialization postponed from PIXIE-PROPOSAL-004

**Surface:** proposal

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## PIXIE-PROPOSAL-005

**State:** promoted

**Idea:** A game lives in a project folder of its own, outside the engine's
repository, created and opened from the editor. Scenes are saved in it, so this
phase carries part 3 of
[PIXIE-PROPOSAL-004](scene-model-and-anchors.md) (scene serialization). Eight
parts.

1. **The flow.** The editor started without `--game` or `--project` opens a
   start screen: New project, Open project, and the recent projects. New
   project asks for a folder, empty or not yet existing; the project takes the
   folder's name, and the editor writes the project and opens it. Open project
   asks for a folder that holds a `.pixie` file. No command-line tool, `PATH`
   change, or environment variable is involved. `--project <dir>` opens a
   project directly; `--game` stays for the samples and the tests.
2. **The folder.** `<Name>.pixie`, a small JSON file (`format`
   `"pixie-project"`, `version`, `name`, and the start scene), and
   `scenes/main.scene`. Every path in it is relative to the folder, and nothing
   in it is specific to a machine. The editor creates no version-control files:
   versioning belongs to the user. What belongs to one machine (the recent
   projects, the dock layout) stays in the editor's own per-user settings.
3. **The engine's location.** A project never records where the engine is.
   The editor runs from the engine's tree and supplies that engine to whatever
   it opens, so the same folder opens on another machine in that machine's
   editor.
4. **Scene serialization.** The format of PIXIE-PROPOSAL-004, part 3: JSON
   through nlohmann/json, with `format`, `version`, and a flat `entities` list;
   each entity has a stable ID (a random u64 written as a hex string), a `name`,
   a `parent` by ID, and `components` by type name, each with its own `v`; the
   World's seed is saved. Added here:
   - An entity belongs to the scene when it has a stable ID (a `SceneId`
     component). Entities that project code generates while building (5,000
     boids from a count) have none and are not saved; the scenario they come
     from is.
   - Components are read and written through the field descriptors, the same
     ones the C API exports. A descriptor also learns to add and remove its
     component on an entity by type name, so a Behaviour declared with its
     fields is saved like any component. What has no descriptor (the contents
     of a particle pool, affectors, systems) is not saved; project code
     rebuilds it.
   - An `Entity` field is written as the stable ID of the entity it
     references, never as its handle.
   - Nothing in a file is lost by opening and saving it: unknown components and
     unknown fields of known components are kept as raw JSON and written back
     unchanged; a field missing from the file keeps its default.
   - No entity has a parent yet; `parent` is written as absent until a
     hierarchy exists.
   - The code lives in the `runtime` module, which already owns the World and
     the descriptors. A separate `scene` boundary is not created.
5. **Edit and play.** A project opens in edit mode: the World is built from
   the scene and does not step. Edits change that World, which stays the
   single source of truth; Save (Ctrl + S) writes its scene entities to the
   file, and the window's title marks unsaved changes. Play keeps the scene
   entities in memory and runs; Stop throws the World away and builds it again
   from that copy, so nothing done in play stays. The editor shows plainly that
   it is playing. "Rebuild the scene" builds the World again from the current
   scene entities.
6. **A project without code.** The engine builds one generic C API library
   whose project only loads the scene, with the engine's own component types.
   A new project opens at once, without a compiler, and its scene has one shape
   at the origin, so the viewport shows something and the inspector has
   something to select. The editor can create an entity and add or remove a
   component from the descriptors' list, which is what authoring a scene
   without code needs.
7. **Code in a project.** Project, Add code, writes `CMakeLists.txt` and
   `src/<Name>Project.{hpp,cpp}` into the project from templates kept in the
   engine's repository: a Project whose build loads the scene, and one example
   Behaviour declared with a field, so it can be added to an entity and edited.
   - `pixie_game(<name> SOURCES ...)` creates the three targets a game has
     today (its library, its player, and its C API library). The player's
     `main` belongs to the engine, with the standard options that are in
     `samples/common` today, and starts the project's start scene.
   - The editor builds the code by calling CMake with the engine it runs from
     (`PIXIE_ENGINE_DIR`), into `<project>/build/`, with binaries in
     `<project>/bin/`; it finds CMake on `PATH` or, failing that, through
     Visual Studio's `vswhere`. It shows the build's output and reopens itself
     on the project's library, since P/Invoke binds one library per process.
     A missing compiler or CMake is reported by name, with how to install it.
   - Code is the only part of a project that needs a toolchain.
8. **Samples as projects.** Each sample gets a `.pixie` file and a scene with
   its authored entities (scenario, controls, attractors, spawners), builds
   through `pixie_game`, and stays in the repository. The engine then uses the
   same path as a user's project, and a CTest test generates a project with
   code from the templates and builds it, so the templates cannot rot.

Out of this phase: exporting a standalone build of a game, prefabs, nested
scenes, a binary scene format, reloading code without restarting the editor,
undo, and a native folder dialog (the first cut draws its folder picker in
ImGui, which works on every editor host).

**Rationale:** The person making a game should go from an empty folder to an
edited, saved scene without a compiler, a terminal, or a path to configure, and
should be able to copy or clone that folder to another machine and open it
there. A first design with a `pixie` command on `PATH` and `git init` was
rejected by the owner as too complicated and tied to one machine. Serialization
comes in the same phase because a project with nothing to save is only a
folder of code.

**Constraints:** The engine never depends on the editor: the player loads a
project's scenes on its own. The editor's folder picker must not assume
Windows. macOS is not weighed.

**Promotion target:** promoted to `TODO.md` on 2026-10-14 as PIXIE-TODO-018 to
PIXIE-TODO-022.

**Source:** [decision record](../archive/projects-and-scenes-decisions-2026-10-14.md)

**Outcome:** promoted on 2026-10-14, as recorded in the
[promotion record](../archive/projects-and-scenes-promotion-2026-10-14.md), and carried out the
same day: PIXIE-TODO-018 to PIXIE-TODO-022 are completed (PIXIE-ARCHIVE-035, 036, 037, 039, and
040).
