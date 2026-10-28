#include "Check.hpp"

#include "pixie/core/Image.hpp"
#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/Picking.hpp"
#include "pixie/runtime/Scene.hpp"
#include "pixie/runtime/Shape.hpp"
#include "pixie/runtime/Sprite.hpp"
#include "pixie/runtime/SpriteDrawing.hpp"
#include "pixie/runtime/SpriteImages.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/World.hpp"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <numbers>
#include <string>
#include <vector>

using namespace pixie;

namespace {

// A fresh, empty folder under the system's temporary folder, removed when the test ends.
struct TemporaryFolder {
    std::filesystem::path path;

    explicit TemporaryFolder(const char* name) : path(std::filesystem::temp_directory_path() / name) {
        std::filesystem::remove_all(path);
        std::filesystem::create_directories(path);
    }
    ~TemporaryFolder() {
        std::error_code ignored;
        std::filesystem::remove_all(path, ignored);
    }
};

// A PNG of `width` x `height` pixels, all of `rgba`.
std::vector<std::uint8_t> FlatPng(int width, int height, std::array<std::uint8_t, 4> rgba) {
    std::vector<std::uint8_t> pixels;
    for (int i = 0; i < width * height; ++i) {
        pixels.insert(pixels.end(), rgba.begin(), rgba.end());
    }
    return EncodePng(width, height, pixels);
}

bool Near(double a, double b) {
    return std::abs(a - b) < 1e-9;
}

bool Near(pxPoint a, pxPoint b) {
    return Near(a.x, b.x) && Near(a.y, b.y);
}

ComponentTypes Types() {
    ComponentTypes types;
    DescribeRuntimeComponents(types);
    return types;
}

Entity SpriteAt(World& world, pxPoint at, Sprite sprite, double scale = 1.0) {
    Entity e = world.Create();
    Transform2D transform{at};
    transform.scale = pxVec2(scale, scale);
    e.Add<Transform2D>(transform);
    e.Add<Sprite>(std::move(sprite));
    return e;
}

// A sprite whose region gives its size, so no image is needed.
Sprite Sized(double width, double height, int layer = 0) {
    Sprite sprite;
    sprite.region = pxRect(0.0, 0.0, width, height);
    sprite.layer = layer;
    return sprite;
}

} // namespace

TEST(APngDecodesWithPremultipliedAlpha) {
    const std::vector<std::uint8_t> straight{200, 100, 50, 128, 10, 20, 30, 255};
    std::string error;
    const auto image = DecodePng(EncodePng(2, 1, straight), error);
    CHECK(image.has_value() && error.empty());
    CHECK(image->width == 2 && image->height == 1);
    const std::vector<std::uint8_t> expected{100, 50, 25, 128, 10, 20, 30, 255};
    CHECK(image->pixels == expected);
}

TEST(WhatIsNotAPngGivesItsReason) {
    std::string error;
    const std::vector<std::uint8_t> garbage{1, 2, 3, 4, 5};
    CHECK(!DecodePng(garbage, error) && error.starts_with("not a PNG"));
    error.clear();
    CHECK(!LoadPngFile(std::filesystem::temp_directory_path() / "pixie-no-such-image.png", error));
    CHECK(error == "the file does not exist");
}

TEST(TheSpriteImageIsATextFieldNamingAPng) {
    const ComponentTypes types = Types();
    const int index = types.IndexOf("Sprite");
    CHECK(index >= 0);
    const FieldDescriptor& image = types.All()[static_cast<std::size_t>(index)].fields[0];
    CHECK(image.name == "image" && image.kind == FieldKind::Text && image.metadata.asset == "png");
    CHECK(image.read == nullptr && image.write == nullptr);
    Sprite sprite;
    image.writeText(&sprite, "images/neko.png");
    CHECK(sprite.image == "images/neko.png" && image.readText(&sprite) == "images/neko.png");
}

TEST(ATextFieldGoesThroughASceneAndASnapshot) {
    const ComponentTypes types = Types();
    World world;
    Sprite sprite;
    // UTF-8, with a space and a folder.
    sprite.image = "images/sub dir/\xC3\xB1" "eko.png";
    sprite.region = pxRect(1.0, 2.0, 3.0, 4.0);
    sprite.layer = 3;
    sprite.filter = SpriteFilter::Pixel;
    Entity e = SceneEntityWith<Transform2D>(world, "neko");
    e.Add<Sprite>(sprite);

    const std::string text = WriteScene(world, types);
    CHECK(text.find("\"image\": \"images/sub dir/\xC3\xB1" "eko.png\"") != std::string::npos);
    CHECK(text.find("\"filter\": \"Pixel\"") != std::string::npos);
    World read;
    const SceneReport report = ReadScene(read, types, text);
    CHECK(report.ok && report.warnings.empty());
    bool found = false;
    read.Each<const Sprite>([&](Entity, const Sprite& s) {
        found = s.image == sprite.image && s.layer == 3 && s.filter == SpriteFilter::Pixel && s.region.width == 3.0;
    });
    CHECK(found);
    CHECK(WriteScene(read, types) == text);

    const std::vector<Entity> chosen{e};
    const EntitiesReport copy = ReadEntities(world, types, WriteEntities(world, types, chosen), SceneIds::New);
    CHECK(copy.ok && copy.entities.size() == 1 && copy.entities[0].Get<Sprite>().image == sprite.image);
}

