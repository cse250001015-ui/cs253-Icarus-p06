// tests/checkers.hpp  --  invariant checkers U1..U5 (INTERFACE.md v0.3 section 9).
// They take only a const UnionFind& and use ONLY the debug accessors (parentOf, rankOf,
// sizeAtRoot, numElements, counters): never find(), which would compress paths and change
// the state being checked. Every walk is bounded, so a corrupt parent array (cycle, index
// out of range) is reported instead of looping forever.
#pragma once
#include "ref_uf.hpp"   // for vname() only
#include "unionfind.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace perc_check {

struct CheckResult {
    bool ok;
    std::string msg;
    explicit operator bool() const { return ok; }
};
inline CheckResult pass() { return {true, ""}; }
inline CheckResult fail(std::string m) { return {false, std::move(m)}; }

// Follow parents from x. Returns the root, or -1 if an index is out of range or the walk
// takes more than n-1 steps (cycle). If depthOut is given it receives the number of steps.
inline int safeRoot(const perc::UnionFind& uf, int x, int* depthOut = nullptr) {
    const int n = uf.numElements();
    int cur = x, steps = 0;
    while (true) {
        const int p = uf.parentOf(cur);
        if (p < 0 || p >= n) return -1;
        if (p == cur) { if (depthOut) *depthOut = steps; return cur; }
        cur = p;
        if (++steps > n - 1) return -1;
    }
}

inline int countSets(const perc::UnionFind& uf) {
    int s = 0;
    for (int x = 0; x < uf.numElements(); ++x) if (uf.parentOf(x) == x) ++s;
    return s;
}

// U1: every parent index is in [0, n) and every element reaches a root (no cycles).
inline CheckResult checkU1(const perc::UnionFind& uf) {
    const int n = uf.numElements();
    for (int x = 0; x < n; ++x) {
        const int p = uf.parentOf(x);
        if (p < 0 || p >= n)
            return fail("U1: parent[" + std::to_string(x) + "] = " + std::to_string(p) + " is outside [0, n)");
    }
    for (int x = 0; x < n; ++x)
        if (safeRoot(uf, x) < 0)
            return fail("U1: following parents from " + std::to_string(x) + " does not reach a root (cycle)");
    return pass();
}

// U2: for every root r, size[r] equals the number of elements whose root is r, and the
// root sizes add up to n.
inline CheckResult checkU2(const perc::UnionFind& uf) {
    CheckResult c1 = checkU1(uf);
    if (!c1) return c1;
    const int n = uf.numElements();
    std::vector<int> cnt(n, 0);
    for (int x = 0; x < n; ++x) ++cnt[safeRoot(uf, x)];
    long long sum = 0;
    for (int r = 0; r < n; ++r) {
        if (uf.parentOf(r) != r) continue;
        sum += uf.sizeAtRoot(r);
        if (uf.sizeAtRoot(r) != cnt[r])
            return fail("U2: root " + std::to_string(r) + " stores size " + std::to_string(uf.sizeAtRoot(r)) +
                        " but its set has " + std::to_string(cnt[r]) + " elements");
    }
    if (sum != n) return fail("U2: root sizes sum to " + std::to_string(sum) + ", not n = " + std::to_string(n));
    return pass();
}

// U3: the partition of the structure equals the expected partition. expectedClass[x] is a
// class id in [0, n); x and y must share a root exactly when they share a class id.
// Checked in O(n) through a bijection between roots and classes.
inline CheckResult checkU3(const perc::UnionFind& uf, const std::vector<int>& expectedClass) {
    const int n = uf.numElements();
    if (static_cast<int>(expectedClass.size()) != n)
        return fail("U3: expected partition has " + std::to_string(expectedClass.size()) +
                    " elements, structure has " + std::to_string(n));
    CheckResult c1 = checkU1(uf);
    if (!c1) return c1;
    std::vector<int> classToRoot(n, -1), classFirst(n, -1), rootToClass(n, -1), rootFirst(n, -1);
    for (int x = 0; x < n; ++x) {
        const int cl = expectedClass[x];
        if (cl < 0 || cl >= n) return fail("U3: expected class id out of range for element " + std::to_string(x));
        const int r = safeRoot(uf, x);
        if (classToRoot[cl] == -1) { classToRoot[cl] = r; classFirst[cl] = x; }
        else if (classToRoot[cl] != r)
            return fail("U3: elements " + std::to_string(classFirst[cl]) + " and " + std::to_string(x) +
                        " should be in the same set but have different roots");
        if (rootToClass[r] == -1) { rootToClass[r] = cl; rootFirst[r] = x; }
        else if (rootToClass[r] != cl)
            return fail("U3: elements " + std::to_string(rootFirst[r]) + " and " + std::to_string(x) +
                        " share a root but should be in different sets");
    }
    return pass();
}

