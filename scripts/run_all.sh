#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p results
prefix="${1:-local}"
make
{ date -u '+UTC %Y-%m-%d %H:%M:%S'; uname -a; c++ --version; } > "results/${prefix}_environment.txt"
./bank_transfer all 1000000 5 > "results/${prefix}_1m.csv"
./bank_transfer all 5000000 5 > "results/${prefix}_5m.csv"
./bank_transfer none 1000000 3 --amplify > "results/${prefix}_amplify.csv"
python3 scripts/verify_results.py "results/${prefix}_1m.csv" "results/${prefix}_5m.csv" "results/${prefix}_amplify.csv" | tee "results/${prefix}_verification.txt"
