#include "compression_uf.hpp"
#include <cassert>
#include <numeric>

namespace perc {
CompressionUF::CompressionUF(int n) : UnionFind(n), parent_(n), size_(n, 1) {
    std::iota(parent_.begin(), parent_.end(), 0);
}
int CompressionUF::find(int x) {
    assert(0 <= x && x < n_);
    cnt_.finds++;
    int root = x;
    while (parent_[root] != root) { root = parent_[root]; cnt_.hops++; }   // pass 1: counted
    while (x != root) {                                                    // pass 2: not counted
        int next = parent_[x];
        parent_[x] = root;
        x = next;
    }
    return root;
}
bool CompressionUF::unite(int a, int b) {
    int ra = find(a), rb = find(b);
    if (ra == rb) return false;
    parent_[ra] = rb;              // same linking as Naive
    size_[rb] += size_[ra];
    cnt_.unions++;
    return true;
}
int CompressionUF::size(int x) { return size_[find(x)]; }
int CompressionUF::parentOf(int x) const { assert(0 <= x && x < n_); return parent_[x]; }
int CompressionUF::rankOf(int x) const { assert(0 <= x && x < n_); (void)x; return -1; }
int CompressionUF::sizeAtRoot(int r) const { assert(0 <= r && r < n_); return size_[r]; }
}
