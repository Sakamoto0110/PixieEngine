#include "PeImage.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <fstream>
#include <set>
#include <stdexcept>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#endif
namespace pixie::packaging {
namespace {
std::string Lower(std::string value) {
    for (auto& c : value) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return value;
}
void LibraryName(const std::string& name) {
    if (name.empty() || name == "." || name == ".." || name.back() == '.' || name.back() == ' ' ||
        name.find_first_of("/\\:") != std::string::npos ||
        std::any_of(name.begin(), name.end(), [](unsigned char c) { return c < 32; }))
        throw std::runtime_error("native import must name a library basename");
}
#ifdef _WIN32
class Reader {
public:
    explicit Reader(const std::filesystem::path& path) {
        std::ifstream input(path, std::ios::binary | std::ios::ate);
        const auto size = input.tellg();
        if (!input || size < 0 || size > 512 * 1024 * 1024) throw std::runtime_error("invalid PE image size");
        bytes.resize(static_cast<std::size_t>(size));
        input.seekg(0);
        if (!input.read(bytes.data(), size)) throw std::runtime_error("could not read PE image");
        const auto dos = Read<IMAGE_DOS_HEADER>(0);
        if (dos.e_magic != IMAGE_DOS_SIGNATURE || dos.e_lfanew < 0) throw std::runtime_error("not a PE image");
        const auto offset = static_cast<std::size_t>(dos.e_lfanew);
        if (Read<DWORD>(offset) != IMAGE_NT_SIGNATURE) throw std::runtime_error("invalid PE signature");
        const auto file = Read<IMAGE_FILE_HEADER>(offset + sizeof(DWORD));
        if (file.Machine != IMAGE_FILE_MACHINE_AMD64 || file.NumberOfSections > 96 ||
            file.SizeOfOptionalHeader < sizeof(IMAGE_OPTIONAL_HEADER64)) throw std::runtime_error("expected a PE64 x64 image");
        header = Read<IMAGE_OPTIONAL_HEADER64>(offset + sizeof(DWORD) + sizeof(file));
        if (header.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC || header.NumberOfRvaAndSizes < 14)
            throw std::runtime_error("invalid PE64 optional header");
        const auto sectionsAt = offset + sizeof(DWORD) + sizeof(file) + file.SizeOfOptionalHeader;
        for (unsigned i = 0; i < file.NumberOfSections; ++i)
            sections.push_back(Read<IMAGE_SECTION_HEADER>(sectionsAt + i * sizeof(IMAGE_SECTION_HEADER)));
    }
    template<class T> T Read(std::size_t offset) const {
        if (offset > bytes.size() || sizeof(T) > bytes.size() - offset) throw std::runtime_error("truncated PE table");
        T value{};
        std::memcpy(&value, bytes.data() + offset, sizeof(value));
        return value;
    }
    std::size_t Offset(DWORD rva, std::size_t size) const {
        if (rva < header.SizeOfHeaders && rva <= bytes.size() && size <= bytes.size() - rva) return rva;
        for (const auto& section : sections) {
            if (rva < section.VirtualAddress) continue;
            const auto relative = static_cast<std::size_t>(rva - section.VirtualAddress);
            if (relative <= section.SizeOfRawData && size <= section.SizeOfRawData - relative) {
                const auto offset = section.PointerToRawData + relative;
                if (offset <= bytes.size() && size <= bytes.size() - offset) return offset;
            }
        }
        throw std::runtime_error("PE RVA outside file-backed sections");
    }
    template<class T> T At(DWORD rva) const { return Read<T>(Offset(rva, sizeof(T))); }
    DWORD Address(DWORD base, DWORD offset) const {
        if (base > MAXDWORD - offset) throw std::runtime_error("overflowing PE table address");
        return base + offset;
    }
    std::string String(DWORD rva) const {
        std::string result;
        for (DWORD i = 0; i < 4096; ++i) {
            if (rva > MAXDWORD - i) throw std::runtime_error("invalid PE string address");
            const auto c = At<char>(rva + i);
            if (!c) return result;
            result += c;
        }
        throw std::runtime_error("unterminated PE name");
    }
    void Imports(DWORD name, DWORD table, PeImage& image) const {
        const auto library = Lower(String(name));
        LibraryName(library);
        image.libraries.push_back(library);
        if (!table) throw std::runtime_error("missing PE import name table");
        for (DWORD i = 0; i < 65536; ++i) {
            if (table > MAXDWORD - i * sizeof(ULONGLONG)) throw std::runtime_error("invalid PE import table");
            const auto thunk = At<ULONGLONG>(table + i * static_cast<DWORD>(sizeof(ULONGLONG)));
            if (!thunk) return;
            if (!(thunk & IMAGE_ORDINAL_FLAG64)) {
                if (thunk > MAXDWORD - sizeof(WORD)) throw std::runtime_error("invalid PE import name");
                image.imports.push_back(String(static_cast<DWORD>(thunk) + static_cast<DWORD>(sizeof(WORD))));
            }
        }
        throw std::runtime_error("unterminated PE import table");
    }
    IMAGE_OPTIONAL_HEADER64 header{};
    std::vector<char> bytes;
private:
    std::vector<IMAGE_SECTION_HEADER> sections;
};
#endif
}
PeImage InspectPe(const std::filesystem::path& path) {
#ifdef _WIN32
    const Reader reader(path);
    PeImage image;
    const auto imports = reader.header.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (imports.VirtualAddress) {
        bool ended = false;
        for (DWORD i = 0; i < std::min<DWORD>(imports.Size / static_cast<DWORD>(sizeof(IMAGE_IMPORT_DESCRIPTOR)), 4096); ++i) {
            const auto entry = reader.At<IMAGE_IMPORT_DESCRIPTOR>(reader.Address(imports.VirtualAddress, i * static_cast<DWORD>(sizeof(IMAGE_IMPORT_DESCRIPTOR))));
            if (!entry.Name) { ended = true; break; }
            reader.Imports(entry.Name, entry.OriginalFirstThunk ? entry.OriginalFirstThunk : entry.FirstThunk, image);
        }
        if (!ended) throw std::runtime_error("unterminated PE import directory");
    }
    struct Delay { DWORD attributes, name, module, iat, names, bound, unload, time; };
    const auto delay = reader.header.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT];
    if (delay.VirtualAddress) {
        bool ended = false;
        for (DWORD i = 0; i < std::min<DWORD>(delay.Size / static_cast<DWORD>(sizeof(Delay)), 4096); ++i) {
            const auto entry = reader.At<Delay>(reader.Address(delay.VirtualAddress, i * static_cast<DWORD>(sizeof(Delay))));
            if (!entry.name) { ended = true; break; }
            if (entry.attributes != 1) throw std::runtime_error("unsupported delay import address mode");
            reader.Imports(entry.name, entry.names, image);
        }
        if (!ended) throw std::runtime_error("unterminated PE delay import directory");
    }
    const auto exports = reader.header.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (exports.VirtualAddress) {
        const auto table = reader.At<IMAGE_EXPORT_DIRECTORY>(exports.VirtualAddress);
        if (table.NumberOfNames > 1000000 || table.NumberOfFunctions > 1000000) throw std::runtime_error("invalid PE export count");
        for (DWORD i = 0; i < table.NumberOfNames; ++i)
            image.exports.push_back(reader.String(reader.At<DWORD>(reader.Address(table.AddressOfNames, i * static_cast<DWORD>(sizeof(DWORD))))));
        for (DWORD i = 0; i < table.NumberOfFunctions; ++i) {
            const auto function = reader.At<DWORD>(reader.Address(table.AddressOfFunctions, i * static_cast<DWORD>(sizeof(DWORD))));
            if (function < exports.VirtualAddress || static_cast<std::uint64_t>(function) >=
                static_cast<std::uint64_t>(exports.VirtualAddress) + exports.Size) continue;
            const auto forwarder = reader.String(function);
            const auto dot = forwarder.rfind('.');
            if (dot == std::string::npos || dot == 0 || dot + 1 == forwarder.size()) throw std::runtime_error("invalid PE export forwarder");
            auto library = Lower(forwarder.substr(0, dot));
            if (!library.ends_with(".dll")) library += ".dll";
            LibraryName(library);
            image.libraries.push_back(library);
            image.imports.push_back(forwarder.substr(dot + 1));
        }
    }
    for (const std::string_view marker : {"pixie_game_create", "PIXIE_DEVELOPMENT_CHANNEL_V1", "--dev-channel",
             "--dev-capture-dir", "--dev-hidden", "--quit-after-steps"})
        image.developmentMarker = image.developmentMarker ||
            std::search(reader.bytes.begin(), reader.bytes.end(), marker.begin(), marker.end()) != reader.bytes.end();
    return image;
#else
    (void)path;
    throw std::runtime_error("native export qualification currently supports Windows PE64 only");
#endif
}
std::string Digest(const std::filesystem::path& path) {
#ifdef _WIN32
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("could not hash export input");
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0)
        throw std::runtime_error("SHA256 provider unavailable");
    if (BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0) < 0) {
        BCryptCloseAlgorithmProvider(algorithm, 0);
        throw std::runtime_error("SHA256 context unavailable");
    }
    std::array<unsigned char, 65536> bytes{};
    bool ok = true;
    while (input.read(reinterpret_cast<char*>(bytes.data()), bytes.size()) || input.gcount())
        if (BCryptHashData(hash, bytes.data(), static_cast<ULONG>(input.gcount()), 0) < 0) ok = false;
    std::array<unsigned char, 32> digest{};
    ok = ok && input.eof() && BCryptFinishHash(hash, digest.data(), static_cast<ULONG>(digest.size()), 0) >= 0;
    BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(algorithm, 0);
    if (!ok) throw std::runtime_error("could not complete SHA256");
    std::string result;
    for (const auto byte : digest) { result += "0123456789abcdef"[byte >> 4]; result += "0123456789abcdef"[byte & 15]; }
    return result;
