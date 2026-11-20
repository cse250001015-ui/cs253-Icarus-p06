// Percolation sweep benchmark for Project 6.
// Times the shared Newman-Ziff sweep and records counters from the actual
// union-find implementations through SweepResult.

#include "sweep.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;
constexpr int kMinimumRepetitions = 5;
constexpr int kMaximumL = 1000;
constexpr perc::Variant kVariants[] = {
    perc::Variant::Naive,
    perc::Variant::Rank,
    perc::Variant::Compression,
    perc::Variant::RankCompression
};
constexpr const char* kVariantNames[] = {
    "naive", "rank", "compress", "rank_compress"
};

struct Options {
    std::vector<int> sizes{50, 100, 200, 500, 1000};
    int repetitions = 5;
    uint64_t seed = 171226;
    std::string out = "results/raw";
};

struct Measurement {
    int L;
    int repetition;
    int variant;
    uint64_t seed;
    int nCritical;
    double timeNs;
    uint64_t finds;
    uint64_t hops;
    uint64_t unions;
};

bool parsePositive(const char* text, int& result) {
    char* end = nullptr;
    const long value = std::strtol(text, &end, 10);
    if (end == text || *end != '\0' || value < 1 || value > 1000000000L)
        return false;
    result = static_cast<int>(value);
    return true;
}

bool parseSizes(const char* text, std::vector<int>& sizes) {
    sizes.clear();
    const char* current = text;
    while (*current != '\0') {
        char* end = nullptr;
        const long value = std::strtol(current, &end, 10);
        if (end == current || value < 1 || value > kMaximumL) return false;
        sizes.push_back(static_cast<int>(value));
        if (*end == '\0') break;
        if (*end != ',') return false;
        current = end + 1;
        if (*current == '\0') return false;
    }
    return !sizes.empty();
}

void usage(const char* program) {
    std::cerr << "Usage: " << program
              << " [--sizes 50,100,200,500,1000] [--repetitions 5]"
                 " [--seed N] [--out DIR]\n"
              << "  --repetitions must be at least " << kMinimumRepetitions << "\n";
}

bool parseArgs(int argc, char** argv, Options& options) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--sizes" && i + 1 < argc) {
            if (!parseSizes(argv[++i], options.sizes)) return false;
        } else if (arg == "--repetitions" && i + 1 < argc) {
            if (!parsePositive(argv[++i], options.repetitions) ||
                options.repetitions < kMinimumRepetitions) return false;
        } else if (arg == "--seed" && i + 1 < argc) {
            const char* value = argv[++i];
            char* end = nullptr;
            const unsigned long long parsed = std::strtoull(value, &end, 10);
            if (end == value || *end != '\0') return false;
            options.seed = static_cast<uint64_t>(parsed);
        } else if (arg == "--out" && i + 1 < argc) {
            options.out = argv[++i];
            if (options.out.empty()) return false;
        } else {
            return false;
        }
    }
    return true;
}

double mean(const std::vector<double>& values) {
    return std::accumulate(values.begin(), values.end(), 0.0) /
           static_cast<double>(values.size());
}

double sampleSd(const std::vector<double>& values, double average) {
    if (values.size() < 2) return 0.0;
    double sum = 0.0;
    for (double value : values) {
        const double delta = value - average;
        sum += delta * delta;
    }
    return std::sqrt(sum / static_cast<double>(values.size() - 1));
}

uint64_t total(uint64_t a, uint64_t b, uint64_t c) {
    return a + b + c;
}

