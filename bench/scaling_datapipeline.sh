#!/usr/bin/env bash


set -euo pipefail

# Finite-size scaling data collection using the project's application.
# Override SIZES, SWEEPS, SEED, CLUSTER_SWEEPS, or OUT for another run.

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

# Project 6 studies grid sides up to 1000. The defaults provide 500 samples
# for the finite-size estimates; smaller values can be supplied for a quick run.
SIZES="${SIZES:-50,100,200,500,1000}"
SWEEPS="${SWEEPS:-100}"
SEED="${SEED:-42}"
CLUSTER_SWEEPS="${CLUSTER_SWEEPS:-500}"

# Keep this experiment's outputs separate from run_all.sh and other experiments.
# The configuration in the directory name helps preserve reproducibility.
SIZE_TAG="${SIZES//,/x}"
OUTPUT_DIR="${OUT:-$PROJECT_ROOT/results/raw/finite_size_scaling_L${SIZE_TAG}_T${SWEEPS}_seed${SEED}}"

if ! command -v make >/dev/null 2>&1; then
    echo "ERROR: make was not found. Use the documented Linux/WSL build environment." >&2
    exit 1
fi

echo "Finite-size scaling data collection"
echo "  sizes: $SIZES"
echo "  sweeps per size: $SWEEPS"
echo "  cluster sweeps: $CLUSTER_SWEEPS"
echo "  seed: $SEED"
echo "  output: $OUTPUT_DIR"

echo "Building application..."
make -C "$PROJECT_ROOT" app

EXECUTABLE="$PROJECT_ROOT/build/percolation_app"
if [[ ! -x "$EXECUTABLE" && -x "$EXECUTABLE.exe" ]]; then
    EXECUTABLE="$EXECUTABLE.exe"
fi
if [[ ! -x "$EXECUTABLE" ]]; then
    echo "ERROR: application executable not found at $PROJECT_ROOT/build/percolation_app" >&2
    exit 1
fi

echo "Running Monte Carlo simulations..."
"$EXECUTABLE" \
    --sizes "$SIZES" \
    --sweeps "$SWEEPS" \
    --variant rank_compress \
    --seed "$SEED" \
    --out "$OUTPUT_DIR" \
    --clusters \
    --cluster-sweeps "$CLUSTER_SWEEPS"

echo "Checking generated datasets..."
for file in sweeps.csv summary.csv extrapolation.csv clusters.csv; do
    if [[ -s "$OUTPUT_DIR/$file" ]]; then
        echo "[OK] $file"
    else
        echo "[ERROR] Missing or empty: $OUTPUT_DIR/$file" >&2
        exit 1
    fi
done

echo "Finite-size scaling data written to: $OUTPUT_DIR"
