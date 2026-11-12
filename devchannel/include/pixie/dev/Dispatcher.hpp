#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#ifdef _WIN32
#ifdef PIXIE_DEV_BUILD
#define PIXIE_DEV_API __declspec(dllexport)
#else
#define PIXIE_DEV_API __declspec(dllimport)
#endif
#else
#define PIXIE_DEV_API __attribute__((visibility("default")))
#endif

namespace pixie {
class ProjectSession;
namespace dev {

inline constexpr std::size_t kMaxRequestBytes = 64 * 1024;
inline constexpr int kResponseCapacity = 4 * 1024 * 1024;

/// Private-profile JSON-RPC adapter over a borrowed session on its owner thread.
/// The session must outlive the dispatcher. The host owns its lifecycle and transport.
class WorldDispatcher {
public:
    PIXIE_DEV_API explicit WorldDispatcher(ProjectSession& session);
    PIXIE_DEV_API ~WorldDispatcher();
    WorldDispatcher(const WorldDispatcher&) = delete;
    WorldDispatcher& operator=(const WorldDispatcher&) = delete;
    /// Runs one bounded request. Empty text means a notification with no response body.
    PIXIE_DEV_API std::string Dispatch(std::string_view request);
    // Consume immediately before a render attempt, never on a skipped frame.
    // A failed allocation/render cancels this override; the host must report capture failure.
    PIXIE_DEV_API std::optional<double> ConsumeRenderAlpha();
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace dev
} // namespace pixie
