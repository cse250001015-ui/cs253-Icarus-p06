// oracle/diff_main.cpp -- command-line differential test.
//   ./diff_test --ops 1000000 --seed 20260928 [--variant all|naive|rank|compress|rank_compress]
//               [--maxL 10] [--check-every 1] [--no-clusters]
//               [--replay-episode SEED] [--shrink-out FILE]
//
// Every operation of every episode is applied to the structure AND to the flood-fill oracle,
// and after it the full state is compared (isOpen/isFull for every site, percolates,
// largestCluster) and the invariants P1-P6 and U1-U5 are checked. Per variant it also runs the
// linking-rule check L1. On a failure the failing episode is shrunk (ddmin + smaller grids) to a
// 1-minimal operation sequence that fails the same way, and printed.
//
//   --no-clusters : build Percolation with trackClusters = false (covers the untracked path:
//                   ufClust() must be null and largestCluster() must throw std::logic_error)
//   --maxL / --check-every : deeper runs, e.g. --maxL 40 --check-every 10 for taller union-find trees
//
// Default build drives the stand-in RefPercolation. Compile with -DUSE_REAL (and -Iinclude -Isrc
// plus the real sources) to drive perc::Percolation: that path compiles and passes against the
// real Structure A (rank_compress, 10^6 operations, seed 20260928); run all four variants once
// every variant's source is on main.
#include "differential.hpp"
#ifdef USE_REAL
#include "percolation.hpp"
#else
#include "ref_percolation.hpp"
#endif
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <memory>
#include <string>

using namespace perc_test;
using perc::Variant;

#ifdef USE_REAL
using SUT = perc::Percolation;
static auto makeSut = [](int L, Variant v, bool track) { return std::make_unique<perc::Percolation>(L, v, track); };
static std::unique_ptr<perc::UnionFind> makeUf(Variant v, int n) { return perc::makeUnionFind(v, n); }
static const char* kSutName = "perc::Percolation (real)";
#else
using SUT = RefPercolation;
static auto makeSut = [](int L, Variant v, bool track) { return std::make_unique<RefPercolation>(L, v, track); };
static std::unique_ptr<perc::UnionFind> makeUf(Variant v, int n) { return std::unique_ptr<perc::UnionFind>(new RefUF(n, v)); }
static const char* kSutName = "RefPercolation (stand-in)";
#endif

static bool parseVariantName(const std::string& s, Variant& v) {
    if (s == "naive") v = Variant::Naive;
    else if (s == "rank") v = Variant::Rank;
    else if (s == "compress") v = Variant::Compression;
    else if (s == "rank_compress") v = Variant::RankCompression;
    else return false;
    return true;
}

static void printCase(FILE* f, const Shrunk& s, const DiffConfig& cfg, uint64_t episodeSeed, const char* indent) {
    std::fprintf(f, "%sgrid L=%d, variant=%s, trackClusters=%d, base seed=%llu, episode seed=%llu\n", indent, s.L,
                 vname(cfg.variant), cfg.trackClusters, (unsigned long long)cfg.seed, (unsigned long long)episodeSeed);
    for (size_t i = 0; i < s.ops.size(); ++i) std::fprintf(f, "%s  %zu: %s\n", indent, i, opToString(s.ops[i]).c_str());
    std::fprintf(f, "%s=> %s\n", indent, s.message.c_str());
}

