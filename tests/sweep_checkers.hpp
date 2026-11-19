// tests/sweep_checkers.hpp  --  invariants S1..S3 of the Newman-Ziff sweep (INTERFACE.md
// section 9). They look only at the SweepRecord sequence and SweepResult.
//   S1: record i has n == i+1 (no gaps, no repeats).
//   S2: the same order gives identical record sequences for all four variants.
//   S3: nCritical is the n of the first record with percolated == true, or -1 if none.
#pragma once
#include "checkers.hpp"
#include "sweep.hpp"
#include <string>
#include <vector>

namespace perc_check {

inline bool sameRecord(const perc::SweepRecord& a, const perc::SweepRecord& b) {
    return a.n == b.n && a.percolated == b.percolated && a.largestCluster == b.largestCluster;
}

inline CheckResult checkS1(const std::vector<perc::SweepRecord>& rec) {
    for (size_t i = 0; i < rec.size(); ++i)
        if (rec[i].n != static_cast<int>(i) + 1)
            return fail("S1: record " + std::to_string(i) + " has n = " + std::to_string(rec[i].n) +
                        ", expected " + std::to_string(i + 1));
    return pass();
}

inline CheckResult checkS2(const std::vector<std::vector<perc::SweepRecord>>& perVariant,
                           const std::vector<std::string>& names) {
    for (size_t v = 1; v < perVariant.size(); ++v) {
        if (perVariant[v].size() != perVariant[0].size())
            return fail("S2: " + names[v] + " produced " + std::to_string(perVariant[v].size()) +
                        " records, " + names[0] + " produced " + std::to_string(perVariant[0].size()));
        for (size_t i = 0; i < perVariant[0].size(); ++i)
            if (!sameRecord(perVariant[v][i], perVariant[0][i]))
                return fail("S2: record " + std::to_string(i) + " of " + names[v] + " differs from " + names[0]);
    }
    return pass();
}

inline CheckResult checkS3(const std::vector<perc::SweepRecord>& rec, int nCritical) {
    int want = -1;
    for (const perc::SweepRecord& r : rec)
        if (r.percolated) { want = r.n; break; }
    if (nCritical != want)
        return fail("S3: nCritical = " + std::to_string(nCritical) + ", first percolating record has n = " +
                    std::to_string(want));
    return pass();
}

}  // namespace perc_check
