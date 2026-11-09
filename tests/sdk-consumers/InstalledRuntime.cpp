#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <pixie_sdk_build.h>
#include <pixie_capi.h>
#include <pixie/runtime/ComponentTypes.hpp>
#include <pixie/runtime/LightingEnvironment2D.hpp>
#include <pixie/runtime/Presentation2D.hpp>
#include <pixie/runtime/Scene.hpp>
#include <pixie/runtime/ViewportCamera2D.hpp>
#include <pixie/runtime/World.hpp>
#include <pixie/math/pxMat.hpp>
#include <Windows.h>
#include <cmath>
#include <filesystem>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#if defined(PIXIE_CORE_BUILD) || defined(PIXIE_PLATFORM_BUILD) || defined(PIXIE_RENDER_BUILD) || \
    defined(PIXIE_APP_BUILD) || defined(PIXIE_RUNTIME_BUILD) || defined(PIXIE_CAPI_BUILD) || defined(GLEW_STATIC)
#error The installed consumer inherited a producer-only export definition.
#endif

namespace {
int checks = 0;
void Check(bool passed, std::string_view name) {
    if (!passed) throw std::runtime_error(std::string(name));
    ++checks;
}
std::string Quote(std::string_view value) {
    std::string result = "\"";
    constexpr char hex[] = "0123456789abcdef";
    for (unsigned char character : value) {
        if (character == '"' || character == '\\') { result += '\\'; result += static_cast<char>(character); }
        else if (character < 32) { result += "\\u00"; result += hex[character >> 4]; result += hex[character & 15]; }
        else result += static_cast<char>(character);
    }
    return result + "\"";
}
std::string Utf8(const std::filesystem::path& path) {
    const auto value = path.generic_u8string();
    return {reinterpret_cast<const char*>(value.data()), value.size()};
}
std::filesystem::path LoadedRuntime() {
    HMODULE module = nullptr;
    Check(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
          reinterpret_cast<LPCWSTR>(&pixie_sdk_identity), &module) != 0, "runtime module address");
    std::vector<wchar_t> path(32768);
    const DWORD length = GetModuleFileNameW(module, path.data(), static_cast<DWORD>(path.size()));
    Check(length > 0 && length < path.size(), "complete loaded-runtime path");
    return std::filesystem::path(std::wstring(path.data(), length));
}
std::string RuntimeIdentity() {
    Check(std::string_view(PIXIE_SDK_ID) == PIXIE_CONSUMER_SELECTED_SDK_ID, "selected SDK header identity");
    Check(std::string_view(PIXIE_SDK_BUILD_ID) == PIXIE_CONSUMER_SELECTED_BUILD_ID, "selected build header identity");
    Check(std::string_view(PIXIE_SDK_CONFIGURATION) == PIXIE_CONSUMER_SELECTED_CONFIGURATION, "selected header configuration");
    Check(std::string_view(PIXIE_SDK_PROFILE_NAME) == PIXIE_CONSUMER_SELECTED_PROFILE, "selected header profile");
    Check(pixie_api_version() == PIXIE_CAPI_VERSION, "C API version");
    Check(pixie_capabilities() == PIXIE_SDK_EXPECTED_CAPABILITIES, "compiled capability profile");
    const std::int32_t length = pixie_sdk_identity(nullptr, 0);
    Check(length > 0 && length <= PIXIE_SDK_IDENTITY_MAX_LENGTH, "bounded identity length");
    std::vector<char> buffer(static_cast<std::size_t>(length) + 2, '\x7f');
    Check(pixie_sdk_identity(buffer.data(), length + 1) == length, "complete identity query");
    Check(buffer[static_cast<std::size_t>(length)] == '\0' && buffer.back() == '\x7f', "identity termination/bounds");
    return std::string(buffer.data(), static_cast<std::size_t>(length));
}
void CheckMath(int argumentCount) {
    pxMat4f matrix(1.0f);
    matrix[0][0] = static_cast<float>(argumentCount + 2);
    matrix[1][1] = 5.0f;
    matrix[2][2] = 7.0f;
    matrix[3][0] = 9.0f;
    // The float mat4 path calls compiled pxMathDetail::InverseSimd on x64.
    const pxMat4f product = matrix * pxInverse(matrix);
    for (int column = 0; column != 4; ++column) {
        for (int row = 0; row != 4; ++row) {
            const float expected = column == row ? 1.0f : 0.0f;
            Check(std::abs(product[column][row] - expected) < 0.0001f, "compiled float matrix inverse");
        }
    }
}
void CheckSceneAndPresentation() {
    using namespace pixie;
    ComponentTypes types;
    DescribeRuntimeComponents(types);
    const std::string input = R"({"format":"pixie-scene","version":2,"seed":19,
      "consumerOpaque":{"keep":"retained-value"},
      "presentation":{"schemaVersion":1,"lightingState":"disabled","camera":"12",
        "environment":{"ambientIntensity":0.5}},
      "sceneScopes":[{"id":"7","name":"room","camera":"11",
        "gravity":{"mode":"override","value":[0,30],"multiplier":2},
        "environment":{"mode":"override","value":{"ambientIntensity":0.25}}},
        {"id":"8","name":"empty"}],
      "entities":[{"id":"11","sceneScope":"7","components":{
        "Transform2D":{"position":[20,30]},"Camera2D":{"zoom":2}}},
        {"id":"12","components":{"Transform2D":{"position":[40,50]},"Camera2D":{"zoom":3}}}]})";
    World world;
    Check(ReadScene(world, types, input).ok, "read authored v2");
    Check(SceneFormatVersion(world) == 2 && world.GetRandom().Seed() == 19, "scene version and seed");
    Check(world.EntityCount() == 2 && world.Scopes().size() == 2, "entities and empty scope");
    bool scoped = false, worldOnly = false;
    world.Each<SceneId>([&](Entity entity, const SceneId& id) {
        const auto scope = world.ScopeOf(entity);
        if (id.value == 0x11) scoped = scope && scope->value == 7;
        if (id.value == 0x12) worldOnly = scope && scope->value == 0;
    });
    Check(scoped && worldOnly, "scope and World-only membership");
    const auto gravity = world.ResolveGravity(ScopeId{7});
    Check(static_cast<bool>(gravity) && gravity.value == pxVec2(0, 30), "public scope gravity resolution");
    const auto presentation = ReadPresentation2D(world);
    Check(presentation.lighting == LightingState2D::Disabled && presentation.camera == 0x12,
          "canonical presentation");
    Check(ValidatePresentation2D(presentation).empty(), "presentation validation");
    const auto environment = ResolveViewportEnvironment2D(world, {EnvironmentSelectionKind2D::Scene, ScopeId{7}});
    Check(environment.kind == EnvironmentResolutionKind2D::SceneOverride
          && environment.lighting.state == LightingState2D::Disabled
          && environment.lighting.environment && environment.lighting.environment->ambientIntensity == 0.25,
          "explicit viewport Environment does not enable disabled lighting");
    Camera2D fallback;
    fallback.SetViewport({640, 480});
    fallback.SetCenter({1, 2});
    const auto camera = ResolveViewportCamera2D(world, fallback,
        {CameraSelectionKind2D::Defaults, 0, ScopeId{7}});
    Check(camera.kind == CameraResolutionKind2D::SceneDefault && camera.entity == 0x11
          && camera.camera.Center() == pxPoint(20, 30) && camera.camera.Zoom() == 2,
          "public viewport Scene camera");
    Check(ResolveViewportCamera2D(world, fallback, {}).camera.Center() == fallback.Center(),
          "explicit free-camera fallback");
    const std::string encoded = WriteScene(world, types);
    Check(encoded.find("consumerOpaque") != std::string::npos
          && encoded.find("retained-value") != std::string::npos, "opaque data retained");
    World reopened;
    Check(ReadScene(reopened, types, encoded).ok && reopened.EntityCount() == 2
          && reopened.Scopes().size() == 2 && reopened.Presentation() == world.Presentation(),
          "full document roundtrip");
    World legacy;
    Check(ReadScene(legacy, types, R"({"format":"pixie-scene","version":1,"seed":23,"entities":[]})").ok,
          "legacy scene admitted");
    const std::string legacyText = WriteScene(legacy, types);
    World legacyReopened;
    Check(SceneFormatVersion(legacy) == 1 && ReadScene(legacyReopened, types, legacyText).ok
          && SceneFormatVersion(legacyReopened) == 1 && legacyReopened.GetRandom().Seed() == 23,
          "ordinary roundtrip retains v1");
}
} // namespace

