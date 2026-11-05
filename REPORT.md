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
 
## 10. Experimental setup (fill in before the first run)
 
Collect with `lscpu`, `lscpu -C`, `uname -a`, `g++ --version`, `free -h`.
 
| Item | Value |
|---|---|
| Processor, cores used, clock | |
| L1d / L2 / L3, cache line | |
| RAM, OS and kernel | |
| Compiler and version, flags | `-O2` |
| Pinning | one pinned core, no other heavy process |
| Grid sizes, T | 50, 100, 200, 500, 1000; T = 1000 |
| Repetitions per timing | at least 5 runs, mean and standard deviation |
| Seeds | `seed(L, i)` and base seed in every CSV header |
| Profiler | `perf stat` events, only if cache claims are made |
 
