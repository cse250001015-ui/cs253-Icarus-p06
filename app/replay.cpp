// app/replay.cpp
#include "replay.hpp"

#include <chrono>
#include <climits>
#include <cstdlib>
#include <ostream>

namespace perc {

namespace {

// Strips a trailing '#' comment, '\r' and surrounding blanks.
void cleanRecord(std::string& s) {
    std::size_t hash = s.find('#');
    if (hash != std::string::npos) s.erase(hash);
    std::size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) { s.clear(); return; }
    std::size_t e = s.find_last_not_of(" \t\r\n");
    s = s.substr(b, e - b + 1);
}

// Parses one whole-token integer starting at s[pos]; advances pos past it.
bool parseInt(const std::string& s, std::size_t& pos, long long& out) {
    while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t')) ++pos;
    if (pos >= s.size()) return false;
    const char* start = s.c_str() + pos;
    char* end = nullptr;
    long long v = std::strtoll(start, &end, 10);
    if (end == start) return false;
    if (*end != '\0' && *end != ' ' && *end != '\t') return false;
    pos += static_cast<std::size_t>(end - start);
    out = v;
    return true;
}

bool atEnd(const std::string& s, std::size_t pos) {
    while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t')) ++pos;
    return pos >= s.size();
}

bool endsWith(const std::string& s, const char* suffix) {
    std::string t(suffix);
    return s.size() >= t.size() && s.compare(s.size() - t.size(), t.size(), t) == 0;
}

}  // namespace

// ---------------------------------------------------------------- reader

void ReplayReader::fail(int line, const std::string& msg) {
    if (error_.empty()) error_ = path_ + ":" + std::to_string(line) + ": " + msg;
}

bool ReplayReader::readRecord(std::string& rec, int& line) {
    if (havePending_) {
        rec = pending_;
        line = pendingLine_;
        havePending_ = false;
        return true;
    }
    std::string s;
    while (std::getline(in_, s)) {
        ++lineNo_;
        cleanRecord(s);
        if (!s.empty()) { rec = s; line = lineNo_; return true; }
    }
    return false;
}

ReplayReader::ReplayReader(const std::string& path) : path_(path), in_(path) {
    if (!in_) { error_ = "cannot open " + path; return; }

    std::string rec;
    int line = 0;
    if (!readRecord(rec, line)) { fail(1, "empty file: missing header line"); return; }

    std::size_t pos = 0;
    long long h = 0;
    if (!parseInt(rec, pos, h) || !atEnd(rec, pos)) {
        fail(line, "header must be a single integer (L or n)");
        return;
    }
    if (h < 1 || h > INT_MAX) { fail(line, "header out of range"); return; }
    header_ = static_cast<int>(h);

    // Decide the kind from the first record after the header.
    if (readRecord(pending_, pendingLine_)) {
        havePending_ = true;
        char c = pending_[0];
        kind_ = (c == 'u' || c == 'f') ? ReplayKind::Unions : ReplayKind::Order;
    } else {
        kind_ = endsWith(path, ".unions") ? ReplayKind::Unions : ReplayKind::Order;
    }

    if (kind_ == ReplayKind::Order && static_cast<long long>(header_) * header_ > INT_MAX) {
        fail(line, "L*L does not fit in int");
    }
}

bool ReplayReader::next(ReplayCommand& cmd) {
    if (!ok()) return false;
    std::string rec;
    int line = 0;
    if (!readRecord(rec, line)) return false;

    std::size_t pos = 0;
    long long a = 0, b = 0;

    if (kind_ == ReplayKind::Order) {
        if (!parseInt(rec, pos, a) || !atEnd(rec, pos)) {
            fail(line, "expected a single site index");
            return false;
        }
        long long cells = static_cast<long long>(header_) * header_;
        if (a < 0 || a >= cells) { fail(line, "site index out of range [0, L*L)"); return false; }
        cmd = {'o', static_cast<int>(a), 0, line};
        return true;
    }

    char op = rec[0];
    pos = 1;
    if (op != 'u' && op != 'f') {
        fail(line, "expected 'u a b' or 'f x'");
        return false;
    }
    bool good = parseInt(rec, pos, a);
    if (good && op == 'u') good = parseInt(rec, pos, b);
    if (!good || !atEnd(rec, pos)) {
        fail(line, op == 'u' ? "expected 'u a b'" : "expected 'f x'");
        return false;
    }
    if (a < 0 || a >= header_ || (op == 'u' && (b < 0 || b >= header_))) {
        fail(line, "element index out of range [0, n)");
        return false;
    }
    cmd = {op, static_cast<int>(a), static_cast<int>(b), line};
    return true;
}

