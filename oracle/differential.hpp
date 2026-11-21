// tests/differential.hpp  --  seeded differential test: Percolation vs FloodFillOracle.
// Works on ANY class P with the Percolation API (section 4), so the same code drives the
// stand-in today and the real perc::Percolation later. After every operation it compares
// numOpen / isOpen / isFull for all sites / percolates / largestCluster with the oracle and
// runs the invariant checkers U1-U5 on ufPerc, ufFull and ufClust.
//
// Reproducibility: the run is a sequence of episodes. Each episode has its own seed derived
// from (base seed, episode index) and determines its grid size and every operation in it,
// so a failing episode can be replayed alone from its seed.
#pragma once
#include "checkers.hpp"
#include "flood_fill.hpp"
#include "percolation_checkers.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace perc_test {

inline uint64_t mix64(uint64_t x) {                       // splitmix64 finaliser
    uint64_t z = x + 0x9E3779B97F4A7C15ULL;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}
struct TestRng {                                          // own generator: same on every platform
    uint64_t state;
    explicit TestRng(uint64_t s) : state(s) {}
    uint64_t next() { state += 0x9E3779B97F4A7C15ULL; return mix64(state - 0x9E3779B97F4A7C15ULL); }
    uint64_t below(uint64_t n) { return next() % n; }
};

struct Op {
    enum Kind { Open, IsOpen, IsFull, Percolates, Largest, BadOpen, BadIsOpen, BadIsFull } kind;
    int r, c;
};
inline std::string opToString(const Op& o) {
    static const char* nm[] = {"open", "isOpen", "isFull", "percolates", "largestCluster",
                               "open(out-of-range)", "isOpen(out-of-range)", "isFull(out-of-range)"};
    std::ostringstream s;
    s << nm[o.kind];
    if (o.kind != Op::Percolates && o.kind != Op::Largest) s << "(" << o.r << "," << o.c << ")";
    return s.str();
}

struct DiffConfig {

    uint64_t seed = 171226;

    uint64_t totalOps = 100000;
    int maxL = 10;                                   // grid side drawn uniformly from 1..maxL
    perc::Variant variant = perc::Variant::RankCompression;
    bool trackClusters = true;
    uint64_t checkEvery = 1;                         // full comparison + checkers every k ops
};

struct DiffResult {
    bool ok = true;
    uint64_t opsRun = 0, episodes = 0;
    uint64_t episodeSeed = 0;                        // set on failure
    int L = 0;                                       // set on failure
    std::string message;                             // set on failure
    std::vector<Op> failingOps;                      // ops of the failing episode, up to the failure
};

enum class Which { Perc, Full, Clust };

// Expected partition of the elements of one union-find, derived ONLY from the oracle's
// component labels (so independent of every union-find). Class ids are element indices.
//   ufClust : sites of one component share a class; closed sites are singletons.
//   ufFull  : plus TOP; every site connected to the top row shares TOP's class.
//   ufPerc  : plus TOP and BOTTOM; components touching the top row join TOP, components
//             touching the bottom row join BOTTOM, and TOP = BOTTOM exactly when the grid
//             percolates.
inline std::vector<int> expectedClasses(const std::vector<int>& label, int L, Which w) {
    const int N = L * L, TOP = N, BOT = N + 1;
    std::vector<char> top(N, 0), bot(N, 0);
    for (int c = 0; c < L; ++c) {
        if (label[c] >= 0) top[label[c]] = 1;
        const int i = (L - 1) * L + c;
        if (label[i] >= 0) bot[label[i]] = 1;
    }
    bool perc = false;
    for (int i = 0; i < N; ++i) if (top[i] && bot[i]) perc = true;
    const int n = (w == Which::Perc) ? N + 2 : (w == Which::Full) ? N + 1 : N;
    std::vector<int> cls(n);
    for (int x = 0; x < N; ++x) {
        if (label[x] < 0) { cls[x] = x; continue; }
        const int l = label[x];
        if (w == Which::Clust)      cls[x] = l;
        else if (w == Which::Full)  cls[x] = top[l] ? TOP : l;
        else if (top[l])            cls[x] = TOP;
        else if (bot[l])            cls[x] = perc ? TOP : BOT;
        else                        cls[x] = l;
    }
    if (w != Which::Clust) cls[TOP] = TOP;
    if (w == Which::Perc)  cls[BOT] = perc ? TOP : BOT;
    return cls;
}

