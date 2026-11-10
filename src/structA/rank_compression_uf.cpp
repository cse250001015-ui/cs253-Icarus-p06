
#include "rank_compression_uf.hpp"

#include <cassert>
#include <numeric>
#include <stdexcept>

namespace perc {

// Constructor: Initially, every element belongs
// to its own separate set.
RankCompressionUF::RankCompressionUF(int n)
    : UnionFind(n),
      parent(n),
      rank(n, 0),
      sz(n, 1) {

    if (n < 0)
        throw std::invalid_argument("Invalid DSU size");

    // Initially, each element is its own parent.
    // Example: parent = [0, 1, 2, 3, 4]
    std::iota(parent.begin(), parent.end(), 0);
}

// Find the representative/root of element x.
// Full path compression makes all nodes on the
// search path point directly to the root.
int RankCompressionUF::find(int x) {

    assert(x >= 0 && x < n_);

    // Count every invocation of find().
    cnt_.finds++;

    // If x is its own parent, it is the root.
    if (parent[x] == x)
        return x;

    // Count one parent-pointer traversal.
    cnt_.hops++;

    // Recursively find root and compress the path.
    parent[x] = find(parent[x]);

    return parent[x];
}

// Merge the sets containing a and b.
// The root with smaller rank is attached
// under the root with larger rank.
bool RankCompressionUF::unite(int a, int b) {

    assert(a >= 0 && a < n_);
    assert(b >= 0 && b < n_);

    int rootA = find(a);
    int rootB = find(b);

    // Already in the same component.
    if (rootA == rootB)
        return false;

    // If rootA has a smaller rank, attach it
    // below rootB.
    if (rank[rootA] < rank[rootB]) {

        parent[rootA] = rootB;
        sz[rootB] += sz[rootA];

    } else if (rank[rootA] > rank[rootB]) {

        // If rootB has a smaller rank,
        // attach it below rootA.
        parent[rootB] = rootA;
        sz[rootA] += sz[rootB];

    } else {

        // Equal ranks: attach rootB under rootA.
        // This is the fixed tie-breaking rule.
        parent[rootB] = rootA;
        rank[rootA]++;
        sz[rootA] += sz[rootB];
    }

    // Count one successful component merge.
    cnt_.unions++;

    return true;
}

// Return the number of elements in the
// component containing x.
int RankCompressionUF::size(int x) {

    assert(x >= 0 && x < n_);

    // Find the root, then return its stored size.
    int root = find(x);

    return sz[root];
}

// Return implementation name used by the
// factory, application and benchmark.
const char* RankCompressionUF::name() const {
    return "rank_compress";
}

// Return the direct parent of element x.
//
// Debug accessor: does not call find(),
// compress paths, or change counters.
int RankCompressionUF::parentOf(int x) const {

    assert(x >= 0 && x < n_);

    return parent[x];
}

// Return the stored rank of element x.
//
// Debug accessor: does not change the DSU.
// Rank is meaningful for root nodes.
int RankCompressionUF::rankOf(int x) const {

    assert(x >= 0 && x < n_);

    return rank[x];
}

// Return the stored component size at index r.
//
// This is meaningful when r is a root.
// Does not call find() or change counters.
int RankCompressionUF::sizeAtRoot(int r) const {

    assert(r >= 0 && r < n_);

    return sz[r];
}

} // namespace perc
