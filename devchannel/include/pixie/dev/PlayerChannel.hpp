#pragma once
#include "pixie/dev/Dispatcher.hpp"
#include "pixie/core/FramebufferSize.hpp"
#include <filesystem>
#include <memory>
#include <string_view>

namespace pixie::dev {
/// Private-profile player transport over a borrowed session. Stop it before session teardown.
class PlayerChannel {
public:
    PIXIE_DEV_API explicit PlayerChannel(ProjectSession& session);
    PIXIE_DEV_API ~PlayerChannel();
    PlayerChannel(const PlayerChannel&) = delete;
    PlayerChannel& operator=(const PlayerChannel&) = delete;
    /// Starts the local channel with a capture directory fixed for this run.
    PIXIE_DEV_API bool Start(std::string_view name, const std::filesystem::path& captureDirectory);
    PIXIE_DEV_API void Stop();
    PIXIE_DEV_API void Advance(double seconds);
    /// Drains queued work on the host thread before rendering.
    PIXIE_DEV_API void Drain();
    PIXIE_DEV_API double RenderAlpha() const;
    /// Completes pending capture using the just-rendered framebuffer.
    PIXIE_DEV_API void AfterRender(FramebufferSize size);
    PIXIE_DEV_API std::string Error() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace pixie::dev
