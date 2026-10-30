#include "pixie/dev/PlayerChannel.hpp"
#include "pixie/platform/LocalCommandChannel.hpp"
#include "pixie/runtime/ProjectSession.hpp"
#include "pixie/render/gl/Gl.hpp"
#include "pixie/core/Image.hpp"
#include "DevError.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cctype>
#include <format>
#include <optional>
#include <vector>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

namespace pixie::dev {
namespace {
using Json = nlohmann::json;
std::string Utf8(const std::filesystem::path& path) {
    const auto text = path.u8string();
    return std::string(reinterpret_cast<const char*>(text.data()), text.size());
}
void CheckCapturePath(const std::filesystem::path& original) {
    // Inspect original components before canonicalization, including Windows junctions.
    std::filesystem::path accumulated;
    for (const auto& part : original) {
        accumulated /= part;
        const auto status = std::filesystem::symlink_status(accumulated);
        if (std::filesystem::is_symlink(status)) throw std::runtime_error("capture directory contains a symlink");
#ifdef _WIN32
        const DWORD attributes = GetFileAttributesW(accumulated.c_str());
        if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT))
            throw std::runtime_error("capture directory contains a reparse point");
#endif
    }
}
std::string CaptureName(const Json& p) {
    if (!p.contains("name") || !p["name"].is_string()) throw std::runtime_error("capture name is required");
    auto name = p["name"].get<std::string>();
    if (name.ends_with(".png")) name.resize(name.size() - 4);
    if (name.empty() || name.size() > 64 || !std::all_of(name.begin(), name.end(), [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_';
    })) throw std::runtime_error("capture name must be a bounded ASCII basename");
    std::string upper = name;
    for (auto& c : upper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    if (upper == "CON" || upper == "PRN" || upper == "AUX" || upper == "NUL" ||
        (upper.size() == 4 && (upper.starts_with("COM") || upper.starts_with("LPT")) && upper[3] >= '1' && upper[3] <= '9'))
        throw std::runtime_error("reserved capture basename");
    return name + ".png";
}
bool SaveExclusive(const std::filesystem::path& path, FramebufferSize size) {
#ifdef _WIN32
    if (size.width <= 0 || size.height <= 0 || size.width > 16384 || size.height > 16384) return false;
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(size.width) * size.height * 4);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, size.width, size.height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    std::vector<std::uint8_t> top(pixels.size());
    const std::size_t stride = static_cast<std::size_t>(size.width) * 4;
    for (int y = 0; y < size.height; ++y)
        for (std::size_t x = 0; x < stride; ++x)
            top[static_cast<std::size_t>(y) * stride + x] = x % 4 == 3 ? 255 : pixels[static_cast<std::size_t>(size.height - 1 - y) * stride + x];
    const auto png = EncodePng(size.width, size.height, top);
    if (png.empty()) return false;
    // CREATE_NEW refuses unknown existing files and never follows a final link.
    const HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    const BOOL ok = png.size() <= MAXDWORD && WriteFile(file, png.data(), static_cast<DWORD>(png.size()), &written, nullptr);
    CloseHandle(file);
    return ok && written == png.size();
#else
    (void)path; (void)size;
    return false;
#endif
}
}

struct PlayerChannel::Impl {
    ProjectSession& session;
    LocalCommandChannel channel;
    std::unique_ptr<WorldDispatcher> dispatcher;
    std::filesystem::path captureRoot;
    std::uint64_t client = 0;
    std::uint64_t frame = 0;
    bool hello = false;
    bool running = false;
    std::string error;
    struct Capture { std::filesystem::path path; Json response; bool notification; };
    std::optional<Capture> capture;
    explicit Impl(ProjectSession& s) : session(s) {}
    Json Fail(const Json& id, PixieResult code, const std::string& message, bool possiblyApplied = false) {
        SetNativeError(code, message);
        return {{"jsonrpc", "2.0"}, {"id", id}, {"error", {{"code", -32000}, {"message", message},
            {"data", {{"pixie_result", code}, {"pixie_last_error", message}, {"possibly_applied", possiblyApplied}, {"frame", frame}}}}}};
    }
    std::optional<std::string> Handle(std::string_view request, uint64_t connection) {
        if (!dispatcher || client != connection) {
            dispatcher = std::make_unique<WorldDispatcher>(session);
            client = connection; hello = false;
        }
        Json input;
        try {
            input = Json::parse(request, [](int depth, Json::parse_event_t, Json&) {
                if (depth > 32) throw std::runtime_error("JSON nesting exceeds 32");
                return true;
            });
        } catch (...) { return dispatcher->Dispatch(request); }
        if (!input.is_object() || !input.contains("method") || !input["method"].is_string()) return dispatcher->Dispatch(request);
        const auto method = input["method"].get<std::string>();
        if (method != "world.capture") {
            const auto response = dispatcher->Dispatch(request);
            if (response.empty()) {
                if (method == "world.hello") hello = true;
                return response;
            }
            auto receipt = Json::parse(response);
            if (receipt.contains("result")) {
                receipt["result"]["frame"] = frame;
                if (method == "world.hello") {
                    hello = true;
                    auto& value = receipt["result"]["value"];
                    value["capabilities"]["capture"] = true;
                    value["capabilities"]["transport"] = true;
                    value["capture_directory"] = Utf8(captureRoot);
                }
            } else if (receipt.contains("error")) receipt["error"]["data"]["frame"] = frame;
            return receipt.dump();
        }
        // Validate the complete envelope through the shared dispatcher before reserving a
        // capture. A stats request gives the same sequence/generation receipt without mutation.
        Json stats = input; stats["method"] = "world.stats";
        const bool notification = !input.contains("id");
        stats["id"] = input.value("id", Json(0));
        const auto response = dispatcher->Dispatch(stats.dump());
        auto receipt = Json::parse(response);
        if (receipt.contains("error")) return notification ? std::string{} : response;
        try {
            if (!hello) throw std::runtime_error("world.hello must be the first call");
            const auto params = input.value("params", Json::object());
            if (!params.is_object()) throw std::runtime_error("capture params must be an object");
            const auto path = captureRoot / CaptureName(params);
            CheckCapturePath(captureRoot);
            if (std::filesystem::exists(std::filesystem::symlink_status(path))) throw std::runtime_error("capture already exists; choose a fresh basename");
            receipt["result"]["frame"] = frame;
            capture = Capture{path, std::move(receipt), notification};
            return std::nullopt;
        } catch (const std::exception& e) {
            return notification ? std::string{} : Fail(input.value("id", Json(nullptr)), PIXIE_ERROR_ARGUMENT, e.what()).dump();
        }
    }
};

