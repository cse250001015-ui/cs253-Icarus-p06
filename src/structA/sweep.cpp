
#include "../sweep.hpp"

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <random>
#include <stdexcept>
#include <vector>

namespace perc {

// SplitMix64 hashing function.
// Used to generate reproducible seeds for each sweep.
uint64_t splitmix64(uint64_t x) {

    uint64_t z = x + 0x9E3779B97F4A7C15ULL;

    z = (z ^ (z >> 30))
        * 0xBF58476D1CE4E5B9ULL;

    z = (z ^ (z >> 27))
        * 0x94D049BB133111EBULL;

    return z ^ (z >> 31);
}

// Generate a deterministic seed for a particular
// grid size and sweep number.
uint64_t sweepSeed(uint64_t baseSeed,
                   int L,
                   int sweepIndex) {

    return splitmix64(
        baseSeed
        + static_cast<uint64_t>(L) * 1000003ULL
        + static_cast<uint64_t>(sweepIndex)
    );
}

// Generate a random permutation of all grid sites.
//
// For the same L and seed, this produces the same
// opening order on all four Union-Find variants.
//
// We use our own Fisher-Yates shuffle rather than
// std::shuffle to follow the interface contract.
std::vector<int> makeOpeningOrder(int L,
                                  uint64_t seed) {

    if (L <= 0 || L > 1000) {
        throw std::invalid_argument(
            "Grid size must be between 1 and 1000"
        );
    }

    int total = L * L;

    std::vector<int> order(total);

    // Initially order = [0, 1, 2, ..., L*L-1]
    std::iota(order.begin(), order.end(), 0);

    // 64-bit Mersenne Twister with fixed seed
    std::mt19937_64 rng(seed);

    // Fisher-Yates shuffle, as specified in INTERFACE.md
    for (int i = total - 1; i > 0; --i) {

        // Pick an index from 0 through i
        int j = static_cast<int>(
            rng() % static_cast<uint64_t>(i + 1)
        );

        // Exchange positions i and j
        std::swap(order[i], order[j]);
    }

    return order;
}

// Run a sweep using an explicitly supplied opening order.
//
// The order may contain fewer than L*L sites.
// Every index must be valid and appear at most once.
//
// For each opened site:
// 1. Update the Percolation object.
// 2. Record whether percolation has occurred.
// 3. Record the largest cluster (when enabled).
// 4. Send the observation to the callback.
//
// Optionally stop immediately after first percolation.
SweepResult runSweepOrder(
    int L,
    Variant v,
    const std::vector<int>& order,
    const SweepCallback& cb,
    bool trackClusters,
    bool stopAtPercolation
) {

    if (L <= 0 || L > 1000) {
        throw std::invalid_argument("Invalid grid size");
    }

    const int total = L * L;

    // Check the supplied order before running the sweep.
    // This prevents invalid indices and duplicate openings.
    std::vector<unsigned char> seen(total, 0);

    for (int idx : order) {

        if (idx < 0 || idx >= total) {
            throw std::out_of_range(
                "Opening order contains invalid site index"
            );
        }

        if (seen[idx]) {
            throw std::invalid_argument(
                "Opening order contains duplicate site index"
            );
        }

        seen[idx] = 1;
    }

    // Create an initially blocked percolation grid.
    Percolation grid(L, v, trackClusters);

    SweepResult result{};

    // -1 indicates that percolation never occurred.
    result.nCritical = -1;

    // Open sites in the supplied order.
    for (int idx : order) {

        // Convert the one-dimensional site index
        // to its row and column coordinates.
        int row = idx / L;
        int col = idx % L;

        // Open the current site.
        grid.open(row, col);

        // Read the updated percolation state.
        bool spanning = grid.percolates();

        // Prepare the observation for this opening.
        SweepRecord record{};

        record.n = grid.numOpen();
        record.percolated = spanning;

        // Cluster tracking is optional.
        record.largestCluster = trackClusters
            ? grid.largestCluster()
            : 0;

        // Record the first occupation count at which
        // the grid percolates.
        if (spanning && result.nCritical == -1) {
            result.nCritical = record.n;
        }

        // Report the observation to the application.
        // The application/Analyst can use this to record
        // occupation-level data and cluster statistics.
        if (cb) {
            cb(record);
        }

        // For threshold-only runs, stop at the first
        // percolating opening if requested.
        if (spanning && stopAtPercolation) {
            break;
        }
    }

    // Save final operation counters from all three DSUs.
    result.perc = grid.countersPerc();
    result.full = grid.countersFull();

    // The cluster DSU is optional.
    if (trackClusters) {
        result.clust = grid.countersClust();
    } else {
        result.clust = Counters{};
    }

    return result;
}

// Convenience function.
//
// Generates a deterministic random opening order
// and then runs the Newman-Ziff sweep.
//
// By default, all sites are opened and observations
// are generated at every occupation count.
SweepResult runSweep(
    int L,
    Variant v,
    uint64_t seed,
    const SweepCallback& cb,
    bool trackClusters,
    bool stopAtPercolation
) {

    // Generate the random permutation.
    std::vector<int> order = makeOpeningOrder(L, seed);

    // Run the sweep with that permutation.
    return runSweepOrder(
        L,
        v,
        order,
        cb,
        trackClusters,
        stopAtPercolation
    );
}

} // namespace perc
