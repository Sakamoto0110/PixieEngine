// Lighting L1 (PIXIE-TASK-214): scenes drawn through a ProjectSession into its render target, as the
// editor draws them, and read back. Needs an OpenGL 4.5 context, which main opens hidden.

#include "Check.hpp"

#include "pixie/core/Image.hpp"
#include "pixie/render/2d/LightRenderer.hpp"
#include "pixie/render/gl/GlLoader.hpp"
#include "pixie/runtime/Lighting2D.hpp"
#include "pixie/runtime/LightingDrawing.hpp"
#include "pixie/runtime/LightingEnvironment2D.hpp"
#include "pixie/runtime/ParticleSystem.hpp"
#include "pixie/runtime/SceneMovement2D.hpp"
#include "pixie/runtime/ProjectSession.hpp"
#include "pixie/runtime/ProjectFile.hpp"
#include "pixie/runtime/Scene.hpp"
#include "pixie/runtime/ScreenCanvas.hpp"
#include "pixie/runtime/Shape.hpp"
#include "pixie/runtime/Sprite.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/World.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <memory>
#include <numbers>
#include <stdexcept>
#include <vector>

namespace {

constexpr int kSide = 128;

struct SessionCustomComponent { int value = 0; };

class TestProject : public pixie::Project {
public:
    pxColorRgba background{255, 255, 255};
    bool screenRect = false;
    bool failBuild = false;
    bool failDraw = false;
    pixie::World* retainedWorld = nullptr;
    int builds = 0;
    int captures = 0;

