// Lighting L1 (PIXIE-TASK-214): scenes drawn through a ProjectSession into its render target, as the
// editor draws them, and read back. Needs an OpenGL 4.5 context, which main opens hidden.

#include "Check.hpp"

#include "pixie/render/2d/LightRenderer.hpp"
#include "pixie/render/gl/GlLoader.hpp"
#include "pixie/runtime/Lighting2D.hpp"
#include "pixie/runtime/LightingDrawing.hpp"
#include "pixie/runtime/SceneMovement2D.hpp"
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

// ---------------------------------------------------------------------------------------------
// L2: occluders and shadows (PIXIE-TASK-215).

namespace {

pixie::Entity Box(Lab& lab, pxPoint center, pxVec2 half, double rotation = 0.0) {
    pixie::Entity entity = lab.Add(center, rotation);
    pixie::Collider2D collider;
    collider.kind = pixie::ColliderKind2D::Box;
    collider.halfSize = half;
    entity.Add<pixie::Collider2D>(collider);
    entity.Add<pixie::Occluder2D>(pixie::Occluder2D{});
    return entity;
}

pixie::Light2D FlatLight(double range, double source) {
    pixie::Light2D light;
    light.range = range;
    light.falloff = 0.0;
    light.sourceRadius = source;
    return light;
}

bool Inside(const std::vector<pxPoint>& polygon, pxPoint p) {
    for (std::size_t i = 0; i < polygon.size(); ++i) {
        const pxPoint& a = polygon[i];
        const pxPoint& b = polygon[(i + 1) % polygon.size()];
        if ((b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x) < 0.0) {
            return false;
        }
    }
    return true;
}

// Where a ray enters a convex polygon of positive area, or a huge distance.
double Entry(const std::vector<pxPoint>& polygon, pxPoint o, double dx, double dy) {
    double in = 0.0, out = 1e30;
    for (std::size_t i = 0; i < polygon.size(); ++i) {
        const pxPoint& a = polygon[i];
        const pxPoint& b = polygon[(i + 1) % polygon.size()];
        const double nx = b.y - a.y, ny = -(b.x - a.x);
        const double num = nx * (a.x - o.x) + ny * (a.y - o.y);
        const double den = nx * dx + ny * dy;
        if (std::abs(den) < 1e-12) {
            if (num < 0.0) return 1e30;
            continue;
        }
        const double t = num / den;
        if (den < 0.0) in = std::max(in, t); else out = std::min(out, t);
    }
    return in <= out ? in : 1e30;
}

// The exact reference of the L0 prototype: the fraction of the source disk's angular extent that no
// occluder hides from p, over 1024 directions; an occluder never hides its own interior.
double ReferenceVisibility(pxPoint p, pxPoint light, double source, const std::vector<std::vector<pxPoint>>& polygons) {
    const double dx = light.x - p.x, dy = light.y - p.y;
    const double d = std::hypot(dx, dy);
    if (d <= source) return 1.0;
    const double alpha = std::asin(source / d);
    const double base = std::atan2(dy, dx);
    int blocked = 0;
    for (int i = 0; i < 1024; ++i) {
        const double phi = -alpha + (i + 0.5) / 1024.0 * 2.0 * alpha;
        const double sp = std::sin(phi);
        const double end = d * std::cos(phi) - std::sqrt(std::max(source * source - d * d * sp * sp, 0.0));
        for (const auto& polygon : polygons) {
            if (!Inside(polygon, p) && Entry(polygon, p, std::cos(base + phi), std::sin(base + phi)) < end) {
                ++blocked;
                break;
            }
        }
    }
    return 1.0 - blocked / 1024.0;
}

double Linear(int value) {
    return pixie::DecodeSrgb(static_cast<float>(value) / 255.0f);
}

} // namespace

// LIT-008: a light inside a closed room of thin walls that touch exactly at the corners lights
// nothing outside it.
TEST(ClosedRoomDoesNotLeak) {
    Lab lab;
    lab.Settings(Dark());
    lab.Add(pxPoint(0.0, 0.0)).Add<pixie::Light2D>(FlatLight(120.0, 4.0));
    Box(lab, pxPoint(0.0, -31.0), pxVec2(32.0, 1.0));
    Box(lab, pxPoint(0.0, 31.0), pxVec2(32.0, 1.0));
    Box(lab, pxPoint(-31.0, 0.0), pxVec2(1.0, 30.0));
    Box(lab, pxPoint(31.0, 0.0), pxVec2(1.0, 30.0));
    const auto pixels = lab.Render();
    int leaks = 0;
    int lit = 0;
    for (int y = 0; y < kSide; ++y) {
        for (int x = 0; x < kSide; ++x) {
            const double wx = x + 0.5 - 64.0, wy = y + 0.5 - 64.0;
            if (std::abs(wx) > 33.0 || std::abs(wy) > 33.0) {
                leaks += Channel(pixels, x, y) != 0 ? 1 : 0;
            } else if (std::abs(wx) < 29.0 && std::abs(wy) < 29.0) {
                lit += Channel(pixels, x, y) == 255 ? 1 : 0;
            }
        }
    }
    std::fprintf(stderr, "  closed room: %d lit pixels outside, %d fully lit inside\n", leaks, lit);
    CHECK(leaks == 0);
    CHECK(lit > 3000);
}

