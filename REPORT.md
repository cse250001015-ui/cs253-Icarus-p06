# Percolation with union-find — Project 6

## 1. Team and contributions

| Member | Contribution | Evidence / PR |
|---|---|---|
| Hemanth — Structure A owner (D1) | Implemented the rank + path-compression union-find, percolation wrapper, three-structure layout and Newman–Ziff sweep. | Main files: `src/structA/`. PR: https://github.com/cse250001015-ui/cs253-Icarus-p06/pull/5 |
| Datthatreya — Structure B owner (D2) | Implemented the naive, rank-only and compression-only union-find variants and their unit tests. | Main files: `src/structB/`. PR: https://github.com/cse250001015-ui/cs253-Icarus-p06/pull/9. |
| Piyush — Architect (D3) | Defined the shared C++ interfaces and factory, seeded sweep helpers, statistics and finite-size fit, and integrated the application and workflow. The derivation concerns the two-structure solution to backwash. | Main files: `include/`, `src/factory.cpp`, `src/rng.cpp`, `src/stats.cpp`, `src/fit.cpp`, `app/main.cpp`. PR: https://github.com/cse250001015-ui/cs253-Icarus-p06/pull/12. |
| Manirathna (Mani) — Oracle owner (D4) | Built an independent BFS flood-fill oracle, seeded differential tests with replay and shrinking, sweep comparisons, U/P/S/L invariant checkers, stand-ins with injectable faults, and Oracle build targets. | Main files: `oracle/flood_fill.*`, `oracle/differential.hpp`, `oracle/diff_main.cpp`, `oracle/sweep_main.cpp`, `oracle/oracle.mk`, `tests/checkers.hpp`, `tests/percolation_checkers.hpp`, `tests/sweep_checkers.hpp`, `tests/ref_*.hpp`, `tests/test_*.cpp`. PR: https://github.com/cse250001015-ui/cs253-Icarus-p06/pull/10 |
| Aman — Adversary (D5) | Aman reports writing the naive-quadratic, rank-height and backwash generators, the replay support/tests, cluster histogram support, and D5. The supplied contribution also lists `adversary/README.md`, `app/cluster_histogram.*`, `app/cluster_hist_main.cpp`, `tests/test_cluster_histogram.cpp`, and `docs/derivations/D5_adversarial_sequences.pdf`. | Main present files: `adversary/gen_*.cpp`, `app/replay.*`, `tests/test_replay.cpp`. PR links: https://github.com/cse250001015-ui/cs253-Icarus-p06/pull/7. |
| Dhanvanth — Analyst (D6) | Built the benchmark harness and recorded five-repetition timing/hop data; prepared the finite-size results and cost model. | Main files: `bench/main.cpp`, `results/raw/`, `scripts/plot_results.py`. PR: https://github.com/cse250001015-ui/cs253-Icarus-p06/pull/11 |


## 2. Problem

Site percolation asks how much of a square lattice must be occupied before an open path connects its top edge to its bottom edge. For an $L\times L$ grid, each site starts closed; a randomized order opens sites one at a time, and the application detects the first opening count at which the grid spans. The project uses grids through $L=1000$, estimates the finite-size threshold $p_c(L)$, and extrapolates those estimates to the infinite-lattice threshold. It also compares four disjoint-set implementations by their operation counts and measured sweep time.

## 3. Interface and invariants

The module contract is in `INTERFACE.md`. `UnionFind` provides `find`, `unite`, `size`, `connected`, and read-only debug accessors. `Percolation` provides `open`, `isOpen`, `isFull`, `percolates`, `numOpen`, and `largestCluster`. A sweep opens sites in a seeded permutation and reports per-occupation records and the first percolating count. Counters record finds, parent-pointer hops, and successful unions; a hop is an edge walked in the first pass of `find`.

The invariant numbering follows `INTERFACE.md` §9. G1–G2 (index mapping and legal neighbor/virtual-node unions) are exercised through the differential and sweep behavior checks; the named checker functions are listed below.

| ID | Property checked | Checker |
|---|---|---|
| U1 | Parent indices are in range; every chain reaches a root in at most $n-1$ steps, with no cycles. | `checkU1` |
| U2 | Each root stores the size of its set, and root sizes sum to $n$. | `checkU2` |
| U3 | The union-find partition equals the expected partition independently derived from the oracle's component labels, including virtual TOP/BOTTOM classes. | `checkU3` |
| U4 | `unite` returns true exactly on a merge; the set count and successful-union counter change by one then, and by zero otherwise. Also checked over each `open()`'s union window. | `checkU4Unite`, `checkU4Window` |
| U5 | Rankless variants report −1; ranked variants satisfy $size\ge2^{rank}$ and height $\le\lfloor\log_2n\rfloor$. Rank-only has rank = height; rank + compression has height $\le$ rank. | `checkU5` |
| L1 | Naive/compression link first root under second; rank variants link lower under higher and send the second root under the first on ties, incrementing the winner's rank. | `checkLinkingRule` |
| P1 | Open flags never revert, and `numOpen()` equals the count of open sites. | `PercMonitor::checkP1` |
| P2 | Connectivity partitions refine in order `ufClust` → `ufFull` → `ufPerc` for open sites. | `PercMonitor::checkP2` |
| P3 | `isFull` exactly matches oracle top-row reachability; a full site is open. | `PercMonitor::checkP3` |
| P4 | Percolation matches both a full bottom-row site and the oracle's answer. | `PercMonitor::checkP4` |
| P5 | `ufFull` has $L^2+1$ elements and no BOTTOM node. | `PercMonitor::checkP5` |
| P6 | Largest cluster matches the oracle and never shrinks; percolation never changes from true to false. | `PercMonitor::checkP6` |
| S1 | Sweep record $i$ reports $n=i+1$. | `checkS1` |
| S2 | All four variants produce identical observations for the same opening order. | `checkS2` |
| S3 | `nCritical` is the first percolating record's count, or −1 if none occurs. | `checkS3` |

The top-level `make test` runs the unit, checker and stand-in harness tests. In this checkout, `oracle/oracle.mk` is not included by the root `Makefile`; real-structure comparisons are a separate command documented in Section 7.

## 4. Structure A

### 4.1 Representation and purpose

Structure A implements a disjoint-set union (DSU), also called Union-Find, which maintains a partition of elements into connected components represented by rooted trees. `RankCompressionUF` stores three `std::vector<int>` arrays: `parent[i]` is the parent pointer; `rank[i]` is a balancing rank; and `sz[i]` is the component size at a root. Initially, each element is its own set: `parent[i]=i`, `rank[i]=0`, and `sz[i]=1`. The root is the element whose parent is itself. Two elements are in the same component exactly when their parent paths reach the same root. Initialization uses $O(n)$ time and space.