    std::string_view Name() const override { return "lighting-tests"; }
    pxColorRgba Background() const override { return background; }
    void Describe(pixie::ComponentTypes& types) const override {
        types.Add<SessionCustomComponent>("SessionCustomComponent").Field<&SessionCustomComponent::value>("value");
    }
    void Build(pixie::World& world, pixie::Camera2D& camera) override {
        ++builds;
        if (failBuild) {
            world.Create("failed candidate effect");
            if (retainedWorld) retainedWorld->Create("retained World effect");
            camera.SetCenter(pxPoint(777.0, 888.0));
            throw std::runtime_error("intentional candidate build failure");
        }
    }
    void Capture(pixie::World&) override { ++captures; }
    void DrawScreen(pixie::World& /*world*/, pixie::ScreenCanvas& screen) override {
        if (failDraw) throw std::runtime_error("intentional viewport draw failure");
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

TEST(ProjectSessionKeepsPublishedWorldWhenV2CandidateFails) {
    const auto folder = std::filesystem::temp_directory_path() /
        ("pixie-session-candidate-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    const auto created = pixie::CreateProjectFolder(folder);
    CHECK(created.ok);
    if (!created.ok) return;
    {
        std::ofstream out(created.StartScenePath(), std::ios::binary);
        out << R"({"format":"pixie-scene","version":2,"seed":43,"sceneScopes":[],"entities":[{"id":"a"}]})";
    }
    {
        Lab lab;
        CHECK(lab.session->OpenProject(folder));
        pixie::World* published = &lab.World();
        const auto revision = lab.session->WorldRevision();
        const auto camera = lab.session->GetCamera();
        const auto mode = lab.session->Mode();
        const int builds = lab.project->builds;
        const auto rejectedFolder = folder / "rejected";
        const auto rejected = pixie::CreateProjectFolder(rejectedFolder);
        CHECK(rejected.ok);
        if (rejected.ok) {
            std::ofstream out(rejected.StartScenePath(), std::ios::binary);
            out << R"({"format":"pixie-scene","version":2,"seed":99,"sceneScopes":[],
                "entities":[{"id":"b","components":{"SessionCustomComponent":{}}}]})";
            out.close();
            CHECK(!lab.session->OpenProject(rejectedFolder));
            CHECK(&lab.World() == published && lab.session->WorldRevision() == revision);
            CHECK(lab.session->Mode() == mode && lab.project->builds == builds);
        }
        const auto entity = lab.World().Wrap(*lab.World().Registry().view<pixie::SceneId>().begin());
        entity.Add<SessionCustomComponent>();
        CHECK(!lab.session->Play());
        CHECK(lab.session->Error().find("SessionCustomComponent") != std::string::npos);
        CHECK(&lab.World() == published && lab.session->WorldRevision() == revision);
        CHECK(lab.session->GetCamera().Center() == camera.Center() && lab.session->Mode() == mode);
        CHECK(lab.project->builds == builds && entity.IsAlive());
        lab.session->Reset();
        CHECK(lab.session->Error().find("SessionCustomComponent") != std::string::npos);
        CHECK(&lab.World() == published && lab.session->WorldRevision() == revision);
        CHECK(lab.project->builds == builds && entity.IsAlive());
    }
    std::filesystem::remove_all(folder);
}

TEST(ProjectSessionEnvironmentSelectionBelongsToOpenedProject) {
    const auto root = std::filesystem::temp_directory_path() /
        ("pixie-environment-projects-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    const auto first = pixie::CreateProjectFolder(root / "first");
    const auto second = pixie::CreateProjectFolder(root / "second");
    CHECK(first.ok && second.ok);
    if (!first.ok || !second.ok) return;
    for (const auto& path : {first.StartScenePath(), second.StartScenePath()}) {
        std::ofstream out(path, std::ios::binary);
        out << R"({"format":"pixie-scene","version":2,"seed":43,
            "sceneScopes":[{"id":"aa","name":"Room"}],"entities":[]})";
    }
    {
        Lab lab;
        CHECK(lab.session->OpenProject(root / "first"));
        const pixie::EnvironmentSelection2D selection{pixie::EnvironmentSelectionKind2D::Scene, pixie::ScopeId{0xaa}};
        lab.session->SetViewportEnvironment(selection);
        CHECK(lab.session->OpenProject(root / "first" / "."));
        CHECK(lab.session->ViewportEnvironmentSelection() == selection);
        lab.project->failBuild = true;
        CHECK(!lab.session->OpenProject(root / "second"));
        CHECK(lab.session->ViewportEnvironmentSelection() == selection);
        lab.project->failBuild = false;
        CHECK(lab.session->OpenProject(root / "second"));
        CHECK(lab.session->ViewportEnvironmentSelection() == pixie::EnvironmentSelection2D{});
    }
    std::filesystem::remove_all(root);
}

TEST(ProjectSessionSavesAndReopensAtlasSpritesAtReducedZoom) {
    const auto folder = std::filesystem::temp_directory_path() /
        ("pixie-atlas-session-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    const auto project = pixie::CreateProjectFolder(folder);
    CHECK(project.ok);
    if (!project.ok) return;
    std::vector<std::uint8_t> rgba(64 * 32 * 4, 255);
    for (std::size_t i = 0; i < rgba.size(); i += 4) {
        const bool leftHalf = (i / 4) % 64 < 32;
        rgba[i] = leftHalf ? 0 : 255;
        rgba[i + 1] = 0;
        rgba[i + 2] = leftHalf ? 255 : 0;
    }
    CHECK(pixie::WriteFileBytes(folder / "sheet.png", pixie::EncodePng(64, 32, rgba)));
    std::ofstream(folder / "sheet.atlas.json") <<
        R"({"format":"pixie-sprite-atlas","version":1,"image":"sheet.png","imageSize":[64,32],)"
        R"("frames":[{"id":"1b681986317d465c9f01b3f303ff89fa","name":"Red","rect":[36,4,24,24],"pivot":[0,1]}]})";
    {
        Lab lab({9, 11, 19});
        CHECK(lab.session->OpenProject(folder));
        pixie::Sprite sprite;
        sprite.atlasFrame = {"sheet.atlas.json", "1b681986317d465c9f01b3f303ff89fa"};
        sprite.filter = pixie::SpriteFilter::Pixel;
        const auto pixel = lab.Add({-60, 20});
        pixel.Add<pixie::Sprite>(sprite);
        pixie::AddToScene(pixel);
        sprite.filter = pixie::SpriteFilter::Smooth;
        const auto smooth = lab.Add({60, 20});
        smooth.Add<pixie::Sprite>(sprite);
        pixie::AddToScene(smooth);
        CHECK(lab.session->Save());
    }
    {
        Lab lab({9, 11, 19});
        CHECK(lab.session->OpenProject(folder));
        lab.session->GetCamera().SetZoom(0.5);
        const auto pixels = lab.Render();
        for (const int x : {34, 94}) {
            CHECK(Channel(pixels, x, 62) == 255);
            CHECK(Channel(pixels, x + 11, 73) == 255);
            CHECK(Channel(pixels, x, 62, 2) == 0);
            CHECK(Channel(pixels, x - 1, 62) == 9);
            CHECK(Channel(pixels, x, 74) == 9);
            CHECK(lab.session->PickViewport({}, {kSide, kSide}, {x + 0.5, 62.5}));
            CHECK(!lab.session->PickViewport({}, {kSide, kSide}, {x - 0.5, 62.5}));
        }
    }
    std::filesystem::remove_all(folder);
}

TEST(ResourceReloadAndReleaseDiscardGpuObjectsAndKeepSharedDependencies) {
    const auto folder = std::filesystem::temp_directory_path() /
        ("pixie-resource-lifetime-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(folder);
    const auto imageFile = folder / "sheet.png";
    const auto atlasFile = folder / "sheet.atlas.json";
    const auto fontFile = folder / "test.ttf";
    const auto otherFile = folder / "other.ttf";
    const auto writePng = [&](std::uint8_t alpha) {
        std::vector<std::uint8_t> rgba(32 * 16 * 4, alpha);
        CHECK(pixie::WriteFileBytes(imageFile, pixie::EncodePng(32, 16, rgba)));
    };
    const auto writeAtlas = [&](int x) {
        std::ofstream(atlasFile) <<
            R"({"format":"pixie-sprite-atlas","version":1,"image":"sheet.png","imageSize":[32,16],)"
            R"("frames":[{"id":"1b681986317d465c9f01b3f303ff89fa","name":"Frame","rect":[)"
            << x << R"(,0,16,16],"pivot":[0,1]}]})";
    };
    writePng(255);
    writeAtlas(0);
    std::filesystem::copy_file(PIXIE_TEST_FONT, fontFile);
    std::filesystem::copy_file(PIXIE_TEST_FONT, otherFile);
    {
        pixie::SpriteAlphaOutlines outlines;
        pixie::SpriteImages images(outlines);
        pixie::TextRenderer renderer;
        pixie::Fonts fonts(renderer);
        images.SetFolder(folder);
        outlines.SetFolder(folder);
        fonts.SetFolder(folder);

        pixie::Sprite sprite;
        sprite.atlasFrame = {"sheet.atlas.json", "1b681986317d465c9f01b3f303ff89fa"};
        auto source = images.Source(sprite);
        const auto firstTexture = images.TextureOf(source.image).Id();
        CHECK(firstTexture != 0 && firstTexture == images.TextureOf("sheet.png").Id());
        pxVec2 pixelSize;
        CHECK(outlines.Edges(source, 0.5, pixelSize).size() == 4);
        const auto imageTime = std::filesystem::last_write_time(imageFile);
        writePng(0);
        std::filesystem::last_write_time(imageFile, imageTime);
        CHECK(images.CheckForChanges() == 0);
        CHECK(images.Reload("sheet.png") && !glIsTexture(firstTexture));
        source = images.Source(sprite);
        CHECK(outlines.Edges(source, 0.5, pixelSize).empty());
        const auto sharedTexture = images.TextureOf(source.image).Id();
        CHECK(images.ReleaseAtlas("sheet.atlas.json"));
        CHECK(!images.ReleaseAtlas("sheet.atlas.json") && glIsTexture(sharedTexture));
        CHECK(images.Source(sprite).region.x == 0);

        const auto atlasTime = std::filesystem::last_write_time(atlasFile);
        writeAtlas(16);
        writePng(255);
        std::filesystem::last_write_time(atlasFile, atlasTime);
        std::filesystem::last_write_time(imageFile, imageTime);
        CHECK(images.CheckForChanges() == 0);
        CHECK(images.ReloadAtlas("sheet.atlas.json") && !glIsTexture(sharedTexture));
        CHECK(images.Source(sprite).region.x == 16);
        const auto reloaded = images.TextureOf("sheet.png").Id();
        CHECK(images.Release("sheet.png") && !glIsTexture(reloaded));
        CHECK(!images.Release("sheet.png") && images.Source(sprite).image == "sheet.png");
        const auto checkerboard = images.TextureOf("missing.png").Id();
        images.ReleaseAll();
        CHECK(!glIsTexture(checkerboard) && images.Folder() == folder);
        CHECK(!images.ReleaseAtlas("sheet.atlas.json") && !images.Release("missing.png"));

        auto* font = fonts.Find("test.ttf");
        auto* other = fonts.Find("other.ttf");
        CHECK(font && other);
        if (font && other) {
            const auto id = font->Id();
            const auto small = renderer.AtlasTexture(*font, 16);
            const auto large = renderer.AtlasTexture(*font, 32);
            const auto otherTexture = renderer.AtlasTexture(*other, 16);
            CHECK(glIsTexture(small) && glIsTexture(large) && glIsTexture(otherTexture));
            const auto fontTime = std::filesystem::last_write_time(fontFile);
            std::ofstream(fontFile, std::ios::binary | std::ios::trunc).close();
            std::filesystem::last_write_time(fontFile, fontTime);
            CHECK(!fonts.Reload("test.ttf"));
            CHECK(!glIsTexture(small) && !glIsTexture(large) && glIsTexture(otherTexture));
            CHECK(!fonts.Find("test.ttf") && !fonts.Problem("test.ttf").empty());
            std::filesystem::copy_file(PIXIE_TEST_FONT, fontFile, std::filesystem::copy_options::overwrite_existing);
            CHECK(fonts.Reload("test.ttf"));
            font = fonts.Find("test.ttf");
            CHECK(font && font->Id() != id);
            if (font) {
                const auto replacement = renderer.AtlasTexture(*font, 16);
                CHECK(glIsTexture(replacement));
                CHECK(fonts.Release("test.ttf") && !glIsTexture(replacement));
                CHECK(!fonts.Release("test.ttf") && glIsTexture(otherTexture));
                CHECK(fonts.Find("test.ttf") && fonts.Problem("test.ttf").empty());
            }
            fonts.ReleaseAll();
            CHECK(!glIsTexture(otherTexture) && fonts.Folder() == folder);
            CHECK(!fonts.Release("other.ttf"));
        }
        renderer.Destroy();
    }
    std::filesystem::remove_all(folder);
}

TEST(ProjectSessionReleasesResourceCachesOnProjectChangeAndStop) {
    const auto root = std::filesystem::temp_directory_path() /
        ("pixie-resource-projects-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    const auto first = pixie::CreateProjectFolder(root / "first");
    const auto second = pixie::CreateProjectFolder(root / "second");
    CHECK(first.ok && second.ok);
    if (!first.ok || !second.ok) return;
    for (const auto& folder : {first.folder, second.folder}) {
        const std::vector<std::uint8_t> pixels(16 * 16 * 4, 255);
        CHECK(pixie::WriteFileBytes(folder / "sprite.png", pixie::EncodePng(16, 16, pixels)));
        std::filesystem::copy_file(PIXIE_TEST_FONT, folder / "font.ttf");
    }
    {
        Lab lab;
        CHECK(lab.session->OpenProject(first.folder));
        auto& images = lab.session->Images();
        auto& fonts = lab.session->GetFonts();
        const auto firstTexture = images.TextureOf("sprite.png").Id();
        const auto* firstFont = fonts.Find("font.ttf");
        CHECK(glIsTexture(firstTexture) && firstFont);
        const auto firstFontId = firstFont ? firstFont->Id() : 0;

        CHECK(lab.session->OpenProject(second.folder));
        CHECK(!glIsTexture(firstTexture));
        CHECK(!images.Release("sprite.png") && !fonts.Release("font.ttf"));
        const auto secondTexture = images.TextureOf("sprite.png").Id();
        const auto* secondFont = fonts.Find("font.ttf");
        CHECK(glIsTexture(secondTexture) && secondFont && secondFont->Id() != firstFontId);

        lab.session->Stop();
        CHECK(!glIsTexture(secondTexture));
        CHECK(!images.Release("sprite.png") && !fonts.Release("font.ttf"));
    }
    std::filesystem::remove_all(root);
}

TEST(ProjectSessionKeepsPublishedWorldWhenCandidateBuildThrows) {
    Lab lab;
    pixie::World* published = &lab.World();
    const auto revision = lab.session->WorldRevision();
    const auto camera = lab.session->GetCamera();
    const auto count = lab.World().EntityCount();
    lab.project->failBuild = true;
    lab.session->Reset();
    CHECK(lab.session->Error().find("intentional candidate build failure") != std::string::npos);
    CHECK(&lab.World() == published && lab.session->WorldRevision() == revision);
    CHECK(lab.session->GetCamera().Center() == camera.Center());
    CHECK(lab.World().EntityCount() == count);
    // Candidate isolation does not revoke pointers retained by arbitrary Project code.
    lab.project->retainedWorld = published;
    lab.session->Reset();
    CHECK(&lab.World() == published && lab.session->WorldRevision() == revision);
    CHECK(lab.World().EntityCount() == count + 1);
    CHECK(lab.session->GetCamera().Center() == camera.Center());
    lab.project->retainedWorld = nullptr;
}

TEST(ProjectSessionFailedStartCanRetryWithoutPublishing) {
    auto project = std::make_unique<TestProject>();
    TestProject* control = project.get();
    pixie::ProjectSession session(std::move(project), {});
    const auto camera = session.GetCamera();
    control->failBuild = true;
    CHECK(!session.Start({kSide, kSide}));
    CHECK(session.WorldRevision() == 0 && session.GetCamera().Viewport().width == camera.Viewport().width);
    CHECK(session.Error().find("intentional candidate build failure") != std::string::npos);
    control->failBuild = false;
    CHECK(session.Start({kSide, kSide}));
    CHECK(session.WorldRevision() == 1);
    session.Stop();
}

TEST(ProjectSessionFailedStopPlayingRetainsPlayWorld) {
    const auto folder = std::filesystem::temp_directory_path() /
        ("pixie-session-stop-candidate-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    const auto created = pixie::CreateProjectFolder(folder);
    CHECK(created.ok);
    if (!created.ok) return;
    {
        std::ofstream out(created.StartScenePath(), std::ios::binary);
        out << R"({"format":"pixie-scene","version":2,"seed":43,"sceneScopes":[],"entities":[{"id":"a"}]})";
    }
    {
        Lab lab;
        CHECK(lab.session->OpenProject(folder));
        CHECK(lab.session->Play());
        pixie::World* published = &lab.World();
        const auto revision = lab.session->WorldRevision();
        const auto camera = lab.session->GetCamera();
        lab.project->failBuild = true;
        CHECK(!lab.session->StopPlaying());
        CHECK(lab.session->Mode() == pixie::SessionMode::Play);
        CHECK(&lab.World() == published && lab.session->WorldRevision() == revision);
        CHECK(lab.session->GetCamera().Center() == camera.Center());
        lab.project->failBuild = false;
        CHECK(lab.session->StopPlaying());
        CHECK(lab.session->Mode() == pixie::SessionMode::Edit);
    }
    std::filesystem::remove_all(folder);
}

TEST(ProjectSessionRestoresCompleteV2AuthoredSceneThroughCandidate) {
    const auto folder = std::filesystem::temp_directory_path() /
        ("pixie-session-restore-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    const auto created = pixie::CreateProjectFolder(folder);
    CHECK(created.ok);
    if (!created.ok) return;
    auto diskBytes = [&] {
        std::ifstream in(created.StartScenePath(), std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    };
    const std::string legacyBytes = diskBytes();
    {
        Lab lab;
        CHECK(lab.session->OpenProject(folder));
        pixie::World* original = &lab.World();
        const auto revision = lab.session->WorldRevision();
        const int captures = lab.project->captures;
        lab.session->GetCamera().SetCenter(pxPoint(5.0, 6.0));
        const std::string snapshot = R"({"format":"pixie-scene","version":2,"seed":99,
            "sceneScopes":[{"id":"aa","name":"Empty","futureScope":{"keep":true}}],
            "futureRoot":{"keep":true},
            "entities":[{"id":"a","components":{"Transform2D":{"position":[3,4]},
                "FutureType":{"value":7}}}]})";
        CHECK(lab.session->RestoreAuthoredScene(snapshot));
        CHECK(&lab.World() != original && lab.session->WorldRevision() == revision + 1);
        CHECK(lab.session->Mode() == pixie::SessionMode::Edit && lab.session->HasUnsavedChanges());
        CHECK(lab.project->captures == captures);
        CHECK(lab.session->GetCamera().Center() == pxPoint(5.0, 6.0));
        CHECK(lab.World().GetRandom().Seed() == 99 && lab.World().EntityCount() == 1);
        lab.World().GetRandom().NextU64();
        auto continuedRandom = lab.World().GetRandom();
        CHECK(lab.session->RestoreAuthoredScene(snapshot, nullptr, false, true));
        CHECK(lab.World().GetRandom().NextU64() == continuedRandom.NextU64());
        auto changedSeed = snapshot;
        const std::string seedToken = "\"seed\":99";
        changedSeed.replace(changedSeed.find(seedToken), seedToken.size(), "\"seed\":123");
        CHECK(lab.session->RestoreAuthoredScene(changedSeed, nullptr, false, true));
        CHECK(lab.World().GetRandom().Seed() == 123);
        pixie::Random reseeded(123);
        CHECK(lab.World().GetRandom().NextU64() == reseeded.NextU64());
        CHECK(lab.session->RestoreAuthoredScene(snapshot, nullptr, false, true));
        CHECK(lab.World().Scopes().size() == 1 && lab.World().Scopes()[0].id == pixie::ScopeId{0xaa});
        CHECK(lab.World().ResolveGravity(pixie::ScopeId{0xaa}).value == pxVec2(0, 1100));
        const auto found = lab.World().Registry().view<const pixie::SceneId>();
        CHECK(found.begin() != found.end());
        if (found.begin() != found.end()) {
            CHECK(found.get<const pixie::SceneId>(*found.begin()).value == 0xa);
            CHECK(lab.World().ScopeOf(lab.World().Wrap(*found.begin())) == pixie::ScopeId{});
        }
        const std::string saved = pixie::WriteScene(lab.World(), lab.session->Types());
        CHECK(saved.find("\"futureRoot\"") != std::string::npos);
        CHECK(saved.find("\"futureScope\"") != std::string::npos);
        CHECK(saved.find("\"FutureType\"") != std::string::npos);
        CHECK(saved.find("\"value\": 7") != std::string::npos);

        pixie::World* published = &lab.World();
        const auto publishedRevision = lab.session->WorldRevision();
        const std::string before = pixie::WriteScene(lab.World(), lab.session->Types());
        CHECK(!lab.session->Save());
        CHECK(lab.session->Error().find("explicit") != std::string::npos);
        CHECK(diskBytes() == legacyBytes && lab.session->HasUnsavedChanges());
        CHECK(&lab.World() == published && lab.session->WorldRevision() == publishedRevision);
        CHECK(pixie::WriteScene(lab.World(), lab.session->Types()) == before);
        CHECK(!std::filesystem::exists(created.StartScenePath().string() + ".v1-backup"));
        const std::string rejected = R"({"format":"pixie-scene","version":2,"seed":1,
            "sceneScopes":[],"entities":[{"id":"b","components":{"SessionCustomComponent":{}}}]})";
        CHECK(!lab.session->RestoreAuthoredScene(rejected));
        CHECK(lab.session->Error().find("SessionCustomComponent") != std::string::npos);
        CHECK(&lab.World() == published && lab.session->WorldRevision() == publishedRevision);
        CHECK(lab.session->GetCamera().Center() == pxPoint(5.0, 6.0));
        CHECK(lab.session->Mode() == pixie::SessionMode::Edit && lab.session->HasUnsavedChanges());
        CHECK(lab.project->captures == captures);
        CHECK(pixie::WriteScene(lab.World(), lab.session->Types()) == before);

        CHECK(lab.session->Play());
        pixie::World* playWorld = &lab.World();
        const auto playRevision = lab.session->WorldRevision();
        CHECK(!lab.session->Save() && diskBytes() == legacyBytes);
        CHECK(&lab.World() == playWorld && lab.session->WorldRevision() == playRevision);
        CHECK(lab.session->HasUnsavedChanges() && lab.session->Mode() == pixie::SessionMode::Play);
        CHECK(!lab.session->RestoreAuthoredScene(snapshot));
        CHECK(&lab.World() == playWorld && lab.session->Mode() == pixie::SessionMode::Play);
        CHECK(lab.session->StopPlaying());
        CHECK(lab.World().Scopes().size() == 1);
        CHECK(pixie::WriteScene(lab.World(), lab.session->Types()) == before);

        // A pre-existing v2 scene is writable. Inspect the current disk envelope, not the
        // restored sceneText_: a later external replacement with v1 must close this path again.
        CHECK(pixie::SaveTextFile(snapshot, created.StartScenePath()).ok);
        CHECK(lab.session->OpenProject(folder));
        CHECK(lab.session->RestoreAuthoredScene(snapshot));
        CHECK(lab.session->Save() && !lab.session->HasUnsavedChanges());
        CHECK(diskBytes().find("\"version\": 2") != std::string::npos);
        CHECK(lab.session->RestoreAuthoredScene(snapshot));
        CHECK(pixie::SaveTextFile(legacyBytes, created.StartScenePath()).ok);
        CHECK(!lab.session->Save() && lab.session->HasUnsavedChanges());
        CHECK(diskBytes() == legacyBytes);
    }
    std::filesystem::remove_all(folder);
}

TEST(TwoViewportContextsShareWorldWithoutLeakingCameraEnvironmentOrViewSettings) {
    Lab lab({0, 0, 0});
    auto firstShape = lab.Quad({0, 0}, 12, {255, 255, 255});
    auto secondShape = lab.Quad({200, 0}, 12, {255, 255, 255});
    auto firstCamera = lab.Add({0, 0}); firstCamera.Add<pixie::SceneId>(pixie::SceneId{1001}); firstCamera.Add<pixie::CameraComponent2D>();
    auto secondCamera = lab.Add({200, 0}); secondCamera.Add<pixie::SceneId>(pixie::SceneId{1002}); secondCamera.Add<pixie::CameraComponent2D>().zoom = 2;
    pixie::WorldPresentation2D root; root.lighting = pixie::LightingState2D::Configured;
    CHECK(lab.World().SetPresentation(root));
    CHECK(lab.World().CreateScope(pixie::ScopeId{1}, "red"));
    CHECK(lab.World().CreateScope(pixie::ScopeId{2}, "green"));
    pixie::SceneEnvironment2D red; red.overrideWorld = true; red.value.ambient = {255, 0, 0};
    pixie::SceneEnvironment2D green; green.overrideWorld = true; green.value.ambient = {0, 255, 0};
    CHECK(lab.World().SetScopeEnvironment(pixie::ScopeId{1}, red));
    CHECK(lab.World().SetScopeEnvironment(pixie::ScopeId{2}, green));
    const pixie::CameraSelection2D a{pixie::CameraSelectionKind2D::Entity, 1001, {}};
    const pixie::CameraSelection2D b{pixie::CameraSelectionKind2D::Entity, 1002, {}};
    const auto primaryCamera = lab.session->ViewportCameraSelection();
    const auto primaryEnvironment = lab.session->ViewportEnvironmentSelection();
    const auto center = lab.session->GetCamera().Center();
    pixie::RenderTarget first, second;
    glViewport(3, 4, 117, 119);
    const auto textureA = lab.session->RenderViewportToTarget(first, {kSide, kSide}, a,
        {pixie::EnvironmentSelectionKind2D::Scene, pixie::ScopeId{1}}, 1);
    pixie::ViewLighting2D dim; dim.exposure = 0.5;
    const auto textureB = lab.session->RenderViewportToTarget(second, {96, 64}, b,
        {pixie::EnvironmentSelectionKind2D::Scene, pixie::ScopeId{2}}, 1, dim);
    CHECK(textureA != 0 && textureB != 0 && textureA != textureB);
    std::vector<std::uint8_t> pixelsA(static_cast<std::size_t>(kSide) * kSide * 4), pixelsB(96 * 64 * 4);
    glGetTextureImage(textureA, 0, GL_RGBA, GL_UNSIGNED_BYTE, static_cast<GLsizei>(pixelsA.size()), pixelsA.data());
    glGetTextureImage(textureB, 0, GL_RGBA, GL_UNSIGNED_BYTE, static_cast<GLsizei>(pixelsB.size()), pixelsB.data());
    CHECK(Channel(pixelsA, 64, 64, 0) > 200 && Channel(pixelsA, 64, 64, 1) < 4);
    CHECK(pixelsB[(32 * 96 + 48) * 4 + 1] > 100 && pixelsB[(32 * 96 + 48) * 4] < 4);
    CHECK(lab.session->PickViewport(a, {kSide, kSide}, {64, 64}).Id() == firstShape.Id());
    CHECK(lab.session->PickViewport(b, {96, 64}, {48, 32}).Id() == secondShape.Id());
    CHECK(lab.session->ViewportCameraSelection() == primaryCamera);
    CHECK(lab.session->ViewportEnvironmentSelection() == primaryEnvironment);
    CHECK(lab.session->GetCamera().Center() == center && lab.session->GetCamera().Zoom() == 1);
    CHECK(lab.World().Presentation()->view.exposure == 1);
    GLint viewport[4]{}; glGetIntegerv(GL_VIEWPORT, viewport);
    CHECK(viewport[0] == 3 && viewport[1] == 4 && viewport[2] == 117 && viewport[3] == 119);
    first.Bind();
    GLint previousFramebuffer = 0; glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousFramebuffer);
    GLint previousViewport[4]{}; glGetIntegerv(GL_VIEWPORT, previousViewport);
    lab.project->failDraw = true;
    bool refused = false;
    try { lab.session->RenderViewportToTarget(second, {96, 64}, b,
        {pixie::EnvironmentSelectionKind2D::Scene, pixie::ScopeId{2}}, 1, dim); }
    catch (const std::runtime_error&) { refused = true; }
    lab.project->failDraw = false;
    GLint framebuffer = 0; glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &framebuffer);
    glGetIntegerv(GL_VIEWPORT, viewport);
    CHECK(refused && framebuffer == previousFramebuffer);
    for (int i = 0; i < 4; ++i) CHECK(viewport[i] == previousViewport[i]);
    CHECK(lab.session->ViewportCameraSelection() == primaryCamera && lab.session->ViewportEnvironmentSelection() == primaryEnvironment);
    CHECK(lab.session->GetCamera().Viewport().width == kSide && lab.session->GetCamera().Viewport().height == kSide);
    CHECK(lab.World().Presentation()->view.exposure == 1);
    pixie::RenderTarget::BindDefault(); first.Destroy(); second.Destroy();
}

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

TEST(DisabledLightingDrawsTheUnlitImageAndKeepsItsSettings) {
    Lab lab(pxColorRgba(30, 60, 90));
    AddSomeContent(lab);
    const auto unlit = lab.Render();
    auto settings = Dark();
    settings.fog = true;
    settings.bloom = true;
    settings.enabled = false;
    lab.Settings(settings);
    CHECK(lab.Render() == unlit);
    auto* authored = pixie::EditLegacyLightingSettings2D(lab.World());
    CHECK(authored != nullptr);
    if (authored == nullptr) return;
    authored->enabled = true;
    CHECK(lab.Render() != unlit);
    CHECK(authored->fog && authored->bloom);
}

TEST(ViewportEnvironmentChoicePreservesCameraAndLegacyImage) {
    Lab lab(pxColorRgba(30, 60, 90));
    AddSomeContent(lab);
    lab.Settings(Dark());
    const auto original = lab.Render();
    const auto center = lab.session->GetCamera().Center();
    const double zoom = lab.session->GetCamera().Zoom();
    const pixie::ScopeId room{81};
    CHECK(lab.World().CreateScope(room, "room"));
    const pixie::EnvironmentSelection2D selection{pixie::EnvironmentSelectionKind2D::Scene, room};
    lab.session->SetViewportEnvironment(selection);
    CHECK(lab.session->ViewportEnvironmentSelection() == selection);
    CHECK(lab.session->ViewportEnvironment()->kind == pixie::EnvironmentResolutionKind2D::SceneInheritedWorld);
    CHECK(lab.Render() == original);
    CHECK(lab.session->GetCamera().Center() == center && lab.session->GetCamera().Zoom() == zoom);

    // Entity membership does not drive the viewport choice; editor selection is not a runtime input.
    const pixie::ScopeId otherRoom{82};
    CHECK(lab.World().CreateScope(otherRoom, "other room"));
    auto entity = lab.World().Create();
    CHECK(lab.World().SetScope(entity, otherRoom));
    CHECK(lab.session->ViewportEnvironmentSelection() == selection);
    CHECK(lab.Render() == original);

    CHECK(lab.World().RemoveScope(room));
    const auto fallback = lab.session->ViewportEnvironment();
    CHECK(fallback && fallback->kind == pixie::EnvironmentResolutionKind2D::RemovedSceneFallback);
    if (fallback) CHECK(!fallback->diagnostic.empty());
    CHECK(lab.Render() == original);
    CHECK(lab.session->ViewportEnvironmentSelection() == selection);
    CHECK(lab.World().CreateScope(room, "restored room"));
    CHECK(lab.session->ViewportEnvironment()->kind == pixie::EnvironmentResolutionKind2D::SceneInheritedWorld);
    CHECK(lab.Render() == original);
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

namespace {

double OutlineArea(const std::vector<pixie::OutlineEdge>& edges) {
    double sum = 0.0;
    for (const auto& e : edges) sum += e.a.x * e.b.y - e.b.x * e.a.y;
    return sum / 2.0;
}

double EdgeArea(const std::vector<pixie::OccluderEdge>& edges) {
    double sum = 0.0;
    for (const auto& e : edges) sum += static_cast<double>(e.ax) * e.by - static_cast<double>(e.bx) * e.ay;
    return sum / 2.0;
}

} // namespace

// LIT-006: a sprite's alpha outline is the boundary between solid and empty cells, merged into
// runs, wound so that it encloses a positive area: an L of 48 of 64 pixels has 6 edges and 0.75 of
// the sprite's area.
TEST(SpriteAlphaOutlineTracesTheBoundary) {
    std::vector<std::uint8_t> rgba(8 * 8 * 4, 0);
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            rgba[(static_cast<std::size_t>(y) * 8 + x) * 4 + 3] = x < 4 || y >= 4 ? 255 : 10;
        }
    }
    const auto edges = pixie::SpriteAlphaOutlines::Trace(8, 8, rgba, 0.5, 8);
    CHECK(edges.size() == 6);
    CHECK(std::abs(OutlineArea(edges) - 0.75) < 1e-9);
    CHECK(pixie::SpriteAlphaOutlines::Trace(8, 8, rgba, 0.01, 8).size() == 4);
}

