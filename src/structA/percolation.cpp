
#include "percolation.hpp"

#include <algorithm>
#include <memory>
#include <stdexcept>

using namespace std;

namespace perc {

// Constructor: create an L x L blocked grid
Percolation::Percolation(int L, Variant v, bool trackClusters)
    : n_(L),
      virtualTop_(L * L),
      virtualBottom_(L * L + 1),
      openCount_(0),
      maxClusterSize_(0),
      opened_(L * L, false),
      ufPerc_(L * L + 2, v),
      ufFull_(L * L + 1, v) {

    // Check grid size
    if (L <= 0 || L > 1000)
        throw invalid_argument("Invalid grid size");

    // Create cluster DSU only when requested
    if (trackClusters) {
        ufClust_ = make_unique<UnionFind>(L * L, v);
    }
}

// Open the site at (r, c)
bool Percolation::open(int r, int c) {

    // Check whether coordinates are valid
    if (r < 0 || r >= n_ || c < 0 || c >= n_)
        throw out_of_range("Invalid row or column");

    int id = r * n_ + c;

    // Return false if already open
    if (opened_[id])
        return false;

    // Mark site as open
    opened_[id] = true;
    openCount_++;

    // Connect top-row sites to virtual top
    if (r == 0) {
        ufPerc_.unite(id, virtualTop_);
        ufFull_.unite(id, virtualTop_);
    }

    // Connect bottom-row sites to virtual bottom
    if (r == n_ - 1) {
        ufPerc_.unite(id, virtualBottom_);
    }

    // Four neighboring positions
    const int dr[4] = {-1, 1, 0, 0};
    const int dc[4] = {0, 0, -1, 1};

    for (int i = 0; i < 4; i++) {

        int nr = r + dr[i];
        int nc = c + dc[i];

        // Skip neighbors outside the grid
        if (nr < 0 || nr >= n_ ||
            nc < 0 || nc >= n_)
            continue;

        int neighborId = nr * n_ + nc;

        // Skip blocked neighbors
        if (!opened_[neighborId])
            continue;

        // Connect in the percolation DSU
        ufPerc_.unite(id, neighborId);

        // Connect in the fullness DSU
        ufFull_.unite(id, neighborId);

        // Connect in cluster DSU if enabled
        if (ufClust_) {
            ufClust_->unite(id, neighborId);
        }
    }

    // Update largest open-site cluster
    if (ufClust_) {
        maxClusterSize_ = max(
            maxClusterSize_,
            ufClust_->size(id)
        );
    }

    return true;
}

// Check whether a site is open
bool Percolation::isOpen(int r, int c) const {

    if (r < 0 || r >= n_ || c < 0 || c >= n_)
        return false;

    return opened_[r * n_ + c];
}

// Check whether an open site connects to top
bool Percolation::isFull(int r, int c) {

    // A blocked site cannot be full
    if (!isOpen(r, c))
        return false;

    int id = r * n_ + c;

    // Fullness DSU excludes virtual bottom,
    // preventing the backwash problem
    return ufFull_.connected(id, virtualTop_);
}

// Check whether the grid percolates
bool Percolation::percolates() {

    return ufPerc_.connected(
        virtualTop_,
        virtualBottom_
    );
}

// Return the number of open sites
int Percolation::numOpen() const {
    return openCount_;
}

// Return largest open-site cluster
int Percolation::largestCluster() const {

    if (!ufClust_)
        throw logic_error("Cluster tracking is disabled");

    return maxClusterSize_;
}

// Return grid dimension
int Percolation::L() const {
    return n_;
}

// Return combined counters
Counters Percolation::counters() const {

    Counters result = ufPerc_.counters();

    result += ufFull_.counters();

    if (ufClust_) {
        result += ufClust_->counters();
    }

    return result;
}

// Counters from percolation DSU
const Counters& Percolation::countersPerc() const {
    return ufPerc_.counters();
}

// Counters from fullness DSU
const Counters& Percolation::countersFull() const {
    return ufFull_.counters();
}

// Counters from cluster DSU
const Counters& Percolation::countersClust() const {

    if (!ufClust_)
        throw logic_error("Cluster tracking is disabled");

    return ufClust_->counters();
}

// Reset operation counters
void Percolation::resetCounters() {

    ufPerc_.resetCounters();
    ufFull_.resetCounters();

    if (ufClust_) {
        ufClust_->resetCounters();
    }
}

// Read-only access to percolation DSU
const UnionFind& Percolation::ufPerc() const {
    return ufPerc_;
}

// Read-only access to fullness DSU
const UnionFind& Percolation::ufFull() const {
    return ufFull_;
}

// Read-only access to cluster DSU
const UnionFind* Percolation::ufClust() const {
    return ufClust_.get();
}

} // namespace perc

