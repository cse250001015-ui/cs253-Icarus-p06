// app/main.cpp -- owner: Architect. The Monte Carlo experiment (INTERFACE.md section 8).
//
// For every variant and grid size it runs T sweeps (Newman-Ziff), records one threshold sample per
// sweep, and writes threshold samples, per-level occupation observables, summary statistics,
// finite-size extrapolation, and optionally cluster-size histograms.
#include "fit.hpp"
#include "replay.hpp"
#include "stats.hpp"
#include "sweep.hpp"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace perc;

namespace {

// Newman & Ziff (2000). Used only as the occupation level at which cluster-size histograms are taken.
constexpr double kPublishedPc = 0.592746;
// Correlation-length exponent of 2D percolation (D6).
constexpr double kNu = 4.0 / 3.0;
constexpr int kMaxL = 46340;   // L*L must fit in int

struct Options {
    std::vector<int>     sizes = {50};
    int                  sweeps = 100;
    std::vector<Variant> variants = {Variant::RankCompression};
    uint64_t             seed = 1;
    std::string          out = "results/raw";
    bool                 clusters = false;
    int                  clusterSweeps = 10;
    int                  naiveMaxL = kMaxL;
    std::string          replay;
};

void usage(const char* prog) {
    std::fprintf(stderr,
        "Usage: %s [--sizes 50,100,200] [--sweeps T] [--variant V] [--seed S] [--out DIR]\n"
        "          [--clusters] [--cluster-sweeps K] [--naive-max-L N] [--replay FILE]\n"
        "  --sizes          comma-separated grid sides, each in [1, %d]   (default 50)\n"
        "  --sweeps         number of sweeps T per size, >= 1             (default 100)\n"
        "  --variant        naive | rank | compress | rank_compress | all (default rank_compress)\n"
        "  --seed           base seed; every sweep's seed is derived from it (default 1)\n"
        "  --out            output directory, created if missing         (default results/raw)\n"
        "  --clusters       also write clusters.csv (cluster sizes at the occupation level 0.592746)\n"
        "  --cluster-sweeps number of sweeps K used for clusters.csv      (default 10)\n"
        "  --naive-max-L    skip Naive sweeps above N (default %d)\n"
        "  --replay         replay a .unions/.order file; --variant all runs all four variants\n",
        prog, kMaxL, kMaxL);
}

bool parseSizes(const char* text, std::vector<int>& out) {
    out.clear();
    const char* p = text;
    while (*p) {
        char* end = nullptr;
        long v = std::strtol(p, &end, 10);
        if (end == p || v < 1 || v > kMaxL) return false;
        out.push_back(static_cast<int>(v));
        p = end;
        if (*p == ',') ++p;
        else if (*p != '\0') return false;
    }
    return !out.empty();
}

bool parsePositiveInt(const char* text, int& out) {
    char* end = nullptr;
    long v = std::strtol(text, &end, 10);
    if (end == text || *end != '\0' || v < 1 || v > 1000000000L) return false;
    out = static_cast<int>(v);
    return true;
}

bool parseArgs(int argc, char** argv, Options& o) {
    for (int i = 1; i < argc; ++i) {
        const char* a = argv[i];
        auto next = [&]() -> const char* { return (i + 1 < argc) ? argv[++i] : nullptr; };
        if (!std::strcmp(a, "--sizes")) {
            const char* v = next();
            if (!v || !parseSizes(v, o.sizes)) return false;
        } else if (!std::strcmp(a, "--sweeps")) {
            const char* v = next();
            if (!v || !parsePositiveInt(v, o.sweeps)) return false;
        } else if (!std::strcmp(a, "--variant")) {
            const char* v = next();
            if (!v) return false;
            if (!std::strcmp(v, "all")) {
                o.variants = {Variant::Naive, Variant::Rank, Variant::Compression, Variant::RankCompression};
            } else {
                Variant var;
                if (!parseVariant(v, var)) return false;
                o.variants = {var};
            }
        } else if (!std::strcmp(a, "--seed")) {
            const char* v = next();
            if (!v) return false;
            char* end = nullptr;
            o.seed = std::strtoull(v, &end, 10);
            if (end == v || *end != '\0') return false;
        } else if (!std::strcmp(a, "--out")) {
            const char* v = next();
            if (!v) return false;
            o.out = v;
        } else if (!std::strcmp(a, "--clusters")) {
            o.clusters = true;
        } else if (!std::strcmp(a, "--cluster-sweeps")) {
            const char* v = next();
            if (!v || !parsePositiveInt(v, o.clusterSweeps)) return false;
        } else if (!std::strcmp(a, "--naive-max-L")) {
            const char* v = next();
            if (!v || !parsePositiveInt(v, o.naiveMaxL) || o.naiveMaxL > kMaxL) return false;
        } else if (!std::strcmp(a, "--replay")) {
            const char* v = next();
            if (!v) return false;
            o.replay = v;
        } else {
            return false;
        }
    }
    return true;
}

// A CSV file that writes its header on creation and closes itself when it goes out of scope.
class Csv {
public:
    Csv(const std::string& path, const char* header) : f_(std::fopen(path.c_str(), "w")) {
        if (!f_) throw std::runtime_error("cannot open " + path + " for writing");
        std::fprintf(f_, "%s\n", header);
    }
    ~Csv() { if (f_) std::fclose(f_); }
    Csv(const Csv&) = delete;
    Csv& operator=(const Csv&) = delete;
    FILE* get() { return f_; }
private:
    FILE* f_;
};

// Cluster sizes at occupation level at_n = floor(0.592746 * L^2), summed over K sweeps.
// Only the sites opened so far are looked at, and sizes are read from the cluster structure
// (the one without virtual nodes) through the read-only accessors.
void writeClusters(Csv& csv, Variant v, int L, const Options& o) {
    const int total = L * L;
    int atN = static_cast<int>(std::floor(kPublishedPc * total));
    if (atN < 1) atN = 1;

    std::vector<long long> hist(static_cast<size_t>(total) + 1, 0);   // hist[s] = number of clusters of size s
    for (int k = 0; k < o.clusterSweeps; ++k) {
        std::vector<int> order = makeOpeningOrder(L, sweepSeed(o.seed, L, k));
        Percolation p(L, v, true);
        for (int i = 0; i < atN; ++i) p.open(order[i] / L, order[i] % L);

        const UnionFind& u = *p.ufClust();
        for (int x = 0; x < total; ++x) {
            // a cluster is counted once, at its root; closed sites are singleton roots and are skipped
            if (u.parentOf(x) == x && p.isOpen(x / L, x % L)) ++hist[u.sizeAtRoot(x)];
        }
    }
    for (size_t s = 1; s < hist.size(); ++s) {
        if (hist[s] > 0)
            std::fprintf(csv.get(), "%s,%d,%d,%zu,%lld\n", variantName(v), L, atN, s, hist[s]);
    }
}

int run(const Options& o) {
    std::printf("application settings: sweeps=%d seed=%llu; Naive capped at L<=%d\n",
                o.sweeps, static_cast<unsigned long long>(o.seed), o.naiveMaxL);
    std::error_code ec;
    std::filesystem::create_directories(o.out, ec);
    if (ec) throw std::runtime_error("cannot create directory " + o.out + ": " + ec.message());
    std::filesystem::create_directories(o.out + "/occupation", ec);
    if (ec) throw std::runtime_error("cannot create occupation output directory: " + ec.message());

    Csv sweepsCsv(o.out + "/sweeps.csv",
        "variant,L,sweep,seed,n_critical,p_hat,finds_perc,hops_perc,unions_perc,"
        "finds_full,hops_full,unions_full,finds_clust,hops_clust,unions_clust,time_ns");
    Csv summaryCsv(o.out + "/summary.csv", "variant,L,T,mean_p,sd,se,ci_low,ci_high");
    Csv extrapCsv(o.out + "/extrapolation.csv", "variant,nu,p_c,a,p_c_se");
    std::unique_ptr<Csv> clustersCsv;
    if (o.clusters) clustersCsv = std::make_unique<Csv>(o.out + "/clusters.csv", "variant,L,at_n,size,count");

    for (Variant v : o.variants) {
        std::vector<double> Ls, means, ses;

        for (int L : o.sizes) {
            if (v == Variant::Naive && L > o.naiveMaxL) {
                std::printf("%-13s L=%-5d skipped (--naive-max-L=%d)\n",
                            variantName(v), L, o.naiveMaxL);
                continue;
            }
            std::vector<double> samples;
            samples.reserve(o.sweeps);
            const int totalSites = L * L;
            std::vector<uint64_t> percolatingAt(static_cast<size_t>(totalSites) + 1, 0);
            std::vector<uint64_t> largestClusterSum(static_cast<size_t>(totalSites) + 1, 0);

            for (int k = 0; k < o.sweeps; ++k) {
                const uint64_t seed = sweepSeed(o.seed, L, k);

                const SweepCallback collectOccupation = [&](const SweepRecord& record) {
                    if (record.percolated) ++percolatingAt[record.n];
                    largestClusterSum[record.n] += static_cast<uint64_t>(record.largestCluster);
                };
                auto t0 = std::chrono::steady_clock::now();
                // Complete the Newman-Ziff sweep so every occupation level is recorded.
                SweepResult r = runSweep(L, v, seed, collectOccupation, true, false);
                auto t1 = std::chrono::steady_clock::now();
                long long ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();

                if (r.nCritical < 1) throw std::runtime_error("sweep never percolated (internal error)");
                const double pHat = static_cast<double>(r.nCritical) / (static_cast<double>(L) * L);
                samples.push_back(pHat);

                std::fprintf(sweepsCsv.get(),
                    "%s,%d,%d,%llu,%d,%.12g,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%lld\n",
                    variantName(v), L, k, static_cast<unsigned long long>(seed), r.nCritical, pHat,
                    (unsigned long long)r.perc.finds, (unsigned long long)r.perc.hops, (unsigned long long)r.perc.unions,
                    (unsigned long long)r.full.finds, (unsigned long long)r.full.hops, (unsigned long long)r.full.unions,
                    (unsigned long long)r.clust.finds, (unsigned long long)r.clust.hops, (unsigned long long)r.clust.unions,
                    ns);
            }

            const std::string occupationPath = o.out + "/occupation/occupation_" +
                variantName(v) + "_L" + std::to_string(L) + ".csv";
            Csv occupationCsv(occupationPath,
                "n,p,percolation_probability,mean_largest_cluster");
            for (int n = 1; n <= totalSites; ++n) {
                const double p = static_cast<double>(n) / totalSites;
                const double percProbability = static_cast<double>(percolatingAt[n]) / o.sweeps;
                const double meanLargest = static_cast<double>(largestClusterSum[n]) / o.sweeps;
                std::fprintf(occupationCsv.get(), "%d,%.8g,%.6g,%.6g\n",
                             n, p, percProbability, meanLargest);
            }

            Summary s = summarize(samples);
            std::fprintf(summaryCsv.get(), "%s,%d,%d,%.12g,%.12g,%.12g,%.12g,%.12g\n",
                         variantName(v), L, s.T, s.mean, s.sd, s.se, s.ciLow, s.ciHigh);
            std::printf("%-13s L=%-5d T=%-4d p_c estimate = %.5f +/- %.5f  (95%% CI)\n",
                        variantName(v), L, s.T, s.mean, 1.96 * s.se);

            Ls.push_back(L);
            means.push_back(s.mean);
            ses.push_back(s.se);

            if (clustersCsv) writeClusters(*clustersCsv, v, L, o);
        }

        if (Ls.empty()) {
            std::printf("%-13s no sizes at or below the configured Naive cap; no estimates written\n",
                        variantName(v));
            continue;
        }

        // extrapolate to an infinite lattice: p(L) = p_c + a * L^(-1/nu)
        FitResult f = fitFiniteSize(Ls, means, ses, kNu);
        if (f.ok) {
            std::fprintf(extrapCsv.get(), "%s,%.12g,%.12g,%.12g,%.12g\n", variantName(v), kNu, f.pc, f.a, f.pcSe);
            std::printf("%-13s extrapolated p_c = %.5f +/- %.5f  (published value %.6f)%s\n",
                        variantName(v), f.pc, 1.96 * f.pcSe, kPublishedPc,
                        f.weighted ? "" : "  [unweighted fit: some size had zero spread]");
        } else {
            std::printf("%-13s extrapolation skipped: it needs at least two different sizes\n", variantName(v));
        }
    }
    std::printf("wrote %s/{sweeps,summary,extrapolation%s}.csv and per-level data in %s/occupation/\n",
                o.out.c_str(), o.clusters ? ",clusters" : "", o.out.c_str());
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    Options o;
    if (!parseArgs(argc, argv, o)) {
        usage(argv[0]);
        return 1;
    }
    if (!o.replay.empty()) {
        try {
            if (o.variants.size() == 4)
                return replayFileAll(o.replay, std::cout, std::cerr);
            return replayFile(o.replay, o.variants.front(), std::cout, std::cerr);
        } catch (const std::exception& e) {
            std::fprintf(stderr, "replay error: %s\n", e.what());
            return 1;
        }
    }
    try {
        return run(o);
    } catch (const std::logic_error& e) {
        std::fprintf(stderr, "error: %s\n(if this says 'not implemented', a Structure A or B module is still a stub)\n", e.what());
        return 2;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
}