// LIT-007: soft shadows agree with the exact area-light reference, umbra and penumbra.
TEST(SoftShadowsMatchTheReference) {
    Lab lab;
    lab.Settings(Dark());
    const pxPoint light(-30.0, 0.0);
    lab.Add(light).Add<pixie::Light2D>(FlatLight(150.0, 10.0));
    pixie::Entity box = lab.Add(pxPoint(5.0, 4.0), 0.3);
    pixie::Occluder2D occluder;
    occluder.source = pixie::OccluderSource2D::Polygon;
    occluder.polygon = {{-8.0, -12.0}, {8.0, -12.0}, {8.0, 12.0}, {-8.0, 12.0}};
    box.Add<pixie::Occluder2D>(occluder);
    const auto polygons = pixie::OccluderPolygons(occluder, pixie::Transform2D{pxPoint(5.0, 4.0), 0.3, pxVec2(1.0, 1.0)},
                                                  nullptr, nullptr, nullptr);
    const auto pixels = lab.Render();
    double worst = 0.0;
    int penumbra = 0;
    for (int y = 2; y < kSide; y += 5) {
        for (int x = 2; x < kSide; x += 5) {
            const pxPoint p(x + 0.5 - 64.0, y + 0.5 - 64.0);
            if (std::hypot(p.x - light.x, p.y - light.y) > 148.0) continue;
            const double expected = ReferenceVisibility(p, light, 10.0, polygons);
            penumbra += expected > 0.05 && expected < 0.95 ? 1 : 0;
            worst = std::max(worst, std::abs(Linear(Channel(pixels, x, y)) - expected));
        }
    }
    std::fprintf(stderr, "  worst visibility error %.4f over the grid, %d penumbra samples\n", worst, penumbra);
    CHECK(worst < 0.03);
    CHECK(penumbra > 10);
}

// LIT-006: an occluder casts from its interpolated pose: halfway between two steps, its shadow is
// the shadow of the occluder standing halfway.
TEST(OccludersCastFromTheirInterpolatedPose) {
    std::vector<std::uint8_t> moving, still;
    for (const bool interpolate : {true, false}) {
        Lab lab;
        lab.Settings(Dark());
        lab.Add(pxPoint(-40.0, 0.0)).Add<pixie::Light2D>(FlatLight(150.0, 6.0));
        pixie::Entity box = Box(lab, pxPoint(interpolate ? 30.0 : 20.0, 0.0), pxVec2(6.0, 10.0), interpolate ? 0.4 : 0.2);
        if (interpolate) {
            lab.World().Registry().emplace_or_replace<pixie::PreviousTransform2D>(
                box.Id(), pixie::PreviousTransform2D{pixie::Transform2D{pxPoint(10.0, 0.0), 0.0, pxVec2(1.0, 1.0)}});
        }
        const unsigned texture = lab.session->RenderToTarget(0.5);
        std::vector<std::uint8_t>& out = interpolate ? moving : still;
        out.resize(static_cast<std::size_t>(kSide) * kSide * 4);
        glGetTextureImage(texture, 0, GL_RGBA, GL_UNSIGNED_BYTE, static_cast<GLsizei>(out.size()), out.data());
    }
    std::size_t differing = 0;
    for (std::size_t i = 0; i < moving.size(); ++i) {
        differing += std::abs(moving[i] - still[i]) > 1 ? 1 : 0;
    }
    std::fprintf(stderr, "  interpolated against midpoint: %zu bytes differ by more than 1\n", differing);
    CHECK(differing == 0);
}

// LIT-005/006: a light is shadowed only by occluders of its shadow layers, and not at all without
// castsShadows.
TEST(ShadowLayersChooseTheOccluders) {
    const auto behind = [](int occluderLayers, int shadowLayers, bool casts) {
        Lab lab;
        lab.Settings(Dark());
        pixie::Light2D light = FlatLight(120.0, 2.0);
        light.shadowLayers = shadowLayers;
        light.castsShadows = casts;
        lab.Add(pxPoint(-40.0, 0.0)).Add<pixie::Light2D>(light);
        Box(lab, pxPoint(0.0, 0.0), pxVec2(6.0, 12.0)).Get<pixie::Occluder2D>().layers = occluderLayers;
        return At(lab.Render(), 30.0, 0.0);
    };
    CHECK(Near(behind(1, 0xFF, true), 0));
    CHECK(Near(behind(2, 1, true), 255));
    CHECK(Near(behind(2, 3, true), 0));
    CHECK(Near(behind(1, 0xFF, false), 255));
}

