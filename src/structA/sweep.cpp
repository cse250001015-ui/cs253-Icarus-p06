
#include <algorithm>
#include <numeric>
#include <random>
#include <vector>
#include <stdexcept>
#include <cstdint>

#include "percolation.hpp"

using namespace std;

// Stores the results of one complete Newman-Ziff sweep
struct SweepResult {

    // Fraction of open sites when percolation first occurs
    double threshold = 0.0;

    // Whether the grid percolates after k sites are open
    // Index k represents exactly k open sites
    vector<bool> percolatesAtCount;

    // Size of the largest open cluster after k openings
    vector<int> largestClusterAtCount;
};

// Generate a random permutation of all grid sites
// Each site appears exactly once in the opening order
vector<int> makeOpeningOrder(int n, mt19937& rng) {

    if (n <= 0 || n > 1000)
        throw invalid_argument("Invalid grid size");

    int total = n * n;

    vector<int> order(total);

    // Initially: 0, 1, 2, ..., N*N - 1
    iota(order.begin(), order.end(), 0);

    // Randomly rearrange site indices
    shuffle(order.begin(), order.end(), rng);

    return order;
}

// Perform one complete Newman-Ziff sweep
SweepResult runSweepOrder(int n, mt19937& rng) {

    // Generate random order of site openings
    vector<int> order = makeOpeningOrder(n, rng);

    // Create an initially blocked grid
    Percolation p(n);

    int total = n * n;

    SweepResult result;

    // Allocate memory for occupation counts 0 to N*N
    result.percolatesAtCount.reserve(total + 1);
    result.largestClusterAtCount.reserve(total + 1);

    // Initially no sites are open
    result.percolatesAtCount.push_back(false);
    result.largestClusterAtCount.push_back(0);

    // Track whether percolation has happened for the first time
    bool firstPercolationFound = false;

    // Open all sites one by one
    for (int k = 0; k < total; k++) {

        // Get the next site from the random order
        int site = order[k];

        // Convert 1D site index into row and column
        int row = site / n;
        int col = site % n;

        // Open the selected site
        // The Percolation class handles DSU unions
        p.open(row, col);

        // Check whether the top connects to the bottom
        bool spanning = p.percolates();

        // Record percolation status at occupation count k+1
        result.percolatesAtCount.push_back(spanning);

        // Record largest cluster after this opening
        result.largestClusterAtCount.push_back(
            p.largestCluster()
        );

        // Record threshold only when percolation first occurs
        if (spanning && !firstPercolationFound) {

            result.threshold =
                static_cast<double>(k + 1) / total;

            firstPercolationFound = true;
        }
    }

    return result;
}

// Repeat the sweep T times using the same RNG stream
// Each sweep generates a new random opening order
vector<double> runThresholdTrials(
    int n,
    int trials,
    uint32_t seed
) {

    if (trials <= 0)
        throw invalid_argument("Trials must be positive");

    mt19937 rng(seed);

    vector<double> thresholds;
    thresholds.reserve(trials);

    for (int t = 0; t < trials; t++) {

        SweepResult result = runSweepOrder(n, rng);

        thresholds.push_back(result.threshold);
    }

    return thresholds;
}
