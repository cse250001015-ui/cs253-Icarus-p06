// adversary/gen_naive_quadratic.cpp
// Writes a .unions file that makes Naive linking cost Theta(n^2).
//
// NOTE: this sequence is tied to the fixed linking rules of INTERFACE.md
// (v0.3, section 3.1): Naive links parent[ra] = rb, so "u i i+1" builds the
// chain 0->1->...->n-1. If section 3.1 changes, revisit this generator.
//
// Usage: gen_naive_quadratic --n 1000 --out naive_quadratic_1000.unions
//                            [--finds K | --union-only]
//   --n          number of elements (>= 2)
//   --out        output file
//   --finds      number of "f 0" lines after the chain is built (default: n)
//   --union-only write only the "u" lines, no "f" lines (same as --finds 0).
//                Useful as a baseline: building the chain costs 0 hops
//                (every unite joins two roots), so all the quadratic cost
//                comes from the "f 0" lines. Cannot be combined with
//                --finds K for K > 0.

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>

namespace perc {

static void usage(const char* prog) {
    std::cerr << "Usage: " << prog
              << " --n N --out FILE [--finds K | --union-only]\n";
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

    if (n < 2 || outPath == nullptr) {
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

    // Header comments (lines starting with '#' are ignored by the replay tool).
    out << "# naive-quadratic: chain 0->1->...->n-1, then repeated find(0)\n";
    out << "# n=" << n << " finds=" << finds
        << (unionOnly ? " (union-only)" : "") << "\n";
    out << "# linking rules: INTERFACE.md v0.3 section 3.1 (Naive: parent[ra] = rb)\n";

    // Line 1 after comments: number of elements.
    out << n << "\n";

    // Build the chain. At step i, element i is still a root, so under Naive
    // linking (parent[ra] = rb) this sets parent[i] = i+1.
    for (long long i = 0; i + 1 < n; ++i) {
        out << "u " << i << " " << (i + 1) << "\n";
    }

    // Each find(0) walks n-1 edges. Naive never compresses, so every repeat
    // pays the full cost again: about finds * (n-1) hops in total.
    for (long long k = 0; k < finds; ++k) {
        out << "f 0\n";
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
