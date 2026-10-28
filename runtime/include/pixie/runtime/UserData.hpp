#pragma once

#include "pixie/runtime/Export.hpp"

#include <filesystem>
#include <string>
#include <string_view>

namespace pixie {

// The folder for a game's own files, such as its player's saves (SaveSceneFile, WriteEntities),
// made on first use: `%APPDATA%/<project>/` on Windows, `$XDG_DATA_HOME/<project>/` elsewhere
// (`~/.local/share/<project>/` when XDG_DATA_HOME is not set). When the environment variable
// PIXIE_USER_DATA names a folder, it is used in place of those, as tests do so they never touch the
// real one.
//
// `projectName` is the project's name, in UTF-8; characters a file name cannot hold become `_`.
// Returns an empty path, after logging why, when the name is empty or the folder cannot be made.
PIXIE_RUNTIME_API std::filesystem::path UserDataFolder(std::string_view projectName);

// `name` as a folder name: every character Windows refuses in a file name (`<>:"/\|?*` and control
// characters) becomes `_`, and trailing dots and spaces are dropped; empty when nothing is left, or
// for "." and "..".
PIXIE_RUNTIME_API std::string UserDataFolderName(std::string_view name);

} // namespace pixie
