// src/structA/percolation.cpp -- owner: Structure A owner.
// The plumbing (constructor, getters) is done. The four methods that hold the logic are TODO:
// open, isFull, percolates, largestCluster. Follow INTERFACE.md 4.2 step by step.
#include "percolation.hpp"
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
bool Percolation::open(int, int) {
    throw std::logic_error("Percolation::open not implemented (INTERFACE.md 4.2)");
}
bool Percolation::isFull(int, int) {
    throw std::logic_error("Percolation::isFull not implemented");
}
bool Percolation::percolates() {
    throw std::logic_error("Percolation::percolates not implemented");
}
int Percolation::largestCluster() const {
    // TODO: if (!trackClusters_) throw std::logic_error("largestCluster: clusters are not tracked");
    //       then return largest_ (INTERFACE.md section 4).
    throw std::logic_error("Percolation::largestCluster not implemented");
}

}  // namespace perc
