#pragma once
#include "pixie/dev/Dispatcher.hpp"
#include "pixie/core/FramebufferSize.hpp"
#include <filesystem>
#include <memory>
#include <string_view>

namespace pixie::dev {
// Player-side development host. Destroy it before stopping the borrowed session.
class PlayerChannel {
public:
    PIXIE_DEV_API explicit PlayerChannel(ProjectSession& session);
    PIXIE_DEV_API ~PlayerChannel();
    PlayerChannel(const PlayerChannel&) = delete;
    PlayerChannel& operator=(const PlayerChannel&) = delete;
    PIXIE_DEV_API bool Start(std::string_view name, const std::filesystem::path& captureDirectory);
    PIXIE_DEV_API void Stop();
    PIXIE_DEV_API void Advance(double seconds);
    PIXIE_DEV_API void Drain();
    PIXIE_DEV_API double RenderAlpha() const;
    PIXIE_DEV_API void AfterRender(FramebufferSize size);
    PIXIE_DEV_API std::string Error() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace pixie::dev
