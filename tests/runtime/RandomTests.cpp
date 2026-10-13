#include "Check.hpp"

#include "pixie/runtime/Random.hpp"
#include "pixie/runtime/World.hpp"

#include <cmath>
#include <vector>

using pixie::Random;

namespace {

std::vector<std::uint64_t> Draw(Random& random, int count) {
    std::vector<std::uint64_t> values;
    for (int i = 0; i < count; ++i) {
        values.push_back(random.NextU64());
    }
    return values;
}

} // namespace

TEST(SameSeedGivesTheSameSequence) {
    Random a(42);
    Random b(42);
    CHECK(Draw(a, 1000) == Draw(b, 1000));
    pixie::World w1(7);
    pixie::World w2(7);
    CHECK(Draw(w1.GetRandom(), 100) == Draw(w2.GetRandom(), 100));
}

TEST(DifferentSeedsGiveDifferentSequences) {
    Random a(1);
    Random b(2);
    CHECK(Draw(a, 16) != Draw(b, 16));
}

TEST(ReseedRestartsTheSequence) {
    Random random(9);
    const auto first = Draw(random, 32);
    random.Reseed(9);
    CHECK(Draw(random, 32) == first);
    CHECK(random.Seed() == 9);
}

TEST(KnownFirstValueForSeedZero) {
    // Pinned so that a change to the generator, which would change every seeded World, is seen.
    Random random(0);
    CHECK(random.NextU64() == 0x99EC5F36CB75F2B4ull);
}

TEST(RangesStayInBounds) {
    Random random(3);
    for (int i = 0; i < 10000; ++i) {
        const double d = random.NextDouble();
        CHECK(d >= 0.0 && d < 1.0);
        const int n = random.Int(-3, 5);
        CHECK(n >= -3 && n <= 5);
        const pxVec2 v = random.Direction();
        CHECK(std::abs(glm::length(v) - 1.0) < 1e-12);
    }
}
