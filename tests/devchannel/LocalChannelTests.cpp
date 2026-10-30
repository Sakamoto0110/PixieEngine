#include "Check.hpp"
#include "pixie/platform/LocalCommandChannel.hpp"
#include "pixie/dev/PlayerChannel.hpp"
#include "pixie/runtime/ProjectSession.hpp"
#include "pixie/runtime/SceneProject.hpp"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <atomic>
#include <chrono>
#include <format>
#include <thread>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <aclapi.h>

namespace {
std::string UniqueName() {
    return std::format("test-{}-{}", GetCurrentProcessId(), std::chrono::steady_clock::now().time_since_epoch().count());
}
HANDLE Connect(const std::string& name) {
    const auto path = L"\\\\.\\pipe\\pixie-" + std::wstring(name.begin(), name.end());
    return CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE | READ_CONTROL, 0, nullptr, OPEN_EXISTING, 0, nullptr);
}
bool Write(HANDLE pipe, const char* text) {
    DWORD written = 0;
    const DWORD size = static_cast<DWORD>(std::char_traits<char>::length(text));
    return WriteFile(pipe, text, size, &written, nullptr) && written == size;
}
bool DrainUntil(pixie::LocalCommandChannel& channel, const pixie::LocalCommandChannel::Handler& handler, const std::atomic<bool>& done) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (!done && std::chrono::steady_clock::now() < deadline) {
        channel.Drain(handler);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return done;
}
}

TEST(LocalChannelRejectsNamesAndStopsWhileWaitingForConnect) {
    pixie::LocalCommandChannel channel;
    CHECK(!channel.Start("../bad"));
    CHECK(!channel.Start(std::string(65, 'x')));
    CHECK(channel.Start(UniqueName()));
    const auto start = std::chrono::steady_clock::now();
    channel.Stop();
    CHECK(std::chrono::steady_clock::now() - start < std::chrono::seconds(1));
}

TEST(LocalChannelOwnerOnlyAclExclusiveInstanceAndSingleClient) {
    pixie::LocalCommandChannel channel;
    const auto name = UniqueName();
    CHECK(channel.Start(name));
    pixie::LocalCommandChannel collision;
    CHECK(!collision.Start(name));
    const HANDLE client = Connect(name);
    CHECK(client != INVALID_HANDLE_VALUE);
    const HANDLE second = Connect(name);
    CHECK(second == INVALID_HANDLE_VALUE);
    if (second != INVALID_HANDLE_VALUE) CloseHandle(second);
    PACL dacl = nullptr;
    PSECURITY_DESCRIPTOR descriptor = nullptr;
    CHECK(GetSecurityInfo(client, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, &dacl, nullptr, &descriptor) == ERROR_SUCCESS);
    if (descriptor && dacl) {
        CHECK(dacl->AceCount == 1);
        SECURITY_DESCRIPTOR_CONTROL control = 0;
        DWORD revision = 0;
        CHECK(GetSecurityDescriptorControl(descriptor, &control, &revision) != FALSE);
        CHECK((control & SE_DACL_PROTECTED) != 0);
        void* ace = nullptr;
        CHECK(GetAce(dacl, 0, &ace) != FALSE);
        HANDLE token = nullptr;
        CHECK(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token) != FALSE);
        DWORD length = 0;
        GetTokenInformation(token, TokenUser, nullptr, 0, &length);
        std::vector<unsigned char> buffer(length);
        CHECK(GetTokenInformation(token, TokenUser, buffer.data(), length, &length) != FALSE);
        if (ace) CHECK(EqualSid(&static_cast<ACCESS_ALLOWED_ACE*>(ace)->SidStart,
            reinterpret_cast<TOKEN_USER*>(buffer.data())->User.Sid) != FALSE);
        if (token) CloseHandle(token);
        LocalFree(descriptor);
    }
    CloseHandle(client);
    channel.Stop();
}