PlayerChannel::PlayerChannel(ProjectSession& session) : impl_(std::make_unique<Impl>(session)) {}
PlayerChannel::~PlayerChannel() { Stop(); }
bool PlayerChannel::Start(std::string_view name, const std::filesystem::path& captureDirectory) {
    if (impl_->running) { impl_->error = "player channel already started; its capture root is fixed"; return false; }
    try {
        const auto original = std::filesystem::absolute(captureDirectory);
        CheckCapturePath(original);
        const auto absolute = original.lexically_normal();
        CheckCapturePath(absolute);
        std::filesystem::create_directories(absolute);
        CheckCapturePath(absolute);
        if (!std::filesystem::is_directory(absolute)) throw std::runtime_error("capture root must be a directory");
        impl_->captureRoot = std::filesystem::canonical(absolute);
        if (!impl_->channel.Start(name)) { impl_->error = impl_->channel.Error(); return false; }
        impl_->running = true;
        impl_->client = 0; impl_->hello = false; impl_->frame = 0;
        return true;
    } catch (const std::exception& e) { impl_->error = e.what(); return false; }
}
void PlayerChannel::Stop() {
    impl_->channel.Stop(); // Cancels I/O/queued responses and joins before borrowed references die.
    impl_->capture.reset(); impl_->dispatcher.reset();
    impl_->client = 0; impl_->hello = false; impl_->running = false;
}
void PlayerChannel::Advance(double seconds) { impl_->session.Advance(seconds); }
void PlayerChannel::Drain() {
    ++impl_->frame;
    impl_->channel.Drain([&](std::string_view request, uint64_t client) { return impl_->Handle(request, client); });
}
double PlayerChannel::RenderAlpha() const {
    const auto alpha = impl_->dispatcher ? impl_->dispatcher->ConsumeRenderAlpha() : std::nullopt;
    return impl_->capture ? 1.0 : alpha.value_or(impl_->session.Alpha());
}
void PlayerChannel::AfterRender(FramebufferSize size) {
    if (!impl_->capture) return;
    auto capture = std::move(*impl_->capture); impl_->capture.reset();
    if (size.width <= 0 || size.height <= 0) {
        impl_->channel.Complete(capture.notification ? std::string{} : impl_->Fail(capture.response["id"],
            PIXIE_ERROR_GL, "capture framebuffer is unavailable; restore the window before capturing").dump());
        return;
    }
    try {
        CheckCapturePath(impl_->captureRoot);
        if (!SaveExclusive(capture.path, size)) throw std::runtime_error("exclusive PNG capture failed; destination may contain a partial owned file");
        capture.response["result"]["value"] = {{"path", Utf8(capture.path)}, {"width", size.width}, {"height", size.height},
            {"alpha", 1.0}, {"steps", impl_->session.GetWorld().Steps()}};
        impl_->channel.Complete(capture.notification ? std::string{} : capture.response.dump());
    } catch (const std::exception& e) {
        impl_->channel.Complete(capture.notification ? std::string{} : impl_->Fail(capture.response["id"], PIXIE_ERROR_IO, e.what(), true).dump());
    }
}
std::string PlayerChannel::Error() const { return impl_->error; }
} // namespace pixie::dev
