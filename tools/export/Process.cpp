#include "Process.hpp"
#include <stdexcept>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <spawn.h>
#include <sys/wait.h>
extern char** environ;
#endif
namespace pixie::packaging {
std::string Utf8(const std::filesystem::path& path) {
    const auto text = path.generic_u8string();
    return {reinterpret_cast<const char*>(text.data()), text.size()};
}
std::filesystem::path FromUtf8(const std::string& text) {
    return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(text.data()), text.size()));
}
#ifdef _WIN32
namespace {
struct Handle {
    HANDLE value = INVALID_HANDLE_VALUE;
    ~Handle() { if (value && value != INVALID_HANDLE_VALUE) CloseHandle(value); }
};
HANDLE StandardHandle(DWORD standard, DWORD access) {
    const auto source = GetStdHandle(standard);
    HANDLE result = INVALID_HANDLE_VALUE;
    if (source && source != INVALID_HANDLE_VALUE && DuplicateHandle(GetCurrentProcess(), source,
            GetCurrentProcess(), &result, 0, TRUE, DUPLICATE_SAME_ACCESS)) return result;
    SECURITY_ATTRIBUTES attributes{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    result = CreateFileW(L"NUL", access, FILE_SHARE_READ | FILE_SHARE_WRITE, &attributes,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (result == INVALID_HANDLE_VALUE) throw std::runtime_error("could not prepare child standard handles");
    return result;
}
std::wstring Quote(const std::wstring& value) {
    std::wstring result = L"\"";
    std::size_t slashes = 0;
    for (const auto character : value) {
        if (character == L'\\') { ++slashes; continue; }
        result.append(slashes * (character == L'"' ? 2 : 1), L'\\');
        slashes = 0;
        if (character == L'"') result += L'\\';
        result += character;
    }
    result.append(slashes * 2, L'\\');
    return result + L'"';
}
}
#endif
int RunProcess(const std::filesystem::path& executable, const std::vector<std::string>& arguments) {
#ifdef _WIN32
    std::wstring command = Quote(executable.wstring());
    for (const auto& argument : arguments) command += L" " + Quote(FromUtf8(argument).wstring());
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    const Handle input{StandardHandle(STD_INPUT_HANDLE, GENERIC_READ)};
    const Handle output{StandardHandle(STD_OUTPUT_HANDLE, GENERIC_WRITE)};
    const Handle error{StandardHandle(STD_ERROR_HANDLE, GENERIC_WRITE)};
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = input.value;
    startup.hStdOutput = output.value;
    startup.hStdError = error.value;
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, TRUE,
            CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process))
        throw std::runtime_error("could not start " + Utf8(executable));
    CloseHandle(process.hThread);
    const DWORD waited = WaitForSingleObject(process.hProcess, INFINITE);
    DWORD code = 1;
    if (waited != WAIT_OBJECT_0 || !GetExitCodeProcess(process.hProcess, &code)) {
        CloseHandle(process.hProcess);
        throw std::runtime_error("could not wait for build process");
    }
    CloseHandle(process.hProcess);
    return static_cast<int>(code);
#else
    std::vector<std::string> storage{Utf8(executable)};
    storage.insert(storage.end(), arguments.begin(), arguments.end());
    std::vector<char*> argv;
    for (auto& item : storage) argv.push_back(item.data());
    argv.push_back(nullptr);
    pid_t child = 0;
    if (posix_spawn(&child, executable.c_str(), nullptr, nullptr, argv.data(), environ) != 0)
        throw std::runtime_error("could not start build process");
    int status = 0;
    if (waitpid(child, &status, 0) != child || !WIFEXITED(status)) return 1;
    return WEXITSTATUS(status);
#endif
}
}
