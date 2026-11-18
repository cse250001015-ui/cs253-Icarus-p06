// src/fit.cpp -- owner: Architect.
#include "fit.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>

namespace perc {

FitResult fitFiniteSize(const std::vector<double>& L, const std::vector<double>& mean,
                        const std::vector<double>& se, double nu) {
    const size_t n = L.size();
    if (mean.size() != n || se.size() != n) throw std::invalid_argument("fitFiniteSize: size mismatch");

    FitResult r;
    if (n < 2) return r;

    // x = L^(-1/nu); weights 1/se^2 when all se are positive and finite
    std::vector<double> x(n), w(n);
    bool useWeights = true;
    for (size_t i = 0; i < n; ++i) {
        x[i] = std::pow(L[i], -1.0 / nu);
        if (!(se[i] > 0) || !std::isfinite(se[i])) useWeights = false;
    }
    for (size_t i = 0; i < n; ++i) w[i] = useWeights ? 1.0 / (se[i] * se[i]) : 1.0;

    // normal equations of weighted least squares
    double S = 0, Sx = 0, Sy = 0, Sxx = 0, Sxy = 0;
    for (size_t i = 0; i < n; ++i) {
        S   += w[i];
        Sx  += w[i] * x[i];
        Sy  += w[i] * mean[i];
        Sxx += w[i] * x[i] * x[i];
        Sxy += w[i] * x[i] * mean[i];
    }
    const double delta = S * Sxx - Sx * Sx;
    if (!(std::fabs(delta) > 1e-300)) return r;     // all sizes equal: no line can be fitted

    r.a  = (S * Sxy - Sx * Sy) / delta;
    r.pc = (Sxx * Sy - Sx * Sxy) / delta;
    r.weighted = useWeights;
    r.ok = true;

    // standard errors: var(pc) = Sxx/delta, var(a) = S/delta (valid when w = 1/sigma^2)
    double scale = 1.0;
    if (!useWeights) {
        // ordinary least squares: estimate sigma^2 from the residuals (needs n > 2)
        if (n > 2) {
            double rss = 0;
            for (size_t i = 0; i < n; ++i) {
                double res = mean[i] - (r.pc + r.a * x[i]);
                rss += res * res;
            }
            scale = rss / static_cast<double>(n - 2);
        } else {
            r.pcSe = r.aSe = std::numeric_limits<double>::quiet_NaN();
            return r;
        }
    }
    r.pcSe = std::sqrt(scale * Sxx / delta);
    r.aSe  = std::sqrt(scale * S / delta);
    return r;
}

}  // namespace perc
