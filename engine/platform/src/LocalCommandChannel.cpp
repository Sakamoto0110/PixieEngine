#include "pixie/platform/LocalCommandChannel.hpp"
#include <algorithm>
#include <array>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <sddl.h>
#endif

namespace pixie {
namespace {
#ifdef _WIN32
constexpr std::size_t kMaxLine = 64 * 1024;
constexpr std::size_t kMaxResponse = 4 * 1024 * 1024;
#endif
bool ValidName(std::string_view name) {
    return !name.empty() && name.size() <= 64 && std::all_of(name.begin(), name.end(), [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_';
    });
}
#ifdef _WIN32
struct Handle {
    HANDLE value = INVALID_HANDLE_VALUE;
    explicit Handle(HANDLE handle = INVALID_HANDLE_VALUE) : value(handle) {}
    ~Handle() { if (value && value != INVALID_HANDLE_VALUE) CloseHandle(value); }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
};
struct LocalMemory {
    HLOCAL value = nullptr;
    ~LocalMemory() { if (value) LocalFree(value); }
};
#endif
}

struct LocalCommandChannel::Impl {
    mutable std::mutex mutex;
    std::condition_variable changed;
    bool stopping = false;
    std::string error;
    std::thread worker;
    struct Pending {
        std::string request;
        std::string response;
        std::uint64_t client = 0;
        bool ready = false;
        bool claimed = false;
    };
    std::shared_ptr<Pending> pending;
    std::shared_ptr<Pending> inFlight;
#ifdef _WIN32
    Handle stopEvent{CreateEventW(nullptr, TRUE, FALSE, nullptr)};
    Handle pipe;
    bool Wait(OVERLAPPED& operation, BOOL immediate, DWORD immediateError, DWORD& transferred) {
        if (immediate) return true;
        if (immediateError != ERROR_IO_PENDING) return false;
        const HANDLE events[]{stopEvent.value, operation.hEvent};
        const DWORD result = WaitForMultipleObjects(2, events, FALSE, INFINITE);
        if (result != WAIT_OBJECT_0 + 1) {
            CancelIoEx(pipe.value, &operation);
            // The OVERLAPPED/event storage must outlive even a cancelled operation.
            GetOverlappedResult(pipe.value, &operation, &transferred, TRUE);
            return false;
        }
        return GetOverlappedResult(pipe.value, &operation, &transferred, FALSE) != FALSE;
    }
    bool Read(char* buffer, DWORD capacity, DWORD& count) {
        Handle event(CreateEventW(nullptr, TRUE, FALSE, nullptr));
        if (!event.value) return false;
        OVERLAPPED operation{}; operation.hEvent = event.value;
        const BOOL result = ReadFile(pipe.value, buffer, capacity, &count, &operation);
        const DWORD failure = result ? ERROR_SUCCESS : GetLastError();
        return Wait(operation, result, failure, count) && count > 0;
    }
    bool Write(std::string response) {
        if (response.empty()) return true; // Valid notification.
        if (response.size() >= kMaxResponse || response.find('\n') != std::string::npos) return false;
        response += '\n';
        std::size_t offset = 0;
        while (offset < response.size()) {
            Handle event(CreateEventW(nullptr, TRUE, FALSE, nullptr));
            if (!event.value) return false;
            OVERLAPPED operation{}; operation.hEvent = event.value;
            DWORD count = 0;
            const BOOL result = WriteFile(pipe.value, response.data() + offset,
                static_cast<DWORD>(response.size() - offset), &count, &operation);
            const DWORD failure = result ? ERROR_SUCCESS : GetLastError();
            if (!Wait(operation, result, failure, count) || count == 0) return false;
            offset += count;
        }
        return true;
    }
    void Serve() {
        std::uint64_t client = 0;
        for (;;) {
            if (WaitForSingleObject(stopEvent.value, 0) == WAIT_OBJECT_0) return;
            Handle event(CreateEventW(nullptr, TRUE, FALSE, nullptr));
            if (!event.value) return;
            OVERLAPPED operation{}; operation.hEvent = event.value;
            const BOOL connected = ConnectNamedPipe(pipe.value, &operation);
            const DWORD failure = connected ? ERROR_SUCCESS : GetLastError();
            DWORD ignored = 0;
            if (failure != ERROR_PIPE_CONNECTED && !Wait(operation, connected, failure, ignored)) {
                if (WaitForSingleObject(stopEvent.value, 0) == WAIT_OBJECT_0) return;
                DisconnectNamedPipe(pipe.value);
                continue;
            }
            ++client;
            std::string line;
            std::array<char, 4096> chunk{};
            bool healthy = true;
            while (healthy) {
                DWORD count = 0;
                if (!Read(chunk.data(), static_cast<DWORD>(chunk.size()), count)) break;
                for (DWORD i = 0; i < count; ++i) {
                    const char c = chunk[i];
                    if (c != '\n') {
                        if (line.size() >= kMaxLine || c == '\0') { healthy = false; break; }
                        line += c;
                        continue;
                    }
                    if (!line.empty() && line.back() == '\r') line.pop_back();
                    auto item = std::make_shared<Pending>();
                    item->request = std::move(line); item->client = client;
                    line.clear();
                    {
                        std::unique_lock lock(mutex);
                        if (stopping) return;
                        pending = item;
                        changed.wait(lock, [&] { return stopping || item->ready; });
                        pending.reset();
                        if (stopping) return;
                    }
                    if (!Write(std::move(item->response))) { healthy = false; break; }
                }
            }
            DisconnectNamedPipe(pipe.value);
        }
    }
#endif
};

LocalCommandChannel::LocalCommandChannel() : impl_(std::make_unique<Impl>()) {}
LocalCommandChannel::~LocalCommandChannel() { Stop(); }

bool LocalCommandChannel::Start(std::string_view name) {
    std::lock_guard lock(impl_->mutex);
    if (impl_->worker.joinable()) { impl_->error = "channel already started"; return false; }
    if (!ValidName(name)) { impl_->error = "PIXIE_DEVELOPMENT_CHANNEL_V1: name must contain 1..64 ASCII letters, digits, _ or -"; return false; }
#ifdef _WIN32
    if (!impl_->stopEvent.value) { impl_->error = "cannot create channel cancellation event"; return false; }
    HANDLE rawToken = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &rawToken)) { impl_->error = "cannot query channel owner token"; return false; }
    Handle token(rawToken);
    DWORD length = 0;
    GetTokenInformation(token.value, TokenUser, nullptr, 0, &length);
    std::vector<unsigned char> buffer(length);
    if (!GetTokenInformation(token.value, TokenUser, buffer.data(), length, &length)) { impl_->error = "cannot query channel owner SID"; return false; }
    LPWSTR sid = nullptr;
    if (!ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER*>(buffer.data())->User.Sid, &sid)) { impl_->error = "cannot format channel owner SID"; return false; }
    LocalMemory sidMemory; sidMemory.value = sid;
    const std::wstring acl = std::wstring(L"D:P(A;;GA;;;") + sid + L")";
    PSECURITY_DESCRIPTOR descriptor = nullptr;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(acl.c_str(), SDDL_REVISION_1, &descriptor, nullptr)) { impl_->error = "cannot create owner-only channel ACL"; return false; }
    LocalMemory descriptorMemory; descriptorMemory.value = descriptor;
    SECURITY_ATTRIBUTES attributes{sizeof(SECURITY_ATTRIBUTES), descriptor, FALSE};
    const std::wstring path = L"\\\\.\\pipe\\pixie-" + std::wstring(name.begin(), name.end());
    impl_->pipe.value = CreateNamedPipeW(path.c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
        1, 65536, 65536, 0, &attributes);
    if (impl_->pipe.value == INVALID_HANDLE_VALUE) { impl_->error = "cannot create exclusive local channel (name in use or access denied)"; return false; }
    impl_->stopping = false;
    ResetEvent(impl_->stopEvent.value);
    impl_->worker = std::thread([this] { impl_->Serve(); });
    return true;
