// tests/test_cluster_histogram.cpp
// Checks app/cluster_histogram against an independent flood-fill count, for all four variants.
// Built without -DNDEBUG so assert is active. Tests may use std containers.
#include "../app/cluster_histogram.hpp"
#include "../include/sweep.hpp"

#include <cassert>
#include <iostream>
#include <sstream>
#include <vector>

using namespace perc;

static const Variant kVariants[] = {Variant::Naive, Variant::Rank, Variant::Compression,
                                    Variant::RankCompression};

// Independent reference: BFS over open flags, histogram of component sizes.
static std::vector<uint64_t> bruteForce(int L, const std::vector<char>& open) {
    std::vector<uint64_t> h(static_cast<std::size_t>(L) * L + 1, 0);
    std::vector<char> seen(open.size(), 0);
    std::vector<int> stack;
    for (int s = 0; s < L * L; ++s) {
        if (!open[s] || seen[s]) continue;
        int size = 0;
        stack.assign(1, s);
        seen[s] = 1;
        while (!stack.empty()) {
            int x = stack.back();
            stack.pop_back();
            ++size;
            int r = x / L, c = x % L;
            int nb[4] = {r > 0 ? x - L : -1, r < L - 1 ? x + L : -1, c > 0 ? x - 1 : -1, c < L - 1 ? x + 1 : -1};
            for (int y : nb)
                if (y >= 0 && open[y] && !seen[y]) { seen[y] = 1; stack.push_back(y); }
        }
        ++h[size];
    }
    return h;
}

static void testExampleFromInterface() {
    // INTERFACE.md 4.1: on 3x3, open (0,0) then (0,2). Two clusters of size 1, not one of size 2.
    for (Variant v : kVariants) {
        Percolation p(3, v, true);
        p.open(0, 0);
        p.open(0, 2);
        std::vector<uint64_t> h(10, 0);
        assert(addClusterSizes(p, h.data()));
        assert(h[1] == 2);
        for (int s = 2; s <= 9; ++s) assert(h[s] == 0);
    }
}

static void testAgainstBruteForce() {
    const int L = 8;
    for (uint64_t seed = 1; seed <= 20; ++seed) {
        std::vector<int> order = makeOpeningOrder(L, sweepSeed(seed, L, 0));
        for (Variant v : kVariants) {
            Percolation p(L, v, true);
            std::vector<char> open(L * L, 0);
            for (int i = 0; i < L * L; ++i) {
                p.open(order[i] / L, order[i] % L);
                open[order[i]] = 1;
                std::vector<uint64_t> got(L * L + 1, 0);
                assert(addClusterSizes(p, got.data()));
                std::vector<uint64_t> want = bruteForce(L, open);
                assert(got == want);
                // consistency with the Percolation layer
                uint64_t cells = 0;
                int largest = 0;
                for (int s = 1; s <= L * L; ++s) {
                    cells += got[s] * s;
                    if (got[s] > 0) largest = s;
                }
                assert(cells == static_cast<uint64_t>(p.numOpen()));
                assert(largest == p.largestCluster());
            }
        }
    }
}

static void testNoTrackingAndNoCounterChange() {
    Percolation off(4, Variant::RankCompression, false);
    off.open(1, 1);
    std::vector<uint64_t> h(17, 0);
    assert(!addClusterSizes(off, h.data()));
    for (uint64_t x : h) assert(x == 0);

    Percolation on(4, Variant::Naive, true);
    on.open(0, 0);
    on.open(0, 1);
    Counters before = on.counters();
    addClusterSizes(on, h.data());
    Counters after = on.counters();
    assert(before.finds == after.finds && before.hops == after.hops && before.unions == after.unions);
}

static void testSweepsAndCsv() {
    const int L = 6, N = L * L;
    const int at[2] = {10, 20};
    std::vector<uint64_t> a(2 * (N + 1), 0), b(2 * (N + 1), 0);
    // same seed, different variants: identical histograms (S2)
    clusterHistogramSweeps(L, Variant::Naive, 7, 5, at, 2, a.data());
    clusterHistogramSweeps(L, Variant::RankCompression, 7, 5, at, 2, b.data());
    assert(a == b);
    for (int j = 0; j < 2; ++j) {
        uint64_t cells = 0;
        for (int s = 1; s <= N; ++s) cells += a[j * (N + 1) + s] * s;
        assert(cells == static_cast<uint64_t>(at[j]) * 5);   // 5 sweeps x at[j] open sites
    }
    std::ostringstream os;
    writeClusterCsvHeader(os);
    writeClusterCsvRows(os, Variant::Rank, L, 10, a.data());
    std::string text = os.str();
    assert(text.rfind("variant,L,at_n,size,count\n", 0) == 0);
    assert(text.find("rank,6,10,") != std::string::npos);
}

int main() {
    testExampleFromInterface();
    testAgainstBruteForce();
    testNoTrackingAndNoCounterChange();
    testSweepsAndCsv();
    std::cout << "test_cluster_histogram: all tests passed\n";
    return 0;
}
