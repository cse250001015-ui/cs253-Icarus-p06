// include/stats.hpp -- owner: Architect. Sample statistics for the threshold estimates (D6).
#pragma once
#include <vector>

namespace perc {

struct Summary {
    int    T      = 0;     // number of samples
    double mean   = 0;
    double sd     = 0;     // sample standard deviation (divides by T-1); 0 when T == 1
    double se     = 0;     // standard error = sd / sqrt(T)
    double ciLow  = 0;     // mean - 1.96 * se
    double ciHigh = 0;     // mean + 1.96 * se
};

// Needs at least one sample, otherwise throws std::invalid_argument.
Summary summarize(const std::vector<double>& samples);

}  // namespace perc
