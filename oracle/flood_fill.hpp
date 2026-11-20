// oracle/flood_fill.hpp
// Oracle for percolation: BFS flood fill from the top row. Written to be obviously
// correct, not fast. Uses NO union-find code (INTERFACE.md section 6) and no library
// container except std::vector as plain array storage (decision 11). The BFS queue is
// a hand-written array queue (head/tail indices over a preallocated array).
#pragma once
#include <vector>

namespace perc {

class FloodFillOracle {
public:
    explicit FloodFillOracle(int L);   // throws std::invalid_argument unless 1 <= L <= 46340

    bool open(int r, int c);           // true if newly opened, false if already open
    bool isOpen(int r, int c) const;
    bool isFull(int r, int c);         // open AND reachable from an open top-row site
    bool percolates();                 // some bottom-row site is full
    int  numOpen() const;
    int  largestCluster();             // size of largest component of open sites; 0 if none
    int  L() const;

    // Extension, not in INTERFACE.md section 6: component label of every site.
    // label[i] = smallest site index in i's component of open sites, or -1 if i is closed.
    // The differential harness uses it as ground truth for the partition checks (U3).
    std::vector<int> componentLabels();

private:
    void check(int r, int c) const;                  // throws std::out_of_range
    void refresh();                                  // recompute full_ by BFS from top row
    int  labelComponents(std::vector<int>& label);   // returns size of largest component

    int  L_;
    int  numOpen_;
    std::vector<char> open_;
    std::vector<char> full_;     // valid only when !dirty_
    std::vector<int>  queue_;    // array-queue storage, capacity L*L (each site is queued once)
    bool dirty_;
};

}  // namespace perc
