// src/rng.cpp -- owner: Architect. Exactly as specified in INTERFACE.md 5.1.
#include "sweep.hpp"
#include <random>
#include <utility>

namespace perc {

uint64_t splitmix64(uint64_t x) {
    uint64_t z = x + 0x9E3779B97F4A7C15ULL;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

uint64_t sweepSeed(uint64_t baseSeed, int L, int sweepIndex) {
    // Mix at every step so that neighbouring base seeds do not share sweeps (INTERFACE.md 5.1).
    return splitmix64(splitmix64(splitmix64(baseSeed) ^ static_cast<uint64_t>(L)) ^ static_cast<uint64_t>(sweepIndex));
}

std::vector<int> makeOpeningOrder(int L, uint64_t seed) {
    const int N = L * L;
    std::vector<int> a(N);
    for (int i = 0; i < N; ++i) a[i] = i;
    std::mt19937_64 rng(seed);
    for (int i = N - 1; i >= 1; --i) {
        int j = static_cast<int>(rng() % static_cast<uint64_t>(i + 1));
        std::swap(a[i], a[j]);
    }
    return a;
}

}  // namespace perc