// LIT-006: an occluder's interior is lit as if it were not there, unless it shadows itself.
TEST(SelfShadowDarkensOnlyWhenAsked) {
    for (const bool self : {false, true}) {
        Lab lab;
        lab.Settings(Dark());
        lab.Add(pxPoint(-40.0, 0.0)).Add<pixie::Light2D>(FlatLight(120.0, 2.0));
        pixie::Entity box = Box(lab, pxPoint(0.0, 0.0), pxVec2(10.0, 10.0));
        box.Get<pixie::Occluder2D>().selfShadow = self;
        const auto pixels = lab.Render();
        CHECK(self ? At(pixels, 2.0, 0.0) < 10 : At(pixels, 2.0, 0.0) == 255);
        // Behind it, both shadow.
        CHECK(Near(At(pixels, 30.0, 0.0), 0));
    }
}

// LIT-003/007: a directional light's shadows are parallel, from wherever the occluder stands.
TEST(DirectionalShadowsAreParallel) {
    Lab lab;
    lab.Settings(Dark());
    pixie::Light2D sun;
    sun.type = pixie::LightType2D::Directional;
    sun.sourceAngle = 0.01;
    lab.Add(pxPoint(0.0, 0.0), 0.0).Add<pixie::Light2D>(sun);
    Box(lab, pxPoint(-20.0, -20.0), pxVec2(6.0, 6.0));
    const auto pixels = lab.Render();
    CHECK(Near(At(pixels, 40.0, -20.0), 0));
    CHECK(Near(At(pixels, 10.0, -20.0), 0));
    CHECK(Near(At(pixels, 40.0, -5.0), 255));
    CHECK(Near(At(pixels, -50.0, -20.0), 255));
}

// LIT-006: occluder geometry follows the collider's offset, rotation and the entity's scale; a
// circle's chords stay within 0.5 units of it; a character casts from its capsule; sensors and
// disabled shapes cast nothing.
TEST(OccluderGeometryFollowsColliders) {
    const pixie::Occluder2D occluder;
    pixie::Collider2D box;
    box.kind = pixie::ColliderKind2D::Box;
    box.halfSize = pxVec2(10.0, 5.0);
    box.offset = pxPoint(5.0, 0.0);
    box.rotation = std::numbers::pi / 2.0;
    const pixie::Transform2D at{pxPoint(100.0, 0.0), 0.0, pxVec2(2.0, 1.0)};
    const auto boxes = pixie::OccluderPolygons(occluder, at, &box, nullptr, nullptr);
    CHECK(boxes.size() == 1 && boxes[0].size() == 4);
    if (!boxes.empty()) {
        double minX = 1e9, maxX = -1e9, minY = 1e9, maxY = -1e9;
        for (const pxPoint& p : boxes[0]) {
            minX = std::min(minX, p.x); maxX = std::max(maxX, p.x);
            minY = std::min(minY, p.y); maxY = std::max(maxY, p.y);
        }
        // Turned a quarter: 5 wide by 10 tall around the offset (5, 0), then twice as wide.
        CHECK(std::abs(minX - 100.0) < 1e-9 && std::abs(maxX - 120.0) < 1e-9);
        CHECK(std::abs(minY + 10.0) < 1e-9 && std::abs(maxY - 10.0) < 1e-9);
    }
    pixie::Collider2D circle;
    circle.kind = pixie::ColliderKind2D::Circle;
    circle.radius = 20.0;
    const auto circles = pixie::OccluderPolygons(occluder, pixie::Transform2D{}, &circle, nullptr, nullptr);
    CHECK(circles.size() == 1);
    if (!circles.empty()) {
        const auto& ring = circles[0];
        for (std::size_t i = 0; i < ring.size(); ++i) {
            const pxPoint& a = ring[i];
            const pxPoint& b = ring[(i + 1) % ring.size()];
            CHECK(20.0 - std::hypot((a.x + b.x) / 2.0, (a.y + b.y) / 2.0) <= 0.5);
        }
    }
    pixie::Collider2D sensor = box;
    sensor.sensor = true;
    CHECK(pixie::OccluderPolygons(occluder, at, &sensor, nullptr, nullptr).empty());
    pixie::CharacterMotor2D motor;
    motor.radius = 10.0;
    motor.halfSegment = 15.0;
    const auto capsule = pixie::OccluderPolygons(occluder, pixie::Transform2D{pxPoint(0.0, 0.0), 1.0, pxVec2(3.0, 3.0)},
                                                 nullptr, nullptr, &motor);
    CHECK(capsule.size() == 1);
    if (!capsule.empty()) {
        double minY = 1e9, maxY = -1e9, maxX = 0.0;
        for (const pxPoint& p : capsule[0]) {
            minY = std::min(minY, p.y); maxY = std::max(maxY, p.y); maxX = std::max(maxX, std::abs(p.x));
        }
        CHECK(std::abs(minY + 25.0) < 1e-9 && std::abs(maxY - 25.0) < 1e-9 && std::abs(maxX - 10.0) < 1e-9);
    }
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