TEST(LocalChannelAppliesOnlyWhenOwnerDrainsAndCanReconnect) {
    pixie::LocalCommandChannel channel;
    const auto name = UniqueName();
    CHECK(channel.Start(name));
    std::uint64_t previousClient = 0;
    for (int attempt = 0; attempt < 2; ++attempt) {
        HANDLE client = INVALID_HANDLE_VALUE;
        const auto connectDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
        while (client == INVALID_HANDLE_VALUE && std::chrono::steady_clock::now() < connectDeadline) {
            client = Connect(name);
            if (client == INVALID_HANDLE_VALUE) std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        CHECK(client != INVALID_HANDLE_VALUE);
        if (client == INVALID_HANDLE_VALUE) break;
        CHECK(Write(client, "{\"jsonrpc\":\"2.0\",\"method\":\"test\"}\n"));
        int applied = 0;
        bool readOkay = false;
        std::atomic<bool> done = false;
        std::thread reader([&] {
            char response[128]{};
            DWORD count = 0;
            const BOOL success = ReadFile(client, response, 127, &count, nullptr);
            readOkay = success && std::string_view(response, count) == "{\"ok\":true}\n";
            done = true;
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        CHECK(applied == 0 && !done);
        const bool completed = DrainUntil(channel, [&](std::string_view text, std::uint64_t connection) -> std::optional<std::string> {
            CHECK(text.find("test") != std::string_view::npos);
            CHECK(connection > previousClient);
            previousClient = connection;
            ++applied;
            return "{\"ok\":true}";
        }, done);
        CHECK(completed);
        if (!completed) channel.Stop();
        reader.join();
        CHECK(readOkay);
        CHECK(applied == 1);
        CloseHandle(client);
    }
    channel.Stop();
}

TEST(LocalChannelShutdownCancelsDeferredResponseAndIdleRead) {
    for (const bool defer : {false, true}) {
        pixie::LocalCommandChannel channel;
        const auto name = UniqueName();
        CHECK(channel.Start(name));
        const HANDLE client = Connect(name);
        CHECK(client != INVALID_HANDLE_VALUE);
        if (defer) {
            CHECK(Write(client, "{}\n"));
            std::atomic<bool> claimed = false;
            CHECK(DrainUntil(channel, [&](std::string_view, std::uint64_t) -> std::optional<std::string> {
                claimed = true; return std::nullopt;
            }, claimed));
        }
        const auto start = std::chrono::steady_clock::now();
        channel.Stop();
        CHECK(std::chrono::steady_clock::now() - start < std::chrono::seconds(1));
        channel.Complete("late completion"); // Cancelled work never reaches a later connection.
        CloseHandle(client);
    }
}

TEST(PlayerChannelRestartsAndRejectsEmptyFramebufferWithoutBlockingQueue) {
    using Json = nlohmann::json;
    pixie::ProjectSession session(std::make_unique<pixie::SceneProject>(), {});
    CHECK(session.Start({96, 64}));
    pixie::dev::PlayerChannel host(session);
    const auto name = UniqueName();
    const auto root = std::filesystem::temp_directory_path() / name / std::filesystem::path(u8"capturas com espaço-ação");
    for (int attempt = 0; attempt < 2; ++attempt) {
        CHECK(host.Start(name, root));
        CHECK(!host.Start(name, root / "must-not-create"));
        CHECK(!std::filesystem::exists(root / "must-not-create"));
        const HANDLE client = Connect(name);
        CHECK(client != INVALID_HANDLE_VALUE);
        auto call = [&](const char* method, const Json& arguments) {
            const auto request = Json{{"jsonrpc", "2.0"}, {"id", 1}, {"method", method}, {"params", arguments}}.dump() + "\n";
            CHECK(Write(client, request.c_str()));
            std::string response;
            std::atomic<bool> done = false;
            std::thread reader([&] {
                while (response.size() < 65536) {
                    char chunk[512]{}; DWORD count = 0;
                    if (!ReadFile(client, chunk, 512, &count, nullptr) || count == 0) break;
                    response.append(chunk, count);
                    if (response.ends_with('\n')) break;
                }
                done = true;
            });
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
            while (!done && std::chrono::steady_clock::now() < deadline) {
                host.Drain();
                host.AfterRender({0, 0}); // The same completion path used by a minimized Run.
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            const bool completed = done;
            if (!completed) host.Stop();
            reader.join();
            CHECK(completed);
            return Json::parse(response, nullptr, false);
        };
        CHECK(call("world.hello", Json::object()).contains("result"));
        const auto failure = call("world.capture", {{"name", "minimized"}});
        CHECK(failure["error"]["data"]["pixie_result"] == 3);
        CHECK(failure["error"]["data"]["possibly_applied"] == false);
        CHECK(!std::filesystem::exists(root / "minimized.png"));
        CHECK(call("world.stats", Json::object()).contains("result"));
        CloseHandle(client);
        host.Stop();
    }
    session.StepOnce();
    CHECK(session.GetWorld().Steps() == 1);
    session.Stop();
}
#endif
