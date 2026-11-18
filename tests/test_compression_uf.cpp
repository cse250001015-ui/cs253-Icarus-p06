// tests/test_compression_uf.cpp (Structure B owner). INTERFACE.md v0.3.
// Run with: make test (asserts must stay enabled; do not build with -DNDEBUG)
#include "structB/compression_uf.hpp"
#include "ref_partition.hpp"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <random>
#include <vector>

using perc::CompressionUF;

static void test_fresh_state_and_rank() {
    CompressionUF uf(6);
    for (int x = 0; x < 6; ++x) {
        assert(uf.parentOf(x) == x && uf.sizeAtRoot(x) == 1 && uf.rankOf(x) == -1);
        assert(uf.find(x) == x && uf.size(x) == 1);
    }
}

static void test_linking_same_as_naive() {
    CompressionUF uf(4);
    assert(uf.unite(0, 1));
    assert(uf.parentOf(0) == 1);                  // parent[ra] = rb
    assert(uf.unite(2, 0));                       // ra = 2, rb = 1
    assert(uf.parentOf(2) == 1);
    assert(!uf.unite(2, 1));
    assert(uf.sizeAtRoot(1) == 3);
    assert(uf.counters().unions == 2);
}

static void test_compression_effect() {
    const int n = 50;
    CompressionUF uf(n);
    for (int i = 0; i + 1 < n; ++i) assert(uf.unite(i, i + 1));  // path 0 -> 1 -> ... -> n-1
    uf.resetCounters();
    assert(uf.find(0) == n - 1);
    assert(uf.counters().finds == 1);
    assert(uf.counters().hops == (uint64_t)(n - 1));              // pass 1 only is counted
    for (int i = 0; i < n - 1; ++i) assert(uf.parentOf(i) == n - 1);  // all repointed to root
    uf.resetCounters();
    assert(uf.find(0) == n - 1);
    assert(uf.counters().hops == 1);                              // now one hop from the root
    uf.resetCounters();
    assert(uf.find(n - 1) == n - 1 && uf.counters().hops == 0);   // root costs 0
}

static void test_partial_path_compression() {
    // 0 -> 1 -> 2 -> 3 (root); a side child 5 -> 2 must stay put.
    CompressionUF uf(6);
    uf.unite(0, 1); uf.unite(1, 2); uf.unite(2, 3); uf.unite(5, 2);
    assert(uf.parentOf(5) == 3);                                  // unite(5,2) compressed 2 -> 3
    assert(uf.find(0) == 3);
    assert(uf.parentOf(0) == 3 && uf.parentOf(1) == 3 && uf.parentOf(2) == 3);
    assert(uf.parentOf(4) == 4);                                  // untouched element
}

static void test_accessors_do_not_compress() {
    CompressionUF uf(5);
    uf.unite(0, 1); uf.unite(1, 2);
    // after unite(1,2): find(1) was a root; parent[0]=1, parent[1]=2
    assert(uf.parentOf(0) == 1 && uf.parentOf(1) == 2);
    uf.resetCounters();
    for (int x = 0; x < 5; ++x) { (void)uf.parentOf(x); (void)uf.rankOf(x); (void)uf.sizeAtRoot(x); }
    assert(uf.counters().finds == 0 && uf.counters().hops == 0);
    assert(uf.parentOf(0) == 1);                                  // still uncompressed
}

static void test_iterative_find_on_huge_chain() {
    // A recursive find would overflow the stack here. INTERFACE 3.1: Compression must be iterative.
    const int n = 1000000;
    CompressionUF uf(n);
    for (int i = 0; i + 1 < n; ++i) uf.unite(i, i + 1);
    uf.resetCounters();
    assert(uf.find(0) == n - 1);
    assert(uf.counters().hops == (uint64_t)(n - 1));
    assert(uf.parentOf(0) == n - 1 && uf.parentOf(n / 2) == n - 1);
}

static void test_random_differential() {
    const int n = 200;
    std::mt19937_64 rng(777);
    CompressionUF uf(n);
    RefPartition ref(n);
    uint64_t unions = 0;
    for (int step = 0; step < 3000; ++step) {
        int a = (int)(rng() % n), b = (int)(rng() % n);
        bool got = uf.unite(a, b), want = ref.unite(a, b);
        assert(got == want);                                      // U4
        if (want) ++unions;
        assert(uf.counters().unions == unions);
        if (step % 50 == 0) {
            int total = 0;
            for (int x = 0; x < n; ++x) {
                int steps = 0, r = x;
                while (uf.parentOf(r) != r) { r = uf.parentOf(r); assert(++steps <= n - 1); }  // U1
                assert(uf.sizeAtRoot(r) == ref.sizeOf(x));                                      // U2
                if (uf.parentOf(x) == x) total += uf.sizeAtRoot(x);
            }
            assert(total == n);
            for (int x = 0; x < n; ++x) {
                int y = (int)(rng() % n);
                assert(uf.connected(x, y) == (ref.label[x] == ref.label[y]));                   // U3
            }
        }
    }
}

int main() {
    test_fresh_state_and_rank();
    test_linking_same_as_naive();
    test_compression_effect();
    test_partial_path_compression();
    test_accessors_do_not_compress();
    test_iterative_find_on_huge_chain();
    test_random_differential();
    std::puts("CompressionUF: all tests passed");
    return 0;
}
