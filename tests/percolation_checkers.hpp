// tests/percolation_checkers.hpp  --  invariant checkers P1..P6 (INTERFACE.md v0.3 section 9).
// Written against the public Percolation API plus ufPerc()/ufFull()/ufClust(), so they run
// unchanged on perc::Percolation and on the stand-in. Note: the real percolates() and isFull()
// are NON-const, so the checks that call them take P& (not const P&). P1 and P6 mention history ("never
// decreases", "stays open"), so they live in PercMonitor, which remembers the previous state;
// make one monitor per grid and call the checks after operations (every k-th is fine: the
// monotonicity checks then compare with the previous check).
#pragma once
#include "checkers.hpp"
#include "flood_fill.hpp"
#include <string>
#include <vector>

namespace perc_check {

// P2: if two open sites are connected in `fine` they are connected in `coarse`.
// Equivalent to: the partition of `fine` refines that of `coarse`; checked in O(n) by
// mapping every fine root to one coarse root.
inline CheckResult checkRefines(const perc::UnionFind& fine, const perc::UnionFind& coarse,
                                const std::vector<char>& openFlag, const char* fineName,
                                const char* coarseName) {
    const int N = static_cast<int>(openFlag.size());
    std::vector<int> toCoarse(fine.numElements(), -1), first(fine.numElements(), -1);
    for (int x = 0; x < N; ++x) {
        if (!openFlag[x]) continue;
        const int rf = safeRoot(fine, x), rc = safeRoot(coarse, x);
        if (rf < 0 || rc < 0) return fail("P2: cannot follow parents (see U1)");
        if (toCoarse[rf] == -1) { toCoarse[rf] = rc; first[rf] = x; }
        else if (toCoarse[rf] != rc)
            return fail(std::string("P2: open sites ") + std::to_string(first[rf]) + " and " + std::to_string(x) +
                        " are connected in " + fineName + " but not in " + coarseName);
    }
    return pass();
}

template <class P>
class PercMonitor {
public:
    explicit PercMonitor(int L) : L_(L), wasOpen_(static_cast<size_t>(L) * L, 0), prevLargest_(0), prevPerc_(false) {}

    // P1: a site, once open, stays open; numOpen equals the number of open flags.
    CheckResult checkP1(const P& sut) {
        int cnt = 0;
        for (int r = 0; r < L_; ++r)
            for (int c = 0; c < L_; ++c) {
                const bool o = sut.isOpen(r, c);
                const int i = r * L_ + c;
                if (wasOpen_[i] && !o)
                    return fail("P1: site (" + std::to_string(r) + "," + std::to_string(c) + ") was open and is now closed");
                wasOpen_[i] = o ? 1 : 0;
                if (o) ++cnt;
            }
        if (cnt != sut.numOpen())
            return fail("P1: numOpen() = " + std::to_string(sut.numOpen()) + " but " + std::to_string(cnt) + " open flags are set");
        return pass();
    }

    // P2: connected in ufClust => connected in ufFull => connected in ufPerc (open sites).
    CheckResult checkP2(const P& sut) const {
        std::vector<char> openFlag(static_cast<size_t>(L_) * L_);
        for (int r = 0; r < L_; ++r)
            for (int c = 0; c < L_; ++c) openFlag[r * L_ + c] = sut.isOpen(r, c) ? 1 : 0;
        if (sut.ufClust() != nullptr) {
            CheckResult x = checkRefines(*sut.ufClust(), sut.ufFull(), openFlag, "ufClust", "ufFull");
            if (!x) return x;
        }
        return checkRefines(sut.ufFull(), sut.ufPerc(), openFlag, "ufFull", "ufPerc");
    }

    // P3: isFull is true exactly when the site is open and reachable from the top row
    // through open sites (the oracle's definition). Also: full implies open.
    CheckResult checkP3(P& sut, perc::FloodFillOracle& orc) const {
        for (int r = 0; r < L_; ++r)
            for (int c = 0; c < L_; ++c) {
                const bool f = sut.isFull(r, c), g = orc.isFull(r, c);
                const std::string at = "P3: isFull(" + std::to_string(r) + "," + std::to_string(c) + ") ";
                if (f && !sut.isOpen(r, c)) return fail(at + "is true for a closed site");
                if (f != g) return fail(at + "is " + (f ? "true" : "false") + " but the oracle says " + (g ? "true" : "false"));
            }
        return pass();
    }

    // P4: percolates() is true exactly when some bottom-row site is full (by the structure's
    // own isFull, and by the oracle).
    CheckResult checkP4(P& sut, perc::FloodFillOracle& orc) const {
        bool anyFull = false;
        for (int c = 0; c < L_; ++c) if (sut.isFull(L_ - 1, c)) anyFull = true;
        if (sut.percolates() != anyFull)
            return fail(std::string("P4: percolates() is ") + (sut.percolates() ? "true" : "false") +
                        " but " + (anyFull ? "a bottom-row site is full" : "no bottom-row site is full"));
        if (sut.percolates() != orc.percolates())
            return fail(std::string("P4: percolates() is ") + (sut.percolates() ? "true" : "false") + " but the oracle says " +
                        (orc.percolates() ? "true" : "false"));
        return pass();
    }

    // P5: ufFull has exactly L*L+1 elements (no BOTTOM), so backwash cannot happen.
    CheckResult checkP5(const P& sut) const {
        const int want = L_ * L_ + 1;
        if (sut.ufFull().numElements() != want)
            return fail("P5: ufFull has " + std::to_string(sut.ufFull().numElements()) + " elements, expected L*L+1 = " +
                        std::to_string(want));
        return pass();
    }

    // P6: largestCluster() equals the oracle's value and never decreases; percolated never
    // goes from true to false. (largestCluster is skipped when clusters are not tracked.)
    CheckResult checkP6(P& sut, perc::FloodFillOracle& orc, bool trackClusters) {
        const bool p = sut.percolates();
        if (prevPerc_ && !p) return fail("P6: percolates() went from true to false");
        prevPerc_ = p;
        if (trackClusters) {
            const int lc = sut.largestCluster();
            if (lc < prevLargest_)
                return fail("P6: largestCluster() decreased from " + std::to_string(prevLargest_) + " to " + std::to_string(lc));
            prevLargest_ = lc;
            const int want = orc.largestCluster();
            if (lc != want)
                return fail("P6: largestCluster() = " + std::to_string(lc) + " but the largest component has " + std::to_string(want) + " sites");
        }
        return pass();
    }

    // All six, in the order P5, P1, P3, P4, P2, P6. Returns the first failure.
    CheckResult checkAll(P& sut, perc::FloodFillOracle& orc, bool trackClusters) {
        CheckResult r;
        if (!(r = checkP5(sut))) return r;
        if (!(r = checkP1(sut))) return r;
        if (!(r = checkP3(sut, orc))) return r;
        if (!(r = checkP4(sut, orc))) return r;
        if (!(r = checkP2(sut))) return r;
        return checkP6(sut, orc, trackClusters);
    }

private:
    int L_;
    std::vector<char> wasOpen_;
    int prevLargest_;
    bool prevPerc_;
};

}  // namespace perc_check
