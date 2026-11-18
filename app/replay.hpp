// app/replay.hpp
// Reader and executor for the replay files of INTERFACE.md section 7:
//   *.order   : "L" then distinct site indices, one per line
//   *.unions  : "n" then "u a b" / "f x" commands, one per line
// '#' starts a comment (whole line or after a command); blank lines are ignored.
// The file kind is detected from the content, so --replay accepts both kinds
// (open decision 10); the extension is only used for a file with no records.
#pragma once
#include "../include/percolation.hpp"
#include "../include/unionfind.hpp"

#include <cstdint>
#include <fstream>
#include <iosfwd>
#include <string>

namespace perc {

enum class ReplayKind { Order, Unions };

struct ReplayCommand {
    char op;      // 'o' open site a (Order files), 'u' unite(a, b), 'f' find(a)
    int  a;
    int  b;       // only used by 'u'
    int  line;    // 1-based line number in the file, for error messages
};

// Streams records one at a time, so large files are never held in memory.
class ReplayReader {
public:
    explicit ReplayReader(const std::string& path);

    bool ok() const                  { return error_.empty(); }
    const std::string& error() const { return error_; }   // includes file:line
    ReplayKind kind() const          { return kind_; }
    int  header() const              { return header_; }   // L (Order) or n (Unions)

    // Next command. Returns false at end of file or on error; check ok().
    bool next(ReplayCommand& cmd);

private:
    bool readRecord(std::string& rec, int& line);   // next non-empty record
    void fail(int line, const std::string& msg);

    std::string path_;
    std::ifstream in_;
    std::string error_;
    ReplayKind kind_ = ReplayKind::Order;
    int header_ = 0;
    int lineNo_ = 0;
    bool havePending_ = false;
    std::string pending_;
    int pendingLine_ = 0;
};

// Replays `path` on a fresh structure of variant `v` and prints a summary to
// `out` (counters, and for .order files the percolation state). Returns 0 on
// success, 1 on a file or format error (message on `err`).
int replayFile(const std::string& path, Variant v, std::ostream& out, std::ostream& err);

// Same, for all four variants in the order naive, rank, compress, rank_compress
// (for `--variant all`). Stops at the first error.
int replayFileAll(const std::string& path, std::ostream& out, std::ostream& err);

}  // namespace perc
