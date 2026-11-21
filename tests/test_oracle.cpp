// tests/test_oracle.cpp  --  hand-worked examples for FloodFillOracle (Nov 8 task).
// Build WITHOUT -DNDEBUG so assert is active.
#include "flood_fill.hpp"
#include <cassert>
#include <cstdio>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

using perc::FloodFillOracle;

static void openAll(FloodFillOracle& o, std::vector<std::pair<int,int>> cells) {
    for (auto& p : cells) o.open(p.first, p.second);
}

// Example 1: L=1. Closed -> nothing; open -> full and percolating at once.
static void testL1() {
    FloodFillOracle o(1);
    assert(!o.percolates() && !o.isFull(0, 0));
    assert(o.open(0, 0));
    assert(o.isFull(0, 0) && o.percolates());
    assert(o.largestCluster() == 1);
}

// Example 2: L=3, middle column open. Every site of the column is full; percolates.
static void testColumn() {
    FloodFillOracle o(3);
    openAll(o, {{0,1},{1,1}});
    assert(!o.percolates());
    o.open(2, 1);
    assert(o.percolates());
    for (int r = 0; r < 3; ++r) assert(o.isFull(r, 1));
    assert(!o.isFull(0, 0));           // a closed site is never full
}

// Example 3: diagonal sites are NOT neighbours.
static void testDiagonal() {
    FloodFillOracle o(3);
    openAll(o, {{0,0},{1,1},{2,2}});
    assert(o.isFull(0, 0));
    assert(!o.isFull(1, 1) && !o.isFull(2, 2));
    assert(!o.percolates());
}

// Example 4: no wraparound, horizontally and vertically.
static void testNoWrap() {
    FloodFillOracle h(3);              // (1,2) would touch (1,0) if columns wrapped
    openAll(h, {{0,0},{1,0},{1,2}});
    assert(h.isFull(1, 0));
    assert(!h.isFull(1, 2));
    FloodFillOracle v(3);              // (2,1) would touch (0,1) if rows wrapped
    openAll(v, {{0,1},{2,1}});
    assert(v.isFull(0, 1) && !v.isFull(2, 1));
    assert(!v.percolates());
}

// Example 5: BACKWASH grid. L=4, column 1 open -> percolates. Then open (3,3): its
// neighbours (2,3) and (3,2) are closed, so it is NOT full. A single union-find holding
// both virtual nodes would wrongly report it full.
static void testBackwash() {
    FloodFillOracle o(4);
    for (int r = 0; r < 4; ++r) o.open(r, 1);
    assert(o.percolates());
    o.open(3, 3);
    assert(!o.isFull(3, 3));
    assert(o.isFull(3, 1));
    assert(o.percolates());            // still true after the extra opening
    o.open(3, 0);                      // adjacent to the full site (3,1): now full
    assert(o.isFull(3, 0));
    o.open(3, 2);                      // joins (3,1) and (3,3): (3,3) becomes full
    assert(o.isFull(3, 3));
}

// Example 6: snake path (0,0)(1,0)(1,1)(1,2)(2,2) on L=3.
static void testSnake() {
    FloodFillOracle o(3);
    openAll(o, {{0,0},{1,0},{1,1},{1,2}});
    assert(!o.percolates());
    assert(o.isFull(1, 2));
    o.open(2, 2);
    assert(o.percolates() && o.isFull(2, 2));
}

// Example 7: an isolated open site in the middle is not full.
static void testIsolated() {
    FloodFillOracle o(3);
    o.open(1, 1);
    assert(!o.isFull(1, 1) && !o.percolates());
}

static void testOpenTwiceAndRange() {
    FloodFillOracle o(3);
    assert(o.open(1, 1));
    assert(!o.open(1, 1));
    assert(o.numOpen() == 1);
    bool threw = false;
    try { o.open(3, 0); } catch (const std::out_of_range&) { threw = true; }
    assert(threw);
    threw = false;
    try { o.isFull(0, -1); } catch (const std::out_of_range&) { threw = true; }
    assert(threw);
    threw = false;
    try { o.isOpen(-1, 0); } catch (const std::out_of_range&) { threw = true; }
    assert(threw);
}

