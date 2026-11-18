#include "rank_uf.hpp"
#include <cassert>
#include <numeric>

namespace perc {
RankUF::RankUF(int n) : UnionFind(n), parent_(n), size_(n, 1), rank_(n, 0) {
    std::iota(parent_.begin(), parent_.end(), 0);
}
int RankUF::find(int x) {
    assert(0 <= x && x < n_);
    cnt_.finds++;
    while (parent_[x] != x) { x = parent_[x]; cnt_.hops++; }   // no compression
    return x;
}
bool RankUF::unite(int a, int b) {
    int ra = find(a), rb = find(b);
    if (ra == rb) return false;
    if (rank_[ra] < rank_[rb]) {            // lower rank goes under higher rank
        parent_[ra] = rb;
        size_[rb] += size_[ra];
    } else if (rank_[ra] > rank_[rb]) {
        parent_[rb] = ra;
        size_[ra] += size_[rb];
    } else {                                // INTERFACE 3.1 (v0.3): tie -> rb under ra, rank[ra]++
        parent_[rb] = ra;
        rank_[ra]++;
        size_[ra] += size_[rb];
    }
    cnt_.unions++;
    return true;
}
int RankUF::size(int x) { return size_[find(x)]; }
int RankUF::parentOf(int x) const { assert(0 <= x && x < n_); return parent_[x]; }
int RankUF::rankOf(int x) const { assert(0 <= x && x < n_); return rank_[x]; }
int RankUF::sizeAtRoot(int r) const { assert(0 <= r && r < n_); return size_[r]; }
}
