// tests/test_stats.cpp -- owner: Architect.
#include "stats.hpp"
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <vector>

static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); ++failures; } } while (0)
static bool near(double a, double b, double tol = 1e-9) { return std::fabs(a - b) <= tol; }

int main() {
    using namespace perc;

    // hand-checked: samples 1..5 -> mean 3, sample variance 10/4, sd = sqrt(2.5)
    Summary s = summarize({1, 2, 3, 4, 5});
    CHECK(s.T == 5);
    CHECK(near(s.mean, 3.0));
    CHECK(near(s.sd, std::sqrt(2.5)));            // 1.58113883...
    CHECK(near(s.se, std::sqrt(2.5) / std::sqrt(5.0)));   // 0.70710678...
    CHECK(near(s.ciLow, 3.0 - 1.96 * s.se) && near(s.ciHigh, 3.0 + 1.96 * s.se));

    // identical samples: no spread
    Summary c = summarize({0.5, 0.5, 0.5});
    CHECK(near(c.mean, 0.5) && near(c.sd, 0.0) && near(c.ciLow, 0.5) && near(c.ciHigh, 0.5));

    // one sample: sd and se are 0 by convention, CI collapses to the mean
    Summary one = summarize({0.59});
    CHECK(one.T == 1 && near(one.mean, 0.59) && near(one.sd, 0.0) && near(one.se, 0.0));

    // two samples: sd = |a-b| / sqrt(2)
    Summary two = summarize({0.0, 1.0});
    CHECK(near(two.mean, 0.5) && near(two.sd, 1.0 / std::sqrt(2.0)));

    bool threw = false;
    try { summarize({}); } catch (const std::invalid_argument&) { threw = true; }
    CHECK(threw);

    if (failures == 0) std::puts("test_stats: all checks passed");
    return failures ? 1 : 0;
}
