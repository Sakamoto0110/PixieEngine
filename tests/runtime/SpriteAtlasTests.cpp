#include "Check.hpp"

#include "pixie/runtime/SpriteAtlas.hpp"

#include <nlohmann/json.hpp>

#include <format>
#include <utility>

using namespace pixie;

namespace {

using Json = nlohmann::json;
constexpr auto frameId = "1b681986317d465c9f01b3f303ff89fa";
constexpr auto otherId = "a9f4e02f43904c2ea9baa8af9f3c848e";

Json Descriptor() {
    return {
        {"format", "pixie-sprite-atlas"}, {"version", 1},
        {"image", "assets/characters/Sheet.PNG"}, {"imageSize", {128, 64}},
        {"frames", Json::array({
            {{"id", frameId}, {"name", "walk"}, {"rect", {16, 24, 32, 40}}, {"pivot", {0.5, 1}}},
            {{"id", otherId}, {"name", "walk"}, {"rect", {48, 0, 32, 32}}, {"pivot", {0, 0.25}}}
        })}
    };
}

} // namespace

TEST(SpriteAtlasFrameIdentitySurvivesRepackAndRename) {
    const SpriteAtlasFrameRef ref{"assets/characters/actors.atlas.json", frameId};
    CHECK(ref.IsValid());
    auto document = Descriptor();
    std::string problem = "stale diagnostic";
    const auto atlas = SpriteAtlas::Parse(document.dump(), problem);
    CHECK(atlas.has_value());
    CHECK(problem.empty());
    if (!atlas) return;

    CHECK(atlas->Image() == "assets/characters/Sheet.PNG");
    CHECK(atlas->ImageSize().width == 128 && atlas->ImageSize().height == 64);
    CHECK(atlas->Frames().size() == 2);
    const auto* original = atlas->FindFrame(ref.frameId);
    CHECK(original && original->name == "walk" && original->region.x == 16 && original->pivot.y == 1.0);

    document["frames"][0]["name"] = "run";
    document["frames"][0]["rect"] = {0, 0, 48, 32};
    std::swap(document["frames"][0], document["frames"][1]);
    const auto repacked = SpriteAtlas::Parse(document.dump(), problem);
    CHECK(repacked.has_value());
    const auto* updated = repacked ? repacked->FindFrame(ref.frameId) : nullptr;
    CHECK(updated && updated->name == "run" && updated->region.x == 0 && updated->region.width == 48);

    document["frames"].erase(1);
    const auto removed = SpriteAtlas::Parse(document.dump(), problem);
    CHECK(removed && !removed->FindFrame(ref.frameId) && removed->FindFrame(otherId));
    document["frames"] = Json::array();
    const auto empty = SpriteAtlas::Parse(document.dump(), problem);
    CHECK(empty && empty->Frames().empty());
    CHECK(ref.frameId == frameId);
}

TEST(SpriteAtlasRejectsInvalidDescriptorsAndFrameReferences) {
    auto document = Descriptor();
    std::string problem;
    const auto reject = [&](std::string_view diagnostic) {
        CHECK(!SpriteAtlas::Parse(document.dump(), problem));
        CHECK(problem.find(diagnostic) != std::string::npos);
        document = Descriptor();
    };

    document["version"] = 2;
    reject("version");
    document["image"] = "assets/../outside.png";
    reject("project-relative PNG");
    document["image"] = "assets/NUL.png";
    reject("project-relative PNG");
    document["imageSize"][0] = 18446744073709551615ull;
    reject("imageSize");
    document["frames"][1]["id"] = frameId;
    reject("frames[1]: duplicate frame id");
    document["frames"][0]["rect"][0] = 120;
    reject("frames[0]: rect");
    document["frames"][0]["rect"][2] = 1.25;
    reject("frames[0]: rect");
    document["frames"][0]["pivot"][1] = 1.5;
    reject("frames[0]: pivot");
    document["frames"][0] = 0;
    reject("frames[0]: id");
    CHECK(!SpriteAtlas::Parse(R"({"version": 1, "version": 2})", problem));
    CHECK(problem.find("unique members") != std::string::npos);
    CHECK(!SpriteAtlas::Parse(R"({"version": )", problem));

    CHECK((!SpriteAtlasFrameRef{"assets/../a.atlas.json", frameId}.IsValid()));
    CHECK((!SpriteAtlasFrameRef{"C:/assets/a.atlas.json", frameId}.IsValid()));
    CHECK((!SpriteAtlasFrameRef{"assets/.atlas.json", frameId}.IsValid()));
    CHECK((!SpriteAtlasFrameRef{"assets/Lpt1/a.atlas.json", frameId}.IsValid()));
    CHECK((!SpriteAtlasFrameRef{"assets/a.png", frameId}.IsValid()));
    CHECK((!SpriteAtlasFrameRef{"assets/a.atlas.json", "00000000000000000000000000000000"}.IsValid()));
    CHECK((!SpriteAtlasFrameRef{"assets/a.atlas.json", "1B681986317D465C9F01B3F303FF89FA"}.IsValid()));
}

TEST(SpriteAtlasLooksUpFramesByIdInALargeDescriptor) {
    auto document = Descriptor();
    const auto prototype = document["frames"][0];
    auto& frames = document["frames"];
    frames = Json::array();
    for (int i = 1; i <= 512; ++i) {
        auto frame = prototype;
        frame["id"] = std::format("{:032x}", i);
        frame["name"] = std::format("Frame {}", i);
        frames.push_back(std::move(frame));
    }
    std::string problem;
    const auto atlas = SpriteAtlas::Parse(document.dump(), problem);
    CHECK(atlas && atlas->Frames().size() == 512);
    const auto* last = atlas ? atlas->FindFrame("00000000000000000000000000000200") : nullptr;
    CHECK(last && last->name == "Frame 512");
}