TEST(ATextFieldThatIsNotTextIsLeftAtItsDefault) {
    const ComponentTypes types = Types();
    World world;
    SceneEntityWith<Sprite>(world, "neko");
    std::string text = WriteScene(world, types);
    const std::string from = "\"image\": \"\"";
    CHECK(text.find(from) != std::string::npos);
    text.replace(text.find(from), from.size(), "\"image\": 7");
    World read;
    const SceneReport report = ReadScene(read, types, text);
    CHECK(report.ok && report.warnings.size() == 1 && report.warnings[0].find("expected text") != std::string::npos);
}

TEST(SpriteImagesReadSizesAndProblemsWithoutGl) {
    TemporaryFolder folder("pixie-sprite-images");
    std::filesystem::create_directories(folder.path / "images");
    CHECK(WriteFileBytes(folder.path / "images" / "a.png", FlatPng(4, 2, {255, 0, 0, 255})));
    WriteFileBytes(folder.path / "broken.png", std::vector<std::uint8_t>{1, 2, 3});

    SpriteImages images;
    images.SetFolder(folder.path);
    CHECK(images.Resolve("images/a.png") == folder.path / "images" / "a.png");
    const auto size = images.Size("images/a.png");
    CHECK(size && size->x == 4.0 && size->y == 2.0 && images.Problem("images/a.png").empty());
    CHECK(!images.Size("images/missing.png") && images.Problem("images/missing.png") == "the file does not exist");
    CHECK(!images.Size("broken.png") && images.Problem("broken.png").starts_with("not a PNG"));
    CHECK(!images.Size("") && images.Problem("") == "no image is set");
    CHECK(images.CheckForChanges() == 0);
}

TEST(SpriteImagesReadAChangedFileAgain) {
    TemporaryFolder folder("pixie-sprite-reload");
    const auto file = folder.path / "a.png";
    CHECK(WriteFileBytes(file, FlatPng(4, 2, {255, 0, 0, 255})));
    SpriteImages images;
    images.SetFolder(folder.path);
    CHECK(images.Size("a.png")->x == 4.0);

    // Saved again, larger, by a painting program.
    CHECK(WriteFileBytes(file, FlatPng(8, 8, {0, 255, 0, 255})));
    std::filesystem::last_write_time(file, std::filesystem::last_write_time(file) + std::chrono::seconds(5));
    CHECK(images.CheckForChanges() == 1);
    CHECK(images.Size("a.png")->x == 8.0 && images.Size("a.png")->y == 8.0);
    CHECK(images.CheckForChanges() == 0);

    // Removed, then a missing file that appears.
    std::filesystem::remove(file);
    CHECK(images.CheckForChanges() == 1 && !images.Size("a.png"));
    CHECK(!images.Size("later.png"));
    CHECK(WriteFileBytes(folder.path / "later.png", FlatPng(2, 2, {0, 0, 255, 255})));
    CHECK(images.CheckForChanges() == 1 && images.Size("later.png")->x == 2.0);
}

TEST(ASpriteIsSizedByItsRegionItsImageOrTheMissingSize) {
    TemporaryFolder folder("pixie-sprite-size");
    CHECK(WriteFileBytes(folder.path / "a.png", FlatPng(30, 20, {255, 255, 255, 255})));
    SpriteImages images;
    images.SetFolder(folder.path);
    Sprite sprite;
    sprite.image = "a.png";
    CHECK(SpritePixelSize(sprite, images) == pxVec2(30.0, 20.0));
    sprite.region = pxRect(5.0, 5.0, 10.0, 8.0);
    CHECK(SpritePixelSize(sprite, images) == pxVec2(10.0, 8.0));
    sprite.region = pxRect(0.0, 0.0, 0.0, 0.0);
    sprite.image = "missing.png";
    CHECK(SpritePixelSize(sprite, images) == pxVec2(SpriteImages::kMissingSize, SpriteImages::kMissingSize));
}

