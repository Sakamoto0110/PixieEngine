#include "Export.hpp"
#include "Process.hpp"
#include <exception>
#include <iostream>
static int Main(int argc, char** argv) {
    try {
        const auto usage = [] {
            std::cerr << "usage: pixie-export --sdk <root> [--work <directory>] <project> <out>\n"
                         "       pixie-export --sdk <root> --identity\n"
                         "       pixie-export --sdk <root> --verify <manifest.json> <out>\n"
                         "       pixie-export --sdk <root> --from-manifest <manifest.json> <out>\n";
            return 2;
        };
        std::string root, mode = "export"; std::optional<std::filesystem::path> work;
        std::vector<std::string> positional;
        for (int i = 1; i < argc; ++i) {
            const std::string argument = argv[i];
            if (argument == "--sdk" || argument == "--work") {
                if (i + 1 >= argc || std::string(argv[i + 1]).starts_with("--")) return usage();
                const std::string value = argv[++i];
                if (argument == "--sdk") { if (!root.empty()) return usage(); root = value; }
                else { if (work) return usage(); work = pixie::packaging::FromUtf8(value); }
            } else if (argument == "--verify" || argument == "--from-manifest" || argument == "--identity") {
                if (mode != "export") return usage();
                mode = argument;
            } else if (argument.starts_with("--")) return usage();
            else positional.push_back(argument);
        }
        if (root.empty() || (mode == "--identity" ? !positional.empty() : positional.size() != 2) || (work && mode != "export")) return usage();
        const auto sdk = pixie::packaging::ExportSdk::Load(pixie::packaging::FromUtf8(root));
        if (mode == "--identity") std::cout << sdk.ToolIdentity().dump(2) << '\n';
        else if (mode == "--verify") pixie::packaging::VerifyExport(sdk, pixie::packaging::FromUtf8(positional[0]), pixie::packaging::FromUtf8(positional[1]));
        else if (mode == "--from-manifest") pixie::packaging::CopyManifest(sdk, pixie::packaging::FromUtf8(positional[0]), pixie::packaging::FromUtf8(positional[1]));
        else pixie::packaging::ExportProject(sdk, pixie::packaging::FromUtf8(positional[0]), pixie::packaging::FromUtf8(positional[1]), work);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "export failed: " << error.what() << '\n';
        return 1;
    }
}
#ifdef _WIN32
int wmain(int argc, wchar_t** argv) {
    std::vector<std::string> storage;
    for (int i = 0; i < argc; ++i) storage.push_back(pixie::packaging::Utf8(std::filesystem::path(argv[i])));
    std::vector<char*> arguments;
    for (auto& argument : storage) arguments.push_back(argument.data());
    return Main(argc, arguments.data());
}
#else
int main(int argc, char** argv) { return Main(argc, argv); }
#endif
