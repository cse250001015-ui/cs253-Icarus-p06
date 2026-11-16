#include "../src/structA/rank_compression_uf.hpp"
#include "../include/sweep.hpp"

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace perc;

void testFindCountersAndCompression() {
    RankCompressionUF uf(4);

    // Form two rank-1 trees.
    uf.unite(0, 1);
    uf.unite(2, 3);

    // Equal-rank merge: root 2 becomes a child of root 0.
    uf.unite(0, 2);

    // Path: 3 -> 2 -> 0
    assert(uf.parentOf(3) == 2);
    assert(uf.parentOf(2) == 0);

    uf.resetCounters();

    // First find crosses two parent links.
    assert(uf.find(3) == 0);
    assert(uf.counters().finds == 1);
    assert(uf.counters().hops == 2);

    // Path compression makes 3 point directly to 0.
    assert(uf.parentOf(3) == 0);

    uf.resetCounters();

    // Second find crosses only one link.
    assert(uf.find(3) == 0);
    assert(uf.counters().finds == 1);
    assert(uf.counters().hops == 1);

    std::cout << "PASS: find counters and path compression\n";
}

void testTieBreakingAndSizes() {
    RankCompressionUF uf(6);

    // Union of two rank-0 roots: first root must win.
    assert(uf.unite(0, 1));
    assert(uf.parentOf(1) == 0);
    assert(uf.rankOf(0) == 1);

    assert(uf.unite(2, 3));
    assert(uf.parentOf(3) == 2);

    // Equal-rank union: first argument's root wins.
    assert(uf.unite(0, 2));
    assert(uf.parentOf(2) == 0);
    assert(uf.rankOf(0) == 2);

    // Duplicate union should return false.
    assert(!uf.unite(0, 3));

    // All four sites are in the same component.
    assert(uf.size(0) == 4);
    assert(uf.size(1) == 4);
    assert(uf.size(2) == 4);
    assert(uf.size(3) == 4);

    // Other sites remain singleton components.
    assert(uf.size(4) == 1);
    assert(uf.size(5) == 1);

    // Root metadata must agree with component size.
    assert(uf.sizeAtRoot(0) == 4);

    std::cout << "PASS: tie-breaking, duplicate unions and sizes\n";
}

void testUnionCounters() {
    RankCompressionUF uf(3);
    uf.resetCounters();

    assert(uf.unite(0, 1));
    assert(uf.counters().unions == 1);
    assert(uf.counters().finds == 2);

    // Both sites already belong to the same component.
    assert(!uf.unite(0, 1));
    assert(uf.counters().unions == 1);
    assert(uf.counters().finds == 4);

    std::cout << "PASS: union counters\n";
}

void testSweepEmptyCallback() {
    std::vector<int> order = {0, 2, 1, 3};

    // Empty callback must not cause a crash.
    SweepCallback cb;

    SweepResult result = runSweepOrder(
        2, Variant::RankCompression, order, cb, true, false
    );

    // The fully opened 2x2 grid must percolate.
    assert(result.nCritical >= 1);
    assert(result.nCritical <= 4);

    std::cout << "PASS: sweep with empty callback\n";
}

void testSweepPartialOrder() {
    // In a 3x3 grid, these open only the top row.
    std::vector<int> order = {0, 1, 2};

    std::vector<SweepRecord> records;

    SweepResult result = runSweepOrder(
        3,
        Variant::RankCompression,
        order,
        [&](const SweepRecord& record) {
            records.push_back(record);
        },
        true,
        false
    );

    // No top-to-bottom path can exist.
    assert(result.nCritical == -1);
    assert(records.size() == 3);

    for (int i = 0; i < 3; ++i) {
        assert(records[i].n == i + 1);
        assert(!records[i].percolated);
        assert(records[i].largestCluster == i + 1);
    }

    std::cout << "PASS: sweep with partial opening order\n";
}

void testSweepDuplicateIndices() {
    // Opening site 0 twice is invalid.
    std::vector<int> order = {0, 1, 0};

    bool caught = false;

    try {
        runSweepOrder(
            2,
            Variant::RankCompression,
            order,
            SweepCallback{},
            true,
            false
        );
    }
    catch (const std::invalid_argument&) {
        caught = true;
    }

    assert(caught);

    std::cout << "PASS: duplicate opening indices rejected\n";
}

int main() {
    testFindCountersAndCompression();
    testTieBreakingAndSizes();
    testUnionCounters();

    testSweepEmptyCallback();
    testSweepPartialOrder();
    testSweepDuplicateIndices();

    std::cout << "\nAll Structure A tests passed!\n";
    return 0;
}