int main(int argc, char** argv) {
    DiffConfig cfg;
    std::string variantArg = "all", shrinkOut;
    bool replay = false;
    uint64_t replaySeed = 0;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        auto need = [&](const char* name) -> const char* {
            if (i + 1 >= argc) { std::fprintf(stderr, "missing value for %s\n", name); std::exit(2); }
            return argv[++i];
        };
        if (a == "--ops") cfg.totalOps = std::strtoull(need("--ops"), nullptr, 10);
        else if (a == "--seed") cfg.seed = std::strtoull(need("--seed"), nullptr, 10);
        else if (a == "--maxL") cfg.maxL = std::atoi(need("--maxL"));
        else if (a == "--check-every") cfg.checkEvery = std::strtoull(need("--check-every"), nullptr, 10);
        else if (a == "--variant") variantArg = need("--variant");
        else if (a == "--no-clusters") cfg.trackClusters = false;
        else if (a == "--shrink-out") shrinkOut = need("--shrink-out");
        else if (a == "--replay-episode") { replay = true; replaySeed = std::strtoull(need("--replay-episode"), nullptr, 10); }
        else { std::fprintf(stderr, "unknown argument %s\n", a.c_str()); return 2; }
    }
    if (cfg.checkEvery == 0 || cfg.maxL < 1) { std::fprintf(stderr, "bad --check-every or --maxL\n"); return 2; }

    std::vector<Variant> variants;
    if (variantArg == "all") variants = {Variant::Naive, Variant::Rank, Variant::Compression, Variant::RankCompression};
    else { Variant v; if (!parseVariantName(variantArg, v)) { std::fprintf(stderr, "unknown variant\n"); return 2; } variants = {v}; }

    std::printf("differential test: %s vs FloodFillOracle, seed=%llu, ops=%llu per variant, maxL=%d, check-every=%llu, clusters=%s\n",
                kSutName, (unsigned long long)cfg.seed, (unsigned long long)cfg.totalOps, cfg.maxL,
                (unsigned long long)cfg.checkEvery, cfg.trackClusters ? "tracked" : "NOT tracked");
    bool allOk = true;
    for (Variant v : variants) {
        cfg.variant = v;

        // L1: direction of linking (INTERFACE.md 3.1), checked once per variant on fresh instances.
        const perc_check::CheckResult link = perc_check::checkLinkingRule(v, [v](int n) { return makeUf(v, n); });
        if (!link) {
            allOk = false;
            std::printf("  %-14s FAIL %s\n", vname(v), link.msg.c_str());
            continue;
        }

        DiffResult r = replay ? replayEpisode<SUT>(cfg, makeSut, replaySeed) : runDifferential<SUT>(cfg, makeSut);
        if (r.ok) {
            std::printf("  %-14s OK   ops=%llu episodes=%llu (linking rule L1 ok)\n", vname(v), (unsigned long long)r.opsRun,
                        (unsigned long long)r.episodes);
            continue;
        }
        allOk = false;
        std::printf("  %-14s FAIL %s\n", vname(v), r.message.c_str());
        std::printf("    replay the whole episode with: --variant %s --maxL %d%s --replay-episode %llu   (grid L=%d, %zu ops)\n",
                    vname(v), cfg.maxL, cfg.trackClusters ? "" : " --no-clusters", (unsigned long long)r.episodeSeed, r.L,
                    r.failingOps.size());
        Shrunk s = shrinkFailure<SUT>(makeSut, r.L, r.failingOps, cfg);
        if (!s.reproduced) {
            std::printf("    shrinker: the episode did not fail when replayed op by op (check-every 1); not shrunk\n");
            continue;
        }
        std::printf("    minimal failing case: %zu operation(s) on a %dx%d grid (shrunk from %zu ops, %llu replays)\n",
                    s.ops.size(), s.L, s.L, r.failingOps.size(), (unsigned long long)s.replays);
        printCase(stdout, s, cfg, r.episodeSeed, "      ");
        if (!shrinkOut.empty()) {
            FILE* f = std::fopen(shrinkOut.c_str(), "a");
            if (f) { printCase(f, s, cfg, r.episodeSeed, ""); std::fprintf(f, "\n"); std::fclose(f); }
            else std::fprintf(stderr, "cannot write %s\n", shrinkOut.c_str());
        }
    }
    std::puts(allOk ? "DIFFERENTIAL TEST PASSED" : "DIFFERENTIAL TEST FAILED");
    return allOk ? 0 : 1;
}