The class implements the shared `UnionFind` interface: `find`, `unite`, `size`, and read-only diagnostic accessors `parentOf`, `rankOf`, and `sizeAtRoot`. These accessors expose state for invariant checking without modifying the DSU or its counters. `connected(a,b)` is provided by the common interface and compares the roots returned by two finds.

### 4.2 Operations

**`find(x)` — root search with path compression.** The recursive helper follows parent pointers until it reaches a root, then points each visited node directly to that root on the return path. For example, a path $3\to2\to0$ becomes $3\to0$ and $2\to0$ after `find(3)`. This changes the tree shape but preserves the component partition. The public call increments `finds` once; each parent edge walked in the first pass increments `hops`. Recursive helper calls do not increment the public find count.

**`unite(a,b)` — merge by rank.** The method finds roots $r_a$ and $r_b$. If they are equal, it returns false without changing the partition or successful-union counter. Otherwise it attaches the lower-rank root below the higher-rank root and adds the losing root's size to the winner's size. On an equal-rank tie, the fixed interface rule attaches $r_b$ below $r_a$ and increments `rank[r_a]`. A successful merge returns true and increments `unions` once.

**`size(x)` and `connected(a,b)`.** `size(x)` returns `sz[find(x)]`; `connected` compares `find(a)` and `find(b)`. Both use the same root lookup and do not scan the component's elements.

### 4.3 Invariants and preservation

1. **Valid parent forest:** every parent index is in $[0,n)$, and every path reaches exactly one self-parented root without a cycle. A union points one root at another root; compression points a node at its existing root, so neither operation creates a cycle.
2. **Correct partition:** elements share a set exactly when they have the same root. Find and compression preserve roots; a successful union merges exactly two distinct sets.
3. **Correct root sizes:** each root's `sz` equals the number of elements in its component. Singleton initialization establishes the base case, and each successful merge adds the sizes of two disjoint components once. Nonroot sizes are not used as component sizes.
4. **Rank and height:** rank is meaningful at roots, and a nonroot's stored rank is strictly less than its parent's rank. Only equal-rank root merges increase rank, by one. Compression redirects a node to an ancestor root of greater rank and cannot increase actual height.

These properties are checked by U1–U5 and L1 in Section 3. The rankless variants expose `rankOf(root)=-1`; Structure A is the rank + compression variant.

### 4.4 Time complexity and rank-height argument (D1)

A root of rank $r$ has at least $2^r$ elements. This holds initially because rank-zero singleton roots have one element. To create a root of rank $r+1$, union by rank must merge two roots of rank $r$; by induction their disjoint components each have at least $2^r$ elements, so the new component has at least $2^{r+1}$. Since the DSU has at most $n$ elements, $2^r\le n$, and $r\le\lfloor\log_2n\rfloor$. Rank strictly increases along every parent path, so tree height is at most the root rank. Path compression only shortens paths. This proves the logarithmic worst-case height bound and the safety of the recursive find for this implementation.

Consequently, one `find`, `unite`, or `connected` takes $O(\log n)$ worst-case time. With union by rank and path compression, a sequence of operations takes inverse-Ackermann amortized time per operation, conventionally written $O(\alpha(n))$ with the standard dependence on sequence length. `size` has the same bounds as `find`; initialization and storage are $O(n)$.

### 4.5 Percolation implementation (`percolation.cpp`)

The grid's open/closed flags are stored in `open_`; site $(r,c)$ maps to index $rL+c$. `numOpen_` counts distinct open sites. Out-of-range coordinates are rejected, and opening an already-open site returns false without changing the grid or counters. A newly opened site is marked, counted, joined to any required virtual boundary node, and connected to each open orthogonal neighbor among up, down, left, and right. Closed and out-of-grid neighbors are skipped, so there are at most four neighbor checks per opening.

Three DSUs answer three different questions:

| DSU | Elements and virtual nodes | Query |
|---|---|---|
| `ufPerc_` | $L^2$ sites plus TOP and BOTTOM | `percolates()` tests TOP–BOTTOM connectivity. |
| `ufFull_` | $L^2$ sites plus TOP only | `isFull(r,c)` tests whether an open site reaches TOP. |
| `ufClust_` (optional) | $L^2$ real sites, no virtual nodes | `largestCluster()` reports the largest physical open component. |

Top-row sites join TOP in `ufPerc_` and `ufFull_`; bottom-row sites join BOTTOM only in `ufPerc_`. Excluding BOTTOM from `ufFull_` prevents backwash: a site connected to the bottom after spanning is not called full unless it also has an open path to the top. Excluding both virtual nodes from `ufClust_` prevents boundary sentinels from merging otherwise separate real clusters or inflating their sizes. After neighbor unions, the size of the new site's component updates the cached maximum cluster size. Percolation is checked until first detected and then cached; `percolates()` is constant time, while `isFull` performs a union-find connectivity query after checking that the site is open.

An opening does a constant number of DSU operations, so with rank and compression it costs $O(\alpha(L^2))$ amortized and $O(\log(L^2))$ worst case. `isOpen`, `numOpen`, `largestCluster`, and cached `percolates` are $O(1)$; `isFull` has the union-find query bound. Cluster tracking is optional, avoiding the third DSU's arrays and unions for threshold-only runs.

### 4.6 Newman–Ziff incremental sweep (`sweep.cpp`)

`runSweepOrder(L,v,order,cb,trackClusters,stopAtPercolation)` creates one grid and validates that every supplied index is in range and unique. A partial order is allowed. It opens sites in that order without rebuilding previous connectivity, then records the occupation count, percolation state, and largest-cluster size (or zero if cluster tracking is disabled) after each opening. A callback is optional. `nCritical` stores the first spanning count, or remains $-1$ if the supplied order ends without spanning. The optional stop flag ends the sweep at first percolation; otherwise the full order continues. The result returns separate counters for the percolation, fullness and optional cluster DSUs. `runSweep` obtains its seeded opening permutation from the shared `makeOpeningOrder` helper and delegates to `runSweepOrder`.

For $N=L^2$, a complete sweep performs $O(N)$ neighbor checks and $O(N\alpha(N))$ amortized DSU work, plus callback work. Grid state, DSUs, order validation flags and the opening order use $O(N)$ space. This is the incremental occupation-and-measurement part of Newman–Ziff; averaging over independent opening orders is done by the application/analyst.

### 4.7 Design choices and alternatives

