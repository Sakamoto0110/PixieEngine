#include "pixie/Log.hpp"

#include <cstdio>

namespace pixie {

void Log(LogLevel level, std::string_view message) {
    const char* tag = "info";
    if (level == LogLevel::Warning) {
        tag = "warning";
    } else if (level == LogLevel::Error) {
        tag = "error";
    }
    std::fprintf(stderr, "[%s] %.*s\n", tag, static_cast<int>(message.size()), message.data());
}

} // namespace pixie
