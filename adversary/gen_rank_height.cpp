// adversary/gen_rank_height.cpp
// Writes a .unions file that builds a binomial tree under Rank linking,
// so the deepest node sits at depth log2(n), the maximum U5 allows.
//
// NOTE: this sequence is tied to the fixed linking rules of INTERFACE.md
// (v0.3, section 3.1): on equal rank, parent[rb] = ra and rank[ra]++. That
// tie direction is what makes node n-1 the deepest. If section 3.1 changes
// (e.g. the tie direction flips), revisit this generator.
//
// Usage: gen_rank_height --n 1024 --out rank_height_1024.unions
//                        [--finds K | --union-only]
//   --n          number of elements, must be a power of two (>= 2)
//   --out        output file
//   --finds      number of "f n-1" lines after the tree is built (default: n)
//   --union-only write only the "u" lines, no "f" lines (same as --finds 0).
//                Useful as a baseline: every unite joins two roots, so the
//                build costs 0 hops and all the find cost comes from the
//                "f n-1" lines. Cannot be combined with --finds K for K > 0.

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>

namespace perc {

static void usage(const char* prog) {
    std::cerr << "Usage: " << prog
              << " --n N --out FILE [--finds K | --union-only]\n"
              << "  N must be a power of two (>= 2)\n";
}

int run(int argc, char** argv) {
    long long n = 0;
    long long finds = -1;          // -1 means "use n"
    bool unionOnly = false;
    const char* outPath = nullptr;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--n") == 0 && i + 1 < argc) {
            n = std::atoll(argv[++i]);
        } else if (std::strcmp(argv[i], "--finds") == 0 && i + 1 < argc) {
            finds = std::atoll(argv[++i]);
        } else if (std::strcmp(argv[i], "--union-only") == 0) {
            unionOnly = true;
        } else if (std::strcmp(argv[i], "--out") == 0 && i + 1 < argc) {
            outPath = argv[++i];
        } else {
            usage(argv[0]);
            return 1;
        }
    }

    if (n < 2 || (n & (n - 1)) != 0 || outPath == nullptr) {
        usage(argv[0]);
        return 1;
    }
    if (unionOnly) {
        if (finds > 0) {
            std::cerr << "--union-only cannot be combined with --finds K > 0\n";
            usage(argv[0]);
            return 1;
        }
        finds = 0;
    }
    if (finds < 0) finds = n;

    std::ofstream out(outPath);
    if (!out) {
        std::cerr << "Cannot open " << outPath << " for writing\n";
        return 1;
    }

    int k = 0;
    while ((1LL << k) < n) ++k;    // n = 2^k

    out << "# rank-height: binomial tree, deepest node n-1 at depth log2(n)\n";
    out << "# n=" << n << " log2n=" << k << " finds=" << finds
        << (unionOnly ? " (union-only)" : "") << "\n";
    out << "# linking rules: INTERFACE.md v0.3 section 3.1 (Rank tie: parent[rb] = ra)\n";
    out << n << "\n";

    // Round s: every multiple of 2^s is a root of rank s. Uniting two such
    // roots is a tie, so parent[rb] = ra and rank[ra] becomes s+1.
    // After k rounds, one tree of height k remains, rooted at 0.
    for (int s = 0; s < k; ++s) {
        long long half = 1LL << s;
        for (long long i = 0; i + half < n; i += 2 * half) {
            out << "u " << i << " " << (i + half) << "\n";
        }
    }

    // Node n-1 has depth popcount(n-1) = k, the deepest in the tree.
    // Rank never compresses, so every find costs k hops.
    for (long long j = 0; j < finds; ++j) {
        out << "f " << (n - 1) << "\n";
    }

    if (!out) {
        std::cerr << "Write error on " << outPath << "\n";
        return 1;
    }
    return 0;
}

}  // namespace perc

int main(int argc, char** argv) {
    return perc::run(argc, argv);
}
