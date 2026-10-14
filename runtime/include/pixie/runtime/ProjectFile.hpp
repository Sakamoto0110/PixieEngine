#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace pixie {

// The project file's format name, the newest version this engine reads and writes, and its
// extension. A project is a folder holding one `<Name>.pixie` file and its scenes.
inline constexpr std::string_view kProjectFormat = "pixie-project";
inline constexpr int kProjectVersion = 1;
inline constexpr std::string_view kProjectExtension = ".pixie";

// What a project file says. Paths are relative to the project's folder, with forward slashes, and
// never leave it: nothing in a project depends on the machine it is on, or on where the engine is.
struct ProjectFile {
    std::string name;
    std::string startScene = "scenes/main.scene";
    // Keys of the file that this engine does not know, as JSON text, written back unchanged.
    std::string extras;
};

// A project found in a folder: its file's contents, the folder, and the file itself.
struct OpenedProject {
    bool ok = true;
    std::string error;
    ProjectFile file;
    std::filesystem::path folder;
    std::filesystem::path path;

    // The start scene's file.
    std::filesystem::path StartScenePath() const;
};

// Reads the one `.pixie` file in `folder`. Fails when there is none, more than one, or it cannot be
// read; when its format or version is not this engine's; or when its start scene is not a relative
// path inside the folder.
OpenedProject OpenProjectFolder(const std::filesystem::path& folder);

// Writes a project file; returns what went wrong, or nothing.
std::string WriteProjectFile(const std::filesystem::path& path, const ProjectFile& project);

// Makes a new project in `folder`, which must be empty or not exist yet: the project takes the
// folder's name, and gets `<Name>.pixie` and `scenes/main.scene`, whose one entity is a shape at
// the origin. Returns the project as OpenProjectFolder would.
OpenedProject CreateProjectFolder(const std::filesystem::path& folder);

// True when `path` is relative, uses forward slashes, and has no "." or ".." part, so it stays
// inside the folder it is relative to.
bool IsProjectRelativePath(std::string_view path);

} // namespace pixie
