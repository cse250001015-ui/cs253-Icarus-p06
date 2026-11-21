// tests/test_checkers.cpp -- proves the U1..U5 checkers accept valid states and reject
// hand-built corrupt ones, and that each injected UF bug is caught by the intended checker.
#include "checkers.hpp"
#include "ref_uf.hpp"
#include <cassert>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace perc_check;
using perc::Variant;
using perc_test::RefUF;
using perc_test::UfBug;

// Test double whose state is set directly, so corrupt states can be built by hand.
class ArrayUF : public perc::UnionFind {
public:
    ArrayUF(std::vector<int> parent, std::vector<int> rank, std::vector<int> size)
        : perc::UnionFind(static_cast<int>(parent.size())), p_(std::move(parent)), r_(std::move(rank)), s_(std::move(size)) {}
    int find(int) override { throw std::logic_error("ArrayUF: find unsupported"); }
    bool unite(int, int) override { throw std::logic_error("ArrayUF: unite unsupported"); }
    int size(int) override { throw std::logic_error("ArrayUF: size unsupported"); }
    const char* name() const override { return "array"; }
    int parentOf(int x) const override { return p_[x]; }
    int rankOf(int x) const override { return r_[x]; }
    int sizeAtRoot(int r) const override { return s_[r]; }
private:
    std::vector<int> p_, r_, s_;
};

static void expectFail(const CheckResult& r, const char* tag) {
    if (r.ok || r.msg.find(tag) == std::string::npos) {
        std::fprintf(stderr, "expected a failure mentioning %s, got ok=%d msg='%s'\n", tag, r.ok, r.msg.c_str());
        assert(false);
    }
}
static const Variant kAll[4] = {Variant::Naive, Variant::Rank, Variant::Compression, Variant::RankCompression};

// Valid states: random unites on every variant; all checks pass after every single unite.
static void testValidStates() {
    for (Variant v : kAll) {
        const int n = 40;
        RefUF uf(n, v);
        std::vector<int> cls(n);                       // independent reference partition
        for (int i = 0; i < n; ++i) cls[i] = i;
        uint64_t s = 99;
        for (int k = 0; k < 300; ++k) {
            s = s * 6364136223846793005ULL + 1442695040888963407ULL;
            const int a = static_cast<int>((s >> 33) % n);
            s = s * 6364136223846793005ULL + 1442695040888963407ULL;
            const int b = static_cast<int>((s >> 33) % n);
            CheckResult u4 = checkU4Unite(uf, a, b);
            assert(u4.ok);
            if (cls[a] != cls[b]) { const int cb = cls[b]; for (int& c : cls) if (c == cb) c = cls[a]; }
            assert(checkU1(uf).ok && checkU2(uf).ok && checkU3(uf, cls).ok && checkU5(uf, v).ok);
            uf.find(static_cast<int>((s >> 20) % n));  // finds (and compression) keep it valid too
            assert(checkU1(uf).ok && checkU2(uf).ok && checkU3(uf, cls).ok && checkU5(uf, v).ok);
        }
    }
}

