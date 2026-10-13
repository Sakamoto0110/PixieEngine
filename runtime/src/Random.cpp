#include "pixie/runtime/Random.hpp"

#include <bit>
#include <cmath>
#include <numbers>

namespace pixie {

namespace {

std::uint64_t SplitMix64(std::uint64_t& x) {
    std::uint64_t z = (x += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

} // namespace

void Random::Reseed(std::uint64_t seed) {
    seed_ = seed;
    std::uint64_t x = seed;
    for (auto& word : state_) {
        word = SplitMix64(x);
    }
}

std::uint64_t Random::NextU64() {
    const std::uint64_t result = std::rotl(state_[1] * 5, 7) * 9;
    const std::uint64_t t = state_[1] << 17;
    state_[2] ^= state_[0];
    state_[3] ^= state_[1];
    state_[1] ^= state_[2];
    state_[0] ^= state_[3];
    state_[2] ^= t;
    state_[3] = std::rotl(state_[3], 45);
    return result;
}

double Random::NextDouble() {
    // The top 53 bits, as a fraction of 2^53.
    return static_cast<double>(NextU64() >> 11) * 0x1.0p-53;
}

int Random::Int(int min, int max) {
    if (max <= min) {
        return min;
    }
    const auto span = static_cast<std::uint64_t>(static_cast<std::int64_t>(max) - min) + 1;
    return static_cast<int>(min + static_cast<std::int64_t>(NextU64() % span));
}

pxVec2 Random::Direction() {
    const double angle = Range(0.0, 2.0 * std::numbers::pi);
    return pxVec2(std::cos(angle), std::sin(angle));
}

} // namespace pixie