#else
    impl_->error = "local development transport is not implemented on this platform";
    return false;
#endif
}

void LocalCommandChannel::Stop() {
    {
        std::lock_guard lock(impl_->mutex);
        impl_->stopping = true;
#ifdef _WIN32
        if (impl_->stopEvent.value) SetEvent(impl_->stopEvent.value);
#endif
    }
    impl_->changed.notify_all();
    if (impl_->worker.joinable()) impl_->worker.join();
    std::lock_guard lock(impl_->mutex);
    impl_->pending.reset(); impl_->inFlight.reset();
#ifdef _WIN32
    if (impl_->pipe.value != INVALID_HANDLE_VALUE) { CloseHandle(impl_->pipe.value); impl_->pipe.value = INVALID_HANDLE_VALUE; }
#endif
}

void LocalCommandChannel::Drain(const Handler& handler) {
    std::shared_ptr<Impl::Pending> item;
    {
        std::lock_guard lock(impl_->mutex);
        if (impl_->stopping || impl_->inFlight || !impl_->pending || impl_->pending->claimed) return;
        item = impl_->pending; item->claimed = true; impl_->inFlight = item;
    }
    try {
        auto response = handler(item->request, item->client);
        if (response) Complete(std::move(*response));
    } catch (...) {
        Complete(R"({"jsonrpc":"2.0","id":null,"error":{"code":-32603,"message":"host command handler failed; do not retry mutations","data":{"possibly_applied":true}}})");
    }
}

void LocalCommandChannel::Complete(std::string response) {
    {
        std::lock_guard lock(impl_->mutex);
        if (!impl_->inFlight || impl_->stopping) return;
        impl_->inFlight->response = std::move(response);
        impl_->inFlight->ready = true;
        impl_->inFlight.reset();
    }
    impl_->changed.notify_all();
}

std::string LocalCommandChannel::Error() const {
    std::lock_guard lock(impl_->mutex);
    return impl_->error;
}
} // namespace pixie
