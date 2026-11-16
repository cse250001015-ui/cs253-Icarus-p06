
#ifndef RANK_COMPRESSION_UF_HPP
#define RANK_COMPRESSION_UF_HPP

#include "unionfind.hpp"

#include <vector>
#include <cstdint>

namespace perc {

// Union-Find implementation using union by rank
// and full path compression.
//
// This class inherits from the common UnionFind
// interface defined by the Architect.

class RankCompressionUF : public UnionFind {

private:

    // parent[i] stores the parent of element i.
    // A root satisfies parent[i] == i.
    std::vector<int> parent;

    // rank[i] stores an upper bound on tree height.
    // It is meaningful mainly for root elements.
    std::vector<int> rank;

    // sz[i] stores the number of elements in the set.
    // The correct size is stored at the root.
    std::vector<int> sz;

    // Recursive helper for path compression.
    // Does not count additional find calls.
    int findRoot(int x);

public:

    // Construct n independent singleton sets.
    explicit RankCompressionUF(int n);

    // Find the representative of x.
    // Compress the path to reduce future find costs.
    int find(int x) override;

    // Merge sets containing a and b using rank.
    // Return true only if two sets were merged.
    bool unite(int a, int b) override;

    // Return number of elements in x's component.
    int size(int x) override;

    // Name used by benchmarks and application.
    const char* name() const override;

    // Debug accessors required by invariant checkers.
    // They must not modify the DSU or counters.

    // Return the direct parent of x.
    int parentOf(int x) const override;

    // Return the stored rank of x.
    int rankOf(int x) const override;

    // Return the stored size at index r.
    int sizeAtRoot(int r) const override;
};

} // namespace perc

#endif // RANK_COMPRESSION_UF_HPP
