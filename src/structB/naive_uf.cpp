#include "naive_uf.hpp"
#include <cassert>
#include <numeric>

namespace perc {
NaiveUF::NaiveUF(int n) : UnionFind(n), parent_(n), size_(n, 1) {
    std::iota(parent_.begin(), parent_.end(), 0);
}
int NaiveUF::find(int x) {
    assert(0 <= x && x < n_);
    cnt_.finds++;
    while (parent_[x] != x) { x = parent_[x]; cnt_.hops++; }   // no compression
    return x;
}
bool NaiveUF::unite(int a, int b) {
    int ra = find(a), rb = find(b);
    if (ra == rb) return false;
    parent_[ra] = rb;              // INTERFACE 3.1: Naive links ra under rb
    size_[rb] += size_[ra];
    cnt_.unions++;
    return true;
}
int NaiveUF::size(int x) { return size_[find(x)]; }
int NaiveUF::parentOf(int x) const { assert(0 <= x && x < n_); return parent_[x]; }
int NaiveUF::rankOf(int x) const { assert(0 <= x && x < n_); (void)x; return -1; }
int NaiveUF::sizeAtRoot(int r) const { assert(0 <= r && r < n_); return size_[r]; }
}
