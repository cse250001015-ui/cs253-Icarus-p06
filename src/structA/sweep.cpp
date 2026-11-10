// src/structA/sweep.cpp -- owner: Structure A owner.
// runSweepOrder is the Newman-Ziff sweep (INTERFACE.md section 5). TODO.
#include "sweep.hpp"
#include <stdexcept>

namespace perc {

SweepResult runSweepOrder(int, Variant, const std::vector<int>&, const SweepCallback&,
                          bool, bool) {
    throw std::logic_error("runSweepOrder not implemented (INTERFACE.md section 5)");
}

SweepResult runSweep(int L, Variant v, uint64_t seed, const SweepCallback& cb,
                     bool trackClusters, bool stopAtPercolation) {
    return runSweepOrder(L, v, makeOpeningOrder(L, seed), cb, trackClusters, stopAtPercolation);
}

}  // namespace perc