// LIT-006: a SpriteAlpha occluder casts from its sprite's outline at the entity's transform, mirrored
// sprites included.
TEST(SpriteAlphaOccludersFollowTheSprite) {
    const std::filesystem::path folder = std::filesystem::temp_directory_path() / "pixie-lighting-tests";
    std::filesystem::create_directories(folder);
    const std::vector<std::uint8_t> opaque(8 * 8 * 4, 255);
    CHECK(pixie::WriteFileBytes(folder / "solid.png", pixie::EncodePng(8, 8, opaque)));
    pixie::SpriteAlphaOutlines outlines;
    outlines.SetFolder(folder);
    for (const double flip : {1.0, -1.0}) {
        pixie::World world;
        pixie::Entity entity = world.Create();
        entity.Add<pixie::Transform2D>(pixie::Transform2D{pxPoint(100.0, 0.0), 0.5, pxVec2(2.0 * flip, 2.0)});
        pixie::Sprite sprite;
        sprite.image = "solid.png";
        entity.Add<pixie::Sprite>(sprite);
        pixie::Occluder2D occluder;
        occluder.source = pixie::OccluderSource2D::SpriteAlpha;
        entity.Add<pixie::Occluder2D>(occluder);
        std::vector<pixie::OccluderEdge> edges;
        pixie::GatherOccluders(world, 1.0, edges, &outlines);
        CHECK(edges.size() == 4);
        CHECK(std::abs(EdgeArea(edges) - 256.0) < 1e-3);
        double cx = 0.0;
        for (const auto& e : edges) cx += e.ax / 4.0;
        CHECK(std::abs(cx - 100.0) < 1e-3);
        // Without outlines, a SpriteAlpha occluder casts nothing.
        pixie::GatherOccluders(world, 1.0, edges);
        CHECK(edges.empty());
    }
}