// U4 (single call): unite(a,b) returns true iff a and b were in different sets; the number
// of sets drops by exactly 1 on true and 0 on false; counters().unions rises likewise.
inline CheckResult checkU4Unite(perc::UnionFind& uf, int a, int b, bool* returned = nullptr) {
    const bool different = safeRoot(uf, a) != safeRoot(uf, b);
    const int sets0 = countSets(uf);
    const uint64_t u0 = uf.counters().unions;
    const bool ret = uf.unite(a, b);
    if (returned) *returned = ret;
    const int sets1 = countSets(uf);
    const uint64_t u1 = uf.counters().unions;
    const std::string tag = "U4: unite(" + std::to_string(a) + "," + std::to_string(b) + ") ";
    if (ret != different) return fail(tag + "returned " + (ret ? "true" : "false") + " but the elements were " +
                                      (different ? "in different sets" : "already in the same set"));
    if (sets0 - sets1 != (ret ? 1 : 0)) return fail(tag + "changed the number of sets by " + std::to_string(sets0 - sets1));
    if (u1 - u0 != (ret ? 1u : 0u)) return fail(tag + "changed counters().unions by " + std::to_string(u1 - u0));
    return pass();
}

// U4 (window): between two snapshots, the drop in the number of sets must equal the rise in
// counters().unions. Used where unite() is called inside another object (Percolation::open).
struct UfSnapshot { int sets; uint64_t unions; };
inline UfSnapshot snapshot(const perc::UnionFind& uf) { return {countSets(uf), uf.counters().unions}; }
inline CheckResult checkU4Window(const UfSnapshot& before, const perc::UnionFind& uf) {
    const UfSnapshot now = snapshot(uf);
    const long long dSets = static_cast<long long>(before.sets) - now.sets;
    const long long dUnions = static_cast<long long>(now.unions) - static_cast<long long>(before.unions);
    if (dSets != dUnions)
        return fail("U4: number of sets dropped by " + std::to_string(dSets) +
                    " but counters().unions rose by " + std::to_string(dUnions));
    return pass();
}

// U5: rank facts.
//   Naive / Compression: no rank is kept, so rankOf(root) must be -1.
//   Rank: for every root, rank == tree height, size >= 2^rank, height <= floor(log2 n).
//   RankCompression: the same, except height <= rank (compression may shorten trees).
inline CheckResult checkU5(const perc::UnionFind& uf, perc::Variant v) {
    CheckResult c1 = checkU1(uf);
    if (!c1) return c1;
    const int n = uf.numElements();
    const bool rankful = (v == perc::Variant::Rank || v == perc::Variant::RankCompression);
    if (!rankful) {
        for (int r = 0; r < n; ++r)
            if (uf.parentOf(r) == r && uf.rankOf(r) != -1)
                return fail("U5: variant " + std::string(perc_test::vname(v)) + " keeps no rank, but rankOf(" +
                            std::to_string(r) + ") = " + std::to_string(uf.rankOf(r)));
        return pass();
    }
    std::vector<int> height(n, 0);
    for (int x = 0; x < n; ++x) {
        int d = 0;
        const int r = safeRoot(uf, x, &d);
        if (d > height[r]) height[r] = d;
    }
    int fl = 0;                                  // floor(log2 n)
    while ((1LL << (fl + 1)) <= n) ++fl;
    for (int r = 0; r < n; ++r) {
        if (uf.parentOf(r) != r) continue;
        const int rk = uf.rankOf(r);
        const std::string at = "U5: root " + std::to_string(r) + " ";
        if (rk < 0 || rk > 62) return fail(at + "has invalid rank " + std::to_string(rk));
        if (uf.sizeAtRoot(r) < (1LL << rk))
            return fail(at + "has size " + std::to_string(uf.sizeAtRoot(r)) + " < 2^rank = " + std::to_string(1LL << rk));
        if (height[r] > fl)
            return fail(at + "has height " + std::to_string(height[r]) + " > floor(log2 n) = " + std::to_string(fl));
        if (v == perc::Variant::Rank && rk != height[r])
            return fail(at + "has rank " + std::to_string(rk) + " but height " + std::to_string(height[r]) +
                        " (without compression they must be equal)");
        if (v == perc::Variant::RankCompression && height[r] > rk)
            return fail(at + "has height " + std::to_string(height[r]) + " > rank " + std::to_string(rk));
    }
    return pass();
}

