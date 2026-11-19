// tests/test_naive_uf.cpp  (Structure B owner)
// Unit tests for perc::NaiveUF against INTERFACE.md v0.2.
// Run with: make test (asserts must stay enabled; do not build with -DNDEBUG)
#include "structB/naive_uf.hpp"
#include "ref_partition.hpp"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <random>
#include <vector>

using perc::NaiveUF;

static int rootViaParentOf(const NaiveUF& uf, int x) {
    int steps = 0;
    while (uf.parentOf(x) != x) {
        x = uf.parentOf(x);
        ++steps;
        assert(steps <= uf.numElements() - 1);   // U1: no cycles
    }
    return x;
}

static void test_fresh_state() {
    NaiveUF uf(10);
    assert(uf.numElements() == 10);
    for (int x = 0; x < 10; ++x) {
        assert(uf.parentOf(x) == x);
        assert(uf.sizeAtRoot(x) == 1);
        assert(uf.rankOf(x) == -1);
    }
    for (int x = 0; x < 10; ++x) {
        assert(uf.find(x) == x);
        assert(uf.size(x) == 1);
    }
}

static void test_unite_return_values() {
    NaiveUF uf(6);
    assert(uf.unite(0, 1) == true);
    assert(uf.unite(0, 1) == false);   // repeat
    assert(uf.unite(1, 0) == false);   // symmetric repeat
    assert(uf.unite(2, 2) == false);   // self
    assert(uf.unite(1, 2) == true);
    assert(uf.unite(0, 2) == false);   // already joined transitively
    assert(uf.counters().unions == 2); // only successful merges counted
}

static void test_linking_rule() {
    // INTERFACE 3.1: Naive does parent[ra] = rb.
    NaiveUF uf(4);
    assert(uf.unite(0, 1));
    assert(uf.parentOf(0) == 1);       // root of a goes under root of b
    assert(uf.find(0) == 1);
    assert(uf.unite(2, 0));            // ra = 2, rb = find(0) = 1
    assert(uf.parentOf(2) == 1);
    assert(uf.find(2) == 1);
    assert(uf.sizeAtRoot(1) == 3);
}

static void test_connected_symmetric_transitive() {
    NaiveUF uf(8);
    uf.unite(0, 1); uf.unite(1, 2); uf.unite(5, 6);
    assert(uf.connected(0, 2) && uf.connected(2, 0));
    assert(uf.connected(5, 6) && uf.connected(6, 5));
    assert(!uf.connected(0, 5) && !uf.connected(5, 0));
    assert(!uf.connected(3, 4));
    assert(uf.connected(7, 7));
}

static void test_counters_single_unite() {
    NaiveUF uf(4);
    assert(uf.unite(0, 1));
    const auto& c = uf.counters();
    assert(c.finds == 2);              // two finds inside unite
    assert(c.hops == 0);               // both were roots
    assert(c.unions == 1);
}

static void test_counters_size_and_connected() {
    NaiveUF uf(4);
    uf.unite(0, 1);                    // parent[0] = 1
    uf.resetCounters();
    assert(uf.size(0) == 2);           // size() makes one find; 0 -> 1 is one hop
    assert(uf.counters().finds == 1 && uf.counters().hops == 1);
    uf.resetCounters();
    assert(uf.connected(0, 1));        // connected() makes two finds
    assert(uf.counters().finds == 2 && uf.counters().hops == 1);
    assert(uf.counters().unions == 0);
}

static void test_reset_counters() {
    NaiveUF uf(5);
    uf.unite(0, 1); uf.unite(1, 2); uf.find(0);
    uf.resetCounters();
    assert(uf.counters().finds == 0 && uf.counters().hops == 0 && uf.counters().unions == 0);
}

static void test_debug_accessors_are_free() {
    NaiveUF uf(6);
    uf.unite(0, 1); uf.unite(1, 2); uf.unite(3, 4);
    uf.resetCounters();
    for (int x = 0; x < 6; ++x) { (void)uf.parentOf(x); (void)uf.rankOf(x); (void)uf.sizeAtRoot(x); }
    assert(uf.counters().finds == 0 && uf.counters().hops == 0 && uf.counters().unions == 0);
    // parentOf must not compress: structure unchanged
    assert(uf.parentOf(0) == 1 && uf.parentOf(1) == 2 && uf.parentOf(2) == 2);
}

static void test_chain_worst_case() {
    // unite(i, i+1) for i = 0..n-2 builds the path 0 -> 1 -> ... -> n-1.
    const int n = 1000;
    NaiveUF uf(n);
    for (int i = 0; i + 1 < n; ++i) assert(uf.unite(i, i + 1));
    assert(uf.counters().unions == (uint64_t)(n - 1));
    for (int i = 0; i + 1 < n; ++i) assert(uf.parentOf(i) == i + 1);
    assert(uf.sizeAtRoot(n - 1) == n);

    uf.resetCounters();
    assert(uf.find(0) == n - 1);
    assert(uf.counters().finds == 1);
    assert(uf.counters().hops == (uint64_t)(n - 1));   // Theta(n) find, no compression

    // No compression: the path is still intact after the find.
    for (int i = 0; i + 1 < n; ++i) assert(uf.parentOf(i) == i + 1);

    // Total hops over find(0..n-1) = (n-1) + (n-2) + ... + 0 = n(n-1)/2, i.e. quadratic.
    uf.resetCounters();
    for (int i = 0; i < n; ++i) uf.find(i);
    assert(uf.counters().hops == (uint64_t)n * (n - 1) / 2);
    assert(uf.counters().finds == (uint64_t)n);
}

static void test_random_differential() {
    // U1, U2, U3, U4 against a brute-force partition, fixed seed.
    const int n = 200;
    std::mt19937_64 rng(12345);
    NaiveUF uf(n);
    RefPartition ref(n);
    uint64_t expectedUnions = 0;

    for (int step = 0; step < 3000; ++step) {
        int a = (int)(rng() % n), b = (int)(rng() % n);
        bool got = uf.unite(a, b);
        bool want = ref.unite(a, b);
        assert(got == want);                      // U4
        if (want) ++expectedUnions;
        assert(uf.counters().unions == expectedUnions);

        if (step % 50 == 0) {
            // U3: same partition as the reference.
            for (int x = 0; x < n; ++x) {
                int y = (int)(rng() % n);
                assert(uf.connected(x, y) == (ref.label[x] == ref.label[y]));
            }
            // U1 and U2: roots reachable, sizes correct and summing to n.
            int total = 0;
            for (int x = 0; x < n; ++x) {
                int r = rootViaParentOf(uf, x);
                assert(uf.parentOf(r) == r);
                assert(uf.sizeAtRoot(r) == ref.sizeOf(x));
                if (uf.parentOf(x) == x) total += uf.sizeAtRoot(x);
            }
            assert(total == n);
        }
    }
}

static void test_single_element() {
    NaiveUF uf(1);
    assert(uf.find(0) == 0);
    assert(uf.size(0) == 1);
    assert(!uf.unite(0, 0));
}

int main() {
    test_fresh_state();
    test_unite_return_values();
    test_linking_rule();
    test_connected_symmetric_transitive();
    test_counters_single_unite();
    test_counters_size_and_connected();
    test_reset_counters();
    test_debug_accessors_are_free();
    test_chain_worst_case();
    test_random_differential();
    test_single_element();
    std::puts("NaiveUF: all tests passed");
    return 0;
}
