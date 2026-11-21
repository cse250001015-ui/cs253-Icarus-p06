// src/stats.cpp -- owner: Architect.
#include "stats.hpp"
#include <cmath>
#include <stdexcept>

namespace perc {

Summary summarize(const std::vector<double>& x) {
    if (x.empty()) throw std::invalid_argument("summarize: need at least one sample");

    Summary s;
    s.T = static_cast<int>(x.size());

    // mean
    double sum = 0;
    for (double v : x) sum += v;
    s.mean = sum / s.T;

    // sample standard deviation: two passes (first the mean, then the squared deviations)
    if (s.T > 1) {
        double ss = 0;
        for (double v : x) {
            double d = v - s.mean;
            ss += d * d;
        }
        s.sd = std::sqrt(ss / (s.T - 1));
        s.se = s.sd / std::sqrt(static_cast<double>(s.T));
    }

    // 95% confidence interval (normal approximation), the formula used in D6
    s.ciLow  = s.mean - 1.96 * s.se;
    s.ciHigh = s.mean + 1.96 * s.se;
    return s;
}

}  // namespace perc