// ---------------------------------------------------------------------------------------------
// L3: emission and particle light (PIXIE-TASK-216).

namespace {

pixie::ParticleSystem& Pool(Lab& lab, pxColorRgba color, float size, bool additive = true) {
    pixie::Entity entity = lab.World().Create();
    pixie::ParticleSystem system;
    system.style.color = color;
    system.style.size = size;
    system.style.additive = additive;
    system.style.fade = false;
    entity.Add<pixie::ParticleSystem>(std::move(system));
    return entity.Get<pixie::ParticleSystem>();
}

} // namespace

// LIT-009: an emissive shape shows its emissive color in darkness, at full strength with intensity 1.
TEST(EmissionShowsInTheDark) {
    Lab lab;
    lab.Settings(Dark());
    lab.Quad(pxPoint(-30.0, 0.0), 12.0, pxColorRgba(255, 0, 0)).Add<pixie::Emission2D>(pixie::Emission2D{pxColorRgba(200, 100, 50), 1.0});
    lab.Quad(pxPoint(30.0, 0.0), 12.0, pxColorRgba(255, 0, 0));
    const auto pixels = lab.Render();
    CHECK(Near(At(pixels, -30.0, 0.0, 0), 200) && Near(At(pixels, -30.0, 0.0, 1), 100) && Near(At(pixels, -30.0, 0.0, 2), 50));
    CHECK(At(pixels, 30.0, 0.0, 0) == 0);
}

