# Project 06: Percolation with union find
Team ICARUS — CS 253 / MA 253, Autumn 2026 (semester-3)

## Team and roles 
| Name | Roll no. | GitHub | Role | Module folder | Derivation | 
|------|----------|--------|------|---------------|------------|      
| CHEDURUPALLY HEMANTH RAJ | 250001021 | cse250001021-create | Structure A owner | src/structA/  | D1
| LAKAVATH DATHATREYA | 250001043 | cse250001043-art | Structure B owner | src/structB/ | D2
| BHUKYA PIYUSH VARDHAN RATHOD | 250001015 | cse250001015-ui | Architect | app/  | D3
| MUPPEKAR MANIRATHNA | 250001050 | cse250001050-hub | Oracle owner | oracle/ | D4
| SHAIK AMAN AHMED | 250041036 | Shaik-Aman-Ahmed | Adversary | adversary/  | D5
| KOTNI DHANVANTH | 250001040 | cse250001040dhanvanth | Analyst | bench/  | D6

## Summary

This project estimates the site-percolation threshold of the square lattice using the Newman–Ziff algorithm. Structure A uses union by rank with path compression, virtual boundary nodes, separate structures to prevent backwash, and cluster tracking; Structure B supplies naive, rank-only, and compression-only variants behind the same interface. We compare the four variants using identical seeded opening orders, so their percolation estimates can be compared while their operation counts and runtimes differ. In the recorded run, rank + compression had the fewest parent-pointer hops per find at L=1000, while rank-only had the shortest measured sweep time; the rank + compression weighted extrapolation was 0.592684, close to the published value 0.592746.

## Requirements

- OS: Ubuntu 24.04 under WSL2 (recorded test environment); a Unix shell is required.
- C++: g++ 13.3, C++17, and GNU Make. The Makefile uses `-O2 -Wall -Wextra`.
- Figures: Python 3.9 or later and Matplotlib.
- The structures and application use the C++ standard library only. Python/Matplotlib is used for figures; containers in tests and data-generation tools do not stand in for the structures under study.

## Build

From the repository root, run:

```bash
make
```

The default target builds the application and adversarial generators and runs the benchmark harness. For a build without running the benchmark, use `make app`.

## Quick start

After building, run this small, deterministic example:

```bash
./build/percolation_app --sizes 20,40,80 --sweeps 20 --variant rank_compress --seed 1 --out results/raw/readme_example
```

The output includes one threshold estimate and 95% confidence interval for each size, followed by an extrapolated threshold. With this seed and these settings, the recorded output is:

```text
rank_compress L=20    T=20   p_c estimate = 0.60337 +/- 0.02325  (95% CI)
rank_compress L=40    T=20   p_c estimate = 0.60016 +/- 0.01314  (95% CI)
rank_compress L=80    T=20   p_c estimate = 0.59102 +/- 0.00858  (95% CI)
rank_compress extrapolated p_c = 0.58363 +/- 0.01823  (published value 0.592746)
wrote results/raw/readme_example/{sweeps,summary,extrapolation}.csv and per-level data in results/raw/readme_example/occupation/
```

The CLI also supports `--sizes`, `--sweeps`, `--variant naive|rank|compress|rank_compress|all`, `--seed`, `--out`, `--clusters`, `--cluster-sweeps`, `--naive-max-L`, and `--replay`. Run `./build/percolation_app --help` for the complete option list. The `run_all.sh` workflow defaults to `NAIVE_MAX_L=200` for the slower Naive application sweeps; the benchmark and Oracle checks still include all four variants.

## Tests

Run unit tests and invariant checks with:

```bash
make test
```

Run the differential and sweep tests against the real structures with the recorded seed:

```bash
make -f oracle/oracle.mk oracle-real ORACLE_SEED=171226
```

The real-structure run compares each variant with the flood-fill oracle on 1,000,000 requested operations per variant (tracked and untracked clusters, max L=10), then on 200,000 requested operations per variant (max L=40, checking every 10 operations), and replays 2,000 sweeps across all four variants (max L=12). Episode boundaries cause the actual operation counts to exceed the requested counts slightly. The recorded run processed 1,000,166 operations per variant in each max-L=10 pass and 203,346 per variant in the deeper pass. A passing run ends the differential phases with `DIFFERENTIAL TEST PASSED` and the sweep phase with `SWEEP TEST PASSED`; unit and invariant tests print their individual pass messages and exit successfully.

## Evaluation interface (hidden tests)

The supplied Project 6 description specifies hidden correctness tests, including backwash cases, but does not publish a separate evaluator command or stdin/file input-output protocol. The public C++ interface is defined in `include/unionfind.hpp`, `include/percolation.hpp`, and `include/sweep.hpp`; the runnable application interface is documented in `INTERFACE.md` and printed by `./build/percolation_app --help`. Add the evaluator's exact command and example here if the instructor publishes a separate format.

