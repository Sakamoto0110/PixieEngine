#pragma once

#include "pixie/core/Export.hpp"

#include <string_view>

namespace pixie {

enum class LogLevel { Info, Warning, Error };

// Provisional: writes to stderr. The logging design is still open.
PIXIE_CORE_API void Log(LogLevel level, std::string_view message);

inline void LogInfo(std::string_view message) { Log(LogLevel::Info, message); }
inline void LogWarning(std::string_view message) { Log(LogLevel::Warning, message); }
inline void LogError(std::string_view message) { Log(LogLevel::Error, message); }

} // namespace pixie