TEST(ASpriteCoversARectangleAroundItsPivotTurnedWithTheEntity) {
    // 4 x 2 pixels, scaled 2, standing on its feet at (10, 20).
    Sprite sprite = Sized(4.0, 2.0);
    sprite.pivot = pxVec2(0.5, 1.0);
    Transform2D transform{pxPoint(10.0, 20.0)};
    transform.scale = pxVec2(2.0, 2.0);
    const pxVec2 size(4.0, 2.0);
    auto corners = SpriteCorners(sprite, transform, size);
    CHECK(Near(corners[0], pxPoint(6.0, 16.0)) && Near(corners[1], pxPoint(14.0, 16.0)));
    CHECK(Near(corners[2], pxPoint(14.0, 20.0)) && Near(corners[3], pxPoint(6.0, 20.0)));
    CHECK(SpriteContains(sprite, transform, size, pxPoint(13.0, 17.0), 0.0));
    CHECK(!SpriteContains(sprite, transform, size, pxPoint(13.0, 21.0), 0.0));
    CHECK(SpriteContains(sprite, transform, size, pxPoint(13.0, 21.0), 1.5));

    // A long sprite turned a quarter: it now runs along Y.
    Sprite bar = Sized(40.0, 10.0);
    Transform2D turned;
    turned.rotation = std::numbers::pi / 2.0;
    CHECK(SpriteContains(bar, turned, pxVec2(40.0, 10.0), pxPoint(0.0, 15.0), 0.0));
    CHECK(!SpriteContains(bar, turned, pxVec2(40.0, 10.0), pxPoint(15.0, 0.0), 0.0));
    corners = SpriteCorners(bar, turned, pxVec2(40.0, 10.0));
    CHECK(Near(corners[0], pxPoint(5.0, -20.0)));

    // A negative scale mirrors it, and it still covers its rectangle.
    Transform2D mirrored;
    mirrored.scale = pxVec2(-1.0, 1.0);
    CHECK(SpriteContains(bar, mirrored, pxVec2(40.0, 10.0), pxPoint(-15.0, 0.0), 0.0));
}

TEST(SpritesAreDrawnByLayerThenOldestFirst) {
    World world;
    const Entity top = SpriteAt(world, pxPoint(0.0, 0.0), Sized(10.0, 10.0, 1));
    const Entity first = SpriteAt(world, pxPoint(0.0, 0.0), Sized(10.0, 10.0));
    const Entity second = SpriteAt(world, pxPoint(0.0, 0.0), Sized(10.0, 10.0));
    std::vector<entt::entity> order;
    ForEachSpriteInDrawOrder(world, [&](entt::entity id, const Transform2D&, const Sprite&) { order.push_back(id); });
    CHECK(order == (std::vector<entt::entity>{first.Id(), second.Id(), top.Id()}));
}

TEST(PickAtFindsTheSpriteDrawnOnTopAndShapesBeforeSprites) {
    World world;
    SpriteImages images;
    const Entity high = SpriteAt(world, pxPoint(0.0, 0.0), Sized(20.0, 20.0, 1));
    const Entity low = SpriteAt(world, pxPoint(0.0, 0.0), Sized(100.0, 100.0));
    const Entity newer = SpriteAt(world, pxPoint(30.0, 0.0), Sized(20.0, 20.0));
    // Without the images, sprites are not picked.
    CHECK(PickAt(world, pxPoint(0.0, 0.0), 0.0) == entt::null);
    CHECK(PickAt(world, pxPoint(0.0, 0.0), 0.0, &images) == high.Id());
    CHECK(PickAt(world, pxPoint(-40.0, 0.0), 0.0, &images) == low.Id());
    // Same layer: the newer one is drawn over the older.
    CHECK(PickAt(world, pxPoint(30.0, 0.0), 0.0, &images) == newer.Id());
    CHECK(PickAt(world, pxPoint(0.0, 60.0), 0.0, &images) == entt::null);
    CHECK(PickAt(world, pxPoint(0.0, 60.0), 15.0, &images) == low.Id());

    Entity shape = world.Create();
    shape.Add<Transform2D>(Transform2D{pxPoint(0.0, 0.0)});
    shape.Add<Shape>(Shape{ShapeKind::Circle, pxVec2(5.0, 5.0)});
    CHECK(PickAt(world, pxPoint(0.0, 0.0), 0.0, &images) == shape.Id());

    const auto boxed = PickIn(world, pxRect(-1.0, -1.0, 2.0, 2.0));
    CHECK(boxed == (std::vector<entt::entity>{high.Id(), low.Id(), shape.Id()}));
}
