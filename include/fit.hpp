// include/fit.hpp -- owner: Architect. Finite-size extrapolation of the threshold (D6).
#pragma once
#include <vector>

namespace perc {

// Model: p(L) = pc + a * L^(-1/nu).  With x = L^(-1/nu) this is a straight line y = pc + a*x.
struct FitResult {
    bool   ok       = false;   // false if the data cannot be fitted (fewer than 2 distinct sizes)
    bool   weighted = false;   // true: weights 1/se^2 were used; false: all weights 1
    double pc       = 0;       // extrapolated threshold (the intercept)
    double a        = 0;       // slope
    double pcSe     = 0;       // standard error of pc (NaN if it cannot be estimated)
    double aSe      = 0;       // standard error of a
};

// L[i] is a grid side, mean[i] the mean threshold estimate for that size, se[i] its standard error.
// Weighted least squares with weights 1/se^2 when every se > 0. If some se is 0 (for example T == 1),
// falls back to ordinary least squares and estimates the errors from the residuals.
FitResult fitFiniteSize(const std::vector<double>& L, const std::vector<double>& mean,
                        const std::vector<double>& se, double nu = 4.0 / 3.0);

}  // namespace perc
