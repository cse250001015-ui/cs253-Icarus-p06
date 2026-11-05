// adversary/gen_backwash.cpp
// Writes an .order file that exposes backwash.
//
// Column 0 is a full top-to-bottom path (it makes the grid percolate).
// Stubs are vertical segments in columns 2, 4, 6, ... covering rows 1..L-1.
// Odd columns stay closed, so no stub touches column 0, another stub, or
// the top row. The correct isFull answer is therefore: only column 0 is full.
// A buggy single-structure implementation reports every stub as full, because
// each stub's bottom site joins BOTTOM, and BOTTOM is connected to TOP.
//
// Usage: gen_backwash --L 5 --mode percolate_first|stubs_first --out FILE
//   --L     grid side length (>= 3)
//   --mode  percolate_first: column 0 first, then stubs (bug shows right away)
//           stubs_first:     stubs first, column 0 last (bug shows at the
//                            moment of percolation)
//   --out   output file

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>

namespace perc {

static void usage(const char* prog) {
    std::cerr << "Usage: " << prog
              << " --L N --mode percolate_first|stubs_first --out FILE\n"
              << "  N must be >= 3\n";
}

static void writeColumn0(std::ofstream& out, long long L) {
    for (long long r = 0; r < L; ++r) {            // top to bottom
        out << r * L << "\n";
    }
}

static void writeStubs(std::ofstream& out, long long L) {
    for (long long c = 2; c < L; c += 2) {
        for (long long r = L - 1; r >= 1; --r) {   // bottom row first, then up
            out << r * L + c << "\n";
        }
    }
}

int run(int argc, char** argv) {
    long long L = 0;
    const char* mode = nullptr;
    const char* outPath = nullptr;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--L") == 0 && i + 1 < argc) {
            L = std::atoll(argv[++i]);
        } else if (std::strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
            mode = argv[++i];
        } else if (std::strcmp(argv[i], "--out") == 0 && i + 1 < argc) {
            outPath = argv[++i];
        } else {
            usage(argv[0]);
            return 1;
        }
    }

    bool percolateFirst = mode != nullptr && std::strcmp(mode, "percolate_first") == 0;
    bool stubsFirst     = mode != nullptr && std::strcmp(mode, "stubs_first") == 0;

    if (L < 3 || outPath == nullptr || (!percolateFirst && !stubsFirst)) {
        usage(argv[0]);
        return 1;
    }

    std::ofstream out(outPath);
    if (!out) {
        std::cerr << "Cannot open " << outPath << " for writing\n";
        return 1;
    }

    out << "# backwash: column 0 percolates, stubs in columns 2,4,... must stay not-full\n";
    out << "# L=" << L << " mode=" << mode << "\n";
    out << L << "\n";

    if (percolateFirst) {
        writeColumn0(out, L);
        writeStubs(out, L);
    } else {
        writeStubs(out, L);
        writeColumn0(out, L);
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