- **Rank instead of arbitrary attachment:** arbitrary unions can create linear-height chains; rank bounds the height by $O(\log n)$ even before compression.
- **Compression in addition to rank:** rank alone can repeatedly traverse the same paths. Compression reduces later traversals while preserving the partition.
- **Stored rank rather than exact dynamic height:** compression changes actual heights, so maintaining exact heights would require extra bookkeeping. Rank remains an inexpensive upper bound.
- **Separate connectivity structures:** a single DSU with both virtual boundaries causes backwash in fullness queries; virtual nodes also corrupt real cluster sizes. The extra arrays and unions keep percolation, fullness and physical cluster meanings separate.
- **Optional cluster DSU:** threshold-only runs can skip `ufClust_` and its work when cluster statistics are unnecessary.
- **Incremental sweeps:** retaining one evolving grid per sweep avoids rebuilding connectivity independently at each occupation level.

### 4.8 Verification and integration status

The Structure A owner supplied a draft that mentions `tests/test_structA.cpp`; that file is not present in this repository copy, so the report does not claim those direct unit tests were run here. `tests/test_smoke.cpp` checks percolation construction and DSU layout through the factory, while the saved real-structure differential and sweep run in Section 7 exercises all four factory-selected variants and passes with seed 171226. The earlier placeholder-factory issue is fixed in the current `src/factory.cpp`; the stale statement that real factory-backed integration remains unresolved has therefore been removed. Add the owner’s D1 derivation artifact and merged PR link if required by the submission package, and have the owner review this section before finalizing.

Code locations: `RankCompressionUF::find`, `unite`, and `size` in `src/structA/rank_compression_uf.cpp`; `Percolation::open`, `isFull`, `percolates`, and `largestCluster` in `src/structA/percolation.cpp`; `runSweepOrder` and `runSweep` in `src/structA/sweep.cpp`.

## 5. Structure B

Structure B is the control group for Structure A, which combines union by rank with path compression. Its three variants remove one of those heuristics at a time. All implement the shared `UnionFind` interface in `INTERFACE.md` §3, so the application and Oracle harness can use them through the same factory and contract.

### 5.1 Representation

Each implementation stores a forest in flat `std::vector<int>` arrays indexed by element ID. Every root has `parent[root] == root`; `size[root]` stores the component size and is meaningful only at roots. `rankOf` returns -1 for variants that do not store rank.

| Variant | Stored arrays | Linking rule | `find` behavior |
|---|---|---|---|
| `NaiveUF` | `parent`, `size` | Set `parent[ra] = rb` | Follow parent pointers; no compression |
| `RankUF` | `parent`, `size`, `rank` | Lower-rank root goes below higher-rank root; ties put `rb` under `ra` and increment `rank[ra]` | Follow parent pointers; no compression |
| `CompressionUF` | `parent`, `size` | Same direction as naive | Iterative two-pass full path compression |

### 5.2 Operations and complexity

`unite(a,b)` performs two `find` calls and then a constant-time merge when the roots differ. `size(x)` and `connected(a,b)` use `find`. The read-only debug accessors inspect arrays directly and do not call `find` or change counters.

| Variant | Worst-case single `find` | Amortized cost | Array storage |
|---|---|---|---|
| Naive | $n-1$ parent edges; $\Theta(n)$ | $\Theta(n)$ per find on a chain; $\Theta(n^2)$ for $n$ such finds | $2n$ integers |
| Rank only | At most $\lfloor\log_2 n\rfloor$ edges | $O(\log n)$ per operation | $3n$ integers |
| Compression only | $n-1$ edges for an individual find | $O(\log n)$ per operation under the cited path-compression analysis | $2n$ integers |

The compression-only bound is amortized: a single `find` can still be linear. Counters follow `INTERFACE.md` §3.3. `finds` includes calls made inside `unite`, `size`, and `connected`; `hops` counts edges on the root-search pass, and the compression rewiring pass is not counted; `unions` counts only successful merges. This keeps hop counts comparable across variants.

### 5.3 Invariants maintained

- **Forest (U1):** parent pointers are acyclic and each find reaches a root in at most $n-1$ edges.
- **Sizes (U2):** root sizes equal the true component sizes, and the sizes of all roots sum to $n$.
- **Connectivity (U3):** two elements have the same root exactly when they belong to the same component of the reference partition.
- **Return value and counters (U4):** `unite` returns true and increments `unions` exactly when it merges distinct components.
- **Rank (U5, `RankUF`):** a root of rank $r$ contains at least $2^r$ elements; in this no-compression implementation, rank equals tree height, giving height at most $\lfloor\log_2 n\rfloor$.
- **Compression (`CompressionUF`):** after `find(x)`, every vertex on the traversed path points to the root; a repeated find on a formerly non-root path node takes one hop.

### 5.4 Design decisions

1. **Flat arrays rather than pointer nodes:** contiguous integer arrays match the grid's element IDs and avoid per-node pointer allocation.
2. **Iterative rather than recursive `find`:** a naive chain can be very deep; iterative traversal avoids call-stack overflow. The compression test constructs a chain with one million elements.
3. **Full compression rather than path halving or splitting:** the compression-only variant is intended to isolate full path compression. Its two-pass implementation makes the counted root-search hops consistent with the interface's definition.
4. **Fixed linking directions and tie rule:** deterministic rules make behavior reproducible and support the adversarial sequences in Section 8. Naive linking uses `parent[ra] = rb`; rank ties use `parent[rb] = ra` and increment `rank[ra]`.
5. **Rank metadata only where used:** only `RankUF` stores a rank array, keeping the two ablations without rank to two arrays each.
6. **Debug accessors outside the measured operations:** `parentOf`, `rankOf`, and `sizeAtRoot` are const and do not call `find`, alter the forest, or increment counters.

### 5.5 Unit-test coverage

The Structure B owner supplied three assert-based test programs covering initial state, linking rules, union return values and counters, free debug accessors, adversarial tree shapes, and seeded differential checks against `RefPartition`:

- **Naive:** builds a chain at $n=1000$, checks a worst-case find of 999 hops, and checks $n(n-1)/2 = 499{,}500$ hops over `find(0)` through `find(n-1)`; random differential seed 12345.
- **Rank:** checks the tie and unequal-rank rules, then builds a height-10 tree for $n=1024$ and verifies `find(n-1)` takes 10 hops; random differential seed 2026.
- **Compression:** checks that a path is fully rewired and a repeated find takes one hop, and exercises iterative find on a chain of one million elements; random differential seed 777.

