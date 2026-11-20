// tests/test_fit.cpp -- owner: Architect.
#include "fit.hpp"
#include <cmath>
#include <cstdio>
#include <vector>

static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); ++failures; } } while (0)
static bool near(double a, double b, double tol = 1e-9) { return std::fabs(a - b) <= tol; }

int main() {
    using namespace perc;
    const double nu = 4.0 / 3.0;

    // exact straight line y = 0.592746 + 0.9 * L^(-3/4): both variants must recover it
    std::vector<double> L = {50, 100, 200, 400, 800};
    std::vector<double> y;
    for (double l : L) y.push_back(0.592746 + 0.9 * std::pow(l, -1.0 / nu));

    FitResult w = fitFiniteSize(L, y, std::vector<double>(5, 0.001), nu);
    CHECK(w.ok && w.weighted);
    CHECK(near(w.pc, 0.592746) && near(w.a, 0.9));
    CHECK(w.pcSe > 0);

    FitResult u = fitFiniteSize(L, y, std::vector<double>(5, 0.0), nu);   // se = 0 -> ordinary LS
    CHECK(u.ok && !u.weighted);
    CHECK(near(u.pc, 0.592746) && near(u.a, 0.9));
    CHECK(near(u.pcSe, 0.0, 1e-9));                                       // perfect line: zero residuals

    // hand-checked ordinary least squares. nu = -1 makes x = L^(+1) = L, so x = 1, 2, 3.
    // points (1,2), (2,4), (3,5): slope 1.5, intercept 2/3
    FitResult h = fitFiniteSize({1, 2, 3}, {2, 4, 5}, {0, 0, 0}, -1.0);
    CHECK(h.ok && near(h.a, 1.5) && near(h.pc, 2.0 / 3.0));

    // weights matter: a point with a huge standard error is nearly ignored
    FitResult g = fitFiniteSize({1, 2, 3}, {2, 4, 50}, {1e-3, 1e-3, 1e6}, -1.0);
    CHECK(g.ok && g.weighted && near(g.a, 2.0, 1e-6) && near(g.pc, 0.0, 1e-6));

    // not enough data
    CHECK(!fitFiniteSize({100}, {0.59}, {0.001}, nu).ok);
    CHECK(!fitFiniteSize({100, 100}, {0.59, 0.60}, {0.001, 0.001}, nu).ok);   // same size twice
    // two points, no spread information: fit works but the error cannot be estimated
    FitResult two = fitFiniteSize({50, 100}, {0.60, 0.595}, {0, 0}, nu);
    CHECK(two.ok && std::isnan(two.pcSe));

    if (failures == 0) std::puts("test_fit: all checks passed");
    return failures ? 1 : 0;
}