int run(const Options& options) {
    std::error_code ec;
    std::filesystem::create_directories(options.out, ec);
    if (ec) throw std::runtime_error("cannot create output directory: " + ec.message());

    const std::string rawPath = options.out + "/percolation_sweeps.csv";
    const std::string summaryPath = options.out + "/percolation_sweep_summary.csv";
    std::ofstream raw(rawPath);
    std::ofstream summary(summaryPath);
    if (!raw) throw std::runtime_error("cannot open " + rawPath);
    if (!summary) throw std::runtime_error("cannot open " + summaryPath);

    raw << "variant,L,repetition,seed,n_critical,time_ns,finds,hops,unions,"
           "perc_hops,full_hops,clust_hops\n";
    summary << "variant,L,repetitions,mean_time_ns,sd_time_ns,min_time_ns,max_time_ns,"
               "mean_hops,sd_hops,mean_n_critical\n";
    raw << std::setprecision(12);
    summary << std::setprecision(12);

    std::vector<Measurement> measurements;
    measurements.reserve(options.sizes.size() * options.repetitions * 4);

    for (int L : options.sizes) {
        for (int repetition = 0; repetition < options.repetitions; ++repetition) {
            const uint64_t seed = perc::sweepSeed(options.seed, L, repetition);
            int expectedCritical = -1;

            // Reuse the same deterministic seed for all variants in this trial.
            for (int v = 0; v < 4; ++v) {
                const auto start = Clock::now();
                const perc::SweepResult result = perc::runSweep(
                    L, kVariants[v], seed, perc::SweepCallback{}, true, true);
                const auto finish = Clock::now();

                if (result.nCritical < 1)
                    throw std::runtime_error("sweep did not reach percolation");
                if (expectedCritical < 0) expectedCritical = result.nCritical;
                else if (result.nCritical != expectedCritical)
                    throw std::runtime_error("variants disagree on the percolation threshold sample");

                const uint64_t finds = total(result.perc.finds, result.full.finds,
                                             result.clust.finds);
                const uint64_t hops = total(result.perc.hops, result.full.hops,
                                            result.clust.hops);
                const uint64_t unions = total(result.perc.unions, result.full.unions,
                                              result.clust.unions);
                const double elapsed = std::chrono::duration<double, std::nano>(finish - start).count();

                measurements.push_back({L, repetition, v, seed, result.nCritical,
                                        elapsed, finds, hops, unions});
                raw << kVariantNames[v] << ',' << L << ',' << repetition << ',' << seed << ','
                    << result.nCritical << ',' << elapsed << ',' << finds << ',' << hops << ','
                    << unions << ',' << result.perc.hops << ',' << result.full.hops << ','
                    << result.clust.hops << '\n';
            }
        }

        std::cout << "Completed L=" << L << " (" << options.repetitions
                  << " paired repetitions; four variants)\n";
    }

    for (int v = 0; v < 4; ++v) {
        for (int L : options.sizes) {
            std::vector<double> times;
            std::vector<double> hops;
            std::vector<double> critical;
            for (const Measurement& m : measurements) {
                if (m.variant == v && m.L == L) {
                    times.push_back(m.timeNs);
                    hops.push_back(static_cast<double>(m.hops));
                    critical.push_back(static_cast<double>(m.nCritical));
                }
            }

            const double averageTime = mean(times);
            const double averageHops = mean(hops);
            const auto bounds = std::minmax_element(times.begin(), times.end());
            summary << kVariantNames[v] << ',' << L << ',' << times.size() << ','
                    << averageTime << ',' << sampleSd(times, averageTime) << ','
                    << *bounds.first << ',' << *bounds.second << ','
                    << averageHops << ',' << sampleSd(hops, averageHops) << ','
                    << mean(critical) << '\n';

            std::cout << "  " << kVariantNames[v] << " L=" << L
                      << " mean time=" << std::fixed << std::setprecision(0)
                      << averageTime << " ns, SD=" << sampleSd(times, averageTime)
                      << " ns, mean hops=" << std::setprecision(1) << averageHops << '\n';
        }
    }

    std::cout << "Wrote " << rawPath << " and " << summaryPath << '\n';
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    Options options;
    if (!parseArgs(argc, argv, options)) {
        usage(argv[0]);
        return 1;
    }
    try {
        return run(options);
    } catch (const std::exception& e) {
        std::cerr << "benchmark error: " << e.what() << '\n';
        return 1;
    }
}
