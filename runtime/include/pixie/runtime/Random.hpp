#pragma once

#include <pixie/math/pxVec.hpp>

#include <array>
#include <cstdint>

namespace pixie {

// A seeded random number generator (xoshiro256**, seeded through splitmix64). Its sequences depend
// only on the seed, on every compiler and platform, unlike the distributions of <random>.
class Random {
public:
    explicit Random(std::uint64_t seed = 0) { Reseed(seed); }

    void Reseed(std::uint64_t seed);
    std::uint64_t Seed() const { return seed_; }

    std::uint64_t NextU64();
    // In [0, 1).
    double NextDouble();
    // In [min, max).
    double Range(double min, double max) { return min + (max - min) * NextDouble(); }
    // In [min, max], both included.
    int Int(int min, int max);
    // True with probability p.
    bool Chance(double p) { return NextDouble() < p; }
    // A vector of length 1 in a uniformly random direction.
    pxVec2 Direction();

private:
    std::uint64_t seed_ = 0;
    std::array<std::uint64_t, 4> state_{};
};

} // namespace pixie
