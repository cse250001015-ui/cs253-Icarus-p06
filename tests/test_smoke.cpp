// tests/test_smoke.cpp -- owner: Architect. Checks the pieces that exist today.
#include "sweep.hpp"
#include <algorithm>
#include <cstdio>
#include <stdexcept>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); ++failures; } } while (0)

int main() {
    using namespace perc;

    // splitmix64: first output for state 0 is a published reference value
    CHECK(splitmix64(0) == 0xE220A8397B1DCDAFULL);

    // opening order: a permutation, deterministic, depends on the seed
    auto a = makeOpeningOrder(10, 1), b = makeOpeningOrder(10, 1), c = makeOpeningOrder(10, 2);
    CHECK(a == b);
    CHECK(a != c);
    auto sorted = a; std::sort(sorted.begin(), sorted.end());
    bool perm = sorted.size() == 100;
    for (int i = 0; i < 100 && perm; ++i) perm = (sorted[i] == i);
    CHECK(perm);
    CHECK(sweepSeed(7, 50, 3) == sweepSeed(7, 50, 3));
    CHECK(sweepSeed(7, 50, 3) != sweepSeed(7, 50, 4));
    CHECK(sweepSeed(7, 50, 4) != sweepSeed(8, 50, 3));   // neighbouring base seeds must not share sweeps

    // union-find through the factory
    auto uf = makeUnionFind(Variant::Naive, 10);
    CHECK(uf->numElements() == 10);
    CHECK(uf->unite(0, 1));
    CHECK(!uf->unite(0, 1));
    CHECK(uf->connected(0, 1));
    CHECK(!uf->connected(0, 2));
    CHECK(uf->size(0) == 2);
    CHECK(uf->counters().unions == 1);
    CHECK(uf->parentOf(0) == 1 && uf->parentOf(1) == 1);   // naive rule: root(a) under root(b)
    CHECK(uf->sizeAtRoot(1) == 2);

    // percolation plumbing
    Percolation p(3, Variant::RankCompression);
    CHECK(p.L() == 3 && p.numOpen() == 0 && !p.isOpen(0, 0));
    CHECK(p.ufPerc().numElements() == 11 && p.ufFull().numElements() == 10);
    CHECK(p.ufClust() != nullptr && p.ufClust()->numElements() == 9);
    bool threw = false;
    try { p.isOpen(3, 0); } catch (const std::out_of_range&) { threw = true; }
    CHECK(threw);
    Percolation q(3, Variant::Naive, false);
    CHECK(q.ufClust() == nullptr);

    // constructor guards: L must be in [1, 46340], checked before any allocation
    bool big = false, zero = false;
    try { Percolation bad(46341, Variant::Naive, false); } catch (const std::invalid_argument&) { big = true; }
    try { Percolation bad(0, Variant::Naive, false); } catch (const std::invalid_argument&) { zero = true; }
    CHECK(big);
    CHECK(zero);

    if (failures == 0) std::puts("test_smoke: all checks passed");
    return failures ? 1 : 0;
}
