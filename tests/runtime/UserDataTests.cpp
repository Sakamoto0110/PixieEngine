#include "Check.hpp"

#include "pixie/runtime/UserData.hpp"

#include <cstdlib>
#include <filesystem>
#include <string>

using namespace pixie;

namespace {

// Points PIXIE_USER_DATA at `folder` for the test's length, so the real user data folder is untouched.
class UserDataAt {
public:
    explicit UserDataAt(const std::filesystem::path& folder) { Set(folder.string()); }
    ~UserDataAt() { Set(""); }

private:
    static void Set(const std::string& value) {
#ifdef _WIN32
        _putenv_s("PIXIE_USER_DATA", value.c_str());
#else
        if (value.empty()) {
            unsetenv("PIXIE_USER_DATA");
        } else {
            setenv("PIXIE_USER_DATA", value.c_str(), 1);
        }
#endif
    }
};

} // namespace

TEST(NamesBecomeFolderNames) {
    CHECK(UserDataFolderName("Logic Sandbox") == "Logic Sandbox");
    CHECK(UserDataFolderName(R"(a/b\c:d*e?f"g<h>i|j)") == "a_b_c_d_e_f_g_h_i_j");
    CHECK(UserDataFolderName("tabs\tand\nlines") == "tabs_and_lines");
    CHECK(UserDataFolderName("trailing. . ") == "trailing");
    CHECK(UserDataFolderName("Ação") == "Ação");
    CHECK(UserDataFolderName("").empty() && UserDataFolderName(".").empty() && UserDataFolderName("..").empty());
}

TEST(TheUserDataFolderIsMadeOnFirstUse) {
    const std::filesystem::path root = std::filesystem::temp_directory_path() / "pixie-user-data-test";
    std::filesystem::remove_all(root);
    const UserDataAt at(root);

    const std::filesystem::path folder = UserDataFolder("Logic: Sandbox");
    CHECK(folder == root / "Logic_ Sandbox");
    CHECK(std::filesystem::is_directory(folder));
    // A second time, the same folder, already there.
    CHECK(UserDataFolder("Logic: Sandbox") == folder);
    // A name that makes no folder name gives none.
    CHECK(UserDataFolder("..").empty());
    std::filesystem::remove_all(root);
}
