#!/usr/bin/env bash
# The one command: build, test, run the application at full scale, run the benchmarks.
# Quick check with a small configuration:   SIZES=20,40 SWEEPS=5 ./scripts/run_all.sh
set -euo pipefail
cd "$(dirname "$0")/.."

SIZES="${SIZES:-50,100,200,500,1000}"   # grid sides, up to 1000 x 1000
SWEEPS="${SWEEPS:-100}"                  # sweeps T per size
SEED="${SEED:-171226}"                   # base seed; every sweep's seed is derived from it
OUT="${OUT:-results/raw}"
NAIVE_MAX_L="${NAIVE_MAX_L:-200}"        # avoid full-grid naive sweeps at the largest sizes

make test
make app build/bench adversary
./build/percolation_app --sizes "$SIZES" --sweeps "$SWEEPS" --variant all --seed "$SEED" \
  --naive-max-L "$NAIVE_MAX_L" --clusters --out "$OUT"
make bench
python3 scripts/plot_results.py \
  --scaling-csv "$OUT/summary.csv" \
  --benchmark-csv results/raw/percolation_sweep_summary.csv \
  --out-dir results/figures
