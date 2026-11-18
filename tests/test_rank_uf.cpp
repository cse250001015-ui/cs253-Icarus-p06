// tests/test_rank_uf.cpp (Structure B owner). INTERFACE.md v0.3.
// Run with: make test (asserts must stay enabled; do not build with -DNDEBUG)
#include "structB/rank_uf.hpp"
#include "ref_partition.hpp"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <random>
#include <vector>

using perc::RankUF;

static void test_fresh_state() {
    RankUF uf(8);
    for (int x = 0; x < 8; ++x) {
        assert(uf.parentOf(x) == x && uf.rankOf(x) == 0 && uf.sizeAtRoot(x) == 1);
        assert(uf.find(x) == x && uf.size(x) == 1);
    }
}

static void test_tie_rule() {
    // Equal rank: parent[rb] = ra, rank[ra]++ (INTERFACE 3.1, v0.3).
    RankUF uf(8);
    assert(uf.unite(0, 1));
    assert(uf.parentOf(1) == 0 && uf.parentOf(0) == 0 && uf.rankOf(0) == 1);
    assert(uf.unite(2, 3));
    assert(uf.parentOf(3) == 2 && uf.rankOf(2) == 1);
    assert(uf.unite(0, 2));                       // tie again: root 2 goes under root 0
    assert(uf.parentOf(2) == 0 && uf.rankOf(0) == 2);
    assert(uf.sizeAtRoot(0) == 4);
}

static void test_lower_under_higher() {
    RankUF uf(8);
    uf.unite(0, 1);                               // root 0, rank 1
    assert(uf.unite(4, 0));                       // ra=4 (rank 0) < rb=0 (rank 1): 4 under 0
    assert(uf.parentOf(4) == 0 && uf.rankOf(0) == 1);
    assert(uf.unite(0, 5));                       // ra=0 (rank 1) > rb=5 (rank 0): 5 under 0
    assert(uf.parentOf(5) == 0 && uf.rankOf(0) == 1);
    assert(uf.sizeAtRoot(0) == 4);
}

static void test_unite_return_and_counters() {
    RankUF uf(6);
    assert(uf.unite(0, 1));
    assert(uf.counters().finds == 2 && uf.counters().hops == 0 && uf.counters().unions == 1);
    assert(!uf.unite(1, 0));                      // same set, nothing changes
    assert(!uf.unite(3, 3));
    assert(uf.counters().unions == 1);
    uf.resetCounters();
    assert(uf.size(1) == 2);                      // 1 -> 0: one find, one hop
    assert(uf.counters().finds == 1 && uf.counters().hops == 1);
}

static void test_no_compression_and_free_accessors() {
    RankUF uf(8);
    uf.unite(0, 1); uf.unite(2, 3); uf.unite(0, 2);   // 3 -> 2 -> 0
    int before3 = uf.parentOf(3);
    uf.resetCounters();
    for (int x = 0; x < 8; ++x) { (void)uf.parentOf(x); (void)uf.rankOf(x); (void)uf.sizeAtRoot(x); }
    assert(uf.counters().finds == 0 && uf.counters().hops == 0);
    assert(uf.find(3) == 0 && uf.counters().hops == 2);
    assert(uf.parentOf(3) == before3);                // find did not compress
}

static void test_binomial_height_log2n() {
    // Merge equal-rank trees round by round: height reaches log2 n,
    // and find(n-1) walks exactly log2 n hops (the "rank f n-1" generator).
    const int k = 10, n = 1 << k;
    RankUF uf(n);
    for (int s = 1; s < n; s *= 2)
        for (int i = 0; i + s < n; i += 2 * s) assert(uf.unite(i, i + s));
    assert(uf.rankOf(0) == k);
    assert(uf.sizeAtRoot(0) == n);
    uf.resetCounters();
    assert(uf.find(n - 1) == 0);
    assert(uf.counters().hops == (uint64_t)k);
}

static int depthOf(const RankUF& uf, int x) {
    int d = 0;
    while (uf.parentOf(x) != x) { x = uf.parentOf(x); ++d; assert(d <= uf.numElements() - 1); }
    return d;
}

static void test_random_differential_and_U5() {
    const int n = 256;
    std::mt19937_64 rng(2026);
    RankUF uf(n);
    RefPartition ref(n);
    for (int step = 0; step < 2000; ++step) {
        int a = (int)(rng() % n), b = (int)(rng() % n);
        assert(uf.unite(a, b) == ref.unite(a, b));            // U4
        if (step % 25 == 0) {
            std::vector<int> height(n, 0);
            for (int x = 0; x < n; ++x) {                      // roots and heights
                int r = x; while (uf.parentOf(r) != r) r = uf.parentOf(r);
                int d = depthOf(uf, x);
                if (d > height[r]) height[r] = d;
            }
            int total = 0;
            for (int x = 0; x < n; ++x) {
                if (uf.parentOf(x) != x) continue;
                total += uf.sizeAtRoot(x);                      // U2
                assert(uf.sizeAtRoot(x) == ref.sizeOf(x));
                assert(uf.sizeAtRoot(x) >= (1 << uf.rankOf(x))); // U5: size >= 2^rank
                assert(uf.rankOf(x) == height[x]);               // Rank: rank == height
            }
            assert(total == n);
            for (int x = 0; x < n; ++x) {                       // U3 + height <= floor(log2 n)
                int y = (int)(rng() % n);
                assert(uf.connected(x, y) == (ref.label[x] == ref.label[y]));
                assert(depthOf(uf, x) <= 8);                     // height <= floor(log2 256) = 8
            }
        }
    }
}

int main() {
    test_fresh_state();
    test_tie_rule();
    test_lower_under_higher();
    test_unite_return_and_counters();
    test_no_compression_and_free_accessors();
    test_binomial_height_log2n();
    test_random_differential_and_U5();
    std::puts("RankUF: all tests passed");
    return 0;
}