// -------------------------------------------------------------- executors

namespace {

void printCounters(std::ostream& out, const char* label, const Counters& c) {
    out << "  " << label << ": finds=" << c.finds << " hops=" << c.hops
        << " unions=" << c.unions << "\n";
}

int replayUnions(ReplayReader& rd, Variant v, std::ostream& out, std::ostream& err) {
    std::unique_ptr<UnionFind> uf = makeUnionFind(v, rd.header());
    uint64_t uniteCalls = 0, findCalls = 0;
    ReplayCommand cmd;

    auto t0 = std::chrono::steady_clock::now();
    while (rd.next(cmd)) {
        if (cmd.op == 'u') { uf->unite(cmd.a, cmd.b); ++uniteCalls; }
        else               { uf->find(cmd.a);         ++findCalls;  }
    }
    auto t1 = std::chrono::steady_clock::now();
    if (!rd.ok()) { err << rd.error() << "\n"; return 1; }

    const Counters& c = uf->counters();
    long long ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
    out << "variant=" << uf->name() << " kind=unions n=" << rd.header() << "\n";
    out << "  commands: unite=" << uniteCalls << " find=" << findCalls << "\n";
    printCounters(out, "counters", c);
    out << "  hops/find=" << (c.finds ? static_cast<double>(c.hops) / c.finds : 0.0)
        << " time_ns=" << ns << "\n";
    return 0;
}

int replayOrder(ReplayReader& rd, Variant v, std::ostream& out, std::ostream& err) {
    const int L = rd.header();
    Percolation perc(L, v);
    int nCritical = -1;
    uint64_t duplicates = 0;
    ReplayCommand cmd;

    auto t0 = std::chrono::steady_clock::now();
    while (rd.next(cmd)) {
        if (!perc.open(cmd.a / L, cmd.a % L)) { ++duplicates; continue; }
        if (nCritical < 0 && perc.percolates()) nCritical = perc.numOpen();
    }
    auto t1 = std::chrono::steady_clock::now();
    if (!rd.ok()) { err << rd.error() << "\n"; return 1; }
    if (duplicates > 0) {
        err << "warning: " << duplicates
            << " repeated indices ignored (an .order file must have distinct indices)\n";
    }

    // Counters are read before the isFull scan below, which adds finds of its own.
    Counters total = perc.counters();
    Counters cp = perc.countersPerc(), cf = perc.countersFull(), cc = perc.countersClust();
    long long ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();

    // Backwash check: number of full sites at the end. For gen_backwash files
    // the correct value is L (column 0 only); a backwash bug reports more.
    long long fullSites = 0;
    for (int r = 0; r < L; ++r)
        for (int c = 0; c < L; ++c)
            if (perc.isFull(r, c)) ++fullSites;

    out << "variant=" << variantName(v) << " kind=order L=" << L << "\n";
    out << "  open=" << perc.numOpen() << " percolates=" << (perc.percolates() ? "yes" : "no")
        << " n_critical=" << nCritical << " full_sites=" << fullSites
        << " largest_cluster=" << perc.largestCluster() << "\n";
    printCounters(out, "total ", total);
    printCounters(out, "perc  ", cp);
    printCounters(out, "full  ", cf);
    printCounters(out, "clust ", cc);
    out << "  time_ns=" << ns << "\n";
    return 0;
}

}  // namespace

int replayFile(const std::string& path, Variant v, std::ostream& out, std::ostream& err) {
    ReplayReader rd(path);
    if (!rd.ok()) { err << rd.error() << "\n"; return 1; }
    return rd.kind() == ReplayKind::Unions ? replayUnions(rd, v, out, err)
                                           : replayOrder(rd, v, out, err);
}

int replayFileAll(const std::string& path, std::ostream& out, std::ostream& err) {
    static const Variant all[] = {Variant::Naive, Variant::Rank,
                                  Variant::Compression, Variant::RankCompression};
    int rc = 0;
    for (Variant v : all) {
        if (replayFile(path, v, out, err) != 0) { rc = 1; break; }   // same file: it would fail again
    }
    return rc;
}

}  // namespace perc
