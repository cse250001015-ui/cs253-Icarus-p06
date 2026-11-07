
#include <bits/stdc++.h>
#include "rank_compression_uf.hpp"

using namespace std;

// Percolation class represents an N x N grid
// Each site is initially blocked
// Open sites are connected using Union-Find

class Percolation {

private:
    int n;                  // Grid dimension
    int virtualTop;         // Virtual node connected to top row
    int virtualBottom;      // Virtual node connected to bottom row

    int openCount = 0;      // Number of open sites
    int maxClusterSize = 0; // Largest cluster of open sites

    // false = blocked, true = open
    vector<vector<bool>> opened;

    // DSU 1: Detects whether the grid percolates
    // Contains both virtual top and bottom nodes
    DSU dsu;

    // DSU 2: Checks whether a site is full
    // Contains only virtual top to prevent backwash
    DSU fullnessDsu;

    // DSU 3: Tracks actual cluster sizes
    // Does not contain virtual nodes
    DSU clusterDsu;

    // Convert row and column to a unique 1D index
    // Example: In a 5x5 grid, (2,3) becomes 13
    int getId(int row, int col) const {
        return row * n + col;
    }

    // Check whether a site lies inside the grid
    bool valid(int row, int col) const {
        return row >= 0 && row < n &&
               col >= 0 && col < n;
    }

    // Connect the current site with a neighboring site
    // Only connect if that neighboring site is open
    void connectNeighbor(int row, int col,
                         int nr, int nc) {

        // Ignore neighbors outside the grid
        if (!valid(nr, nc))
            return;

        // Ignore blocked neighbors
        if (!opened[nr][nc])
            return;

        int a = getId(row, col);
        int b = getId(nr, nc);

        // Update percolation connectivity
        dsu.unite(a, b);

        // Update fullness connectivity
        fullnessDsu.unite(a, b);

        // Update actual cluster connectivity
        clusterDsu.unite(a, b);
    }

public:

    // Constructor: Create an N x N blocked grid
    explicit Percolation(int size)
        : n(size),
          virtualTop(size * size),
          virtualBottom(size * size + 1),

          opened(size, vector<bool>(size, false)),

          // N*N sites + virtual top + virtual bottom
          dsu(size * size + 2),

          // N*N sites + virtual top only
          fullnessDsu(size * size + 1),

          // N*N actual sites only
          clusterDsu(size * size) {

        if (size <= 0 || size > 1000)
            throw invalid_argument("Invalid grid size");
    }

    // Open the site at (row, col)
    void open(int row, int col) {

        // Reject invalid coordinates
        if (!valid(row, col))
            throw out_of_range("Invalid row or column");

        // A site already open should not be counted again
        if (opened[row][col])
            return;

        // Mark site as open
        opened[row][col] = true;
        openCount++;

        int id = getId(row, col);

        // If site is in the top row,
        // connect it to virtual top in both DSUs
        if (row == 0) {
            dsu.unite(id, virtualTop);
            fullnessDsu.unite(id, virtualTop);
        }

        // If site is in the bottom row,
        // connect it to virtual bottom only in dsu
        if (row == n - 1) {
            dsu.unite(id, virtualBottom);
        }

        // Connect all four adjacent open sites
        connectNeighbor(row, col, row - 1, col); // Up
        connectNeighbor(row, col, row + 1, col); // Down
        connectNeighbor(row, col, row, col - 1); // Left
        connectNeighbor(row, col, row, col + 1); // Right

        // Find the size of this site's connected cluster
        int currentSize = clusterDsu.size(id);

        // Update maximum cluster size
        maxClusterSize = max(maxClusterSize, currentSize);
    }

    // Return true if the site is open
    bool isOpen(int row, int col) const {
        return valid(row, col) && opened[row][col];
    }

    // Return true if open sites connect top to bottom
    bool percolates() {
        return dsu.connected(virtualTop, virtualBottom);
    }

    // A site is full if it is open and connected
    // to the top row through open sites
    bool isFull(int row, int col) {

        // A blocked site can never be full
        if (!isOpen(row, col))
            return false;

        // Use fullnessDsu to avoid backwash
        return fullnessDsu.connected(
            getId(row, col), virtualTop
        );
    }

    // Return total number of open sites
    int numOpen() const {
        return openCount;
    }

    // Check whether two open sites are connected
    bool connected(int r1, int c1,
                   int r2, int c2) {

        // Both sites must be open
        if (!isOpen(r1, c1) || !isOpen(r2, c2))
            return false;

        // Use clusterDsu so virtual nodes do not
        // incorrectly connect separate clusters
        return clusterDsu.connected(
            getId(r1, c1),
            getId(r2, c2)
        );
    }

    // Get the number of sites in a connected cluster
    int clusterSize(int row, int col) {

        // Blocked sites belong to no open cluster
        if (!isOpen(row, col))
            return 0;

        return clusterDsu.size(getId(row, col));
    }

    // Return size of largest open cluster
    int largestCluster() const {
        return maxClusterSize;
    }

    // Return grid dimension
    int gridSize() const {
        return n;
    }

    // Return fraction of sites currently open
    double occupationFraction() const {
        return static_cast<double>(openCount) / (n * n);
    }
};
