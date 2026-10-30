#pragma once
#include <filesystem>
#include <string>
#include <vector>
namespace pixie::packaging {
// Executes an argument vector directly, without a shell. Child output is inherited.
int RunProcess(const std::filesystem::path& executable, const std::vector<std::string>& arguments);
std::string Utf8(const std::filesystem::path& path);
std::filesystem::path FromUtf8(const std::string& text);
}
