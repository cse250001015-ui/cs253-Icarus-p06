
#include "rank_compression_uf.hpp"

#include <numeric>
#include <stdexcept>
#include <utility>

// Constructor: initializes n independent elements.
RankCompressionUF::RankCompressionUF(int n) {

    if (n <= 0) {
        throw std::invalid_argument("Union-Find size must be positive");
    }

    // Initially, every element is its own parent.
    parent.resize(n);
    std::iota(parent.begin(), parent.end(), 0);

    // Every element starts with rank 0.
    rank.resize(n, 0);

    // Every element initially belongs to a component of size 1.
    sz.resize(n, 1);
}

// Finds the root of x using path compression.
int RankCompressionUF::find(int x) {

    // Validate the given index.
    if (x < 0 || x >= static_cast<int>(parent.size())) {
        throw std::out_of_range("Invalid Union-Find index");
    }

    ++finds;

    // First, find the root of the component.
    int root = x;

    while (parent[root] != root) {
        ++hops;
        root = parent[root];
    }

    // Compress the path by making visited nodes
    // point directly to the root.
    while (parent[x] != x) {
        int next = parent[x];
        parent[x] = root;
        x = next;
    }

    return root;
}

// Merges two components using union by rank.
bool RankCompressionUF::unite(int x, int y) {

    int rootX = find(x);
    int rootY = find(y);

    // If both elements have the same root,
    // they are already in the same component.
    if (rootX == rootY) {
        return false;
    }

    // Attach the tree with smaller rank
    // below the tree with larger rank.
    if (rank[rootX] < rank[rootY]) {
        std::swap(rootX, rootY);
    }

    // Merge the smaller tree into the larger tree.
    parent[rootY] = rootX;

    // Update the size of the new root's component.
    sz[rootX] += sz[rootY];

    // If both trees had equal rank,
    // increase the rank of the new root.
    if (rank[rootX] == rank[rootY]) {
        ++rank[rootX];
    }

    // Count successful union operations.
    ++unions;

    return true;
}

// Checks whether two elements belong to the same component.
bool RankCompressionUF::connected(int x, int y) {
    return find(x) == find(y);
}

// Returns the size of the component containing x.
int RankCompressionUF::size(int x) {

    int root = find(x);

    return sz[root];
}

// Returns the number of find operations.
long long RankCompressionUF::getFinds() const {
    return finds;
}

// Returns the number of parent-pointer traversals.
long long RankCompressionUF::getHops() const {
    return hops;
}

// Returns the number of successful union operations.
long long RankCompressionUF::getUnions() const {
    return unions;
}

// Returns the parent of element x.
int RankCompressionUF::parentOf(int x) const {

    if (x < 0 || x >= static_cast<int>(parent.size())) {
        throw std::out_of_range("Invalid Union-Find index");
    }

    return parent[x];
}

// Returns the rank of element x.
int RankCompressionUF::rankOf(int x) const {

    if (x < 0 || x >= static_cast<int>(rank.size())) {
        throw std::out_of_range("Invalid Union-Find index");
    }

    return rank[x];
}

// Returns the stored component size at index x.
int RankCompressionUF::sizeAtRoot(int x) const {

    if (x < 0 || x >= static_cast<int>(sz.size())) {
        throw std::out_of_range("Invalid Union-Find index");
    }

    return sz[x];
}

// Returns the name of this Union-Find implementation.
std::string RankCompressionUF::name() const {
    return "RankCompressionUF";
}
