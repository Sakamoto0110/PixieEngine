#pragma once

#include "GameModules.hpp"
#include "pixie/platform/HostSurface.hpp"
#include "pixie/runtime/ProjectSession.hpp"
#include <memory>
#ifdef PIXIE_DEV_CHANNEL
#include "pixie/dev/Dispatcher.hpp"
#include <thread>
#endif

// Private ABI. In shipping, retain the original embedded ProjectSession storage.
struct PixieSession {
#ifdef PIXIE_DEVELOPMENT
    std::shared_ptr<PixieGameModule> module;
#endif
    pixie::HostSurface surface{{0, 0}};
#ifdef PIXIE_DEV_CHANNEL
    std::unique_ptr<pixie::ProjectSession> ownedSession;
    pixie::ProjectSession& session;
    std::unique_ptr<pixie::dev::WorldDispatcher> dispatcher;
    std::thread::id ownerThread = std::this_thread::get_id();
    explicit PixieSession(std::unique_ptr<pixie::Project> project, pixie::SessionSettings settings)
        : ownedSession(std::make_unique<pixie::ProjectSession>(std::move(project), settings)), session(*ownedSession) {}
    // Never returned by the C API and never passed to pixie_session_destroy.
    explicit PixieSession(pixie::ProjectSession& borrowed) : session(borrowed) {}
#else
    pixie::ProjectSession session;
    explicit PixieSession(std::unique_ptr<pixie::Project> project, pixie::SessionSettings settings)
        : session(std::move(project), settings) {}
#endif
};
