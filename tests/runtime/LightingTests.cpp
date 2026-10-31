// Lighting L1 (PIXIE-TASK-214): scenes drawn through a ProjectSession into its render target, as the
// editor draws them, and read back. Needs an OpenGL 4.5 context, which main opens hidden.

#include "Check.hpp"

#include "pixie/render/2d/LightRenderer.hpp"
#include "pixie/render/gl/GlLoader.hpp"
#include "pixie/runtime/Lighting2D.hpp"
#include "pixie/runtime/ProjectSession.hpp"
#include "pixie/runtime/ScreenCanvas.hpp"
#include "pixie/runtime/Shape.hpp"
#include "pixie/runtime/Sprite.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/World.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <numbers>
#include <vector>

namespace {

constexpr int kSide = 128;

class TestProject : public pixie::Project {
public:
    pxColorRgba background{255, 255, 255};
    bool screenRect = false;

    std::string_view Name() const override { return "lighting-tests"; }
    pxColorRgba Background() const override { return background; }
    void Build(pixie::World& /*world*/, pixie::Camera2D& /*camera*/) override {}
    void DrawScreen(pixie::World& /*world*/, pixie::ScreenCanvas& screen) override {
        if (screenRect) {
            screen.Rect(pxPoint(8.0, 8.0), pxVec2(16.0, 16.0), pxColorRgba(255, 255, 255));
        }
    }
};

// A session of kSide x kSide pixels, its camera on the origin at zoom 1: pixel (x, y), from the
// top-left, shows the world point (x + 0.5 - 64, y + 0.5 - 64), Y down.
struct Lab {
    TestProject* project = nullptr;
    std::unique_ptr<pixie::ProjectSession> session;

    explicit Lab(pxColorRgba background = {255, 255, 255}) {
        auto owned = std::make_unique<TestProject>();
        owned->background = background;
        project = owned.get();
        session = std::make_unique<pixie::ProjectSession>(std::move(owned), pixie::SessionSettings{});
        CHECK(session->Start({kSide, kSide}));
    }
    ~Lab() { session->Stop(); }

    pixie::World& World() { return session->GetWorld(); }

    pixie::Entity Add(pxPoint position, double rotation = 0.0) {
        pixie::Entity entity = World().Create();
        entity.Add<pixie::Transform2D>(pixie::Transform2D{position, rotation, pxVec2(1.0, 1.0)});
        World().ResetInterpolation(entity);
        return entity;
    }

    pixie::Entity Quad(pxPoint center, double half, pxColorRgba color) {
        pixie::Entity entity = Add(center);
        entity.Add<pixie::Shape>(pixie::Shape{pixie::ShapeKind::Quad, pxVec2(half, half), color, true});
        return entity;
    }

    void Settings(pixie::LightingSettings2D settings) { Add(pxPoint(0.0, 0.0)).Add<pixie::LightingSettings2D>(settings); }

    // RGBA, first row at the top.
    std::vector<std::uint8_t> Render() {
        const unsigned texture = session->RenderToTarget(1.0);
        std::vector<std::uint8_t> bottomUp(static_cast<std::size_t>(kSide) * kSide * 4);
        glGetTextureImage(texture, 0, GL_RGBA, GL_UNSIGNED_BYTE, static_cast<GLsizei>(bottomUp.size()), bottomUp.data());
        std::vector<std::uint8_t> pixels(bottomUp.size());
        const std::size_t row = static_cast<std::size_t>(kSide) * 4;
        for (std::size_t y = 0; y < kSide; ++y) {
            std::copy_n(bottomUp.begin() + static_cast<std::ptrdiff_t>((kSide - 1 - y) * row), row,
                        pixels.begin() + static_cast<std::ptrdiff_t>(y * row));
        }
        return pixels;
    }
};

int Channel(const std::vector<std::uint8_t>& pixels, int x, int y, int channel = 0) {
    return pixels[(static_cast<std::size_t>(y) * kSide + static_cast<std::size_t>(x)) * 4 + static_cast<std::size_t>(channel)];
}

// The pixel showing world point (wx, wy) at zoom 1.
int At(const std::vector<std::uint8_t>& pixels, double wx, double wy, int channel = 0) {
    return Channel(pixels, static_cast<int>(std::floor(wx + 64.0)), static_cast<int>(std::floor(wy + 64.0)), channel);
}

int Expected(double linear) {
    return static_cast<int>(std::lround(pixie::EncodeSrgb(static_cast<float>(std::clamp(linear, 0.0, 1.0))) * 255.0f));
}

bool Near(int actual, int expected, int tolerance = 1) {
    if (std::abs(actual - expected) > tolerance) {
        std::fprintf(stderr, "  got %d, expected %d\n", actual, expected);
        return false;
    }
    return true;
}

pixie::LightingSettings2D Dark() {
    pixie::LightingSettings2D settings;
    settings.ambient = pxColorRgba(0, 0, 0);
    return settings;
}

void AddSomeContent(Lab& lab) {
    lab.Quad(pxPoint(-30.0, -20.0), 18.0, pxColorRgba(200, 40, 40));
    pixie::Entity circle = lab.Add(pxPoint(12.0, 10.0));
    circle.Add<pixie::Shape>(pixie::Shape{pixie::ShapeKind::Circle, pxVec2(26.0, 26.0), pxColorRgba(40, 200, 90, 128), true});
    pixie::Entity triangle = lab.Add(pxPoint(30.0, -30.0), 0.7);
    triangle.Add<pixie::Shape>(pixie::Shape{pixie::ShapeKind::Triangle, pxVec2(20.0, 20.0), pxColorRgba(250, 220, 30, 200), true});
    // A missing image draws as a checkerboard: a sprite, without a project folder.
    pixie::Entity sprite = lab.Add(pxPoint(-20.0, 30.0), 0.3);
    pixie::Sprite look;
    look.image = "missing.png";
    look.color = pxColorRgba(255, 255, 255, 180);
    sprite.Add<pixie::Sprite>(look);
}

} // namespace