// L1 (linking rule, INTERFACE.md 3.1): black-box check of the DIRECTION of linking, using only
// unite() and the debug accessors on fresh instances made by `make(n)`. The partition checkers
// cannot see this: a structure that links the "wrong way" still has correct sets.
//   Naive / Compression : unite(a,b) sets parent[ra] = rb  (root of the first argument goes under the second)
//   Rank / RankCompression :
//       lower rank goes under higher rank, whichever argument it is;
//       equal ranks: parent[rb] = ra and rank[ra]++  (the FIRST argument's root wins)
//   Naive / Compression keep no rank: rankOf must be -1; ranked variants start at rank 0.
inline CheckResult checkLinkingRule(perc::Variant v,
                                    const std::function<std::unique_ptr<perc::UnionFind>(int)>& make) {
    const bool ranked = (v == perc::Variant::Rank || v == perc::Variant::RankCompression);
    const std::string who = std::string("L1 (") + perc_test::vname(v) + "): ";
    auto got = [](const perc::UnionFind& u, int x) { return std::to_string(u.parentOf(x)); };

    std::unique_ptr<perc::UnionFind> a = make(8);
    for (int x = 0; x < 8; ++x)
        if (a->rankOf(x) != (ranked ? 0 : -1))
            return fail(who + "a fresh element must have rankOf = " + (ranked ? "0" : "-1") + ", got " + std::to_string(a->rankOf(x)));
    if (!ranked) {
        if (!a->unite(0, 1)) return fail(who + "unite(0,1) on fresh elements must return true");
        if (a->parentOf(0) != 1) return fail(who + "unite(0,1) must set parent[0] = 1 (root of the first argument goes under the second), got parent[0] = " + got(*a, 0));
        if (!a->unite(2, 0)) return fail(who + "unite(2,0) must return true");
        if (a->parentOf(2) != 1) return fail(who + "unite(2,0) must set parent[2] = 1, got parent[2] = " + got(*a, 2));
        if (a->sizeAtRoot(1) != 3) return fail(who + "sizeAtRoot(1) must be 3, got " + std::to_string(a->sizeAtRoot(1)));
        return pass();
    }
    // equal ranks: first argument's root wins
    if (!a->unite(0, 1)) return fail(who + "unite(0,1) on fresh elements must return true");
    if (a->parentOf(1) != 0 || a->parentOf(0) != 0)
        return fail(who + "tie: unite(0,1) must set parent[1] = 0 (first root wins), got parent[1] = " + got(*a, 1));
    if (a->rankOf(0) != 1) return fail(who + "tie: rank of the winning root must become 1, got " + std::to_string(a->rankOf(0)));
    a->unite(2, 3);
    if (a->parentOf(3) != 2 || a->rankOf(2) != 1) return fail(who + "tie: unite(2,3) must set parent[3] = 2 and rank[2] = 1");
    a->unite(0, 2);
    if (a->parentOf(2) != 0) return fail(who + "tie of rank-1 roots: unite(0,2) must set parent[2] = 0, got parent[2] = " + got(*a, 2));
    if (a->rankOf(0) != 2) return fail(who + "tie of rank-1 roots: rank of the winner must become 2, got " + std::to_string(a->rankOf(0)));
    if (a->sizeAtRoot(0) != 4) return fail(who + "sizeAtRoot(0) must be 4, got " + std::to_string(a->sizeAtRoot(0)));

    // lower rank under higher rank, for either argument position; the higher rank is unchanged
    std::unique_ptr<perc::UnionFind> b = make(8);
    b->unite(0, 1);                                       // root 0, rank 1
    b->unite(4, 0);                                       // first argument has the lower rank
    if (b->parentOf(4) != 0 || b->rankOf(0) != 1)
        return fail(who + "unite(4,0), rank[4] < rank[0]: 4 must go under 0 and rank[0] must stay 1");
    b->unite(0, 5);                                       // second argument has the lower rank
    if (b->parentOf(5) != 0 || b->rankOf(0) != 1)
        return fail(who + "unite(0,5), rank[0] > rank[5]: 5 must go under 0 and rank[0] must stay 1");
    return pass();
}

}  // namespace perc_check