// LIT-009: game code fades the intensity, and the glow follows it step by step; above 1 it brightens.
TEST(EmissionFollowsItsIntensity) {
    Lab lab;
    lab.Settings(Dark());
    pixie::Entity lamp = lab.Quad(pxPoint(0.0, 0.0), 12.0, pxColorRgba(255, 255, 255));
    lamp.Add<pixie::Emission2D>(pixie::Emission2D{pxColorRgba(100, 100, 100), 1.0});
    int previous = 256;
    for (const double intensity : {1.0, 0.75, 0.5, 0.25, 0.0}) {
        lamp.Get<pixie::Emission2D>().intensity = intensity;
        const int value = At(lab.Render(), 0.0, 0.0);
        std::fprintf(stderr, "  powered-down step: intensity %.2f, pixel %d\n", intensity, value);
        CHECK(value < previous);
        CHECK(Near(value, Expected(pixie::DecodeSrgb(100.0f / 255.0f) * intensity), 2));
        previous = value;
    }
    lamp.Get<pixie::Emission2D>().intensity = 2.0;
    CHECK(Near(At(lab.Render(), 0.0, 0.0), Expected(2.0 * pixie::DecodeSrgb(100.0f / 255.0f)), 2));
}

// LIT-009/012: what is drawn over an emissive sprite covers its glow.
TEST(EmissionIsCoveredByWhatLiesOver) {
    Lab lab;
    lab.Settings(Dark());
    pixie::Sprite glowing;
    glowing.image = "missing.png";
    glowing.layer = 0;
    // A missing image draws as a 64-unit checkerboard: the glowing one spans 64 units, the cover 16.
    pixie::Entity under = lab.Add(pxPoint(0.0, 0.0));
    under.Add<pixie::Sprite>(glowing);
    under.Add<pixie::Emission2D>(pixie::Emission2D{});
    pixie::Sprite cover = glowing;
    cover.layer = 1;
    pixie::Entity over = lab.Add(pxPoint(0.0, 0.0));
    over.Get<pixie::Transform2D>().scale = pxVec2(0.25, 0.25);
    lab.World().ResetInterpolation(over);
    over.Add<pixie::Sprite>(cover);
    const auto pixels = lab.Render();
    int brightOutside = 0, brightUnder = 0;
    for (int y = 0; y < kSide; ++y) {
        for (int x = 0; x < kSide; ++x) {
            const double wx = x + 0.5 - 64.0, wy = y + 0.5 - 64.0;
            const bool bright = Channel(pixels, x, y) > 100;
            if (std::abs(wx) < 6.0 && std::abs(wy) < 6.0) brightUnder += bright ? 1 : 0;
            else if (std::abs(wx) > 12.0 && std::abs(wx) < 28.0 && std::abs(wy) < 28.0) brightOutside += bright ? 1 : 0;
        }
    }
    std::fprintf(stderr, "  emissive sprite: %d bright pixels uncovered, %d under the cover\n", brightOutside, brightUnder);
    CHECK(brightOutside > 100);
    CHECK(brightUnder == 0);
}

