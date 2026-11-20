#pragma once

#include "pixie/platform/Export.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace pixie {
// Private development transport. The worker frames bounded lines for the owner thread.
// The dispatcher owns decoding and session access. Join the worker before releasing the session.
class LocalCommandChannel {
public:
    using Handler = std::function<std::optional<std::string>(std::string_view, std::uint64_t)>;
    PIXIE_PLATFORM_API LocalCommandChannel();
    PIXIE_PLATFORM_API ~LocalCommandChannel();
    LocalCommandChannel(const LocalCommandChannel&) = delete;
    LocalCommandChannel& operator=(const LocalCommandChannel&) = delete;
    PIXIE_PLATFORM_API bool Start(std::string_view name);
    PIXIE_PLATFORM_API void Stop();
    // Call on the owner thread. A nullopt response keeps the item in flight until Complete.
    PIXIE_PLATFORM_API void Drain(const Handler& handler);
    PIXIE_PLATFORM_API void Complete(std::string response);
    PIXIE_PLATFORM_API std::uint64_t ConnectedClient() const;
    PIXIE_PLATFORM_API std::string Error() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace pixie