// LIT-002: lighting settings at their defaults, with no lights, leave the unlit image unchanged, byte
// for byte; and lowering the ambient does change it, so the comparison can fail.
TEST(NeutralSettingsLeaveTheImageUnchanged) {
    Lab lab(pxColorRgba(30, 60, 90));
    AddSomeContent(lab);
    const auto unlit = lab.Render();
    lab.Settings({});
    const auto neutral = lab.Render();
    std::size_t differing = 0;
    for (std::size_t i = 0; i < unlit.size(); ++i) {
        differing += unlit[i] != neutral[i] ? 1 : 0;
    }
    std::fprintf(stderr, "  neutral lighting: %zu of %zu bytes differ\n", differing, unlit.size());
    CHECK(differing == 0);

    pixie::World& world = lab.World();
    for (auto [id, settings] : world.Registry().view<pixie::LightingSettings2D>().each()) {
        settings.ambientIntensity = 0.5;
    }
    CHECK(lab.Render() != unlit);
}

// LIT-002: lights do nothing in a scene without LightingSettings2D.
TEST(LightsWithoutSettingsDoNothing) {
    Lab lab(pxColorRgba(30, 60, 90));
    AddSomeContent(lab);
    const auto before = lab.Render();
    lab.Add(pxPoint(0.0, 0.0)).Add<pixie::Light2D>(pixie::Light2D{});
    CHECK(lab.Render() == before);
}

// LIT-003/004: a point light follows saturate(1 - (d / range)^2)^falloff in linear space, encoded for
// display, and nothing beyond its range; at zoom 2 the same light covers twice the pixels.
TEST(PointLightFollowsItsFalloff) {
    Lab lab;
    lab.Settings(Dark());
    pixie::Light2D light;
    light.range = 50.0;
    light.falloff = 2.0;
    lab.Add(pxPoint(0.0, 0.0)).Add<pixie::Light2D>(light);
    const auto pixels = lab.Render();
    for (const double d : {0.5, 10.5, 25.5, 40.5, 49.5, 55.5, 63.5}) {
        const double x = 1.0 - (d / 50.0) * (d / 50.0);
        const double expected = d < 50.0 ? x * x : 0.0;
        CHECK(Near(At(pixels, d, 0.5), Expected(expected)));
        CHECK(Near(At(pixels, 0.5, -d), Expected(expected)));
    }
    lab.session->GetCamera().SetZoom(2.0);
    const auto zoomed = lab.Render();
    for (const int column : {85, 115}) {
        // Two pixels per world unit around the origin.
        const double d = (column + 0.5 - 64.0) / 2.0;
        const double x = 1.0 - (d / 50.0) * (d / 50.0);
        CHECK(Near(Channel(zoomed, column, 64), Expected(x * x)));
    }
}

// LIT-003: a spot light shines within its cone, around the entity's rotation, and not outside it.
TEST(SpotLightKeepsToItsCone) {
    Lab lab;
    lab.Settings(Dark());
    pixie::Light2D spot;
    spot.type = pixie::LightType2D::Spot;
    spot.range = 120.0;
    spot.falloff = 0.0;
    spot.coneAngle = std::numbers::pi / 3.0;
    spot.coneSoftness = 0.0;
    lab.Add(pxPoint(-50.0, 0.0), 0.0).Add<pixie::Light2D>(spot);
    const auto pixels = lab.Render();
    const auto point = [](double degrees) {
        const double a = degrees * std::numbers::pi / 180.0;
        return pxPoint(-50.0 + 70.0 * std::cos(a), 70.0 * std::sin(a));
    };
    for (const double inside : {0.0, 20.0, -25.0}) {
        CHECK(Near(At(pixels, point(inside).x, point(inside).y), 255));
    }
    for (const double outside : {40.0, -45.0}) {
        CHECK(Near(At(pixels, point(outside).x, point(outside).y), 0));
    }
    // Behind the light.
    CHECK(Near(At(pixels, -60.0, 0.5), 0));
}