#else
    (void)path;
    throw std::runtime_error("SHA256 export qualification currently supports Windows only");
#endif
}
bool IsWindowsLibrary(const std::string& name) {
    LibraryName(name);
    const auto lower = Lower(name);
    // The C++ redistributable must be app-local even if the build machine has it installed.
    if (lower.starts_with("msvcp") || lower.starts_with("vcruntime") || lower.starts_with("concrt")) return false;
    if (lower.starts_with("api-ms-win-") || lower.starts_with("ext-ms-win-")) return true;
    // Presence in System32 alone does not make an installed third-party DLL part of Windows.
    static const std::set<std::string> operatingSystem{
        "advapi32.dll", "avrt.dll", "bcrypt.dll", "bcryptprimitives.dll", "cabinet.dll", "cfgmgr32.dll",
        "clbcatq.dll", "combase.dll", "comctl32.dll", "comdlg32.dll", "crypt32.dll", "cryptbase.dll",
        "cryptsp.dll", "d2d1.dll", "d3d9.dll", "d3d11.dll", "d3d12.dll", "dcomp.dll", "dbghelp.dll",
        "devobj.dll", "dnsapi.dll", "dwmapi.dll", "dwrite.dll", "dxcore.dll", "dxgi.dll", "gdi32.dll",
        "gdi32full.dll", "glu32.dll", "hid.dll", "imm32.dll", "iphlpapi.dll", "kernel32.dll",
        "kernelbase.dll", "mpr.dll", "msasn1.dll", "msctf.dll", "msimg32.dll", "msvcrt.dll",
        "mswsock.dll", "ncrypt.dll", "netapi32.dll", "normaliz.dll", "ntasn1.dll", "ntdll.dll",
        "ole32.dll", "oleaut32.dll", "opengl32.dll", "powrprof.dll", "profapi.dll", "propsys.dll",
        "psapi.dll", "rpcrt4.dll", "rsaenh.dll", "sechost.dll", "secur32.dll", "setupapi.dll",
        "shell32.dll", "shcore.dll", "shlwapi.dll", "sspicli.dll", "ucrtbase.dll", "user32.dll",
        "userenv.dll", "usp10.dll", "uxtheme.dll", "version.dll", "win32u.dll", "windowscodecs.dll",
        "winhttp.dll", "wininet.dll", "winmm.dll", "winspool.drv", "wintrust.dll", "ws2_32.dll",
        "wtsapi32.dll"};
    if (!operatingSystem.contains(lower)) return false;
#ifdef _WIN32
    std::array<wchar_t, MAX_PATH> directory{};
    const auto size = GetSystemDirectoryW(directory.data(), static_cast<UINT>(directory.size()));
    return size && size < directory.size() && std::filesystem::is_regular_file(std::filesystem::path(directory.data()) / name);
#else
    return false;
#endif
}
}