// LIT-010: an emissive pool glows unlit at its own color in darkness; the same pool not emissive is
// lit like the world, so it stays dark.
TEST(EmissiveParticlesGlowUnlit) {
    for (const bool emissive : {true, false}) {
        Lab lab;
        lab.Settings(Dark());
        pixie::ParticleSystem& pool = Pool(lab, pxColorRgba(255, 200, 100), 6.0f);
        pool.Emit(pxPointf(0.0f, 0.0f), pxVec2f(0.0f, 0.0f), 0.0f);
        if (emissive) {
            pixie::ParticleLight2D light;
            light.emissive = true;
            lab.World().Registry().emplace<pixie::ParticleLight2D>(lab.World().Registry().view<pixie::ParticleSystem>().front(), light);
        }
        const auto pixels = lab.Render();
        if (emissive) {
            CHECK(Near(At(pixels, 0.0, 0.0, 0), 255, 2) && Near(At(pixels, 0.0, 0.0, 1), 200, 2) && Near(At(pixels, 0.0, 0.0, 2), 100, 2));
        } else {
            CHECK(At(pixels, 0.0, 0.0, 0) == 0);
        }
    }
}

// LIT-011: an illuminating pool lights the floor around each particle as a point light; a pool that
// does not illuminate lights nothing; caps hold per pool and over all pools.
TEST(IlluminatingParticlesLightTheFloor) {
    for (const bool illuminates : {true, false}) {
        Lab lab;
        lab.Settings(Dark());
        pixie::ParticleSystem& pool = Pool(lab, pxColorRgba(255, 255, 255), 1.0f);
        pool.Emit(pxPointf(0.0f, 0.0f), pxVec2f(0.0f, 0.0f), 0.0f);
        pixie::ParticleLight2D light;
        light.illuminates = illuminates;
        light.lightRadius = 30.0;
        lab.World().Registry().emplace<pixie::ParticleLight2D>(lab.World().Registry().view<pixie::ParticleSystem>().front(), light);
        const auto pixels = lab.Render();
        const double x = 1.0 - (10.5 / 30.0) * (10.5 / 30.0);
        CHECK(Near(At(pixels, 10.5, 0.5), illuminates ? Expected(x * x) : 0));
    }
    Lab lab;
    pixie::ParticleSystem& pool = Pool(lab, pxColorRgba(255, 255, 255), 1.0f);
    for (int i = 0; i < 10; ++i) {
        pool.Emit(pxPointf(static_cast<float>(i), 0.0f), pxVec2f(0.0f, 0.0f), 0.0f);
    }
    pixie::ParticleLight2D light;
    light.illuminates = true;
    light.lightCap = 3;
    lab.World().Registry().emplace<pixie::ParticleLight2D>(lab.World().Registry().view<pixie::ParticleSystem>().front(), light);
    std::vector<pixie::LightInstance> lights;
    CHECK(pixie::GatherParticleLights(lab.World(), 1.0, nullptr, 2048, lights) == 7 && lights.size() == 3);
    lights.clear();
    CHECK(pixie::GatherParticleLights(lab.World(), 1.0, nullptr, 2, lights) == 8 && lights.size() == 2);
}

