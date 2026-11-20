// oracle/sweep_main.cpp -- differential test of the Newman-Ziff sweep (runSweepOrder) against
// the flood-fill oracle, plus invariants S1-S3. Needs the REAL sweep code, so it is built only
// by the oracle-real target.
//   ./sweep_test --sweeps 2000 --seed 20260928 [--maxL 12]
// For every sweep k: draw L, a flags combination (trackClusters, stopAtPercolation) and an
// order (a full opening order from makeOpeningOrder, or a random prefix of one), run all four
// variants on that same order and check
//   * each SweepRecord against the oracle replayed on the same order  (percolated, largestCluster)
//   * S1, S3 per variant, S2 across variants
//   * the number of records (stops right after the first percolation iff stopAtPercolation)
//   * an empty callback gives the same nCritical, counters stay zero for clust when untracked
// plus: makeOpeningOrder is a permutation and is deterministic; bad orders are rejected.
#include "flood_fill.hpp"
#include "sweep.hpp"
#include "sweep_checkers.hpp"
#include "differential.hpp"   // TestRng, mix64
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>

using namespace perc;
using perc_test::TestRng;

static std::string checkOrderBasics(int L, uint64_t seed) {
    std::vector<int> a = makeOpeningOrder(L, seed), b = makeOpeningOrder(L, seed);
    if (a != b) return "makeOpeningOrder is not deterministic";
    if (static_cast<int>(a.size()) != L * L) return "makeOpeningOrder has the wrong length";
    std::vector<char> seen(L * L, 0);
    for (int x : a) {
        if (x < 0 || x >= L * L || seen[x]) return "makeOpeningOrder is not a permutation of 0..L*L-1";
        seen[x] = 1;
    }
    return "";
}

static std::string oneSweep(uint64_t base, int k, int maxL) {
    TestRng rng(perc_test::mix64(base ^ static_cast<uint64_t>(k) * 0x9E3779B97F4A7C15ULL));
    const int L = 1 + static_cast<int>(rng.below(maxL));
    const bool track = rng.below(2) == 0, stop = rng.below(2) == 0;
    const uint64_t seed = sweepSeed(base, L, k);
    std::string m = checkOrderBasics(L, seed);
    if (!m.empty()) return m;
    std::vector<int> order = makeOpeningOrder(L, seed);
    if (rng.below(3) == 0) order.resize(rng.below(order.size() + 1));      // a prefix of the grid

    const Variant vs[4] = {Variant::Naive, Variant::Rank, Variant::Compression, Variant::RankCompression};
    std::vector<std::vector<SweepRecord>> all;
    std::vector<std::string> names;
    for (Variant v : vs) {
        std::vector<SweepRecord> rec;
        SweepResult res = runSweepOrder(L, v, order, [&](const SweepRecord& r) { rec.push_back(r); }, track, stop);
        const std::string who = std::string(perc_test::vname(v)) + " (L=" + std::to_string(L) + ", sweep " +
                                std::to_string(k) + ", track=" + std::to_string(track) + ", stop=" + std::to_string(stop) + "): ";
        perc_check::CheckResult c;
        if (!(c = perc_check::checkS1(rec))) return who + c.msg;
        if (!(c = perc_check::checkS3(rec, res.nCritical))) return who + c.msg;

        FloodFillOracle orc(L);
        size_t expected = order.size();
        for (size_t i = 0; i < order.size(); ++i) {
            orc.open(order[i] / L, order[i] % L);
            if (orc.percolates() && stop) { expected = i + 1; break; }
        }
        if (rec.size() != expected)
            return who + "produced " + std::to_string(rec.size()) + " records, expected " + std::to_string(expected);
        FloodFillOracle orc2(L);
        for (size_t i = 0; i < rec.size(); ++i) {
            orc2.open(order[i] / L, order[i] % L);
            if (rec[i].percolated != orc2.percolates()) return who + "record " + std::to_string(i) + ": percolated differs from the oracle";
            const int want = track ? orc2.largestCluster() : 0;
            if (rec[i].largestCluster != want)
                return who + "record " + std::to_string(i) + ": largestCluster " + std::to_string(rec[i].largestCluster) +
                       ", oracle " + std::to_string(want);
        }
        if (!track && (res.clust.finds != 0 || res.clust.hops != 0 || res.clust.unions != 0))
            return who + "clust counters are not zero although clusters are not tracked";
        SweepResult quiet = runSweepOrder(L, v, order, SweepCallback{}, track, stop);
        if (quiet.nCritical != res.nCritical) return who + "an empty callback changed nCritical";
        all.push_back(rec);
        names.push_back(perc_test::vname(v));
    }
    perc_check::CheckResult c = perc_check::checkS2(all, names);
    if (!c) return "(L=" + std::to_string(L) + ", sweep " + std::to_string(k) + ") " + c.msg;
    return "";
}

static std::string badOrders() {
    bool dup = false, range = false;
    try { runSweepOrder(3, Variant::RankCompression, std::vector<int>{0, 1, 0}, SweepCallback{}); }
    catch (const std::invalid_argument&) { dup = true; }
    try { runSweepOrder(3, Variant::RankCompression, std::vector<int>{0, 9}, SweepCallback{}); }
    catch (const std::out_of_range&) { range = true; }
    if (!dup) return "a duplicate site index was not rejected with std::invalid_argument";
    if (!range) return "an out-of-range site index was not rejected with std::out_of_range";
    return "";
}

int main(int argc, char** argv) {
    uint64_t sweeps = 2000, seed = 20260928;
    int maxL = 12;
    for (int i = 1; i + 1 < argc; i += 2) {
        const std::string a = argv[i];
        if (a == "--sweeps") sweeps = std::strtoull(argv[i + 1], nullptr, 10);
        else if (a == "--seed") seed = std::strtoull(argv[i + 1], nullptr, 10);
        else if (a == "--maxL") maxL = std::atoi(argv[i + 1]);
        else { std::fprintf(stderr, "unknown argument %s\n", a.c_str()); return 2; }
    }
    std::printf("sweep differential test: %llu sweeps x 4 variants, seed=%llu, maxL=%d\n",
                (unsigned long long)sweeps, (unsigned long long)seed, maxL);
    std::string m = badOrders();
    if (!m.empty()) { std::printf("FAIL %s\nSWEEP TEST FAILED\n", m.c_str()); return 1; }
    for (uint64_t k = 0; k < sweeps; ++k) {
        m = oneSweep(seed, static_cast<int>(k), maxL);
        if (!m.empty()) { std::printf("FAIL %s\nSWEEP TEST FAILED\n", m.c_str()); return 1; }
    }
    std::puts("SWEEP TEST PASSED");
    return 0;
}
