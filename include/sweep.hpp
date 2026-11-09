// include/sweep.hpp -- owner: Architect (declaration). Contract: INTERFACE.md section 5.
// splitmix64, sweepSeed, makeOpeningOrder: src/rng.cpp (Architect).
// runSweepOrder, runSweep: src/structA/sweep.cpp (Structure A owner).
#pragma once
#include "percolation.hpp"
#include <cstdint>
#include <functional>
#include <vector>

namespace perc {

struct SweepRecord {          // state after the n-th site has been opened
    int  n;                   // 1, 2, 3, ...
    bool percolated;
    int  largestCluster;      // 0 if clusters are not tracked
};

using SweepCallback = std::function<void(const SweepRecord&)>;

struct SweepResult {
    int      nCritical = -1;  // smallest n with percolated == true; -1 if never
    Counters perc, full, clust;
};

uint64_t splitmix64(uint64_t x);
uint64_t sweepSeed(uint64_t baseSeed, int L, int sweepIndex);
std::vector<int> makeOpeningOrder(int L, uint64_t seed);   // permutation of 0 .. L*L-1

// Opens sites in the given order (distinct indices; need not cover the whole grid).
// cb may be empty (a no-op run): check if (cb) before every call.
SweepResult runSweepOrder(int L, Variant v, const std::vector<int>& order,
                          const SweepCallback& cb, bool trackClusters = true,
                          bool stopAtPercolation = false);

SweepResult runSweep(int L, Variant v, uint64_t seed,
                     const SweepCallback& cb, bool trackClusters = true,
                     bool stopAtPercolation = false);

}  // namespace perc