// ---------------------------------------------------------------------------------------------
// L4: budgets, telemetry and debug views (PIXIE-TASK-217).

// LIT-008: over the quality cap, the shadow-casting lights that contribute least are left out, never
// drawn unshadowed, and unshadowed lights are untouched; the count is reported.
TEST(ShadowedLightsAreCappedByQuality) {
    CHECK(pixie::ShadowedLightCap(pixie::LightQuality2D::Low) == 8 && pixie::ShadowedLightCap(pixie::LightQuality2D::Medium) == 32 &&
          pixie::ShadowedLightCap(pixie::LightQuality2D::High) == 64);
    std::vector<pixie::LightInstance> lights;
    for (int i = 0; i < 6; ++i) {
        pixie::LightInstance light;
        light.x = static_cast<float>(i * 10);
        light.range = 50.0f;
        const float strength = 0.1f * static_cast<float>(i + 1);
        light.color = {strength, strength, strength};
        light.shadowLayers = i == 0 ? 0u : 0xFFu;  // the weakest casts no shadow
        lights.push_back(light);
    }
    const pxRect view{-100.0, -100.0, 300.0, 200.0};
    CHECK(pixie::CapShadowedLights(lights, 3, view) == 2);
    CHECK(lights.size() == 4);
    // The unshadowed one stays, and the two weakest shadowed ones (i = 1, 2) are gone.
    CHECK(lights.size() == 4 && lights[0].x == 0.0f && lights[1].x == 30.0f && lights[2].x == 40.0f && lights[3].x == 50.0f);
}

// LIT-008: the Low preset computes shadows at half resolution: a closed room still lets no light out
// beyond a pixel of filtering at its walls, and soft shadows stay near the reference.
TEST(LowQualityShadowsAtHalfResolution) {
    Lab lab;
    pixie::LightingSettings2D settings = Dark();
    settings.quality = pixie::LightQuality2D::Low;
    lab.Settings(settings);
    lab.Add(pxPoint(0.0, 0.0)).Add<pixie::Light2D>(FlatLight(120.0, 4.0));
    Box(lab, pxPoint(0.0, -31.0), pxVec2(32.0, 1.0));
    Box(lab, pxPoint(0.0, 31.0), pxVec2(32.0, 1.0));
    Box(lab, pxPoint(-31.0, 0.0), pxVec2(1.0, 30.0));
    Box(lab, pxPoint(31.0, 0.0), pxVec2(1.0, 30.0));
    const auto room = lab.Render();
    int leaks = 0;
    for (int y = 0; y < kSide; ++y) {
        for (int x = 0; x < kSide; ++x) {
            const double wx = x + 0.5 - 64.0, wy = y + 0.5 - 64.0;
            if (std::abs(wx) > 34.0 || std::abs(wy) > 34.0) leaks += Channel(room, x, y) != 0 ? 1 : 0;
        }
    }
    std::fprintf(stderr, "  low quality closed room: %d lit pixels beyond a pixel of the walls\n", leaks);
    CHECK(leaks == 0);
}

// LIT-005: light layers have names, per scene, by stable slot: "Default" and "Light layer N" without a
// LightLayers2D, the scene's names with one, and unnamed again for duplicate slots.
TEST(LightLayersHaveNames) {
    pixie::World world;
    auto names = pixie::LightLayerNames(world);
    CHECK(names[0] == "Default" && names[7] == "Light layer 8");
    pixie::LightLayers2D layers;
    layers.layers.push_back({{2, "Background"}, {}});
    pixie::Entity config = world.Create();
    config.Add<pixie::LightLayers2D>(layers);
    names = pixie::LightLayerNames(world);
    CHECK(names[0] == "Default" && names[1] == "Background" && names[2] == "Light layer 3");
    config.Get<pixie::LightLayers2D>().layers.push_back({{2, "Again"}, {}});
    CHECK(pixie::LightLayerNames(world)[1] == "Light layer 2");
}

// LIT-013: ambient occlusion darkens the ambient light just outside occluder edges, fading out over
// its radius, and nowhere else: not inside the occluder, not far from it, not when off.
TEST(AmbientOcclusionDarkensNearEdges) {
    for (const bool on : {true, false}) {
        Lab lab;
        pixie::LightingSettings2D settings;
        settings.ambientOcclusion = on;
        settings.occlusionRadius = 10.0;
        settings.occlusionStrength = 0.5;
        lab.Settings(settings);
        Box(lab, pxPoint(0.0, 0.0), pxVec2(10.0, 10.0));
        const auto pixels = lab.Render();
        const double near = 1.0 - 0.5 * (1.0 - (0.5 * 0.5 / 100.0) * (3.0 - 2.0 * 0.5 / 10.0));
        CHECK(on ? Near(At(pixels, 10.5, 0.0), Expected(near), 2) : At(pixels, 10.5, 0.0) == 255);
        CHECK(At(pixels, 40.0, 0.0) == 255);
        CHECK(At(pixels, 0.0, 0.0) == 255);
    }
}

// LIT-013: fog blends toward its color, more toward the bottom of the world (Y down).
TEST(FogGrowsTowardTheBottom) {
    Lab lab;
    pixie::LightingSettings2D settings;
    settings.fog = true;
    settings.fogColor = pxColorRgba(0, 0, 0);
    settings.fogDensity = 1.0;
    settings.fogTop = -40.0;
    settings.fogBottom = 40.0;
    lab.Settings(settings);
    const auto pixels = lab.Render();
    CHECK(At(pixels, 0.0, -60.0) == 255);
    CHECK(At(pixels, 0.0, 60.0) == 0);
    CHECK(Near(At(pixels, 0.0, 0.5), Expected(1.0 - 0.5 * (1.0 + 0.0)), 6));
}

// LIT-013: bloom spreads light above its threshold around an emissive surface; off, nothing spreads.
TEST(BloomSpreadsBrightLight) {
    for (const bool on : {true, false}) {
        Lab lab;
        pixie::LightingSettings2D settings = Dark();
        settings.bloom = on;
        settings.bloomThreshold = 1.0;
        settings.bloomIntensity = 1.0;
        lab.Settings(settings);
        lab.Quad(pxPoint(0.0, 0.0), 3.0, pxColorRgba(255, 255, 255)).Add<pixie::Emission2D>(pixie::Emission2D{pxColorRgba(255, 255, 255), 8.0});
        const auto pixels = lab.Render();
        CHECK(At(pixels, 0.0, 0.0) == 255);
        CHECK(on ? At(pixels, 6.0, 0.0) > 20 : At(pixels, 6.0, 0.0) == 0);
        CHECK(At(pixels, 40.0, 0.0) == 0);
    }
}