// LIT-003: a directional light lights the whole view evenly.
TEST(DirectionalLightCoversTheView) {
    Lab lab;
    lab.Settings(Dark());
    pixie::Light2D sun;
    sun.type = pixie::LightType2D::Directional;
    sun.intensity = 0.5;
    lab.Add(pxPoint(500.0, 500.0), 1.0).Add<pixie::Light2D>(sun);
    const auto pixels = lab.Render();
    int wrong = 0;
    for (int y = 0; y < kSide; ++y) {
        for (int x = 0; x < kSide; ++x) {
            wrong += std::abs(Channel(pixels, x, y) - Expected(0.5)) > 1 ? 1 : 0;
        }
    }
    CHECK(wrong == 0);
}

// LIT-005: a light reaches the receivers whose light layers it shares, and only those; receivers
// without LightReceiver2D, and the background, are in every layer.
TEST(LightLayersIncludeAndExclude) {
    Lab lab;
    lab.Settings(Dark());
    pixie::Light2D sun;
    sun.type = pixie::LightType2D::Directional;
    sun.lightLayers = 0b10;
    lab.Add(pxPoint(0.0, 0.0)).Add<pixie::Light2D>(sun);
    lab.Quad(pxPoint(-30.0, 0.0), 12.0, pxColorRgba(255, 255, 255)).Add<pixie::LightReceiver2D>(pixie::LightReceiver2D{0b01, false});
    lab.Quad(pxPoint(30.0, 0.0), 12.0, pxColorRgba(255, 255, 255)).Add<pixie::LightReceiver2D>(pixie::LightReceiver2D{0b11, false});
    lab.Quad(pxPoint(0.0, 40.0), 12.0, pxColorRgba(255, 255, 255));
    const auto pixels = lab.Render();
    CHECK(Near(At(pixels, -30.0, 0.0), 0));
    CHECK(Near(At(pixels, 30.0, 0.0), 255));
    CHECK(Near(At(pixels, 0.0, 40.0), 255));
    CHECK(Near(At(pixels, 0.0, -50.0), 255));
}

// LIT-005: an unlit receiver keeps its own color in the dark; a lit one beside it goes black.
TEST(UnlitReceiversKeepTheirColor) {
    Lab lab;
    lab.Settings(Dark());
    lab.Quad(pxPoint(-30.0, 0.0), 12.0, pxColorRgba(200, 100, 50)).Add<pixie::LightReceiver2D>(pixie::LightReceiver2D{0xFF, true});
    lab.Quad(pxPoint(30.0, 0.0), 12.0, pxColorRgba(200, 100, 50));
    const auto pixels = lab.Render();
    CHECK(At(pixels, -30.0, 0.0, 0) == 200 && At(pixels, -30.0, 0.0, 1) == 100 && At(pixels, -30.0, 0.0, 2) == 50);
    CHECK(At(pixels, 30.0, 0.0, 0) == 0 && At(pixels, 30.0, 0.0, 1) == 0);
}

// LIT-004: exposure multiplies the light, and the Aces curve maps 1.0 to its own value.
TEST(ExposureAndToneMapping) {
    Lab lab(pxColorRgba(128, 128, 128));
    pixie::LightingSettings2D settings;
    settings.ambientIntensity = 0.5;
    settings.exposure = 2.0;
    lab.Settings(settings);
    CHECK(Near(At(lab.Render(), 0.0, 0.0), 128));

    Lab white;
    pixie::LightingSettings2D aces;
    aces.toneMapping = pixie::LightToneMapping2D::Aces;
    white.Settings(aces);
    const double curve = (2.51 + 0.03) / (2.43 + 0.59 + 0.14);
    CHECK(Near(At(white.Render(), 0.0, 0.0), Expected(curve)));
}

// LIT-005/012: the screen canvas is drawn after the lit world and is never lit.
TEST(ScreenCanvasIsNeverLit) {
    Lab lab;
    lab.project->screenRect = true;
    lab.Settings(Dark());
    const auto pixels = lab.Render();
    CHECK(Channel(pixels, 16, 16) == 255);
    CHECK(Channel(pixels, 64, 64) == 0);
}

int main() {
    if (!glfwInit()) return 2;
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "lighting-tests", nullptr, nullptr);
    if (window == nullptr) {
        glfwTerminate();
        return 3;
    }
    glfwMakeContextCurrent(window);
    int result = 4;
    if (pixie::LoadGl()) {
        result = check::RunAll();
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return result;
}