// U1, U2, U3 (against the oracle's partition), U5 on one union-find; "" if all pass.
inline std::string checkOneUF(const perc::UnionFind& uf, perc::Variant v,
                              const std::vector<int>& expected, int expectedN, const char* which) {
    if (uf.numElements() != expectedN)
        return std::string(which) + ": has " + std::to_string(uf.numElements()) + " elements, expected " +
               std::to_string(expectedN);
    perc_check::CheckResult r;
    if (!(r = perc_check::checkU1(uf)))           return std::string(which) + ": " + r.msg;
    if (!(r = perc_check::checkU2(uf)))           return std::string(which) + ": " + r.msg;
    if (!(r = perc_check::checkU3(uf, expected))) return std::string(which) + ": " + r.msg;
    if (!(r = perc_check::checkU5(uf, v)))        return std::string(which) + ": " + r.msg;
    return "";
}

struct Snaps { perc_check::UfSnapshot p, f, c; bool haveC; };

template <class P>
Snaps takeSnaps(const P& sut) {
    Snaps s;
    s.p = perc_check::snapshot(sut.ufPerc());
    s.f = perc_check::snapshot(sut.ufFull());
    s.haveC = (sut.ufClust() != nullptr);
    s.c = s.haveC ? perc_check::snapshot(*sut.ufClust()) : perc_check::UfSnapshot{0, 0};
    return s;
}

// Full comparison of the observable state plus invariants. Returns "" if everything agrees.
// Order: direct comparison of numOpen/isOpen with the oracle, then the percolation
// invariants P1-P6 (which also compare isFull, percolates and largestCluster with the
// oracle), then the union-find invariants U1-U5 on all three structures.
template <class P>
std::string compareState(P& sut, perc::FloodFillOracle& orc, int L, const DiffConfig& cfg, Snaps& snaps,
                         perc_check::PercMonitor<P>& mon) {
    if (sut.numOpen() != orc.numOpen())
        return "numOpen: structure " + std::to_string(sut.numOpen()) + ", oracle " + std::to_string(orc.numOpen());
    for (int r = 0; r < L; ++r)
        for (int c = 0; c < L; ++c)
            if (sut.isOpen(r, c) != orc.isOpen(r, c))
                return "isOpen(" + std::to_string(r) + "," + std::to_string(c) + ") differs from the oracle";
    {
        perc_check::CheckResult pr = mon.checkAll(sut, orc, cfg.trackClusters);
        if (!pr) return pr.msg;
    }

    const std::vector<int> label = orc.componentLabels();
    const int N = L * L;
    std::string m;
    if (!(m = checkOneUF(sut.ufPerc(), cfg.variant, expectedClasses(label, L, Which::Perc), N + 2, "ufPerc")).empty()) return m;
    if (!(m = checkOneUF(sut.ufFull(), cfg.variant, expectedClasses(label, L, Which::Full), N + 1, "ufFull")).empty()) return m;
    if (cfg.trackClusters) {
        if (sut.ufClust() == nullptr) return "ufClust() is null although cluster tracking is on";
        if (!(m = checkOneUF(*sut.ufClust(), cfg.variant, expectedClasses(label, L, Which::Clust), N, "ufClust")).empty()) return m;
    } else if (sut.ufClust() != nullptr) {
        return "ufClust() is not null although cluster tracking is off";
    }

    perc_check::CheckResult r;
    if (!(r = perc_check::checkU4Window(snaps.p, sut.ufPerc()))) return "ufPerc: " + r.msg;
    if (!(r = perc_check::checkU4Window(snaps.f, sut.ufFull()))) return "ufFull: " + r.msg;
    if (snaps.haveC && !(r = perc_check::checkU4Window(snaps.c, *sut.ufClust()))) return "ufClust: " + r.msg;
    snaps = takeSnaps(sut);
    return "";
}

