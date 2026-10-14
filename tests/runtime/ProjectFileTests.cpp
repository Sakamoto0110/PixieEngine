#include "Check.hpp"

#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/Name.hpp"
#include "pixie/runtime/ProjectFile.hpp"
#include "pixie/runtime/Scene.hpp"
#include "pixie/runtime/Shape.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/World.hpp"

#include <filesystem>
#include <fstream>
#include <string>

using namespace pixie;

namespace {

// A fresh, empty folder under the system's temporary folder, removed when the test ends.
struct TemporaryFolder {
    std::filesystem::path path;

    explicit TemporaryFolder(const char* name) : path(std::filesystem::temp_directory_path() / name) {
        std::filesystem::remove_all(path);
        std::filesystem::create_directories(path);
    }
    ~TemporaryFolder() {
        std::error_code ignored;
        std::filesystem::remove_all(path, ignored);
    }
};

void WriteFile(const std::filesystem::path& path, const std::string& text) {
    std::ofstream(path, std::ios::binary) << text;
}

std::string ReadFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

} // namespace

TEST(ANewProjectTakesTheFoldersNameAndOpens) {
    TemporaryFolder root("pixie-project-tests-new");
    const std::filesystem::path folder = root.path / "My Game";
    const OpenedProject created = CreateProjectFolder(folder);
    CHECK(created.ok);
    CHECK(created.file.name == "My Game");
    CHECK(created.file.startScene == "scenes/main.scene");
    CHECK(created.path.filename() == "My Game.pixie");
    CHECK(std::filesystem::exists(created.StartScenePath()));

    // The start scene holds one shape at the origin.
    ComponentTypes types;
    DescribeRuntimeComponents(types);
    World world;
    CHECK(LoadSceneFile(world, types, created.StartScenePath()).ok);
    CHECK(world.EntityCount() == 1);
    world.Each<const Name, const Transform2D, const Shape>(
        [](Entity, const Name& name, const Transform2D& transform, const Shape&) {
            CHECK(name.value == "shape");
            CHECK(transform.position == pxPoint(0.0, 0.0));
        });

    // The project file names nothing outside the folder.
    const std::string text = ReadFile(created.path);
    CHECK(text.find("\"startScene\": \"scenes/main.scene\"") != std::string::npos);
    CHECK(text.find(root.path.filename().string()) == std::string::npos);

    // A path with a separator at the end names the same folder.
    const OpenedProject trailing = CreateProjectFolder(root.path / "Other" / "");
    CHECK(trailing.ok && trailing.file.name == "Other");
}

TEST(ANewProjectNeedsAnEmptyOrNewFolder) {
    TemporaryFolder root("pixie-project-tests-busy");
    WriteFile(root.path / "notes.txt", "already here");
    const OpenedProject created = CreateProjectFolder(root.path);
    CHECK(!created.ok && !created.error.empty());
    CHECK(!std::filesystem::exists(root.path / "scenes"));
    // An existing empty folder is fine.
    std::filesystem::create_directories(root.path / "Empty");
    CHECK(CreateProjectFolder(root.path / "Empty").ok);
}

TEST(OnlyAFolderWithOneValidProjectFileOpens) {
    TemporaryFolder root("pixie-project-tests-open");
    CHECK(!OpenProjectFolder(root.path / "missing").ok);
    CHECK(!OpenProjectFolder(root.path).ok);

    const auto opens = [&](const std::string& text) {
        WriteFile(root.path / "Game.pixie", text);
        return OpenProjectFolder(root.path);
    };
    CHECK(!opens("not json").ok);
    CHECK(!opens(R"({"format": "something-else", "version": 1})").ok);
    CHECK(!opens(R"({"format": "pixie-project", "version": 2})").ok);
    CHECK(!opens(R"({"format": "pixie-project"})").ok);
    CHECK(!opens(R"({"format": "pixie-project", "version": 1, "startScene": "C:/scenes/main.scene"})").ok);
    CHECK(!opens(R"({"format": "pixie-project", "version": 1, "startScene": "../main.scene"})").ok);

    // Without a name, the file's own name is the project's.
    const OpenedProject opened = opens(R"({"format": "pixie-project", "version": 1})");
    CHECK(opened.ok && opened.file.name == "Game" && opened.file.startScene == "scenes/main.scene");

    WriteFile(root.path / "Second.pixie", R"({"format": "pixie-project", "version": 1})");
    CHECK(!OpenProjectFolder(root.path).ok);
}

TEST(UnknownProjectKeysAreWrittenBack) {
    TemporaryFolder root("pixie-project-tests-keys");
    WriteFile(root.path / "Game.pixie",
              R"({"format": "pixie-project", "version": 1, "name": "Game", "startScene": "a/b.scene", "icon": "icon.png"})");
    const OpenedProject opened = OpenProjectFolder(root.path);
    CHECK(opened.ok && opened.file.startScene == "a/b.scene");
    CHECK(WriteProjectFile(opened.path, opened.file).empty());
    const std::string text = ReadFile(opened.path);
    CHECK(text.find("\"icon\": \"icon.png\"") != std::string::npos);
    CHECK(OpenProjectFolder(root.path).file.extras == opened.file.extras);

    ProjectFile outside;
    outside.name = "Out";
    outside.startScene = "../escape.scene";
    CHECK(!WriteProjectFile(root.path / "Out.pixie", outside).empty());
}

TEST(ProjectPathsStayInsideTheFolder) {
    CHECK(IsProjectRelativePath("scenes/main.scene"));
    CHECK(IsProjectRelativePath("main.scene"));
    CHECK(!IsProjectRelativePath(""));
    CHECK(!IsProjectRelativePath("/scenes/main.scene"));
    CHECK(!IsProjectRelativePath("C:/scenes/main.scene"));
    CHECK(!IsProjectRelativePath("scenes\\main.scene"));
    CHECK(!IsProjectRelativePath("../main.scene"));
    CHECK(!IsProjectRelativePath("scenes/./main.scene"));
    CHECK(!IsProjectRelativePath("scenes//main.scene"));
    CHECK(!IsProjectRelativePath("scenes/"));
}