static void testU1() {
    expectFail(checkU1(ArrayUF({0, 5, 2}, {-1,-1,-1}, {1,1,1})), "U1");     // parent out of range
    expectFail(checkU1(ArrayUF({0, -1, 2}, {-1,-1,-1}, {1,1,1})), "U1");    // negative parent
    expectFail(checkU1(ArrayUF({1, 0, 2}, {-1,-1,-1}, {1,1,1})), "U1");     // cycle 0 <-> 1
    expectFail(checkU1(ArrayUF({1, 2, 0}, {-1,-1,-1}, {1,1,1})), "U1");     // cycle of length 3
    assert(checkU1(ArrayUF({0, 0, 1}, {-1,-1,-1}, {3,1,1})).ok);            // chain 2->1->0
}
static void testU2() {
    expectFail(checkU2(ArrayUF({0, 0, 2}, {-1,-1,-1}, {1, 1, 1})), "U2");   // root 0 has 2 elements, stores 1
    expectFail(checkU2(ArrayUF({0, 0, 2}, {-1,-1,-1}, {3, 1, 1})), "U2");   // stores 3, has 2
    expectFail(checkU2(ArrayUF({0, 1, 2}, {-1,-1,-1}, {1, 1, 2})), "U2");   // singleton root claims 2
    assert(checkU2(ArrayUF({0, 0, 2}, {-1,-1,-1}, {2, 1, 1})).ok);
}
static void testU3() {
    ArrayUF uf({0, 0, 2, 2}, {-1,-1,-1,-1}, {2, 1, 2, 1});                    // sets {0,1} {2,3}
    assert(checkU3(uf, {0, 0, 2, 2}).ok);
    expectFail(checkU3(uf, {0, 0, 0, 0}), "U3");                              // should be merged
    expectFail(checkU3(uf, {0, 1, 2, 2}), "U3");                              // should be separate
    expectFail(checkU3(uf, {0, 0, 2}), "U3");                                 // wrong length
    expectFail(checkU3(uf, {0, 0, 2, 9}), "U3");                              // class id out of range
}
static void testU4() {
    {   RefUF uf(6, Variant::Rank, UfBug::FalseTrueOnSameSet);
        assert(checkU4Unite(uf, 0, 1).ok);
        expectFail(checkU4Unite(uf, 0, 1), "U4");                             // true on same set
    }
    {   RefUF uf(8, Variant::Rank, UfBug::SkipUnionCount);
        assert(checkU4Unite(uf, 0, 1).ok && checkU4Unite(uf, 2, 3).ok);
        expectFail(checkU4Unite(uf, 4, 5), "U4");                             // 3rd merge not counted
    }
    {   RefUF uf(6, Variant::Naive);                                          // window check, healthy
        UfSnapshot s0 = snapshot(uf);
        uf.unite(0, 1); uf.unite(1, 2); uf.unite(0, 2);
        assert(checkU4Window(s0, uf).ok);
    }
    {   RefUF uf(8, Variant::Naive, UfBug::SkipUnionCount);                   // window check, buggy
        UfSnapshot s0 = snapshot(uf);
        uf.unite(0, 1); uf.unite(2, 3); uf.unite(4, 5);
        expectFail(checkU4Window(s0, uf), "U4");
    }
}
static void testU5() {
    // Naive keeps no rank: a stored rank is a violation.
    expectFail(checkU5(ArrayUF({0, 0}, {0, -1}, {2, 1}), Variant::Naive), "U5");
    assert(checkU5(ArrayUF({0, 0}, {-1, -1}, {2, 1}), Variant::Compression).ok);
    // Rank: rank must equal height.
    expectFail(checkU5(ArrayUF({0, 0}, {0, 0}, {2, 1}), Variant::Rank), "U5");        // height 1, rank 0
    expectFail(checkU5(ArrayUF({0, 0, 0, 0}, {2, 0, 0, 0}, {4, 1, 1, 1}), Variant::Rank), "U5"); // rank 2, height 1
    assert(checkU5(ArrayUF({0, 0}, {1, 0}, {2, 1}), Variant::Rank).ok);
    // size >= 2^rank
    expectFail(checkU5(ArrayUF({0, 1}, {1, 0}, {1, 1}), Variant::RankCompression), "U5");
    // RankCompression: height may be below rank, never above.
    assert(checkU5(ArrayUF({0, 0, 0, 0}, {2, 0, 0, 0}, {4, 1, 1, 1}), Variant::RankCompression).ok);
    expectFail(checkU5(ArrayUF({0, 0}, {0, 0}, {2, 1}), Variant::RankCompression), "U5");
    // Injected bug: tie without rank increment.
    for (Variant v : {Variant::Rank, Variant::RankCompression}) {
        RefUF uf(4, v, UfBug::RankNoIncrement);
        uf.unite(0, 1);
        expectFail(checkU5(uf, v), "U5");
    }
    // Injected bug: size not updated, caught by U2.
    RefUF uf(4, Variant::Naive, UfBug::NoSizeUpdate);
    uf.unite(0, 1);
    expectFail(checkU2(uf), "U2");
}

// L1: the linking-direction checker accepts a correct structure and rejects a reversed one,
// while the partition checkers (U1-U5) accept the reversed one: exactly the gap L1 closes.
static void testLinkingRule() {
    for (Variant v : kAll) {
        auto good = [v](int n) { return std::unique_ptr<perc::UnionFind>(new RefUF(n, v)); };
        auto rev  = [v](int n) { return std::unique_ptr<perc::UnionFind>(new RefUF(n, v, UfBug::LinkReversed)); };
        assert(checkLinkingRule(v, good).ok);
        expectFail(checkLinkingRule(v, rev), "L1");
        // the reversed structure still satisfies every partition/rank invariant
        RefUF r(10, v, UfBug::LinkReversed);
        std::vector<int> cls(10);
        for (int i = 0; i < 10; ++i) cls[i] = i;
        const int pairs[4][2] = {{0, 1}, {2, 3}, {0, 2}, {5, 6}};
        for (auto& pr : pairs) {
            r.unite(pr[0], pr[1]);
            const int cb = cls[pr[1]];
            for (int& c : cls) if (c == cb) c = cls[pr[0]];
        }
        assert(checkU1(r).ok && checkU2(r).ok && checkU3(r, cls).ok && checkU5(r, v).ok);
    }
    // a rank variant that forgets the "lower under higher" rule
    auto sneaky = [](int n) { return std::unique_ptr<perc::UnionFind>(new RefUF(n, Variant::Naive)); };
    expectFail(checkLinkingRule(Variant::Rank, sneaky), "L1");   // naive linking has no ranks at all
}

int main() {
    testLinkingRule();
    testValidStates(); testU1(); testU2(); testU3(); testU4(); testU5();
    std::puts("checkers: all tests passed (valid states accepted, corrupt states and injected bugs rejected)");
    return 0;
}