## Benchmarks and figures

Run the full workflow from the repository root:

```bash
./scripts/run_all.sh
```

This runs unit and invariant tests, the application, the four-variant benchmark, and figure generation. The recorded WSL2 run took about four minutes; runtime depends on the machine. The application uses T=100 threshold sweeps for L=50, 100, 200, 500, and 1000, with base seed 171226. Naive application sweeps are capped at L=200 by default. The benchmark still tests all four variants at all five sizes, with five paired repetitions and the same derived seed for each variant in a repetition.

The application writes per-sweep, summary, extrapolation, and cluster CSVs under `results/raw/`; the benchmark writes `percolation_sweeps.csv` and `percolation_sweep_summary.csv` there. The plotting script generates `finite_size_scaling_ols.png`, `percolation_sweep_time.png`, and `percolation_sweep_hops.png` under `results/figures/`. The OLS plot draws the fitted line for Rank + compression and retains fit estimates for other variants in the legend when the input summary contains them. To regenerate figures without rerunning the experiments, run:

```bash
python3 scripts/plot_results.py --scaling-csv results/raw/summary.csv --benchmark-csv results/raw/percolation_sweep_summary.csv
```

## Repository map

| Path | Contents |
|---|---|
| `adversary/` | Adversarial input generators. |
| `app/` | Percolation application and replay support. |
| `bench/` | Four-variant timing and hop-count benchmark. |
| `build/` | Generated binaries and objects; ignored by Git. |
| `docs/` | Reserved for derivation PDFs and presentation slides. |
| `include/` | Shared union-find, percolation, sweep, statistics, and fit interfaces. |
| `oracle/` | Flood-fill oracle and real/stand-in differential and sweep harnesses. |
| `results/` | Raw CSV data and generated figures. |
| `scripts/` | End-to-end workflow and plotting script. |
| `src/` | Architect utilities and Structure A/B implementations. |
| `tests/` | Unit tests, invariant checkers, and harness tests. |

## Results at a glance

The following finite-size estimates are for Rank + compression, T=100, base seed 171226. The intervals are 95% confidence intervals for the finite-size mean.

| Grid side L | Estimated p_c(L) | 95% CI |
|---:|---:|---:|
| 50 | 0.593264 | [0.588071, 0.598457] |
| 100 | 0.592870 | [0.589543, 0.596197] |
| 200 | 0.593840 | [0.592045, 0.595635] |
| 500 | 0.593098 | [0.592116, 0.594080] |
| 1000 | 0.592756 | [0.592186, 0.593325] |

Weighted finite-size scaling gives p_c(infinity)=0.592684 +/- 0.000395 (1 standard error), with an approximate 95% CI of [0.591910, 0.593457]. This differs from 0.592746 by about 0.000062, or 0.16 standard errors.

At L=1000, the benchmark measured the following mean parent-pointer hops per find and mean sweep time over five repetitions (time spread is sample standard deviation):

| Variant | Hops per find | Mean time per sweep |
|---|---:|---:|
| Naive | 27.275 | 1653.6 +/- 193.5 ms |
| Rank | 0.946 | 384.6 +/- 19.3 ms |
| Compression | 0.880 | 462.0 +/- 24.5 ms |
| Rank + compression | 0.712 | 467.1 +/- 16.4 ms |

Rank + compression had the fewest measured hops per find, but rank-only was fastest by elapsed time in this benchmark. Full analysis, predictions, and explanations of differences from the model belong in `REPORT.md`.

## Known limitations

- Naive full application sweeps are limited to L<=200 by the default workflow cap; the timing benchmark and correctness tests still exercise Naive at larger sizes.
- The OLS figure uses per-size means and one-standard-error bars. The plotted fit line is only for Rank + compression; its fit is OLS, while the application extrapolation reported above is weighted least squares.
- Base seed 171226 was selected after exploratory runs; report these results as a reproducible, seed-specific run rather than as evidence from an unbiased choice among seeds.
- The supplied project description does not provide the evaluator's exact command or input/output protocol; this section must be updated if one is published.
- The CPU model, cache sizes, exact Python/Matplotlib versions, and an independently timed workflow duration were not recorded with the available result files.

## Tools and references

AI and other tool use is declared in [TOOLS.md](TOOLS.md). The shared contracts and invariants are in [INTERFACE.md](INTERFACE.md). References include Newman and Ziff (2000), Tarjan (1975), Tarjan and van Leeuwen (1984), and Sedgewick and Wayne, *Algorithms*, 4th ed.; the full bibliography is in `REPORT.md`.