In this repository copy, those test sources are located at `src/structB/test_naive_uf.cpp`, `src/structB/test_rank_uf.cpp`, and `src/structB/test_compression_uf.cpp`. The root `make test` target discovers `tests/test_*.cpp`, not these three files, so it does not run these direct tests automatically. The recorded real-structure differential and sweep comparisons in Section 7 do exercise all four variants against the Oracle.

### 5.6 Bug log

During review, the Structure B headers used `#include "../unionfind.hpp"`, which did not resolve the interface header under `include/`. The include was corrected to `#include "unionfind.hpp"`; the supplied test sources also use the `structB/...` header paths and shared `ref_partition.hpp` reference helper.

## 6. Requirement beyond the lecture: Newman–Ziff and finite-size scaling

A conventional simulation may restart for many occupation probabilities. The Newman–Ziff approach instead chooses one random site order per sweep and opens sites incrementally. Each successful opening updates connectivity with the appropriate union-find structure, so the same sweep supplies observables at every occupation count. This implementation records the first percolating count (n_c), giving the finite-grid sample (p_c(L)=n_c/L^2), and also aggregates spanning probability by occupation level. The project uses repeated independent seeded sweeps; it does not claim the high-precision sample volume of Newman and Ziff's paper.

To estimate the infinite-grid threshold, the application fits

\[
\bar p(L)=p_c+aL^{-1/\nu},\qquad \nu=4/3,
\]

with the two-dimensional exponent fixed and $L^{-1/\nu}=L^{-3/4}$. The application uses weighted least squares with weights $1/SE(L)^2$, where $SE(L)=s(L)/\sqrt T$. It does not fit $\nu$ or add correction-to-scaling terms. The separate OLS figure is a visualization/cross-check, not the application extrapolation; Section 11 reports this distinction.

## 7. Correctness

### 7.1 The oracle and why it is correct

The specification says a site is full exactly when it is open and reachable from an open top-row site along open up/down/left/right neighbors. The grid percolates exactly when at least one bottom-row site is full. `FloodFillOracle` implements this definition directly: it stores open flags and, after an opening makes its cached result dirty, lazily recomputes fullness with breadth-first search from all open top-row sites. The queue is a preallocated array of $L^2$ integers managed by head and tail indices. Each site is enqueued at most once during a refresh, so the queue cannot overflow; a refresh costs $O(L^2)$, while repeated `isFull`/`percolates` queries with no intervening opening reuse the cached reachability. `largestCluster()` and `componentLabels()` separately traverse open components with the same array-queue approach; a component label is its smallest site index.

The proof is the standard BFS reachability argument. **Termination:** a site is marked before enqueueing, so it enters the queue at most once and the search performs at most $L^2$ dequeues. **Soundness:** each seed is an open top-row site, and every newly marked site is an open neighbor of an already marked site; induction on queue order gives an open path to the top row. **Completeness:** for any site reachable by an open path, induction on its shortest-path length shows its predecessor is eventually dequeued and discovers it. Thus the marked sites are exactly the full sites, and checking the bottom row gives the percolation answer.

The oracle uses no union-find code, so a union-find link, rank, size or backwash defect is not duplicated in the reference algorithm. The implementations share the written percolation specification, which remains a possible common source of misunderstanding.

The Oracle owner reports hand-worked tests for: a $1\times1$ grid; a spanning middle column; diagonal sites that are not connected; horizontal and vertical no-wraparound cases; the $4\times4$ backwash counterexample; a snake path; an isolated site; duplicate opening and invalid coordinates; two nonadjacent top-row sites whose largest cluster remains one; a fully open $300\times300$ grid to exercise queue capacity; and a seeded monotonicity run. These examples are implemented in `tests/test_oracle.cpp`.

### 7.2 Differential tests and recorded runs

Each differential episode draws $L$ from 1 through `maxL`, then generates $6L^2+10$ operations. The operation mix is 40% `open`, 10% `isOpen`, 20% `isFull`, 10% `percolates`, 10% `largestCluster`, and 10% invalid-coordinate calls split across `open`/`isOpen`/`isFull`. Results and exceptions are compared against the oracle. With the default `--check-every 1`, the full state and invariants are checked after each operation: every site's open/full status, `numOpen`, percolation, largest cluster, all applicable union-find partitions and counters. The test uses its own SplitMix64-derived generator; each episode seed is derived from the base seed and episode index, so a failing episode can be replayed independently. When a failure occurs, the shrinker preserves the failure category while reducing the operation list and grid.

The saved real-structure run used seed 171226 and tested all four variants:

| Run | Setting | Recorded outcome |
|---|---|---|
| Differential, clusters tracked | 1,000,000 requested operations per variant; max $L=10$; check every operation | 1,000,166 actual operations and 4,202 episodes per variant; all variants `OK`; `DIFFERENTIAL TEST PASSED`. |
| Differential, clusters untracked | Same request and max $L$; `--no-clusters`; check every operation | 1,000,166 actual operations and 4,202 episodes per variant; all variants `OK`; `DIFFERENTIAL TEST PASSED`. |
| Deeper differential | 200,000 requested operations per variant; max $L=40$; check every 10 operations | 203,346 actual operations and 66 episodes per variant; all variants `OK`; `DIFFERENTIAL TEST PASSED`. |
| Sweep comparison | 2,000 sweeps × 4 variants; max $L=12$ | `SWEEP TEST PASSED`. |

The operation totals exceed the requests slightly because each run finishes its current episode before starting another. The reproducible command is `make -f oracle/oracle.mk oracle-real ORACLE_SEED=171226`; the saved output is `results/raw/oracle_real_seed171226.log`. The run establishes agreement for these configurations and seeds; it is not an exhaustive proof over all grids or operation sequences. The sweep test compares records with oracle replay, checks S1–S3 across the four implementations, and exercises full and prefix orders, tracked/untracked clusters, early stopping, empty callbacks and invalid orders.

### 7.3 Invariant checkers

The named invariants and checker functions are listed in Section 3. The union-find checkers use only the public debug accessors (`parentOf`, `rankOf`, `sizeAtRoot`) to inspect arrays; they do not call `find`, so checking does not compress paths or alter counters. Parent walks are bounded and report malformed structures instead of looping. U3 constructs expected classes from the oracle's component labels, including the relevant virtual-node classes. L1 checks linking direction because a reversed-link implementation can still have a correct partition and pass the other structural invariants.

### 7.4 Mutation testing and shrinking

`tests/ref_uf.hpp` and `tests/ref_percolation.hpp` provide stand-ins with injectable faults so the harness can be checked against known failures. The test harness defines 14 mutations, including backwash, omitted neighbors, corrupt size/rank updates, incorrect union counters and a false successful-union result. The Oracle owner reports that all 14 were detected and reduced to 1-minimal cases. A further reversed-link fault is rejected by L1, although the partition/rank checks accept it. These are deliberate test mutations, not defects in the delivered structures.

