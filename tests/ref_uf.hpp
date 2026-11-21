// tests/ref_uf.hpp  --  TEST-ONLY stand-in for Structure A/B until the real ones land.
// One class implements all four variants of INTERFACE.md v0.3 section 3.1, including the
// tie rule (equal ranks: parent[rb] = ra, rank[ra]++). UfBug injects deliberate bugs so we
// can prove the checkers and the differential harness catch them (mutation testing).
#pragma once
#include "unionfind.hpp"
#include <cassert>
#include <vector>

namespace perc_test {

enum class UfBug {
    None,
    NoSizeUpdate,        // forgets size[newRoot] += size[child]          -> U2
    RankNoIncrement,     // forgets rank[ra]++ on a tie                    -> U5
    SkipUnionCount,      // every 3rd merge is not counted in unions       -> U4
    FalseTrueOnSameSet,  // unite() returns true (and counts) when ra==rb  -> U4
    LinkReversed         // links in the wrong DIRECTION (naive: parent[rb]=ra; rank tie:
                         // parent[ra]=rb). Every partition stays correct, so only the
                         // linking-rule checker L1 can see it.
};

inline const char* vname(perc::Variant v) {
    switch (v) {
        case perc::Variant::Naive:           return "naive";
        case perc::Variant::Rank:            return "rank";
        case perc::Variant::Compression:     return "compress";
        case perc::Variant::RankCompression: return "rank_compress";
    }
    return "?";
}

class RefUF : public perc::UnionFind {
public:
    RefUF(int n, perc::Variant v, UfBug bug = UfBug::None)
        : perc::UnionFind(n), v_(v), bug_(bug), merges_(0),
          parent_(n), rank_(n, 0), size_(n, 1) {
        for (int i = 0; i < n; ++i) parent_[i] = i;
    }

    int find(int x) override {
        assert(x >= 0 && x < n_);
        ++cnt_.finds;
        int root = x;
        while (parent_[root] != root) { root = parent_[root]; ++cnt_.hops; }
        if (compresses()) {
            int cur = x;
            while (cur != root) { int nxt = parent_[cur]; parent_[cur] = root; cur = nxt; }
        }
        return root;
    }

    bool unite(int a, int b) override {
        const int ra = find(a), rb = find(b);
        if (ra == rb) {
            if (bug_ == UfBug::FalseTrueOnSameSet) { ++cnt_.unions; return true; }
            return false;
        }
        const bool rev = (bug_ == UfBug::LinkReversed);
        if (!hasRank())                  { if (rev) link(rb, ra); else link(ra, rb); }   // naive: parent[ra] = rb
        else if (rank_[ra] < rank_[rb])  link(ra, rb);
        else if (rank_[ra] > rank_[rb])  link(rb, ra);
        else {                                                // tie: parent[rb] = ra, rank[ra]++
            const int win = rev ? rb : ra, lose = rev ? ra : rb;
            link(lose, win);
            if (bug_ != UfBug::RankNoIncrement) ++rank_[win];
        }
        ++merges_;
        if (!(bug_ == UfBug::SkipUnionCount && merges_ % 3 == 0)) ++cnt_.unions;
        return true;
    }

    int size(int x) override { return size_[find(x)]; }
    const char* name() const override { return vname(v_); }

    int parentOf(int x)   const override { return parent_[x]; }
    int rankOf(int x)     const override { return hasRank() ? rank_[x] : -1; }
    int sizeAtRoot(int r) const override { return size_[r]; }

private:
    bool hasRank()    const { return v_ == perc::Variant::Rank || v_ == perc::Variant::RankCompression; }
    bool compresses() const { return v_ == perc::Variant::Compression || v_ == perc::Variant::RankCompression; }
    void link(int child, int newRoot) {
        parent_[child] = newRoot;
        if (bug_ != UfBug::NoSizeUpdate) size_[newRoot] += size_[child];
    }

    perc::Variant v_;
    UfBug bug_;
    int merges_;
    std::vector<int> parent_, rank_, size_;
};

}  // namespace perc_test
