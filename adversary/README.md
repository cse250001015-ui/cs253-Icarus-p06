# adversary/

Generators for the replay files of INTERFACE.md section 7. Each one is a small
standalone program (`g++ -O2 -std=c++17 -Wall -Wextra gen_X.cpp -o gen_X`).
Lines starting with `#` in the output are comments, ignored by the replay tool.

All three are tied to fixed rules in INTERFACE.md v0.3 (3.1 linking rules, 4.1/4.2
backwash). If those sections change, the generators must be revisited.

| Generator | Output | What it forces |
|---|---|---|
| `gen_naive_quadratic` | `.unions` | Naive linking builds the chain 0->1->...->n-1; each `f 0` then walks n-1 hops, so the total is about `finds * (n-1)` (Theta(n^2) for the default `finds = n`) |
| `gen_rank_height` | `.unions` | Rank linking builds a binomial tree; node n-1 sits at depth log2(n), the maximum U5 allows, and every `f n-1` costs log2(n) hops |
| `gen_backwash` | `.order` | Column 0 percolates, stubs in columns 2, 4, ... must stay not-full; a single-structure implementation reports the stubs as full |

## Options

```
gen_naive_quadratic --n N --out FILE [--finds K | --union-only]
gen_rank_height     --n N --out FILE [--finds K | --union-only]    (N a power of two)
gen_backwash        --L L --mode percolate_first|stubs_first --out FILE    (L >= 3)
```

- `--finds K` number of `f` lines appended after the unions: `f 0` for
  `gen_naive_quadratic`, `f n-1` for `gen_rank_height`. Default: `K = n`.
  The find lines are where the cost shows. With Compression the first `f` pays
  full price and later ones cost about 1 hop, so K controls how strongly the
  Naive/Rank vs Compression gap shows up.
- `--union-only` writes only the `u` lines (same as `--finds 0`). Use it as a
  baseline: every `unite` in these sequences joins two roots, so building the
  structure costs 0 hops, and all the find cost comes from the `f` lines.
  Cannot be combined with `--finds K` for K > 0.

## Replaying

`percolation_app --replay FILE --variant V` accepts both kinds of file; the kind
is detected from the content (see `app/replay.hpp`). For `.order` files it also
prints `full_sites`: for `gen_backwash` output the correct value is `L`
(column 0 only); a backwash bug reports more.