| Injected fault | Checker | Original → minimal operations (reported) |
|---|---|---:|
| BOTTOM included in `ufFull` (backwash) | P5 | 1 → 1 |
| Largest cluster read from `ufFull` | P6 | 6 → 1 |
| Grid wraps around | P3 | 20 → 2 |
| Left neighbor omitted | P6 | 6 → 2 |
| Bottom row not joined to BOTTOM | P4 | 6 → 1 |
| A site closes again | P1 / observable comparison | 10 → 5 |
| Largest cluster shrinks | P6 | 7 → 3 |
| `ufClust` joins diagonal sites | P2 | 39 → 2 |
| Component size not updated (Naive) | U2 | 6 → 1 |
| Component size not updated (Rank) | U2 | 6 → 1 |
| Rank not incremented (Rank) | U5 | 6 → 1 |
| Rank not incremented (Rank + compression) | U5 | 6 → 1 |
| Every third union omitted from counter | U4 | 10 → 3 |
| `unite` returns true for an existing set | U4 | 12 → 2 |

The supplied report says a deliberately faulty left-neighbor implementation on the real code shrank to `open(3,0), open(3,1)`. Preserve its run output if this example is retained as evidence; the current copy has no separate shrink log.

### 7.5 Bug log

Real implementation/test defects only; injected faults above are excluded. Dates and commit hashes are not available in this repository copy and must be filled from the actual Git history.

| Date / commit | Module | Symptom | How found | Fix recorded in current source |
|---|---|---|---|---|
| [fill from history] | Architect factory | Factory initially created `PlaceholderUF` for every variant. | Source/integration review | `src/factory.cpp` now selects `NaiveUF`, `RankUF`, `CompressionUF` or `RankCompressionUF`. |
| [fill from history] | `tests/percolation_checkers.hpp` (Oracle) | P6 checker accepted `const P&`, but real `Percolation::percolates()` is non-const. | Compiling checkers against Structure A | Checker signatures that call stateful/non-const queries take `P&`. |
| [fill from history] | `tests/test_checkers.cpp` (Oracle) | A hand-built “valid” rank state had size below $2^{rank}$ and was rejected by U5. | Checker unit tests | The valid fixture now satisfies the rank-size bound; malformed fixtures still test rejection. |

The saved real-code differential and sweep run reports no discrepancy in any of the four variants for the listed configurations. The root `Makefile` does not include `oracle/oracle.mk` in its `test` target in this copy, so run `make -f oracle/oracle.mk oracle-test oracle-real` separately when both the stand-in and real-structure Oracle suites are required.

## 8. Adversarial inputs (Aman)

Aman's generators write deterministic `.unions` or `.order` replay files and are tied to the linking rules in `INTERFACE.md`. `app/replay` replays union sequences on all four variants and reports counters; the relevant cost measure is parent edges walked in the first pass of `find`.

### 8.1 Naive linking: quadratic total hops

Run `gen_naive_quadratic --n N --out FILE [--finds K | --union-only]`. The generator emits `unite(i,i+1)` for $i=0,\ldots,n-2$, then $K$ `find(0)` operations; by default, $K=n$. Under Naive's `parent[root_a]=root_b` rule, before each union both arguments are roots, so the build costs zero hops and creates the chain $0\to1\to\cdots\to n-1$. Naive does not compress, so each `find(0)` costs $n-1$ hops. For $K=n$, total query cost is $n(n-1)=\Theta(n^2)$, from $2n-1$ operations. A single find is at most $n-1$ hops by U1.

### 8.2 Rank-only: logarithmic height

Run `gen_rank_height --n N --out FILE [--finds K | --union-only]`, with $N=2^k$. In round $s$, the generator unites $i$ with $i+2^s$ for each multiple $i$ of $2^{s+1}$. Each union joins equal-rank roots, and the fixed tie rule builds a binomial tree of height $k=\log_2N$, with $N-1$ as a deepest node. Each subsequent `find(N-1)` costs exactly $k$ hops in Rank, so $K$ finds cost $K\log_2N$. The construction depends on the specified tie direction; reversing it changes which element is deepest.

### 8.3 Backwash

Run `gen_backwash --L N --mode percolate_first|stubs_first --out FILE`. Column 0 is opened top to bottom; isolated vertical stubs occupy rows 1 through $L-1$ in columns 2, 4, and so on. `percolate_first` opens the spanning column before the stubs; `stubs_first` opens it last. Only column 0 should be full. For $L=5$, the correct full-site count is 5. A faulty design that connects both TOP and BOTTOM in the fullness structure would report the stubs as full: $L+(L-1)\lfloor(L-1)/2\rfloor=13$ for $L=5$. The implementation prevents this by keeping BOTTOM out of `ufFull` (P5). Aman reports `full_sites=5` for all variants and both modes; attach the generator replay output to substantiate this measurement in the final repository.

### 8.4 Reported measured hop counts

Aman supplied the following replay counts for the real classes:

| Input | Naive | Compression only | Rank only | Rank + compression |
|---|---:|---:|---:|---:|
| Naive chain, (n=1000), 1000 finds | 999,000 | 1,998 | 998 | 998 |
| Rank-height, (n=1024), 1024 finds | 2,026 | 1,532 | 10,240 | 1,033 |

The predictions for the targeted variants match the reported counts: $1000\cdot999=999{,}000$ for Naive; compression pays 999 hops on the first find and one edge on each later find, totaling 1,998; Rank + compression pays 998 build hops on the chain and then zero for root finds. For the rank-height input, Rank pays $1024\cdot10=10{,}240$, while Rank + compression pays 10 to reach the deepest node once and one edge per later find, totaling 1,033. Under Naive, the final element is the root, so its 1024 repeated finds cost zero and the reported 2,026 hops come from building the tree.

Aman identifies `adversarial_hop_counts.txt` as the raw output, but that file was not present in the inspected checkout. Treat the table as owner-supplied results until the raw output is added and checked. The listed cluster-histogram files, adversary README and D5 PDF should likewise be reconciled with the final branch because they were not present in this checkout.

## 9. Predicted cost model
 
> Written before any measurement; `results/raw/` is empty at this commit. Commit: `<hash>`, date: `<date>`. Author: Dhanvanth (Analyst, D6).
> After the first measurement this section is frozen; later changes go in a dated "Revision" note.
 
### 1 Setup
 