static void testConstructorRange() {
    bool threw = false;
    try { FloodFillOracle o(0); } catch (const std::invalid_argument&) { threw = true; }
    assert(threw);
    threw = false;
    try { FloodFillOracle o(46341); } catch (const std::invalid_argument&) { threw = true; }
    assert(threw);
}

// Example 8: largest cluster. L=4: a cluster of 3 and a cluster of 2, then merged.
static void testLargestCluster() {
    FloodFillOracle o(4);
    assert(o.largestCluster() == 0);
    openAll(o, {{0,0},{0,1},{1,1}});   // size 3
    openAll(o, {{3,3},{3,2}});         // size 2
    assert(o.largestCluster() == 3);
    openAll(o, {{2,2},{2,1}});         // (1,1)-(2,1)-(2,2)-(3,2)-(3,3) joins them
    assert(o.largestCluster() == 7);
}

// Example 9 (the section 4.1 trap): open (0,0) then (0,2) on L=3. Both touch the top row
// but are NOT adjacent, so the largest cluster is 1, not 2.
static void testTopRowTrap() {
    FloodFillOracle o(3);
    openAll(o, {{0,0},{0,2}});
    assert(o.largestCluster() == 1);
    assert(o.isFull(0, 0) && o.isFull(0, 2));
}

// Component labels: smallest index per component, -1 for closed sites.
static void testLabels() {
    FloodFillOracle o(3);
    openAll(o, {{0,0},{1,0},{2,2},{1,2}});
    std::vector<int> lab = o.componentLabels();
    assert(lab.size() == 9);
    assert(lab[0] == 0 && lab[3] == 0);            // (0,0),(1,0) -> label 0
    assert(lab[5] == 5 && lab[8] == 5);            // (1,2),(2,2) -> label 5
    assert(lab[1] == -1 && lab[4] == -1);
}

// Capacity check of the array queue: everything open on a bigger grid.
static void testBigFullGrid() {
    const int L = 300;
    FloodFillOracle o(L);
    for (int r = 0; r < L; ++r)
        for (int c = 0; c < L; ++c) o.open(r, c);
    assert(o.percolates());
    assert(o.largestCluster() == L * L);
    assert(o.isFull(L - 1, L - 1));
    std::vector<int> lab = o.componentLabels();
    for (int i = 0; i < L * L; ++i) assert(lab[i] == 0);
}

// Random sanity, fixed seed, Fisher-Yates order as in INTERFACE.md 5.1.
static void testRandomMonotone() {
    const int L = 5;
    std::mt19937_64 rng(12345);
    std::vector<int> a(L * L);
    for (int i = 0; i < L * L; ++i) a[i] = i;
    for (int i = L * L - 1; i >= 1; --i) std::swap(a[i], a[rng() % (i + 1)]);

    FloodFillOracle o(L);
    int flips = 0, prevLargest = 0;
    bool prev = false;
    for (int k = 0; k < L * L; ++k) {
        o.open(a[k] / L, a[k] % L);
        assert(o.numOpen() == k + 1);
        bool now = o.percolates();
        assert(!(prev && !now));                  // never goes true -> false
        if (now && !prev) ++flips;
        prev = now;
        int lc = o.largestCluster();
        assert(lc >= prevLargest);                // P6: never decreases
        prevLargest = lc;
        for (int r = 0; r < L; ++r)
            for (int c = 0; c < L; ++c) {
                if (o.isFull(r, c)) assert(o.isOpen(r, c));
                if (r == 0 && o.isOpen(r, c)) assert(o.isFull(r, c));
            }
    }
    assert(flips == 1 && prev);
    assert(o.largestCluster() == L * L);
}

int main() {
    testL1(); testColumn(); testDiagonal(); testNoWrap(); testBackwash();
    testSnake(); testIsolated(); testOpenTwiceAndRange(); testConstructorRange();
    testLargestCluster(); testTopRowTrap(); testLabels(); testBigFullGrid();
    testRandomMonotone();
    std::puts("oracle: all hand-worked tests passed");
    return 0;
}