template <class P>
bool bothThrowOutOfRange(P& sut, perc::FloodFillOracle& orc, Op::Kind k, int r, int c) {
    bool t1 = false, t2 = false;
    try { if (k == Op::BadOpen) sut.open(r, c); else if (k == Op::BadIsOpen) sut.isOpen(r, c); else sut.isFull(r, c); }
    catch (const std::out_of_range&) { t1 = true; }
    try { if (k == Op::BadOpen) orc.open(r, c); else if (k == Op::BadIsOpen) orc.isOpen(r, c); else orc.isFull(r, c); }
    catch (const std::out_of_range&) { t2 = true; }
    return t1 && t2;
}

// Apply ONE operation to the structure and the oracle, compare the results, and (if doCheck)
// run the full state comparison. Returns "" on success, else a description of the discrepancy.
template <class P>
std::string stepOp(P& sut, perc::FloodFillOracle& orc, const Op& op, uint64_t i, int L, const DiffConfig& cfg,
                   Snaps& snaps, perc_check::PercMonitor<P>& mon, bool doCheck) {
    const std::string at = "op " + std::to_string(i) + " " + opToString(op) + ": ";
    try {
        switch (op.kind) {
            case Op::Open: {
                const bool a = sut.open(op.r, op.c), b = orc.open(op.r, op.c);
                if (a != b) return at + "returned " + (a ? "true" : "false") + ", oracle " + (b ? "true" : "false");
                break;
            }
            case Op::IsOpen:
                if (sut.isOpen(op.r, op.c) != orc.isOpen(op.r, op.c)) return at + "differs from oracle";
                break;
            case Op::IsFull:
                if (sut.isFull(op.r, op.c) != orc.isFull(op.r, op.c)) return at + "differs from oracle";
                break;
            case Op::Percolates:
                if (sut.percolates() != orc.percolates()) return at + "differs from oracle";
                break;
            case Op::Largest:
                if (cfg.trackClusters) {
                    if (sut.largestCluster() != orc.largestCluster())
                        return at + "structure " + std::to_string(sut.largestCluster()) + ", oracle " +
                               std::to_string(orc.largestCluster());
                } else {
                    bool threw = false;
                    try { sut.largestCluster(); } catch (const std::logic_error&) { threw = true; }
                    if (!threw) return at + "should throw std::logic_error when tracking is off";
                }
                break;
            default:
                if (!bothThrowOutOfRange(sut, orc, op.kind, op.r, op.c))
                    return at + "both must throw std::out_of_range";
                break;
        }
        if (doCheck) {
            const std::string m = compareState(sut, orc, L, cfg, snaps, mon);
            if (!m.empty()) return at + "after the operation: " + m;
        }
    } catch (const std::exception& e) {
        return at + "unexpected exception: " + e.what();
    }
    return "";
}