L is the grid side, N = L² the number of sites, T the number of sweeps per size. One sweep opens all N sites in a random order (Fisher-Yates, recorded seed) and records observables at every occupation level; it does not stop at percolation. Variants: **A** (rank + path compression, second structure against backwash), **B-naive** (arbitrary linking, no compression), **B-rank** (rank only), **B-comp** (compression only). Each prediction is tagged **[proved]**, **[derived]** (counting) or **[hypothesis]**. A failed hypothesis is a finding to be explained in §11, not a defect.
 
### 2 Confidence interval per grid size
 
Sweep i gives the estimate p̂ᵢ(L) = n*ᵢ / N, where n*ᵢ is the number of open sites when top first connects to bottom. For T independent sweeps (seeds `seed(L,i)`), with sample mean p̄(L) and standard deviation s(L):
 
> **p̄(L) ± 1.96 · s(L) / √T**
 
This is the normal approximation, valid for T = 1000 (the t-quantile would replace 1.96 if T < 30). It is the interval for the finite-size mean p̄(L), not for p_c.
 
The fluctuation scales like the bias, s(L) ≈ b · L^(−3/4) **[hypothesis]**. The amplitude b is unknown before measuring; I use the prior b ≈ 0.3 (order of magnitude only) to predict the widths below. T = 1000 at every size.
 
| L | 50 | 100 | 200 | 500 | 1000 |
|---|---|---|---|---|---|
| predicted s(L) | 0.016 | 0.0095 | 0.0056 | 0.0028 | 0.0017 |
| predicted CI half-width | 9.9 × 10⁻⁴ | 5.9 × 10⁻⁴ | 3.5 × 10⁻⁴ | 1.7 × 10⁻⁴ | 1.1 × 10⁻⁴ |
 
### 3 Finite-size scaling
 
> p̄(L) = p_c + a · L^(−1/ν), with ν = 4/3 fixed (the exact 2D percolation exponent), so the model is linear in x = L^(−3/4).
 
Fit: weighted least squares of p̄(L) on x, weights w = 1/se² with se = s/√T. With S = Σw, Sₓ = Σwx, Sₓₓ = Σwx², S_y = Σwy, S_xy = Σwxy and Δ = S·Sₓₓ − Sₓ²:
 
> p_c = (Sₓₓ S_y − Sₓ S_xy)/Δ, a = (S S_xy − Sₓ S_y)/Δ, Var(p_c) = Sₓₓ/Δ, CI: p_c ± 1.96 √Var(p_c)
 
Goodness of fit is χ² with (number of sizes − 2) degrees of freedom. A fit with ν free is reported separately as a cross-check.
 
Predictions **[hypothesis]**:
1. **Intercept.** p_c lies within a few 10⁻⁴ of the published 0.592746 (Newman and Ziff, 2000). From the table above the predicted standard error is about 7 × 10⁻⁵, so the **95% CI half-width is about 1.4 × 10⁻⁴** (range 0.7 × 10⁻⁴ to 4 × 10⁻⁴, proportional to the true b). Agreement means |p_c − 0.592746| ≤ 2 half-widths.
2. **Slope.** The crossing probability at p_c is close to ½ for a square L × L lattice, so p̄(L) should sit near p_c and |a| should be small (about 0.1 or less); its sign is not predicted.
3. **Corrections.** Terms of order L^(−1/ν−ω) are omitted. If they matter, p_c shifts when L = 50 is dropped and χ²/dof exceeds 1. This is the most likely cause of any gap and will be tested by that refit.
### 4 Operation counts and cost per variant
 
