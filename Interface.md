# INTERFACE.md — Percolation with union-find (Project 6)


**Changes:** `ufFull` has `L*L+1` elements and no `BOTTOM`; equal-rank tie rule changed to match the Structure A prototype; recursive `find` accepted for `RankCompression`; header hygiene rule added; P5 reworded; open decisions rewritten. Changelog at the end.
**Rule:** once merged, this file is frozen. A change needs a PR that names every affected teammate and is approved by each of them.

Everything here is a contract between modules. If your code follows it, it will plug into the application, the oracle, the adversary and the benchmark without changes.

---

## 1. Conventions

| Item | Decision |
|---|---|
| Language / standard | C++17, namespace `perc` |
| Compiler flags | `-O2 -std=c++17 -Wall -Wextra`. Tests are built **without** `-DNDEBUG` so `assert` is active |
| Headers | `#pragma once`; `.hpp` / `.cpp`. Include only what you use: no `<bits/stdc++.h>`, and no `using namespace std` in headers (the demo runs on the examiner's compiler) |
| Grid | L × L, row 0 is the top, row L−1 is the bottom |
| Site index | site (r, c) has index `r*L + c`, 0-indexed, so indices are `0 .. L*L-1` |
| Virtual nodes | `TOP = L*L`, `BOTTOM = L*L + 1` |
| Neighbours | up (r−1,c), down (r+1,c), left (r,c−1), right (r,c+1). **No wraparound** |
| Integer types | site and element indices are `int`; counters are `uint64_t` |
| Library containers | Lab rule: `std::map`, `std::set`, `std::priority_queue` and similar may not stand in for a structure under study; they are allowed only in tests and data generation. In `src/` and `oracle/` we use only plain arrays and write everything else ourselves, including the oracle's BFS queue (a hand-written array queue). Whether `std::vector` as plain array storage is accepted is open decision 11 |

---

## 2. File layout and owners

| Path | Contents | Owner |
|---|---|---|
| `src/unionfind.hpp` | `Counters`, `UnionFind`, `Variant`, `makeUnionFind` | Architect |
| `src/percolation.hpp` | `Percolation` class declaration | Architect |
| `src/sweep.hpp` | `SweepRecord`, `SweepResult`, sweep functions | Architect |
| `src/structA/rank_compression_uf.{hpp,cpp}` | class `RankCompressionUF` | Structure A owner |
| `src/structA/percolation.cpp`, `sweep.cpp` | implementations of the two layers above | Structure A owner |
| `src/structB/naive_uf.*`, `rank_uf.*`, `compression_uf.*` | classes `NaiveUF`, `RankUF`, `CompressionUF` | Structure B owner |
| `src/factory.cpp` | `makeUnionFind` | Architect |
| `oracle/flood_fill.{hpp,cpp}` | `FloodFillOracle` | Oracle owner |
| `adversary/` | generators for the file formats in section 7 | Adversary |
| `app/` | `percolation_app` | Architect (+ Adversary helping) |
| `bench/` | benchmark harness | Analyst |

---

## 3. Union-find interface

```cpp
// src/unionfind.hpp
#pragma once
#include <cstdint>
#include <memory>

namespace perc {

struct Counters {
    uint64_t finds  = 0;   // calls to find(), including those made inside unite()/size()
    uint64_t hops   = 0;   // parent-pointer steps taken walking from x to its root (see 3.2)
    uint64_t unions = 0;   // successful merges (unite() returned true)
};

enum class Variant { Naive, Rank, Compression, RankCompression };

class UnionFind {
public:
    explicit UnionFind(int n) : n_(n) {}
    virtual ~UnionFind() = default;

    virtual int  find(int x)          = 0;  // representative (root) of x's set
    virtual bool unite(int a, int b)  = 0;  // true iff two different sets were merged
    virtual int  size(int x)          = 0;  // number of elements in x's set
    virtual const char* name() const  = 0;

    // Debug accessors (section 3.3): read-only, no side effects, never counted.
    virtual int parentOf(int x)   const = 0;   // parent[x]; a root returns itself
    virtual int rankOf(int x)     const = 0;   // stored rank of root x; -1 if the variant has no rank
    virtual int sizeAtRoot(int r) const = 0;   // size[r]; only meaningful when parentOf(r) == r

    bool connected(int a, int b) { return find(a) == find(b); }
    int  numElements() const     { return n_; }
    const Counters& counters() const { return cnt_; }
    void resetCounters()         { cnt_ = Counters{}; }

protected:
    int n_;
    Counters cnt_;
};

std::unique_ptr<UnionFind> makeUnionFind(Variant v, int n);   // src/factory.cpp
const char* variantName(Variant v);        // "naive" "rank" "compress" "rank_compress"
bool parseVariant(const char* s, Variant& out);

}  // namespace perc
```

Each concrete class has a constructor `Class(int n)` that starts with every element in its own set.

### 3.1 Linking rules (these are fixed, because the Adversary's sequences depend on them)

Let `ra = find(a)`, `rb = find(b)`. If `ra == rb`, return `false` and change nothing.

| Variant | Rule when `ra != rb` |
|---|---|
| `Naive` | `parent[ra] = rb`. `find` does **not** compress |
| `Rank` | Lower rank goes under higher rank. **On equal rank, `parent[rb] = ra` and `rank[ra]++`**. No compression |
| `Compression` | Same linking as `Naive` (`parent[ra] = rb`). `find` uses full path compression |
| `RankCompression` | Same linking as `Rank`. `find` uses full path compression |

Note that the tie rule attaches `rb` under `ra`, the opposite direction from `Naive`. The direction is arbitrary but fixed, and it is the one the Structure A prototype already uses.

All four variants also maintain `size[]` at roots (`size[rb] += size[ra]`), since the application needs cluster sizes.

Path compression means: after finding the root, every node on the path is re-pointed directly to the root (two passes). `find` must be **iterative for `Compression`**, whose paths can be Theta(n) long and would overflow the stack if recursive. For `RankCompression` the height is at most floor(log2 n), so a recursive `find` cannot overflow and is accepted, provided `hops` is counted as defined in 3.2.

### 3.2 Counter definitions

- `finds` increases by 1 on **every** call to `find`, including calls made inside `unite`, `size` and `connected`.
- `hops` increases by the number of **edges walked from x up to its root** in the first pass of `find`. The second (compression) pass is **not** counted. This is the "find-path length" the Analyst reports. If x is a root, the cost is 0 hops.
- `unions` increases by 1 each time `unite` returns `true`.
- Observation calls (`isFull`, `percolates`, `largestCluster`) are counted like any other call. All four variants run the identical call sequence, so comparisons stay fair.

### 3.3 Debug accessors

`parentOf`, `rankOf` and `sizeAtRoot` exist so the invariant checkers (section 9) can inspect the internal arrays from outside.

- They are `const`: no path compression, no counter change, no other side effect. In particular they must not call `find`.
- `parentOf(x)` returns `parent[x]`, so `parentOf(x) == x` means x is a root.
- `rankOf(x)` returns the stored rank when x is a root, and `-1` for every element of `Naive` and `Compression` (which keep no rank). For a non-root x the value is unspecified, but the call must not crash.
- `sizeAtRoot(r)` returns `size[r]`. For a non-root r the value is unspecified, but the call must not crash.
- Arguments outside `[0, n)` are caught by `assert`.

A checker walks parents with `parentOf` to find a root without disturbing the structure, which is how U1 and U2 are tested.

---

## 4. Percolation layer

```cpp
// src/percolation.hpp
#pragma once
#include "unionfind.hpp"
#include <vector>

namespace perc {

class Percolation {
public:
    Percolation(int L, Variant v, bool trackClusters = true);

    bool open(int r, int c);            // true if the site was newly opened, false if already open
    bool isOpen(int r, int c) const;
    bool isFull(int r, int c);          // open AND reachable from the top row through open sites (no backwash)
    bool percolates();                  // some open top-row site is connected to some open bottom-row site
    int  numOpen() const;
    int  largestCluster() const;        // size of the largest cluster of open sites; 0 if none open
                                        // requires trackClusters == true
    int  L() const;

    Counters counters() const;          // sum of the three below
    const Counters& countersPerc() const;
    const Counters& countersFull() const;
    const Counters& countersClust() const;
    void resetCounters();

    // Read-only access for invariant checkers (section 3.3 explains what they may call)
    const UnionFind& ufPerc() const;
    const UnionFind& ufFull() const;
    const UnionFind* ufClust() const;   // nullptr if trackClusters == false
};

}  // namespace perc
```

`open`, `isOpen` and `isFull` throw `std::out_of_range` for coordinates outside `[0, L)`. The union-find classes themselves use `assert` only, because they are hot paths.

### 4.1 Why there are three union-find objects

| Object | Elements | Virtual nodes | Used for |
|---|---|---|---|
| `ufPerc` | `L*L + 2` | TOP and BOTTOM | `percolates()` |
| `ufFull` | `L*L + 1` | TOP only (there is no `BOTTOM` element) | `isFull()` |
| `ufClust` | `L*L` | none | `largestCluster()` |

- **Backwash.** If a single structure held both virtual nodes, then after the grid percolates, a bottom-row site would be connected to `BOTTOM`, and `BOTTOM` to `TOP`, so the site would look full even if no open path reaches it from the top. `ufFull` has no `BOTTOM` element at all, which avoids this. (This is derivation D3.)
- **Cluster sizes.** The set containing a virtual node merges every site touching that row, and those sites need not be adjacent. So sizes read from `ufPerc` or `ufFull` are not true cluster sizes. `ufClust` has no virtual nodes, so its set sizes are exactly the cluster sizes. Example: on a 3x3 grid, open (0,0) and then (0,2). Both join TOP, so a size read from a structure with TOP is 2, but the true largest cluster is 1 (a single-structure prototype reported 2 here).
- **Cost.** `ufClust` is optional (`trackClusters = false`) for runs that only need the threshold.

### 4.2 Exact behaviour of `open(r, c)`

The order of operations is fixed so that counters and adversarial sequences are reproducible. In every `unite(x, y)` below, the **first argument is the newly opened site** and the **second is the virtual node or the neighbour**.

```
idx = r*L + c
if open[idx]: return false
open[idx] = true; numOpen++
if r == 0:      ufPerc.unite(idx, TOP);    ufFull.unite(idx, TOP)
if r == L-1:    ufPerc.unite(idx, BOTTOM)
for nb in [up, down, left, right]:          // in this order, skipping off-grid sites
    if open[nb]:
        ufPerc.unite(idx, nb); ufFull.unite(idx, nb)
        if trackClusters: ufClust.unite(idx, nb)
if trackClusters: largest = max(largest, ufClust.size(idx))
if !percolated:  percolated = ufPerc.connected(TOP, BOTTOM)
return true
```

For `L = 1` the single site is in both the top and bottom row, so it percolates immediately.

`isFull(r,c)` returns `isOpen(r,c) && ufFull.connected(idx, TOP)`.
`percolates()` returns the cached `percolated` flag.

---

## 5. Sweep hook (Newman–Ziff)

One sweep opens the sites in a random order, one at a time, and reports the state after **every** opening. This covers every occupation level in one pass.

```cpp
// src/sweep.hpp
#pragma once
#include "percolation.hpp"
#include <cstdint>
#include <functional>
#include <vector>

namespace perc {

struct SweepRecord {          // state after the n-th site has been opened
    int  n;                   // 1, 2, 3, ... number of open sites
    bool percolated;
    int  largestCluster;      // 0 if clusters are not tracked
};

using SweepCallback = std::function<void(const SweepRecord&)>;

struct SweepResult {
    int      nCritical;       // smallest n with percolated == true; -1 if never
    Counters perc, full, clust;   // final counter values of the three structures
};

uint64_t splitmix64(uint64_t x);                              // definition in 5.1
uint64_t sweepSeed(uint64_t baseSeed, int L, int sweepIndex); // definition in 5.1
std::vector<int> makeOpeningOrder(int L, uint64_t seed);      // permutation of 0 .. L*L-1

// Opens sites in the given order (distinct indices, need not cover the grid).
// If stopAtPercolation is true, returns right after the first percolating opening.
SweepResult runSweepOrder(int L, Variant v, const std::vector<int>& order,
                          const SweepCallback& cb, bool trackClusters = true,
                          bool stopAtPercolation = false);

// Convenience: makeOpeningOrder(L, seed) followed by runSweepOrder.
SweepResult runSweep(int L, Variant v, uint64_t seed,
                     const SweepCallback& cb, bool trackClusters = true,
                     bool stopAtPercolation = false);

}  // namespace perc
```

The threshold sample from one sweep is `p_hat = nCritical / (L*L)`.

### 5.1 Determinism

`std::shuffle` and `std::uniform_int_distribution` give different results on different standard libraries, so the order is generated by our own code, identical everywhere:

```
splitmix64(x):  z = x + 0x9E3779B97F4A7C15
                z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9
                z = (z ^ (z >> 27)) * 0x94D049BB133111EB
                return z ^ (z >> 31)               // all arithmetic mod 2^64

sweepSeed(base, L, k) = splitmix64(base + (uint64_t)L * 1000003 + (uint64_t)k)

makeOpeningOrder(L, seed):
    rng = std::mt19937_64(seed)
    a = [0, 1, ..., L*L-1]
    for i = L*L-1 down to 1:
        j = rng() % (i + 1)
        swap(a[i], a[j])
    return a
```

The same seed gives the same order for all four variants, so their `SweepRecord` sequences must be identical (invariant S2); only counters and time differ.

---

## 6. Oracle interface

`FloodFillOracle` (small grids only) has the same method names as `Percolation` so the differential harness can drive both with one loop:

```cpp
class FloodFillOracle {
public:
    explicit FloodFillOracle(int L);
    bool open(int r, int c);
    bool isOpen(int r, int c) const;
    bool isFull(int r, int c);        // BFS from the top row through open sites
    bool percolates();
    int  numOpen() const;
    int  largestCluster();            // BFS component labelling
};
```

It must not use any union-find code.

---

## 7. File formats

Text files, one record per line. Lines starting with `#` are comments and are ignored.

**Opening-order file** (`*.order`): used for replaying and for backwash-exposing sequences.
```
L
idx
idx
...
```
`L` on the first line, then distinct site indices, one per line. It may stop before covering the whole grid.

**Union-sequence file** (`*.unions`): used for the naive-quadratic and rank-height generators.
```
n
u a b      # unite(a, b)
f x        # find(x)
```
`n` on the first line (number of elements). The `f` lines let a generator force long finds. Commands are replayed in order, directly on a `UnionFind` object of the chosen variant.

**Generators** write to a file given by `--out`; the replay tool reads a file given by `--replay`.

---

## 8. Application and output files

```
percolation_app --sizes 50,100,200,500,1000 --sweeps T --variant rank_compress|naive|rank|compress|all
                --seed S --out DIR [--clusters] [--replay FILE]
```

All output files go to `DIR` (default `results/`). **Provisional**: the exact columns of `extrapolation.csv` and `clusters.csv` are settled with the Analyst.

| File | One row per | Columns |
|---|---|---|
| `sweeps.csv` | sweep | `variant,L,sweep,seed,n_critical,p_hat,finds,hops,unions,time_ns` |
| `summary.csv` | (variant, L) | `variant,L,T,mean_p,sd,se,ci_low,ci_high` |
| `extrapolation.csv` | variant | `variant,nu,p_c,a,p_c_se` |
| `clusters.csv` | (variant, L, size) | `variant,L,at_n,size,count` |

- `se = sd / sqrt(T)`, `ci_low/high = mean_p ∓ 1.96 * se`.
- Extrapolation fits `p(L) = p_c + a * L^(-1/nu)` with `nu = 4/3`, by least squares of `mean_p` against `L^(-3/4)`.
- The seed of every sweep is stored in `sweeps.csv`, so any figure can be regenerated.

---

## 9. Invariants

Each one is checkable. The Oracle owner writes a checker for every line. Checkers read internal state only through the debug accessors (section 3.3) and the `ufPerc()`, `ufFull()`, `ufClust()` accessors (section 4).

**Grid**
- **G1** Indices `0 .. L*L-1` map one-to-one to (r, c); `TOP = L*L`, `BOTTOM = L*L+1`.
- **G2** A site is united only with an open 4-neighbour, or with a virtual node according to its row (section 4.2).

**Union-find (all variants)**
- **U1** For every x, `0 <= parent[x] < n`. Following parents from any x reaches a root (`parent[r] == r`) in at most `n-1` steps, with no cycles.
- **U2** `size[r]` for a root `r` equals the number of elements whose root is `r`. The sizes of all roots sum to `n`.
- **U3** `find(x) == find(y)` exactly when x and y are joined by a chain of successful `unite` calls. All four variants produce the same partition after every call.
- **U4** `unite` returns `true` exactly when it changes the number of sets, and then `unions` increases by 1.
- **U5** (rank variants) For every root `r`: `size[r] >= 2^rank[r]`, hence tree height is at most `floor(log2 n)`. For `Rank` (no compression), `rank[r]` equals the height of the tree at `r`. With compression, `rank[r]` is only an upper bound on the height.

**Percolation**
- **P1** A site, once open, stays open. `numOpen` equals the number of open flags.
- **P2** For open sites x, y: connected in `ufClust` implies connected in `ufFull`, which implies connected in `ufPerc`.
- **P3** `isFull(r,c)` is true exactly when the site is open and reachable from an open top-row site through open sites (the oracle's definition).
- **P4** `percolates()` is true exactly when some bottom-row site is full.
- **P5** `ufFull` has exactly `L*L + 1` elements (no `BOTTOM`), so backwash cannot happen: `ufFull().numElements() == L*L + 1` always.
- **P6** `largestCluster()` equals the size of the largest connected component of open sites. It never decreases, and `percolated` never goes from true to false.

**Sweep**
- **S1** `SweepRecord.n` takes the values 1, 2, 3, ... with no gaps.
- **S2** For the same order, all four variants give identical `SweepRecord` sequences.
- **S3** `nCritical` is the first `n` with `percolated == true`, or `-1` if there is none.

---

## 10. Build and run

| Command | Does |
|---|---|
| `make test` | builds and runs unit tests and the differential test |
| `make app` | builds `percolation_app` |
| `make bench` | builds and runs the benchmark harness |
| `./run_all.sh` | tests, then the application, then all benchmarks (the one command in README) |