// One episode. Returns "" on success, else a description of the first discrepancy.
template <class P, class MakeFn>
std::string runEpisode(MakeFn make, uint64_t epSeed, const DiffConfig& cfg, std::vector<Op>& ops,
                       uint64_t& opsRun, int& Lout) {
    TestRng rng(epSeed);
    const int L = 1 + static_cast<int>(rng.below(static_cast<uint64_t>(cfg.maxL)));
    Lout = L;
    const uint64_t budget = 6ULL * L * L + 10;
    perc::FloodFillOracle orc(L);
    std::unique_ptr<P> sut;
    try { sut = make(L, cfg.variant, cfg.trackClusters); }
    catch (const std::exception& e) { return std::string("constructor threw: ") + e.what(); }
    Snaps snaps = takeSnaps(*sut);
    perc_check::PercMonitor<P> mon(L);

    for (uint64_t i = 0; i < budget; ++i) {
        Op op;
        const uint64_t roll = rng.below(100);
        op.r = static_cast<int>(rng.below(L));
        op.c = static_cast<int>(rng.below(L));
        if      (roll < 40) op.kind = Op::Open;
        else if (roll < 50) op.kind = Op::IsOpen;
        else if (roll < 70) op.kind = Op::IsFull;
        else if (roll < 80) op.kind = Op::Percolates;
        else if (roll < 90) op.kind = Op::Largest;
        else {
            op.kind = (roll < 95) ? Op::BadOpen : (roll < 98) ? Op::BadIsOpen : Op::BadIsFull;
            const int bad = rng.below(2) ? -1 : L;
            if (rng.below(2)) op.r = bad; else op.c = bad;
        }
        ops.push_back(op);
        ++opsRun;
        const bool doCheck = (i + 1 == budget) || (opsRun % cfg.checkEvery) == 0;
        const std::string m = stepOp(*sut, orc, op, i, L, cfg, snaps, mon, doCheck);
        if (!m.empty()) return m;
    }
    return "";
}

// Replay an explicit operation list on a fresh grid of side L, running the FULL comparison
// after every operation. Returns "" if it passes, else the first discrepancy.
template <class P, class MakeFn>
std::string replayOps(MakeFn make, int L, const std::vector<Op>& ops, const DiffConfig& cfg) {
    perc::FloodFillOracle orc(L);
    std::unique_ptr<P> sut;
    try { sut = make(L, cfg.variant, cfg.trackClusters); }
    catch (const std::exception& e) { return std::string("constructor threw: ") + e.what(); }
    Snaps snaps = takeSnaps(*sut);
    perc_check::PercMonitor<P> mon(L);
    for (size_t i = 0; i < ops.size(); ++i) {
        const std::string m = stepOp(*sut, orc, ops[i], i, L, cfg, snaps, mon, true);
        if (!m.empty()) return m;
    }
    return "";
}

// ---------------------------------------------------------------------------------------------
// Shrinking. A failing episode can be hundreds of operations long; shrinkFailure reduces it to a
// short sequence that still fails the SAME way, which is what goes into the bug log.
//   1. cut everything after the failing operation;
//   2. delta debugging (ddmin): repeatedly try removing chunks of operations;
//   3. try smaller grids (drop operations outside the smaller grid);
//   4. a final pass removing single operations until none can go (1-minimal).
// "Fails the same way" means the same failure kind (the checker tag P1..P6/U1..U5/S1..S3, or
// else the operation that disagreed), so shrinking cannot slide over to a different bug.
// ---------------------------------------------------------------------------------------------
inline std::string failureKind(const std::string& msg) {
    for (size_t i = 0; i + 1 < msg.size(); ++i) {
        const char c = msg[i];
        if ((c == 'P' || c == 'U' || c == 'S') && msg[i + 1] >= '1' && msg[i + 1] <= '6' &&
            (i == 0 || msg[i - 1] == ' ') && i + 2 < msg.size() && msg[i + 2] == ':')
            return msg.substr(i, 2);
    }
    if (msg.compare(0, 3, "op ") == 0) {                   // "op 12 open(3,4): ..." -> "open"
        size_t a = msg.find(' ', 3);
        if (a != std::string::npos) {
            size_t b = msg.find_first_of("(:", a + 1);
            if (b != std::string::npos) return msg.substr(a + 1, b - a - 1);
        }
    }
    return msg.substr(0, 12);
}

struct Shrunk {
    int L = 0;
    std::vector<Op> ops;
    std::string message;           // failure of the shrunk sequence
    uint64_t replays = 0;          // how many candidate replays the shrinker needed
    bool reproduced = true;        // false if the original did not fail when replayed (should not happen)
};

