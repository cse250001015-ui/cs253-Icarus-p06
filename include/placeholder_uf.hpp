// include/placeholder_uf.hpp -- TEMPORARY. Owner: Architect.
// A simple correct union-find (naive linking, no compression) so that everything compiles and
// runs before the real classes exist. Remove from factory.cpp once the real classes are merged.
#pragma once
#include "unionfind.hpp"
#include <vector>
using namespace std;

namespace perc {

class PlaceholderUF : public UnionFind {
public:
    explicit PlaceholderUF(int n) : UnionFind(n), parent_(n), size_(n, 1) {
        for (int i = 0; i < n; ++i) parent_[i] = i;
    }

    int find(int x) override {
        ++cnt_.finds;
        while (parent_[x] != x) { x = parent_[x]; ++cnt_.hops; }
        return x;
    }
    bool unite(int a, int b) override {
        int ra = find(a), rb = find(b);
        if (ra == rb) return false;
        parent_[ra] = rb;              // naive rule: root(a) under root(b)
        size_[rb] += size_[ra];
        ++cnt_.unions;
        return true;
    }
    int size(int x) override { return size_[find(x)]; }
    const char* name() const override { return "placeholder"; }

    int parentOf(int x)   const override { return parent_[x]; }
    int rankOf(int)       const override { return -1; }
    int sizeAtRoot(int r) const override { return size_[r]; }

private:
    vector<int> parent_, size_;
};

}  // namespace perc
