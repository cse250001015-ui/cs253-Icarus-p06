// app/cluster_histogram.cpp
#include "cluster_histogram.hpp"
#include "../include/sweep.hpp"

#include <ostream>
#include <vector>

namespace perc {

bool addClusterSizes(const Percolation& p, uint64_t* counts) {
    const UnionFind* uf = p.ufClust();
    if (uf == nullptr) return false;
    const int L = p.L();
    for (int r = 0; r < L; ++r) {
        for (int c = 0; c < L; ++c) {
            if (!p.isOpen(r, c)) continue;          // a closed site is a singleton in ufClust, not a cluster
            const int i = r * L + c;
            if (uf->parentOf(i) != i) continue;     // count each cluster once, at its root
            ++counts[uf->sizeAtRoot(i)];
        }
    }
    return true;
}

void clusterHistogramSweeps(int L, Variant v, uint64_t baseSeed, int sweeps,
                            const int* at, int nAt, uint64_t* counts) {
    const int N = L * L;
    for (int k = 0; k < sweeps; ++k) {
        const std::vector<int> order = makeOpeningOrder(L, sweepSeed(baseSeed, L, k));
        Percolation p(L, v, true);
        int next = 0;
        for (int i = 0; i < N && next < nAt; ++i) {
            p.open(order[i] / L, order[i] % L);
            if (i + 1 == at[next]) {
                addClusterSizes(p, counts + static_cast<std::size_t>(next) * (N + 1));
                ++next;
            }
        }
    }
}

void writeClusterCsvHeader(std::ostream& out) {
    out << "variant,L,at_n,size,count\n";
}

void writeClusterCsvRows(std::ostream& out, Variant v, int L, int atN, const uint64_t* counts) {
    const int N = L * L;
    for (int s = 1; s <= N; ++s) {
        if (counts[s] > 0) {
            out << variantName(v) << ',' << L << ',' << atN << ',' << s << ',' << counts[s] << '\n';
        }
    }
}

}  // namespace perc