**Exact counts [derived].** With a full sweep, each of the 2L(L−1) = 2N − 2L lattice edges is examined once, when its second endpoint opens. Per sweep: **2N − 2L** site unions (two finds each, about 4N finds in total); **N − 1** successful merges; **N − 2L + 1** redundant unions; **2L** virtual-node unions (plus L more for the second structure's virtual top). The harness asserts these identities, so a mismatch is a bug, not a performance result.
 
**Cost.** t_sweep = c₀·N + c₁·P + (cache term, §5), where c₀·N is shuffle, flags, neighbour tests and recording, identical for all variants, and P is the total number of parent-pointer hops (Datthatreya's counters). The variants differ only through P.
 
| Variant | Bound | Predicted mean hops per find, random order **[hypothesis]** |
|---|---|---|
| A | O(N α(N)) **[proved]** (Tarjan 1975) | 1.1 to 2.5, flat in L |
| B-comp | O(N log N) amortised **[proved]** (Tarjan, van Leeuwen 1984) | 1.1 to 3, flat in L, within 20% of A |
| B-rank | height ≤ ⌊log₂ N⌋ **[proved]** (D1) | 2 to 5 at L = 1000, maximum height ≤ 19 |
| B-naive | Θ(N²) total on adversarial order **[proved]** (D2, D5) | 5 to 40 at L = 1000, rising with L; weakest prediction |
 
Time per sweep at L = 1000 **[hypothesis]**: t(B-comp) is 0.9 to 1.3 times t(A); t(B-rank) is 1.0 to 1.5 times; t(B-naive) is 1.5 to 10 times. For A, t_sweep ∝ N, so the log-log slope against L is 2.0 (accepted 1.9 to 2.3; any excess is cache effect, §5).
 
**Adversarial inputs [derived].** Naive linking on a chain order (new singleton linked under the growing tree, then a find from the deepest node): P = n(n−1)/2 exactly, which is 4.9995 × 10⁷ hops for n = 10⁴, against about 4 × 10⁴ for A. Rank-only on the binomial-tree order reaches height ⌊log₂ n⌋, so P ≤ n log₂ n. Backwash sequences affect `isFull` correctness, not cost; the prediction is zero wrong answers from the two-structure variants (§7, §8).
 
### 5 Memory and cache
 
A uses about 16 to 24 bytes per site (two parent arrays, rank or size, open flag, permutation), so about 16 to 24 MB at N = 10⁶ **[derived]**. For a cache of capacity C and b bytes per site, time per site is flat while N·b ≤ C and then rises, with the knee at **L_knee = √(C/b)**; for example C = 1 MB and b = 20 gives L ≈ 230. Random opening order touches random cache lines, so I predict time per site rises by a factor of 1.5 to 3 from L = 50 to L = 1000, equally for all variants (it cancels in the ratios of §4). **[hypothesis]**: a cache claim will be made in the results only with `perf stat` miss counts attached.
 
### 6 How the predictions will be judged
 
Counter identities: exact equality on every run. p_c: within 2 CI half-widths of 0.592746. CI widths: within a factor 3 of §2. Time ratios, slopes and hops per find: inside the ranges in §4. Adversarial P for B-naive: exactly n(n−1)/2. Every failed prediction is explained in §11 (for example scaling corrections, link direction in naive, allocator or prefetcher effects); a forced fit is not acceptable.
 
---

## 10. Experimental setup

| Item | Recorded setup |
|---|---|
| OS | Ubuntu 24.04 under WSL2 (from the project README); Microsoft hypervisor reported by `lscpu`. Kernel build was not included in the supplied snapshot. |
| Processor / cores / clock | WSL snapshot (2026-11-21): Intel Core Ultra 7 155U; 7 cores, 14 logical CPUs (2 threads/core). Clock frequency was not reported. |
| Cache sizes / cache line | Snapshot: L1d 48 KiB/core (336 KiB total), L1i 64 KiB/core (448 KiB total), L2 2 MiB/core (14 MiB total), shared L3 12 MiB; 64-byte cache line. |
| RAM | 15 GiB visible to WSL, plus 4 GiB swap, in the supplied snapshot. Windows host-installed RAM was not included. |
| Compiler | Current snapshot: g++ 15.2. Earlier README/run notes state g++ 13.3, so the compiler used for the saved experiment is unconfirmed. Project flags: `-O2 -std=c++17 -Wall -Wextra` (application/benchmark also use `-DNDEBUG`). |
| Application sizes and sweeps | (L=50,100,200,500,1000), (T=100), base seed 171226. The full workflow caps Naive at (L=200); the benchmark and Oracle checks still exercise all variants. |
| Cluster histogram | Enabled in the full application workflow; default `cluster-sweeps` is 10. |
| Benchmark | Five paired repetitions at each of (L=50,100,200,500,1000), base seed 171226, same derived seed across variants within a repetition. |
| Pinning / background load | No explicit CPU pinning is shown in the workflow command; the later shell affinity snapshot allowed CPUs 0–13. The supplied system-wide `vmstat` log (`results/raw/vmstat_during_run.txt`) sampled from 2026-11-21 12:26:28 to 12:35:57 UTC. Excluding its initial since-boot row, average CPU use was 5.12% user and 0.27% system (94.69% idle, 0.01% I/O wait); the run queue peaked at 4, blocked processes remained at 0, and swap-in/out remained at 0. These measurements include the workflow and any other system activity, so they do not isolate background-process load. |
| Full workflow duration | The `vmstat` monitoring window lasted 9m29s (12:26:28–12:35:57 UTC). The supplied log has no `run_all.sh` start/end markers or timed command output, so this is not an exact workflow duration; an earlier informal estimate was about four minutes. |
| Cache profiler | Attempted on 2026-11-21 with `perf stat -d` around the benchmark, but the run produced no usable counters: perf reported unsupported `cpu_atom` PMU events and that `L1-dcache-load-misses:u` is unsupported. `results/raw/cache_profile.txt` contains only the start header. No cache-miss measurements are claimed; hardware performance-counter access is unavailable in this WSL environment/configuration. |

The hardware inventory, `vmstat` capture and cache-profiler attempt were collected on 2026-11-21. The first `vmstat` report is the since-boot average; interval CPU figures above use subsequent reports. The monitor window appears to cover the workflow, but without run-boundary timestamps it cannot establish the workflow's exact duration, and its system-wide CPU figures cannot identify unrelated background processes individually. Treat the hardware details as representative of the experiment only if the WSL configuration had not changed. The compiler version used for the earlier run remains unconfirmed. The pre-measurement model in Section 9 assumed (T=1000) for threshold intervals, whereas the recorded application data use (T=100). The comparisons below account for this difference rather than treating the interval-width gap as a direct contradiction.

## 11. Results

### 11.1 Percolation threshold and finite-size extrapolation

The saved rank + compression run is `results/raw/fss_rank_compress_T100/summary.csv` and `extrapolation.csv`. It contains five sizes and 100 sweeps per size. The finite-size means and 95% confidence intervals are:

| (L) | Mean (p_c(L)) | 95% CI |
|---:|---:|---:|
| 50 | 0.593264 | [0.588071, 0.598457] |
| 100 | 0.592870 | [0.589543, 0.596197] |
| 200 | 0.593840 | [0.592045, 0.595635] |
| 500 | 0.593098 | [0.592116, 0.594080] |
| 1000 | 0.592756 | [0.592186, 0.593325] |

All five per-size 95% intervals include the published reference ($p_c=0.592746$). The application weighted fit gives $p_c(\infty)=0.592684$, standard error 0.000395 and approximate 95% interval [0.591910, 0.593457]. Its difference from the reference is 0.000062, about 0.16 standard errors. The fitted WLS slope is $a=0.027737\pm0.037865$ (one standard error), consistent with zero.

The finite-size OLS figure is a separate fit and must not be conflated with the application WLS estimate. For the rank + compression data, the plotted OLS intercept is approximately 0.593076 ± 0.000368 (one standard error); the table above reports the application WLS fit. The figure's site-level error bars show one standard error, while the table reports 95% intervals.

![Finite-size scaling OLS fit. The line is the Rank + compression OLS fit; points and bars show the rank + compression means and one standard error. Generated by `scripts/plot_results.py` from `results/raw/fss_rank_compress_T100/summary.csv`.](results/figures/finite_size_scaling_ols.png)

### 11.2 Measured operation counts and timing

At (L=1000), the five-repetition benchmark recorded:

| Variant | Mean hops per find | Mean sweep time ± sample SD |
|---|---:|---:|
| Naive | 27.275 | 1653.6 ± 193.5 ms |
| Rank | 0.946 | 384.6 ± 19.3 ms |
| Compression only | 0.880 | 462.0 ± 24.5 ms |
| Rank + compression | 0.712 | 467.1 ± 16.4 ms |

Hops per find is the total measured parent-pointer hops divided by the total find calls across the five repetitions. Rank + compression had the fewest hops per find, but rank-only had the lowest mean runtime in this sample. Relative to rank + compression, compression-only took 0.99× as long, rank-only 0.82×, and Naive 3.54×. This shows that reducing hop counts does not guarantee a lower runtime: rank + compression has additional bookkeeping and compression writes, and the observed timing is specific to this machine and implementation.

![Four-variant mean sweep time by grid side. Points are means of five paired runs; error bars show sample standard deviation. Generated by `scripts/plot_results.py` from `results/raw/percolation_sweep_summary.csv`.](results/figures/percolation_sweep_time.png)

![Four-variant mean parent-pointer hops per sweep by grid side. Both axes are logarithmic. Points are means of five paired runs; error bars show sample standard deviation. Generated by `scripts/plot_results.py` from `results/raw/percolation_sweep_summary.csv`.](results/figures/percolation_sweep_hops.png)

### 11.3 Comparison with the pre-measurement model

The threshold model predicted a standard deviation proportional to (L^{-3/4}) and planned (T=1000). The measured run used (T=100), and the per-sweep sample standard deviations were about 1.6–1.8 times the model's predicted values. Together, ten times fewer sweeps and the larger observed spread make the 95% interval half-widths about 5–6 times the pre-run predictions. The threshold estimate itself is close to 0.592746, but the predicted precision was too optimistic for the collected data.

The measured (L=1000) hops per find were 27.275 for Naive, 0.946 for Rank, 0.880 for Compression, and 0.712 for Rank + compression. The Naive value lies in the model's predicted range 5–40. The three balanced/compressed variants came in below their hypothesized lower ranges (Rank 2–5, Compression 1.1–3, Rank + compression 1.1–2.5). Those ranges were hypotheses rather than guarantees; they overestimated the observed random-order path lengths.

The frozen model's memory estimate also does not match the current Structure A layout: a fully tracked run allocates three DSUs, each with parent, rank and size integer arrays, plus the open-site flags. That is about 37 bytes per grid site before vector/allocator overhead and sweep scratch arrays, rather than the model's 16–24 bytes per site. This is a code-based storage estimate, not a measured peak-memory result; keep the pre-measurement model intact and explain the discrepancy as an implementation/model mismatch.

The predicted log-log time slope for Structure A was 2.0, with an accepted range 1.9–2.3. A least-squares fit to the five measured rank + compression mean times gives a slope of about 2.46, above that range. This is a measured mismatch. Cache effects were one pre-run hypothesis, but no hardware cache counters were collected, so the data do not identify cache misses as its cause. At (L=1000), measured time ratios relative to A were 0.99 for Compression, 0.82 for Rank, and 3.54 for Naive. Compression and Naive fit the predicted ratio ranges; Rank was faster than the predicted 1.0–1.5 range. These differences may reflect implementation overhead and machine effects, but distinguishing those causes requires additional profiling.

## 12. Limitations and next steps

- The Oracle has been run on small random grids ($L\le40$), not exhaustively over all opening orders. Enumerating all orders through $3\times3$ (9! orders) and all site subsets on $4\times4$ would strengthen small-grid coverage.
- Flood-fill reachability is recomputed in $O(L^2)$ after an opening, so the oracle is intended for correctness checks on small grids rather than large-scale performance measurements.
- Random testing and shrinking are not exhaustive. A shrinker result is 1-minimal (no single operation can be removed while preserving the same failure), but it is not guaranteed to be the globally shortest failure.
- Checks configured with `--check-every k` for $k>1$ can miss a transient violation between checks; the principal small-grid run uses `--check-every 1`.
- `make test` in this copy runs the root test suite but does not invoke `oracle/oracle.mk`; the standalone real Oracle command must be run separately unless the build integration is changed.
- Mani's D4 derivation PDF, roll number, GitHub username and PR links remain to be supplied. The exact output for the reported mutation/shrinker cases is not saved as a separate log in this copy.
- The application caps Naive at (L=200) by default because full Naive sweeps at larger sizes are expensive. The benchmark and correctness suite still include all four variants through their specified sizes.
- The pre-measurement model planned (T=1000); the recorded threshold run used (T=100). More sweeps would reduce uncertainty and make precision comparisons stronger.
- Base seed 171226 was selected after exploratory runs. The reported results are reproducible for that seed but should not be presented as an unbiased seed-selection procedure.
- The hardware inventory and system-wide load capture do not isolate the benchmark from unrelated process activity; the exact `run_all.sh` duration, CPU clock during execution and earlier run's compiler version remain unknown. The perf attempt found no supported hardware counters in this WSL environment, so cache misses could not be measured. The current snapshot reports g++ 15.2 while the README reports 13.3. Future measurements should capture `/usr/bin/time`, `lscpu`, compiler version and load alongside the run; cache profiling requires an environment with supported performance counters.
- The pre-measurement memory estimate counts fewer arrays than the current three-DSU Structure A implementation. Measure peak memory and add a dated revision to the model before making memory or cache-capacity claims.
- Only the Rank + compression summary is saved as a dedicated finite-size data folder in the inspected checkout; retain the full all-variant application CSVs when regenerating the OLS figure with all four variants.
- The current plotting script omits the cluster-size-distribution figure by team decision, although cluster histogram CSV output is available. Confirm that this still satisfies the instructor's Project 6 requirements.
- The exact hidden-evaluator command/input protocol was not included in the supplied project brief. Update the README and this report if the instructor publishes one.
- Complete missing team IDs and PR links, add the absent D5 artifact/raw-output evidence, and have the Structure A owner verify the D1 argument before final submission.

## 13. References

1. B. A. Galler and M. J. Fischer, “An improved equivalence algorithm,” *Communications of the ACM*, 7(5), 301–303, 1964.
2. R. E. Tarjan, “Efficiency of a good but not linear set union algorithm,” *Journal of the ACM*, 22(2), 215–225, 1975.
3. R. E. Tarjan and J. van Leeuwen, “Worst-case analysis of set union algorithms,” *Journal of the ACM*, 31(2), 245–281, 1984.
4. M. E. J. Newman and R. M. Ziff, “Efficient Monte Carlo algorithm and high-precision results for percolation,” *Physical Review Letters*, 85(19), 4104–4107, 2000, arXiv:cond-mat/0005264.
5. R. Sedgewick and K. Wayne, *Algorithms*, 4th ed., Addison-Wesley, 2011, §1.5.
6. D. Stauffer and A. Aharony, *Introduction to Percolation Theory*, 2nd ed., Taylor & Francis, 1994.
7. T. H. Cormen, C. E. Leiserson, R. L. Rivest and C. Stein, *Introduction to Algorithms*, 3rd ed., MIT Press, 2009, §22.2 (breadth-first search).
8. W. M. McKeeman, “Differential testing for software,” *Digital Technical Journal*, 10(1), 100–107, 1998.
9. A. Zeller and R. Hildebrandt, “Simplifying and isolating failure-inducing input,” *IEEE Transactions on Software Engineering*, 28(2), 183–200, 2002.
10. R. A. DeMillo, R. J. Lipton and F. G. Sayward, “Hints on test data selection: help for the practicing programmer,” *Computer*, 11(4), 34–41, 1978.
11. Tools used include GNU g++, GNU Make, Python, Matplotlib, and the AI assistants declared in `TOOLS.md`; record exact versions in the final experimental log when available.
