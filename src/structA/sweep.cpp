#include "../../include/sweep.hpp"
#include <stdexcept>

namespace perc {


SweepResult runSweepOrder(
    int L,
    Variant v,
    const std::vector<int>& order,
    const SweepCallback& cb,
    bool trackClusters,
    bool stopAtPercolation
) {
    
    Percolation grid(L, v, trackClusters);

int total = L * L;

    // Check whether every site index is valid and unique
    std::vector<unsigned char> visited(total, 0);

    for (int idx : order) {

        // Every site must be between 0 and L*L - 1
        if (idx < 0 || idx >= total)
            throw std::out_of_range("Invalid site index");

        // A site cannot appear twice in the opening order
        if (visited[idx])
            throw std::invalid_argument("Duplicate site");

        visited[idx] = 1;
    }

    // Store the results of this sweep
    SweepResult result{};

    // -1 means percolation has not occurred
    result.nCritical = -1;

    // Open sites in the specified order
    for (int idx : order) {

        // Convert 1D site index into row and column
        int row = idx / L;
        int col = idx % L;

        // Open the selected site
        grid.open(row, col);

        // Check whether the grid percolates
        bool spanning = grid.percolates();

        // Record observations after the opening
        SweepRecord record{};

        record.n = grid.numOpen();
        record.percolated = spanning;

        // Store largest cluster if tracking is enabled
        record.largestCluster = trackClusters
            ? grid.largestCluster()
            : 0;

        // Record the first occupation count at which
        // top-to-bottom connectivity is established
        if (spanning && result.nCritical == -1) {
            result.nCritical = record.n;
        }

        // Send the observation to the application
        if (cb) {
            cb(record);
        }

        // Stop only if explicitly requested
        if (spanning && stopAtPercolation) {
            break;
        }
    }

    // Save final Union-Find operation counters
    result.perc = grid.countersPerc();
    result.full = grid.countersFull();

    if (trackClusters) {
        result.clust = grid.countersClust();
    } else {
        result.clust = Counters{};
    }

    return result;
}


SweepResult runSweep(int L, Variant v, uint64_t seed, const SweepCallback& cb,
                     bool trackClusters, bool stopAtPercolation) {
    return runSweepOrder(L, v, makeOpeningOrder(L, seed), cb, trackClusters, stopAtPercolation);
}

}  // namespace perc