// LIT-013: a normal map shades its sprite by relief: a surface facing right is dark under a light on
// its left and lit by one on its right, by the angle the light arrives from at its height; without
// normalMaps on, the sprite keeps its flat light.
TEST(NormalMapsShadeByRelief) {
    const std::filesystem::path folder = std::filesystem::temp_directory_path() / "pixie-lighting-normals";
    std::filesystem::create_directories(folder / "scenes");
    std::ofstream(folder / "normals.pixie") << R"({"format": "pixie-project", "version": 1, "name": "normals", "startScene": "scenes/main.scene"})";
    std::ofstream(folder / "scenes" / "main.scene") << R"({"format": "pixie-scene", "version": 1, "seed": 1, "entities": []})";
    const std::vector<std::uint8_t> white(8 * 8 * 4, 255);
    std::vector<std::uint8_t> right(8 * 8 * 4, 255);
    for (std::size_t i = 0; i < right.size(); i += 4) {
        right[i + 1] = 128;
        right[i + 2] = 128;
    }
    CHECK(pixie::WriteFileBytes(folder / "albedo.png", pixie::EncodePng(8, 8, white)));
    CHECK(pixie::WriteFileBytes(folder / "normal.png", pixie::EncodePng(8, 8, right)));
    const auto shade = [&](double lightX, bool normalMaps) {
        Lab lab;
        CHECK(lab.session->OpenProject(folder));
        lab.session->SetLightingDebug(0);
        pixie::LightingSettings2D settings = Dark();
        settings.normalMaps = normalMaps;
        settings.normalHeight = 48.0;
        lab.Settings(settings);
        lab.Add(pxPoint(lightX, 0.0)).Add<pixie::Light2D>(FlatLight(200.0, 0.0));
        pixie::Entity sprite = lab.Add(pxPoint(0.0, 0.0));
        sprite.Get<pixie::Transform2D>().scale = pxVec2(4.0, 4.0);
        lab.World().ResetInterpolation(sprite);
        pixie::Sprite look;
        look.image = "albedo.png";
        sprite.Add<pixie::Sprite>(look);
        sprite.Add<pixie::NormalMap2D>(pixie::NormalMap2D{"normal.png", 1.0});
        return At(lab.Render(), 0.5, 0.5);
    };
    const double length = std::hypot(30.0, 48.0);
    CHECK(shade(-30.0, true) < 3);
    CHECK(Near(shade(30.0, true), Expected((30.0 / length) / (48.0 / length) * 1.0), 3));
    CHECK(shade(-30.0, false) == 255);
}

// LIT-017: a lit frame records CPU time, GPU time of light accumulation, shadows and combine (from
// timestamp queries, a few frames late), and light, shadow and occluder counts.
TEST(LightingRecordsTelemetry) {
    Lab lab;
    lab.Settings(Dark());
    lab.Add(pxPoint(-40.0, 0.0)).Add<pixie::Light2D>(FlatLight(120.0, 4.0));
    pixie::Light2D plain = FlatLight(60.0, 0.0);
    plain.castsShadows = false;
    lab.Add(pxPoint(40.0, 0.0)).Add<pixie::Light2D>(plain);
    Box(lab, pxPoint(0.0, 0.0), pxVec2(6.0, 12.0));
    pixie::Telemetry& telemetry = lab.World().GetTelemetry();
    telemetry.SetEnabled(true);
    for (int i = 0; i < 8; ++i) {
        lab.Render();
    }
    std::map<std::string, pixie::MetricSnapshot> metrics;
    for (const auto& metric : telemetry.Snapshot()) metrics[metric.name] = metric;
    CHECK(metrics.count("lighting.cpu") == 1 && metrics["lighting.cpu"].timing.samples == 8);
    CHECK(metrics["lighting.gpu.shadows"].timing.samples >= 3 && metrics["lighting.gpu.combine"].timing.samples >= 3);
    CHECK(metrics["lighting.gpu.combine"].timing.lastMs > 0.0);
    CHECK(metrics["lighting.lights"].gauge == 2.0 && metrics["lighting.shadowed_lights"].gauge == 1.0);
    CHECK(metrics["lighting.occluder_edges"].gauge == 4.0 && metrics["lighting.shadowed_lights_dropped"].gauge == 0.0);
    std::fprintf(stderr, "  lighting cpu %.3f ms, gpu lights %.3f, shadows %.3f, combine %.3f ms\n",
                 metrics["lighting.cpu"].timing.lastMs, metrics["lighting.gpu.lights"].timing.lastMs,
                 metrics["lighting.gpu.shadows"].timing.lastMs, metrics["lighting.gpu.combine"].timing.lastMs);
}

// LIT-016: debug views draw light bounds and occluder edges over the frame, unlit; off, nothing.
TEST(LightingDebugDrawsBoundsAndOccluders) {
    for (const std::uint32_t flags : {0u, pixie::kLightingDebugLights2D | pixie::kLightingDebugOccluders2D}) {
        Lab lab;
        lab.Settings(Dark());
        pixie::Light2D light = FlatLight(40.0, 2.0);
        light.intensity = 0.0;
        lab.Add(pxPoint(0.0, 0.0)).Add<pixie::Light2D>(light);
        Box(lab, pxPoint(-50.0, -50.0), pxVec2(6.0, 6.0));
        lab.session->SetLightingDebug(flags);
        const auto pixels = lab.Render();
        // The ring at the range, and the box's right edge at x = -44.
        const bool ring = At(pixels, 40.0, 0.0) > 100 || At(pixels, 39.5, 0.0) > 100;
        const bool edge = At(pixels, -44.0, -50.0, 2) > 100 || At(pixels, -44.5, -50.0, 2) > 100;
        CHECK(ring == (flags != 0) && edge == (flags != 0));
    }
}

// LIT-016: the light buffer view shows the light alone, as a white surface would receive it, and the
// overdraw view how many lights shade each pixel: two flat lights at a quarter, overlapping in the
// middle, one shadowed and one not, over a colored background.
TEST(LightingDebugViewsReplaceTheFrame) {
    Lab lab(pxColorRgba(200, 60, 30));
    lab.Settings(Dark());
    pixie::Light2D left = FlatLight(30.0, 0.0);
    left.intensity = 0.25;
    pixie::Light2D right = left;
    right.castsShadows = false;
    lab.Add(pxPoint(-15.0, 0.0)).Add<pixie::Light2D>(left);
    lab.Add(pxPoint(15.0, 0.0)).Add<pixie::Light2D>(right);
    lab.session->SetLightingDebug(pixie::kLightingDebugLightBuffer2D);
    const auto light = lab.Render();
    for (int channel = 0; channel < 3; ++channel) {
        CHECK(Near(At(light, 0.5, 0.5, channel), Expected(0.5)));
        CHECK(Near(At(light, -40.5, 0.5, channel), Expected(0.25)));
        CHECK(At(light, -60.5, 0.5, channel) == 0);
    }
    lab.session->SetLightingDebug(pixie::kLightingDebugOverdraw2D | pixie::kLightingDebugLightBuffer2D);
    const auto overdraw = lab.Render();
    CHECK(At(overdraw, -40.5, 0.5, 0) == 0 && At(overdraw, -40.5, 0.5, 1) == 0 && At(overdraw, -40.5, 0.5, 2) == 255);
    CHECK(Near(At(overdraw, 0.5, 0.5, 1), 85) && Near(At(overdraw, 0.5, 0.5, 2), 170));
    CHECK(At(overdraw, -60.5, 0.5, 2) == 0);
    lab.session->SetLightingDebug(0);
    CHECK(Near(At(lab.Render(), -40.5, 0.5, 0), Expected(Linear(200) * 0.25)));
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
