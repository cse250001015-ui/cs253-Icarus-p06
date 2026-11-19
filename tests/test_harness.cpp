// tests/test_harness.cpp -- (1) the differential harness passes on a correct stand-in for all
// four variants; (2) it detects every injected bug (mutation testing); (3) the shrinker reduces
// every failing episode to a short 1-minimal sequence that still fails the same way; (4) a
// failing episode replays alone from its seed.
#include "differential.hpp"
#include "ref_percolation.hpp"
#include <cassert>
#include <cstdio>
#include <memory>

using namespace perc_test;
using perc::Variant;

static const Variant kAll[4] = {Variant::Naive, Variant::Rank, Variant::Compression, Variant::RankCompression};

struct Maker {
    PercBug pb;
    UfBug ub;
    std::unique_ptr<RefPercolation> operator()(int L, Variant v, bool track) const {
        return std::make_unique<RefPercolation>(L, v, track, pb, ub);
    }
};

static DiffConfig configFor(Variant v, bool track) {
    DiffConfig cfg;
    cfg.seed = 7; cfg.totalOps = 30000; cfg.variant = v; cfg.trackClusters = track; cfg.maxL = 8;
    return cfg;
}

// 1-minimal: removing any single operation makes the failure disappear or change kind.
static bool isOneMinimal(const Shrunk& s, const DiffConfig& cfg, const Maker& mk) {
    DiffConfig c = cfg; c.checkEvery = 1;
    const std::string kind = failureKind(s.message);
    for (size_t i = 0; i < s.ops.size(); ++i) {
        std::vector<Op> cand = s.ops;
        cand.erase(cand.begin() + i);
        const std::string m = replayOps<RefPercolation>(mk, s.L, cand, c);
        if (!m.empty() && failureKind(m) == kind) return false;
    }
    return true;
}

int main() {
    // 1. A correct implementation passes, for every variant, with and without cluster tracking.
    for (Variant v : kAll) {
        for (bool track : {true, false}) {
            DiffConfig cfg = configFor(v, track);
            DiffResult r = runDifferential<RefPercolation>(cfg, Maker{PercBug::None, UfBug::None});
            if (!r.ok) std::fprintf(stderr, "unexpected failure (%s): %s\n", vname(v), r.message.c_str());
            assert(r.ok);
            std::printf("PASS  %-14s track=%d  ops=%llu episodes=%llu\n", vname(v), track,
                        (unsigned long long)r.opsRun, (unsigned long long)r.episodes);
        }
    }

    // 2+3. Every injected bug must be detected, and its failing episode must shrink.
    struct Mutant { const char* name; Variant v; PercBug pb; UfBug ub; };
    const Mutant mutants[] = {
        {"backwash (BOTTOM in ufFull)",        Variant::RankCompression, PercBug::Backwash,        UfBug::None},
        {"largest cluster read from ufFull",   Variant::RankCompression, PercBug::ClusterFromFull, UfBug::None},
        {"grid wraps around",                  Variant::RankCompression, PercBug::WrapAround,      UfBug::None},
        {"left neighbour forgotten",           Variant::RankCompression, PercBug::SkipLeft,        UfBug::None},
        {"bottom row not united to BOTTOM",    Variant::RankCompression, PercBug::NoBottomUnite,   UfBug::None},
        {"a site closes again (P1)",           Variant::RankCompression, PercBug::ClosesSite,      UfBug::None},
        {"largest cluster shrinks (P6)",       Variant::RankCompression, PercBug::LargestShrinks,  UfBug::None},
        {"ufClust joins diagonals (P2)",       Variant::RankCompression, PercBug::ClustDiagonal,   UfBug::None},
        {"UF: size not updated (naive)",       Variant::Naive,           PercBug::None, UfBug::NoSizeUpdate},
        {"UF: size not updated (rank)",        Variant::Rank,            PercBug::None, UfBug::NoSizeUpdate},
        {"UF: rank not incremented (rank)",    Variant::Rank,            PercBug::None, UfBug::RankNoIncrement},
        {"UF: rank not incremented (rank+pc)", Variant::RankCompression, PercBug::None, UfBug::RankNoIncrement},
        {"UF: every 3rd union not counted",    Variant::Compression,     PercBug::None, UfBug::SkipUnionCount},
        {"UF: unite true on same set",         Variant::Naive,           PercBug::None, UfBug::FalseTrueOnSameSet},
    };
    int detected = 0;
    size_t longest = 0;
    for (const Mutant& m : mutants) {
        const Maker mk{m.pb, m.ub};
        const DiffConfig cfg = configFor(m.v, true);
        DiffResult r = runDifferential<RefPercolation>(cfg, mk);
        if (r.ok) std::fprintf(stderr, "MISSED mutant: %s\n", m.name);
        assert(!r.ok);
        ++detected;

        Shrunk s = shrinkFailure<RefPercolation>(mk, r.L, r.failingOps, cfg);
        assert(s.reproduced);
        assert(!s.ops.empty() && s.ops.size() <= r.failingOps.size() && s.L <= r.L);
        assert(failureKind(s.message) == failureKind(r.message));      // same bug, not a different one
        DiffConfig c1 = cfg; c1.checkEvery = 1;
        const std::string again = replayOps<RefPercolation>(mk, s.L, s.ops, c1);
        assert(!again.empty() && failureKind(again) == failureKind(r.message));   // still fails when replayed
        assert(isOneMinimal(s, cfg, mk));
        if (s.ops.size() > longest) longest = s.ops.size();
        std::printf("CAUGHT %-36s after %5llu ops  shrunk %3zu ops on L=%d -> %zu ops on L=%d (%llu replays)\n   %s\n",
                    m.name, (unsigned long long)r.opsRun, r.failingOps.size(), r.L, s.ops.size(), s.L,
                    (unsigned long long)s.replays, s.message.c_str());
    }
    std::printf("shrinker: every failing case reduced to a 1-minimal sequence; longest is %zu ops\n", longest);
    assert(longest <= 40);     // a readable bug-log entry, never the original hundreds of operations

    // 4. A failing episode replays alone from its seed and fails the same way.
    const Maker bk{PercBug::Backwash, UfBug::None};
    DiffResult bad = runDifferential<RefPercolation>(configFor(Variant::RankCompression, true), bk);
    DiffResult again = replayEpisode<RefPercolation>(configFor(Variant::RankCompression, true), bk, bad.episodeSeed);
    assert(!again.ok && again.message == bad.message && again.failingOps.size() == bad.failingOps.size());
    std::puts("replay: failing episode reproduces from its seed");

    std::printf("harness: all tests passed (%d/%d mutants caught and shrunk)\n", detected, detected);
    return 0;
}