template <class P, class MakeFn>
Shrunk shrinkFailure(MakeFn make, int L, std::vector<Op> ops, const DiffConfig& cfg) {
    DiffConfig c = cfg;
    c.checkEvery = 1;
    Shrunk s;
    s.L = L;
    std::string msg = replayOps<P>(make, L, ops, c);
    ++s.replays;
    if (msg.empty()) { s.ops = ops; s.reproduced = false; return s; }
    const std::string kind = failureKind(msg);

    auto fails = [&](int LL, const std::vector<Op>& cand, std::string& out) {
        out = replayOps<P>(make, LL, cand, c);
        ++s.replays;
        return !out.empty() && failureKind(out) == kind;
    };

    // 1. cut after the failing operation ("op N ...")
    if (msg.compare(0, 3, "op ") == 0) {
        const size_t n = static_cast<size_t>(std::strtoull(msg.c_str() + 3, nullptr, 10));
        if (n + 1 < ops.size()) {
            std::vector<Op> cand(ops.begin(), ops.begin() + n + 1);
            std::string out;
            if (fails(L, cand, out)) { ops = cand; msg = out; }
        }
    }

    auto ddmin = [&]() {
        size_t n = 2;
        while (ops.size() >= 2) {
            const size_t chunk = (ops.size() + n - 1) / n;
            bool reduced = false;
            for (size_t start = 0; start < ops.size(); start += chunk) {
                const size_t end = std::min(start + chunk, ops.size());
                std::vector<Op> cand(ops.begin(), ops.begin() + start);
                cand.insert(cand.end(), ops.begin() + end, ops.end());
                std::string out;
                if (fails(L, cand, out)) { ops = cand; msg = out; n = std::max<size_t>(n - 1, 2); reduced = true; break; }
            }
            if (!reduced) { if (n >= ops.size()) break; n = std::min(ops.size(), n * 2); }
        }
    };
    auto oneMinimal = [&]() {
        bool changed = true;
        while (changed) {
            changed = false;
            for (size_t i = 0; i < ops.size();) {
                std::vector<Op> cand = ops;
                cand.erase(cand.begin() + i);
                std::string out;
                if (fails(L, cand, out)) { ops = cand; msg = out; changed = true; } else ++i;
            }
        }
    };

    ddmin();
    // 3. smaller grid
    for (int LL = 1; LL < L; ++LL) {
        std::vector<Op> cand;
        for (Op o : ops) {
            const bool bad = (o.kind == Op::BadOpen || o.kind == Op::BadIsOpen || o.kind == Op::BadIsFull);
            if (!bad) { if (o.r >= LL || o.c >= LL) continue; }
            else { if (o.r == L) o.r = LL; if (o.c == L) o.c = LL; }
            cand.push_back(o);
        }
        std::string out;
        if (fails(LL, cand, out)) { L = LL; ops = cand; msg = out; break; }
    }
    ddmin();
    oneMinimal();

    s.L = L; s.ops = ops; s.message = msg;
    return s;
}

template <class P, class MakeFn>
DiffResult runDifferential(const DiffConfig& cfg, MakeFn make) {
    DiffResult res;
    while (res.opsRun < cfg.totalOps) {
        const uint64_t es = mix64(mix64(cfg.seed) ^ res.episodes);
        std::vector<Op> ops;
        int L = 0;
        const std::string msg = runEpisode<P>(make, es, cfg, ops, res.opsRun, L);
        ++res.episodes;
        if (!msg.empty()) {
            res.ok = false; res.message = msg; res.episodeSeed = es; res.L = L; res.failingOps = ops;
            return res;
        }
    }
    return res;
}

// Replay a single episode from its seed (printed on failure).
template <class P, class MakeFn>
DiffResult replayEpisode(const DiffConfig& cfg, MakeFn make, uint64_t epSeed) {
    DiffResult res;
    std::vector<Op> ops;
    int L = 0;
    const std::string msg = runEpisode<P>(make, epSeed, cfg, ops, res.opsRun, L);
    res.episodes = 1; res.episodeSeed = epSeed; res.L = L; res.failingOps = ops;
    if (!msg.empty()) { res.ok = false; res.message = msg; }
    return res;
}

}  // namespace perc_test
