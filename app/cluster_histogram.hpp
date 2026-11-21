// app/cluster_histogram.hpp
// Cluster-size histograms for clusters.csv (INTERFACE.md section 8).
//
// At chosen numbers of open sites n ("checkpoints") of a Newman-Ziff sweep, counts how many
// clusters of each size exist. Sizes come from ufClust, the structure without virtual nodes
// (section 4.1), so they are true cluster sizes. Closed sites are not clusters and are skipped.
//
// Columns of clusters.csv: variant,L,at_n,size,count. With more than one sweep, `count` is the
// total over all sweeps (divide by the number of sweeps for the mean per sweep). The columns are
// provisional until the Analyst confirms them (open decision 9).
#pragma once
#include "../include/percolation.hpp"
#include "../include/unionfind.hpp"

#include <cstdint>
#include <iosfwd>

namespace perc {

// Adds one to counts[s] for every cluster of s open sites in p. counts needs L*L + 1 entries
// (index 0 is never used). Reads ufClust only through the debug accessors, so no counter moves.
// Returns false, and changes nothing, if p was built with trackClusters == false.
bool addClusterSizes(const Percolation& p, uint64_t* counts);

// Runs `sweeps` sweeps (seed of sweep k: sweepSeed(baseSeed, L, k)) on a fresh Percolation of
// the given variant. At each checkpoint at[j] (ascending, distinct, each in [1, L*L]) it adds
// that sweep's histogram into counts + j*(L*L+1). counts must hold nAt*(L*L+1) zeroed entries.
void clusterHistogramSweeps(int L, Variant v, uint64_t baseSeed, int sweeps,
                            const int* at, int nAt, uint64_t* counts);

void writeClusterCsvHeader(std::ostream& out);

// One row per size with count > 0, in increasing size.
void writeClusterCsvRows(std::ostream& out, Variant v, int L, int atN, const uint64_t* counts);

}  // namespace perc
