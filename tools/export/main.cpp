#include "Export.hpp"
#include "Process.hpp"
#include <exception>
#include <iostream>
static int Main(int argc, char** argv) {
    try {
        if (argc == 4 && std::string(argv[1]) == "--verify")
            pixie::packaging::VerifyExport(pixie::packaging::FromUtf8(argv[2]), pixie::packaging::FromUtf8(argv[3]));
        else if (argc == 4 && std::string(argv[1]) == "--from-manifest")
            pixie::packaging::CopyManifest(pixie::packaging::FromUtf8(argv[2]), pixie::packaging::FromUtf8(argv[3]));
        else if (argc == 3)
            pixie::packaging::ExportProject(pixie::packaging::FromUtf8(argv[1]), pixie::packaging::FromUtf8(argv[2]));
        else {
            std::cerr << "usage: pixie-export <project> <out>\n"
                         "       pixie-export --verify <manifest.json> <out>\n"
                         "       pixie-export --from-manifest <manifest.json> <out>\n";
            return 2;
        }
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
