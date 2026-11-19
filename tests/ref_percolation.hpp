// tests/ref_percolation.hpp  --  TEST-ONLY stand-in for Hemanth's Percolation (INTERFACE.md
// v0.3 section 4, open() pseudocode of 4.2). PercBug injects typical mistakes so the
// differential harness can be shown to catch them.
#pragma once
#include "ref_uf.hpp"
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <vector>

namespace perc_test {

enum class PercBug {
    None,
    Backwash,         // BOTTOM united into ufFull (the single-structure bug)
    ClusterFromFull,  // largest cluster read from ufFull, whose TOP merges separate clusters
    WrapAround,       // grid edges wrap around
    SkipLeft,         // forgets the left neighbour
    NoBottomUnite,    // forgets to unite bottom-row sites with BOTTOM in ufPerc
    ClosesSite,       // 5th successful open clears the flag of the first opened site -> P1
    LargestShrinks,   // largest = size of the newest cluster instead of the maximum   -> P6
    ClustDiagonal     // ufClust also joins diagonal neighbours                        -> P2/U3
};

class RefPercolation {
public:
    RefPercolation(int L, perc::Variant v, bool trackClusters = true,
                   PercBug pb = PercBug::None, UfBug ub = UfBug::None)
        : L_(checkedL(L)), track_(trackClusters), pb_(pb),
          open_(static_cast<size_t>(L_) * L_, 0), numOpen_(0), largest_(0), percolated_(false),
          perc_(L_ * L_ + 2, v, ub),
          full_(L_ * L_ + 1 + (pb == PercBug::Backwash ? 1 : 0), v, ub) {
        if (track_) clust_.reset(new RefUF(L_ * L_, v, ub));
    }

    bool open(int r, int c) {
        check(r, c);
        const int idx = r * L_ + c, TOP = L_ * L_, BOT = TOP + 1;
        if (open_[idx]) return false;
        open_[idx] = 1;
        ++numOpen_;
        if (pb_ == PercBug::ClosesSite) {
            if (opens_ == 0) firstOpened_ = idx;
            ++opens_;
            if (opens_ == 5) open_[firstOpened_] = 0;
        }
        if (r == 0) { perc_.unite(idx, TOP); full_.unite(idx, TOP); }
        if (r == L_ - 1) {
            if (pb_ != PercBug::NoBottomUnite) perc_.unite(idx, BOT);
            if (pb_ == PercBug::Backwash) full_.unite(idx, BOT);
        }
        static const int dr[4] = {-1, 1, 0, 0}, dc[4] = {0, 0, -1, 1};
        for (int d = 0; d < 4; ++d) {
            if (pb_ == PercBug::SkipLeft && d == 2) continue;
            int nr = r + dr[d], nc = c + dc[d];
            if (pb_ == PercBug::WrapAround) { nr = (nr + L_) % L_; nc = (nc + L_) % L_; }
            else if (nr < 0 || nr >= L_ || nc < 0 || nc >= L_) continue;
            const int nb = nr * L_ + nc;
            if (nb == idx) continue;
            if (open_[nb]) {
                perc_.unite(idx, nb);
                full_.unite(idx, nb);
                if (track_) clust_->unite(idx, nb);
            }
        }
        if (track_ && pb_ == PercBug::ClustDiagonal) {
            for (int dd = -1; dd <= 1; dd += 2)
                for (int de = -1; de <= 1; de += 2) {
                    const int nr = r + dd, nc = c + de;
                    if (nr < 0 || nr >= L_ || nc < 0 || nc >= L_) continue;
                    if (open_[nr * L_ + nc]) clust_->unite(idx, nr * L_ + nc);
                }
        }
        if (track_) {
            const int s = (pb_ == PercBug::ClusterFromFull) ? full_.size(idx) : clust_->size(idx);
            largest_ = (pb_ == PercBug::LargestShrinks) ? s : std::max(largest_, s);
        }
        if (!percolated_) percolated_ = perc_.connected(TOP, BOT);
        return true;
    }

    bool isOpen(int r, int c) const { check(r, c); return open_[r * L_ + c] != 0; }
    bool isFull(int r, int c) {
        check(r, c);
        const int idx = r * L_ + c;
        return open_[idx] && full_.connected(idx, L_ * L_);
    }
    bool percolates() { return percolated_; }   // non-const, like the real class
    int  numOpen() const { return numOpen_; }
    int  largestCluster() const {
        if (!track_) throw std::logic_error("largestCluster: cluster tracking is off");
        return largest_;
    }
    int  L() const { return L_; }

    const perc::UnionFind& ufPerc() const { return perc_; }
    const perc::UnionFind& ufFull() const { return full_; }
    const perc::UnionFind* ufClust() const { return clust_.get(); }

private:
    static int checkedL(int L) {
        if (L < 1 || L > 46340) throw std::invalid_argument("RefPercolation: need 1 <= L <= 46340");
        return L;
    }
    void check(int r, int c) const {
        if (r < 0 || r >= L_ || c < 0 || c >= L_) throw std::out_of_range("RefPercolation: out of range");
    }

    int L_;
    bool track_;
    PercBug pb_;
    std::vector<char> open_;
    int numOpen_, largest_;
    int opens_ = 0, firstOpened_ = 0;
    bool percolated_;
    RefUF perc_, full_;
    std::unique_ptr<RefUF> clust_;
};

}  // namespace perc_test
