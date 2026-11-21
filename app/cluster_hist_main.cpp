// app/cluster_hist_main.cpp
// Standalone tool that writes clusters.csv. Piyush can call the library functions from
// `percolation_app --clusters` later; this tool works on its own in the meantime.
//
// Usage: cluster_hist --L 100 [--variant rank_compress|naive|rank|compress|all]
//                     [--seed 1] [--sweeps 1] [--at 0.5,0.5927,0.7] [--out FILE]
//   --at     comma-separated checkpoints. A value with a '.' is a fraction of L*L sites
//            (rounded); otherwise it is a number of open sites. Default: 0.5,0.5927,0.7
//   --out    output file (default: standard output)
// `count` is summed over the sweeps (see cluster_histogram.hpp).
#include "cluster_histogram.hpp"

#include <climits>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <utility>

using namespace perc;

static void usage(const char* prog) {
    std::cerr << "Usage: " << prog << " --L N [--variant V|all] [--seed S] [--sweeps T]"
              << " [--at LIST] [--out FILE]\n";
}

static const int kMaxAt = 64;

// Parses a checkpoint list into ascending distinct values in [1, N]. Returns the count, or -1.
static int parseCheckpoints(const char* spec, int N, int* out) {
    int n = 0;
    const char* p = spec;
    while (*p != '\0') {
        char* end = nullptr;
        double d = std::strtod(p, &end);
        if (end == p || (*end != ',' && *end != '\0') || d < 0) return -1;
        bool fraction = std::memchr(p, '.', static_cast<std::size_t>(end - p)) != nullptr;
        long long v = fraction ? static_cast<long long>(std::llround(d * N)) : static_cast<long long>(d);
        if (v < 1) v = 1;
        if (v > N) v = N;
        if (n == kMaxAt) return -1;
        out[n++] = static_cast<int>(v);
        p = (*end == ',') ? end + 1 : end;
    }
    // insertion sort, then drop duplicates
    for (int i = 1; i < n; ++i)
        for (int j = i; j > 0 && out[j - 1] > out[j]; --j) std::swap(out[j - 1], out[j]);
    int m = 0;
    for (int i = 0; i < n; ++i)
        if (m == 0 || out[m - 1] != out[i]) out[m++] = out[i];
    return m;
}

int main(int argc, char** argv) {
    long long L = 0, sweeps = 1;
    unsigned long long seed = 1;
    const char* variantArg = "all";
    const char* atArg = "0.5,0.5927,0.7";
    const char* outPath = nullptr;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--L") == 0 && i + 1 < argc)            L = std::atoll(argv[++i]);
        else if (std::strcmp(argv[i], "--variant") == 0 && i + 1 < argc) variantArg = argv[++i];
        else if (std::strcmp(argv[i], "--seed") == 0 && i + 1 < argc)    seed = std::strtoull(argv[++i], nullptr, 10);
        else if (std::strcmp(argv[i], "--sweeps") == 0 && i + 1 < argc)  sweeps = std::atoll(argv[++i]);
        else if (std::strcmp(argv[i], "--at") == 0 && i + 1 < argc)      atArg = argv[++i];
        else if (std::strcmp(argv[i], "--out") == 0 && i + 1 < argc)     outPath = argv[++i];
        else { usage(argv[0]); return 1; }
    }
    if (L < 1 || L * L > INT_MAX || sweeps < 1 || sweeps > INT_MAX) { usage(argv[0]); return 1; }

    Variant variants[4];
    int nVariants = 0;
    if (std::strcmp(variantArg, "all") == 0) {
        variants[0] = Variant::RankCompression; variants[1] = Variant::Rank;
        variants[2] = Variant::Compression;     variants[3] = Variant::Naive;
        nVariants = 4;
    } else if (parseVariant(variantArg, variants[0])) {
        nVariants = 1;
    } else {
        std::cerr << "unknown variant '" << variantArg << "'\n";
        return 1;
    }

    const int N = static_cast<int>(L * L);
    int at[kMaxAt];
    int nAt = parseCheckpoints(atArg, N, at);
    if (nAt < 1) { std::cerr << "bad --at list '" << atArg << "'\n"; return 1; }

    std::ofstream file;
    if (outPath != nullptr) {
        file.open(outPath);
        if (!file) { std::cerr << "Cannot open " << outPath << " for writing\n"; return 1; }
    }
    std::ostream& out = (outPath != nullptr) ? static_cast<std::ostream&>(file) : std::cout;

    writeClusterCsvHeader(out);
    for (int vi = 0; vi < nVariants; ++vi) {
        std::unique_ptr<uint64_t[]> counts(new uint64_t[static_cast<std::size_t>(nAt) * (N + 1)]());
        clusterHistogramSweeps(static_cast<int>(L), variants[vi], seed, static_cast<int>(sweeps),
                               at, nAt, counts.get());
        for (int j = 0; j < nAt; ++j)
            writeClusterCsvRows(out, variants[vi], static_cast<int>(L), at[j],
                                counts.get() + static_cast<std::size_t>(j) * (N + 1));
    }
    if (!out) { std::cerr << "Write error\n"; return 1; }
    return 0;
}
