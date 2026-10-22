#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT
echo "Date (UTC): $(date -u +%Y-%m-%dT%H:%M:%SZ)"
uname -a
g++ --version | head -1
echo "Workload: Existing benchmark.cpp includes randomized order additions and matching; do not compare its numbers with the historical non-crossing insertion benchmark."
for i in 1 2 3 4 5; do
  echo "========== RUN $i / 5 =========="
  g++ -std=c++17 -O2 -Wall -Wextra benchmark.cpp order_book.cpp -o "$tmp/lob_benchmark"
  "$tmp/lob_benchmark"
done
