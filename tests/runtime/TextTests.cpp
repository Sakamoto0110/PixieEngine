#include "Check.hpp"

#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/Fonts.hpp"
#include "pixie/runtime/Picking.hpp"
#include "pixie/runtime/Scene.hpp"
#include "pixie/runtime/Sprite.hpp"
#include "pixie/runtime/SpriteDrawing.hpp"
#include "pixie/runtime/SpriteImages.hpp"
#include "pixie/runtime/Text.hpp"
#include "pixie/runtime/TextDrawing.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/World.hpp"

#include <cmath>
#include <filesystem>
#include <numbers>
#include <string>
#include <vector>

using namespace pixie;

namespace {

bool Near(double a, double b, double tolerance = 1e-9) {
    return std::abs(a - b) <= tolerance;
}

bool Near(pxPoint a, pxPoint b, double tolerance = 1e-9) {
    return Near(a.x, b.x, tolerance) && Near(a.y, b.y, tolerance);
}

ComponentTypes Types() {
    ComponentTypes types;
    DescribeRuntimeComponents(types);
    return types;
}

// Fonts resolved against the folder of the particles sample's copy of Inter.
Fonts SampleFonts() {
    Fonts fonts;
    fonts.SetFolder(std::filesystem::path(PIXIE_TEST_FONT).parent_path());
    return fonts;
}

Text Label(std::string text, double size = 20.0, int layer = 0) {
    Text label;
    label.text = std::move(text);
    label.font = std::filesystem::path(PIXIE_TEST_FONT).filename().string();
    label.size = size;
    label.layer = layer;
    return label;
}

} // namespace

TEST(TheTextComponentNamesAFontAndGoesThroughAScene) {
    const ComponentTypes types = Types();
    const auto& descriptor = types.All()[static_cast<std::size_t>(types.IndexOf("Text"))];
    CHECK(descriptor.fields[1].name == "font" && descriptor.fields[1].kind == FieldKind::Text &&
          descriptor.fields[1].metadata.asset == "ttf");

    World world;
    Entity e = SceneEntityWith<Transform2D>(world, "sign");
    Text sign = Label("Ol\xC3\xA1,\nmundo", 32.0, 2);
    sign.align = TextAlign::Center;
    sign.pivot = pxVec2(0.0, 1.0);
    e.Add<Text>(sign);
    const std::string scene = WriteScene(world, types);
    CHECK(scene.find("\"align\": \"Center\"") != std::string::npos);
    World read;
    const SceneReport report = ReadScene(read, types, scene);
    CHECK(report.ok && report.warnings.empty());
    bool found = false;
    read.Each<const Text>([&](Entity, const Text& t) {
        found = t.text == sign.text && t.font == sign.font && t.size == 32.0 && t.align == TextAlign::Center &&
                t.pivot == sign.pivot && t.layer == 2;
    });
    CHECK(found);
    CHECK(WriteScene(read, types) == scene);

    const std::vector<Entity> chosen{e};
    const EntitiesReport copy = ReadEntities(world, types, WriteEntities(world, types, chosen), SceneIds::New);
    CHECK(copy.ok && copy.entities.size() == 1 && copy.entities[0].Get<Text>().text == sign.text);
}

TEST(ATextsBlockIsItsLinesAndFollowsTheTransform) {
    Fonts fonts = SampleFonts();
    const Text one = Label("Hello", 20.0);
    const Text two = Label("Hello\nHello", 20.0);
    const pxVec2 block = TextBlock(one, fonts);
    const pxVec2 tall = TextBlock(two, fonts);
    CHECK(block.x > 20.0 && block.x < 100.0 && block.y > 15.0 && block.y < 40.0);
    // Two lines: the same width, twice the height. Double the size: double the block.
    CHECK(Near(tall.x, block.x, 1e-6) && Near(tall.y, 2.0 * block.y, 1e-6));
    CHECK(Near(TextBlock(Label("Hello", 40.0), fonts).x, 2.0 * block.x, 1e-6));

    // Pivoted at its center, at (100, 50), turned a quarter, mirrored, and scaled.
    Transform2D transform{pxPoint(100.0, 50.0)};
    const auto corners = TextCorners(one, transform, block);
    CHECK(Near(corners[0], pxPoint(100.0 - block.x / 2.0, 50.0 - block.y / 2.0)));
    CHECK(TextContains(one, transform, block, pxPoint(100.0 + block.x / 2.0 - 1.0, 50.0), 0.0));
    CHECK(!TextContains(one, transform, block, pxPoint(100.0 + block.x / 2.0 + 1.0, 50.0), 0.0));
    transform.rotation = std::numbers::pi / 2.0;
    transform.scale = pxVec2(-2.0, 1.0);
    // Turned a quarter, the block's width runs along Y, twice as long.
    CHECK(TextContains(one, transform, block, pxPoint(100.0, 50.0 + block.x - 1.0), 0.0));
    CHECK(!TextContains(one, transform, block, pxPoint(100.0 + block.y, 50.0), 0.0));
}

TEST(ATextWithoutAReadableFontHasAOneEmBlock) {
    Fonts fonts = SampleFonts();
    Text missing = Label("Hello", 30.0);
    missing.font = "no such font.ttf";
    CHECK(TextBlock(missing, fonts) == pxVec2(30.0, 30.0));
    CHECK(TextBlock(Label("", 30.0), fonts) == pxVec2(30.0, 30.0));
    CHECK(!fonts.Problem("no such font.ttf").empty());
}

TEST(TextsAreDrawnAndPickedAmongSpritesByLayer) {
    World world;
    SpriteImages images;
    Fonts fonts = SampleFonts();
    const auto sprite = [&](int layer) {
        Entity e = world.Create();
        e.Add<Transform2D>(Transform2D{pxPoint(0.0, 0.0)});
        Sprite s;
        s.region = pxRect(0.0, 0.0, 100.0, 100.0);
        s.layer = layer;
        e.Add<Sprite>(s);
        return e;
    };
    const auto text = [&](int layer) {
        Entity e = world.Create();
        e.Add<Transform2D>(Transform2D{pxPoint(0.0, 0.0)});
        e.Add<Text>(Label("Hello", 20.0, layer));
        return e;
    };
    const Entity under = sprite(0);
    const Entity label = text(0);
    const Entity over = sprite(1);
    const Entity behind = text(-1);

    std::vector<entt::entity> order;
    ForEachDrawnInOrder(world, [&](const Drawn& drawn) { order.push_back(drawn.id); });
    CHECK(order == (std::vector<entt::entity>{behind.Id(), under.Id(), label.Id(), over.Id()}));

    CHECK(PickAt(world, pxPoint(0.0, 0.0), 0.0, &images, &fonts) == over.Id());
    world.Wrap(over.Id()).Destroy();
    // The newer of two on the same layer is on top; without fonts, texts are not picked.
    CHECK(PickAt(world, pxPoint(0.0, 0.0), 0.0, &images, &fonts) == label.Id());
    CHECK(PickAt(world, pxPoint(0.0, 0.0), 0.0, &images) == under.Id());
}
