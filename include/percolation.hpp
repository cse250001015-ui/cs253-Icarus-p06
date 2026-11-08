// include/percolation.hpp -- owner: Architect (declaration). Implementation: src/structA/percolation.cpp.
// Contract: INTERFACE.md section 4 (v0.3). The Structure A owner may add private members.
#pragma once
#include "unionfind.hpp"
#include <memory>
#include <vector>
using namespace std;

namespace perc {

class Percolation {
public:
    Percolation(int L, Variant v, bool trackClusters = true);

    bool open(int r, int c);            // true if newly opened; exact order of unions: INTERFACE.md 4.2
    bool isOpen(int r, int c) const;    // throws std::out_of_range outside [0, L)
    bool isFull(int r, int c);          // open AND reachable from the top row (no backwash)
    bool percolates();                  // cached flag
    int  numOpen() const;
    int  largestCluster() const;        // requires trackClusters == true; 0 if none open
    int  L() const;

    Counters counters() const;          // sum of the three below
    const Counters& countersPerc() const;
    const Counters& countersFull() const;
    const Counters& countersClust() const;   // all zero if clusters are not tracked
    void resetCounters();

    // Read-only access for invariant checkers (INTERFACE.md 3.3 and 4)
    const UnionFind& ufPerc() const;
    const UnionFind& ufFull() const;
    const UnionFind* ufClust() const;   // nullptr if trackClusters == false

    int index(int r, int c) const { return r * L_ + c; }
    int topNode() const           { return L_ * L_; }       // virtual TOP
    int bottomNode() const        { return L_ * L_ + 1; }   // virtual BOTTOM

private:
    int  L_;
    bool trackClusters_;
    int  numOpen_    = 0;
    int  largest_    = 0;
    bool percolated_ = false;
    vector<char> open_;
    unique_ptr<UnionFind> ufPerc_;    // L*L+2 elements, TOP and BOTTOM
    unique_ptr<UnionFind> ufFull_;    // L*L+1 elements, TOP only (no BOTTOM)
    unique_ptr<UnionFind> ufClust_;   // L*L elements, no virtual nodes
};

}  // namespace perc
