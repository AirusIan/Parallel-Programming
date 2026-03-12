#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HW1="$ROOT/HW1"

N=${1:-4096}  # default 2^12
PROCS=(1 2 4 8)
TRIALS=${TRIALS:-3}

BIN_BASIC="$HW1/basic"
BIN_ADV="$HW1/advanced"

INPUT_DIR="$HW1/perf_inputs"
OUTPUT_DIR="$HW1/perf_outputs"
RESULTS="$HW1/perf_results.csv"

mkdir -p "$INPUT_DIR" "$OUTPUT_DIR"

# Compile
mpicxx -O2 -std=c++17 -o "$BIN_BASIC" "$HW1/basic.cpp"
mpicxx -O2 -std=c++17 -o "$BIN_ADV" "$HW1/advanced.cpp"

# Generate deterministic input
INPUT_FILE="$INPUT_DIR/input_${N}.bin"
python3 - <<PY
import random, struct
random.seed(42)
N = ${N}
with open("$INPUT_FILE", "wb") as f:
    for _ in range(N):
        f.write(struct.pack('f', random.random()))
PY

# CSV header
if [ ! -f "$RESULTS" ]; then
  echo "impl,n,p,trial,seconds" > "$RESULTS"
fi

run_one() {
  local impl="$1"
  local bin="$2"
  local p="$3"
  local trial="$4"
  local out_file="$OUTPUT_DIR/out_${impl}_n${N}_p${p}_t${trial}.bin"
  local time_file="$OUTPUT_DIR/time_${impl}_n${N}_p${p}_t${trial}.txt"

  /usr/bin/time -f "%e" -o "$time_file" \
    env TMPDIR=/tmp mpirun -np "$p" "$bin" "$N" "$INPUT_FILE" "$out_file"

  local secs
  secs=$(cat "$time_file")
  echo "$impl,$N,$p,$trial,$secs" >> "$RESULTS"
}

for p in "${PROCS[@]}"; do
  for trial in $(seq 1 "$TRIALS"); do
    run_one "basic" "$BIN_BASIC" "$p" "$trial"
    run_one "advanced" "$BIN_ADV" "$p" "$trial"
  done
done

echo "Done. Results: $RESULTS"
