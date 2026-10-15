#include "pixie/runtime/UserData.hpp"

#include "pixie/Log.hpp"

#include <cstdlib>
#include <format>
#include <optional>
#include <string>
#include <system_error>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shlobj.h>
#endif

namespace pixie {

namespace {

// An environment variable naming a folder; nothing when it is not set or empty.
std::optional<std::filesystem::path> EnvironmentFolder(const char* name) {
#ifdef _WIN32
    const std::wstring wide(name, name + std::char_traits<char>::length(name));
    wchar_t* value = nullptr;
    std::size_t length = 0;
    if (_wdupenv_s(&value, &length, wide.c_str()) != 0 || value == nullptr) {
        return std::nullopt;
    }
    std::filesystem::path folder(value);
    std::free(value);
#else
    const char* value = std::getenv(name);
    if (value == nullptr) {
        return std::nullopt;
    }
    std::filesystem::path folder(value);
#endif
    if (folder.empty()) {
        return std::nullopt;
    }
    return folder;
}

// Where games' folders go; nothing, with `error` set, when it cannot be found.
std::optional<std::filesystem::path> UserDataRoot(std::string& error) {
    if (auto chosen = EnvironmentFolder("PIXIE_USER_DATA")) {
        return chosen;
    }
#ifdef _WIN32
    PWSTR roaming = nullptr;
    const HRESULT result = SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, nullptr, &roaming);
    std::optional<std::filesystem::path> root;
    if (SUCCEEDED(result) && roaming != nullptr) {
        root = std::filesystem::path(roaming);
    } else {
        error = "the Roaming AppData folder is unknown";
    }
    CoTaskMemFree(roaming);
    return root;
#else
    if (auto data = EnvironmentFolder("XDG_DATA_HOME")) {
        return data;
    }
    if (auto home = EnvironmentFolder("HOME")) {
        return *home / ".local" / "share";
    }
    error = "neither XDG_DATA_HOME nor HOME is set";
    return std::nullopt;
#endif
}

std::string PathText(const std::filesystem::path& path) {
    const std::u8string text = path.u8string();
    return std::string(reinterpret_cast<const char*>(text.data()), text.size());
}

} // namespace

std::string UserDataFolderName(std::string_view name) {
    std::string out;
    out.reserve(name.size());
    for (const char c : name) {
        const auto byte = static_cast<unsigned char>(c);
        const bool refused = byte < 0x20 || byte == 0x7F || std::string_view(R"(<>:"/\|?*)").find(c) != std::string_view::npos;
        out += refused ? '_' : c;
    }
    while (!out.empty() && (out.back() == '.' || out.back() == ' ')) {
        out.pop_back();
    }
    return out;
}

std::filesystem::path UserDataFolder(std::string_view projectName) {
    const std::string name = UserDataFolderName(projectName);
    if (name.empty()) {
        LogError(std::format("no user data folder for the project \"{}\": its name makes no folder name", projectName));
        return {};
    }
    std::string error;
    const std::optional<std::filesystem::path> root = UserDataRoot(error);
    if (!root) {
        LogError(std::format("no user data folder for {}: {}", name, error));
        return {};
    }
    const std::filesystem::path folder =
        *root / std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(name.data()), name.size()));
    std::error_code made;
    std::filesystem::create_directories(folder, made);
    if (made || !std::filesystem::is_directory(folder, made)) {
        LogError(std::format("the user data folder {} cannot be made: {}", PathText(folder), made.message()));
        return {};
    }
    return folder;
}

} // namespace pixie