// Windows x64/MSVC is the accepted binary tuple. Wide arguments preserve runtime paths.
int wmain(int argc, wchar_t** argv) {
    try {
        const bool identityOnly = argc == 1 || (argc == 2 && std::wstring_view(argv[1]) == L"--identity-only");
        const bool run = argc == 3 && std::wstring_view(argv[1]) == L"--run-after-preflight";
        if (!identityOnly && !run) {
            std::cerr << "usage: pixie-installed-runtime [--identity-only | --run-after-preflight <exact-runtime-path>]\n";
            return 2;
        }
        const std::string identity = RuntimeIdentity();
        const auto loaded = LoadedRuntime();
        if (run) {
            Check(std::filesystem::equivalent(loaded, std::filesystem::path(argv[2])), "same preflight runtime file");
            CheckMath(argc);
            CheckSceneAndPresentation();
        }
        // Default/identity-only never calls World/Scene. The generated CTest runner
        // first verifies loaded bytes and the typed C1 projection, then explicitly
        // invokes --run-after-preflight in the same declared immutable staging area.
        std::cout << "{\"fixture\":\"pixie-installed-headless/1\",\"mode\":"
                  << Quote(identityOnly ? "identity-only" : "headless-runtime")
                  << ",\"checks\":" << checks << ",\"loadedRuntime\":" << Quote(Utf8(loaded))
                  << ",\"expected\":{\"sdkId\":" << Quote(PIXIE_SDK_ID)
                  << ",\"sdkBuildId\":" << Quote(PIXIE_SDK_BUILD_ID)
                  << ",\"profile\":" << Quote(PIXIE_SDK_PROFILE_NAME)
                  << ",\"configuration\":" << Quote(PIXIE_SDK_CONFIGURATION)
                  << "},\"runtimeIdentity\":" << identity << "}\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "headless consumer failed: " << error.what() << '\n';
        return 1;
    }
}
