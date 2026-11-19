// tests/test_pcheckers.cpp -- P1..P6 accept a healthy state and reject hand-built bad ones.
#include "percolation_checkers.hpp"
#include "ref_uf.hpp"
#include <cassert>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

using namespace perc_check;
using perc::FloodFillOracle;
using perc::Variant;
using perc_test::RefUF;

// Percolation look-alike whose observable state is set directly by the test.
struct Scripted {
    int L_;
    std::vector<char> open, full;
    bool perc = false;
    int largest = 0, numOpenVal = 0;
    RefUF uperc, ufull, uclust;
    Scripted(int L, int fullSize)
        : L_(L), open(L * L, 0), full(L * L, 0),
          uperc(L * L + 2, Variant::RankCompression), ufull(fullSize, Variant::RankCompression),
          uclust(L * L, Variant::RankCompression) {}
    bool isOpen(int r, int c) const { return open[r * L_ + c] != 0; }
    bool isFull(int r, int c) { return full[r * L_ + c] != 0; }
    bool percolates() { return perc; }
    int numOpen() const { return numOpenVal; }
    int largestCluster() const { return largest; }
    const perc::UnionFind& ufPerc() const { return uperc; }
    const perc::UnionFind& ufFull() const { return ufull; }
    const perc::UnionFind* ufClust() const { return &uclust; }
};

static void expectFail(const CheckResult& r, const char* tag) {
    if (r.ok || r.msg.find(tag) == std::string::npos) {
        std::fprintf(stderr, "expected failure mentioning %s, got ok=%d msg='%s'\n", tag, r.ok, r.msg.c_str());
        assert(false);
    }
}

// L=3 with (0,1),(1,1) open: both full, no percolation, largest cluster 2.
static void healthy(Scripted& s, FloodFillOracle& o) {
    s.open[1] = s.open[4] = 1;  s.full[1] = s.full[4] = 1;
    s.numOpenVal = 2;  s.largest = 2;
    o.open(0, 1); o.open(1, 1);
    s.ufull.unite(1, 9); s.ufull.unite(4, 1);          // TOP = 9
    s.uperc.unite(1, 9); s.uperc.unite(4, 1);
    s.uclust.unite(1, 4);
}

int main() {
    const int L = 3;
    {   Scripted s(L, L * L + 1); FloodFillOracle o(L); healthy(s, o);
        PercMonitor<Scripted> m(L);
        assert(m.checkAll(s, o, true).ok);
    }
    {   // P5: ufFull with a BOTTOM element
        Scripted s(L, L * L + 2); FloodFillOracle o(L); healthy(s, o);
        PercMonitor<Scripted> m(L);
        expectFail(m.checkP5(s), "P5");
    }
    {   // P1: a site closes again; and numOpen disagreeing with the flags
        Scripted s(L, L * L + 1); FloodFillOracle o(L); healthy(s, o);
        PercMonitor<Scripted> m(L);
        assert(m.checkP1(s).ok);
        s.open[4] = 0;
        expectFail(m.checkP1(s), "P1");
        Scripted t(L, L * L + 1); FloodFillOracle o2(L); healthy(t, o2);
        PercMonitor<Scripted> m2(L);
        t.numOpenVal = 5;
        expectFail(m2.checkP1(t), "P1");
    }
    {   // P2: connected in ufClust but not in ufFull
        Scripted s(L, L * L + 1); FloodFillOracle o(L);
        s.open[0] = s.open[2] = 1; s.numOpenVal = 2;
        s.uclust.unite(0, 2);
        PercMonitor<Scripted> m(L);
        expectFail(m.checkP2(s), "P2");
        // and connected in ufFull but not in ufPerc
        Scripted t(L, L * L + 1);
        t.open[0] = t.open[2] = 1;
        t.ufull.unite(0, 2);
        PercMonitor<Scripted> m2(L);
        expectFail(m2.checkP2(t), "P2");
    }
    {   // P3: closed site reported full; open site wrongly not full / wrongly full
        Scripted s(L, L * L + 1); FloodFillOracle o(L); healthy(s, o);
        PercMonitor<Scripted> m(L);
        s.full[8] = 1;
        expectFail(m.checkP3(s, o), "P3");
        Scripted t(L, L * L + 1); FloodFillOracle o2(L); healthy(t, o2);
        t.full[4] = 0;
        expectFail(m.checkP3(t, o2), "P3");
    }
    {   // P4: percolates() true with no full bottom-row site
        Scripted s(L, L * L + 1); FloodFillOracle o(L); healthy(s, o);
        PercMonitor<Scripted> m(L);
        s.perc = true;
        expectFail(m.checkP4(s, o), "P4");
    }
    {   // P6: largest cluster wrong; decreasing; percolated true -> false
        Scripted s(L, L * L + 1); FloodFillOracle o(L); healthy(s, o);
        PercMonitor<Scripted> m(L);
        assert(m.checkP6(s, o, true).ok);
        s.largest = 1;
        expectFail(m.checkP6(s, o, true), "P6");               // decreased from 2 to 1
        Scripted t(L, L * L + 1); FloodFillOracle o2(L); healthy(t, o2);
        PercMonitor<Scripted> m2(L);
        t.largest = 3;
        expectFail(m2.checkP6(t, o2, true), "P6");             // not the true size
        Scripted u(L, L * L + 1); FloodFillOracle o3(L); healthy(u, o3);
        PercMonitor<Scripted> m3(L);
        u.perc = true;  (void)m3.checkP6(u, o3, true);         // records percolated = true
        u.perc = false;
        expectFail(m3.checkP6(u, o3, true), "P6");
    }
    std::puts("pcheckers: all tests passed (healthy state accepted, bad states rejected)");
    return 0;
}
