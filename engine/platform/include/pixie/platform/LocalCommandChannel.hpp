#pragma once

#include "pixie/platform/Export.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace pixie {
// Development host transport only. It queues bounded UTF-8 lines; it never touches a World.
// Drain/Complete belong to the owner thread; the worker owns all pipe I/O.
class LocalCommandChannel {
public:
    using Handler = std::function<std::optional<std::string>(std::string_view, std::uint64_t)>;
    PIXIE_PLATFORM_API LocalCommandChannel();
    PIXIE_PLATFORM_API ~LocalCommandChannel();
    LocalCommandChannel(const LocalCommandChannel&) = delete;
    LocalCommandChannel& operator=(const LocalCommandChannel&) = delete;
    PIXIE_PLATFORM_API bool Start(std::string_view name);
    PIXIE_PLATFORM_API void Stop();
    // A nullopt response defers completion until a rendered capture is available.
    PIXIE_PLATFORM_API void Drain(const Handler& handler);
    PIXIE_PLATFORM_API void Complete(std::string response);
    PIXIE_PLATFORM_API std::string Error() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace pixie
