// src/structA/percolation.cpp -- owner: Structure A owner.
// The plumbing (constructor, getters) is done. The four methods that hold the logic are TODO:
// open, isFull, percolates, largestCluster. Follow INTERFACE.md 4.2 step by step.
#include "../../include/percolation.hpp"
#include <algorithm>
#include <stdexcept>

namespace perc {

static Counters sumCounters(const Counters& a, const Counters& b) {
    Counters t;
    t.finds = a.finds + b.finds;
    t.hops = a.hops + b.hops;
    t.unions = a.unions + b.unions;
    return t;
}

// Validates L before anything is allocated: 1 <= L <= 46340 (L*L must fit in int).
static size_t checkedCells(int L) {
    if (L < 1 || L > 46340) throw std::invalid_argument("Percolation: L must be in [1, 46340]");
    return static_cast<size_t>(L) * static_cast<size_t>(L);
}

Percolation::Percolation(int L, Variant v, bool trackClusters)
    : L_(L), trackClusters_(trackClusters), open_(checkedCells(L), 0) {
    ufPerc_ = makeUnionFind(v, L * L + 2);
    ufFull_ = makeUnionFind(v, L * L + 1);
    if (trackClusters) ufClust_ = makeUnionFind(v, L * L);
}

bool Percolation::isOpen(int r, int c) const {
    if (r < 0 || r >= L_ || c < 0 || c >= L_) throw std::out_of_range("Percolation::isOpen");
    return open_[index(r, c)] != 0;
}

int Percolation::numOpen() const { return numOpen_; }
int Percolation::L() const { return L_; }

Counters Percolation::counters() const {
    Counters t = sumCounters(ufPerc_->counters(), ufFull_->counters());
    if (ufClust_) t = sumCounters(t, ufClust_->counters());
    return t;
}
const Counters& Percolation::countersPerc() const { return ufPerc_->counters(); }
const Counters& Percolation::countersFull() const { return ufFull_->counters(); }
const Counters& Percolation::countersClust() const {
    static const Counters zero{};
    return ufClust_ ? ufClust_->counters() : zero;
}
void Percolation::resetCounters() {
    ufPerc_->resetCounters();
    ufFull_->resetCounters();
    if (ufClust_) ufClust_->resetCounters();
}

const UnionFind& Percolation::ufPerc() const { return *ufPerc_; }
const UnionFind& Percolation::ufFull() const { return *ufFull_; }
const UnionFind* Percolation::ufClust() const { return ufClust_.get(); }

// ---- TODO (Structure A owner) ----------------------------------------------------------------
bool Percolation::open(int r, int c) {

    // Check whether row and column are inside the grid
    if (r < 0 || r >= L_ || c < 0 || c >= L_)
        throw std::out_of_range("Percolation::open");

    // Convert 2D coordinates into 1D site index
    int idx = index(r, c);

    // If already open, do nothing
    if (open_[idx])
        return false;

    // Mark site open and increase the count
    open_[idx] = 1;
    numOpen_++;

    // Virtual top and bottom indices
    int TOP = L_ * L_;
    int BOTTOM = L_ * L_ + 1;

    // Connect top-row sites to virtual TOP
    if (r == 0) {
        ufPerc_->unite(idx, TOP);
        ufFull_->unite(idx, TOP);
    }

    // Connect bottom-row sites to virtual BOTTOM
    // Only in ufPerc_ to prevent backwash
    if (r == L_ - 1) {
        ufPerc_->unite(idx, BOTTOM);
    }

    // Four neighboring directions:
    // up, down, left, right
    const int dr[4] = {-1, 1, 0, 0};
    const int dc[4] = {0, 0, -1, 1};

    for (int i = 0; i < 4; i++) {

        int nr = r + dr[i];
        int nc = c + dc[i];

        // Skip neighbors outside the grid
        if (nr < 0 || nr >= L_ || nc < 0 || nc >= L_)
            continue;

        int nb = index(nr, nc);

        // Connect only if the neighboring site is open
        if (!open_[nb])
            continue;

        // Update percolation connectivity
        ufPerc_->unite(idx, nb);

        // Update fullness connectivity
        ufFull_->unite(idx, nb);

        // Track real connected clusters if enabled
        if (trackClusters_) {
            ufClust_->unite(idx, nb);
        }
    } 
      // Update the largest cluster size
    if (trackClusters_) {
        largest_ = std::max(
            largest_,
            ufClust_->size(idx)
        );
    }

    // Cache the result once percolation occurs
    if (!percolated_) {
        percolated_ = ufPerc_->connected(TOP, BOTTOM);
    }

    return true;
}

bool Percolation::isFull(int r, int c) {

    // isOpen also validates the coordinates
    if (!isOpen(r, c))
        return false;

    int idx = index(r, c);
    int TOP = L_ * L_;

    // ufFull_ has no BOTTOM, preventing backwash
    return ufFull_->connected(idx, TOP);
}
bool Percolation::percolates() {

    // Return cached result without additional DSU finds
    return percolated_;
}

int Percolation::largestCluster() const {

    // Cluster results are unavailable if tracking is disabled
    if (!trackClusters_)
        throw std::logic_error(
            "largestCluster: clusters are not tracked"
        );

    return largest_;
}

}  // namespace perc
