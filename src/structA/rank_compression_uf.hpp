
#ifndef RANK_COMPRESSION_UF_HPP
#define RANK_COMPRESSION_UF_HPP

#include "../unionfind.hpp"

#include <string>
#include <vector>

// Union-Find implementation using union by rank
// and path compression.
class RankCompressionUF : public UnionFind {
private:

    // parent[i] stores the parent of element i.
    // If parent[i] == i, then i is a root.
    std::vector<int> parent;

    // rank[i] stores the approximate height of the tree
    // rooted at i. Only meaningful for root elements.
    std::vector<int> rank;

    // sz[i] stores the number of elements in the
    // component rooted at i. Only meaningful for roots.
    std::vector<int> sz;

    // Performance counters used to measure the efficiency
    // of the Union-Find operations.
    long long finds = 0;
    long long hops = 0;
    long long unions = 0;

public:

    // Initializes n independent elements.
    explicit RankCompressionUF(int n);

    // Finds the root of x and compresses the path
    // to make future find operations faster.
    int find(int x) override;

    // Merges the components containing x and y.
    // Returns true if merged, false if already connected.
    bool unite(int x, int y) override;

    // Checks whether x and y belong to the same component.
    bool connected(int x, int y) override;

    // Returns the size of the component containing x.
    int size(int x) override;

    // Returns the number of find operations.
    long long getFinds() const;

    // Returns the number of parent-pointer traversals.
    long long getHops() const;

    // Returns the number of successful union operations.
    long long getUnions() const;

    // Debugging functions to inspect internal DSU state.

    // Returns the parent of element x.
    int parentOf(int x) const;

    // Returns the rank of element x.
    int rankOf(int x) const;

    // Returns the stored size at index x.
    int sizeAtRoot(int x) const;

    // Returns the name of this implementation.
    std::string name() const override;
};

#endif
