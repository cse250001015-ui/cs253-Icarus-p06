
#include "../percolation.hpp"

#include <algorithm>
#include <memory>
#include <stdexcept>

namespace perc {

// Check grid size before allocating memory
static int checkGridSize(int L) {
    if (L <= 0 || L > 1000)
        throw std::invalid_argument("Invalid grid size");

    return L;
}

// Constructor: initially all sites are blocked
Percolation::Percolation(int L, Variant v, bool trackClusters)
    : L_(checkGridSize(L)),
      top_(L_ * L_),
      bottom_(L_ * L_ + 1),
      openCount_(0),
      largest_(0),
      percolated_(false),
      opened_(L_ * L_, false),

      // DSU for top-to-bottom percolation
      ufPerc_(makeUnionFind(v, L_ * L_ + 2)),

      // DSU for fullness: no virtual bottom
      ufFull_(makeUnionFind(v, L_ * L_ + 1)),

      // DSU for actual open-site cluster sizes
      ufClust_(trackClusters
          ? makeUnionFind(v, L_ * L_)
          : nullptr) {
}

// Open site (r,c)
bool Percolation::open(int r, int c) {

    // Reject coordinates outside the grid
    if (r < 0 || r >= L_ || c < 0 || c >= L_)
        throw std::out_of_range("Invalid coordinates");

    int idx = r * L_ + c;

    // Opening an already open site changes nothing
    if (opened_[idx])
        return false;

    opened_[idx] = true;
    openCount_++;

    // Connect top-row sites to virtual TOP
    if (r == 0) {
        ufPerc_->unite(idx, top_);
        ufFull_->unite(idx, top_);
    }

    // Connect bottom-row sites to virtual BOTTOM
    // Only the percolation DSU has BOTTOM
    if (r == L_ - 1) {
        ufPerc_->unite(idx, bottom_);
    }

    // Fixed neighbor order: up, down, left, right
    const int dr[4] = {-1, 1, 0, 0};
    const int dc[4] = {0, 0, -1, 1};

    for (int i = 0; i < 4; i++) {

        int nr = r + dr[i];
        int nc = c + dc[i];

        // Skip neighbors outside the grid
        if (nr < 0 || nr >= L_ ||
            nc < 0 || nc >= L_)
            continue;

        int nb = nr * L_ + nc;

        // Connect only with open neighbors
        if (!opened_[nb])
            continue;

        // The first argument must be the new site
        ufPerc_->unite(idx, nb);
        ufFull_->unite(idx, nb);

        if (ufClust_) {
            ufClust_->unite(idx, nb);
        }
    }

    // Update largest cluster using actual sites only
    if (ufClust_) {
        largest_ = std::max(
            largest_,
            ufClust_->size(idx)
        );
    }

    // Check percolation only until it first occurs
    if (!percolated_) {
        percolated_ = ufPerc_->connected(top_, bottom_);
    }

    return true;
}

// Check whether a site is open
bool Percolation::isOpen(int r, int c) const {

    if (r < 0 || r >= L_ || c < 0 || c >= L_)
        throw std::out_of_range("Invalid coordinates");

    return opened_[r * L_ + c];
}

// A full site is an open site connected to TOP
bool Percolation::isFull(int r, int c) {

    if (!isOpen(r, c))
        return false;

    int idx = r * L_ + c;

    // ufFull_ has no virtual BOTTOM
    // This prevents backwash
    return ufFull_->connected(idx, top_);
}

// Return cached percolation status
bool Percolation::percolates() {
    return percolated_;
}

// Number of currently open sites
int Percolation::numOpen() const {
    return openCount_;
}

// Size of the largest connected open-site cluster
int Percolation::largestCluster() const {

    if (!ufClust_) {
        throw std::logic_error(
            "Cluster tracking is disabled"
        );
    }

    return largest_;
}

// Return the grid dimension
int Percolation::L() const {
    return L_;
}

// Sum counters from all active DSUs
Counters Percolation::counters() const {

    Counters result{};

    const Counters& a = ufPerc_->counters();
    const Counters& b = ufFull_->counters();

    result.finds = a.finds + b.finds;
    result.hops = a.hops + b.hops;
    result.unions = a.unions + b.unions;

    if (ufClust_) {
        const Counters& c = ufClust_->counters();

        result.finds += c.finds;
        result.hops += c.hops;
        result.unions += c.unions;
    }

    return result;
}

// Percolation DSU counters
const Counters& Percolation::countersPerc() const {
    return ufPerc_->counters();
}

// Fullness DSU counters
const Counters& Percolation::countersFull() const {
    return ufFull_->counters();
}

// Cluster DSU counters
const Counters& Percolation::countersClust() const {

    if (!ufClust_) {
        throw std::logic_error(
            "Cluster tracking is disabled"
        );
    }

    return ufClust_->counters();
}

// Reset operation counters without changing connectivity
void Percolation::resetCounters() {

    ufPerc_->resetCounters();
    ufFull_->resetCounters();

    if (ufClust_) {
        ufClust_->resetCounters();
    }
}

// Read-only access for invariant checkers
const UnionFind& Percolation::ufPerc() const {
    return *ufPerc_;
}

const UnionFind& Percolation::ufFull() const {
    return *ufFull_;
}

const UnionFind* Percolation::ufClust() const {
    return ufClust_.get();
}

} // namespace perc


