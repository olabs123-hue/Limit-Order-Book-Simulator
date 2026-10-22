# Benchmark of this GitHub repository's current implementation

Run from repository root: `bash benchmarks/run_current_repository.sh`.

The script compiles the **current repository source** in Release/optimized mode and runs its own benchmark five times. Record CPU, compiler, OS, and raw output. Do not equate these numbers with `benchmarks/HISTORICAL_PORTFOLIO_RESULT.md`: that document describes a different, independently packaged implementation.

**Workload:** Existing benchmark.cpp includes randomized order additions and matching; do not compare its numbers with the historical non-crossing insertion benchmark.

This runner does not measure production end-to-end performance. Review the benchmark code and confirm tests pass before quoting results on a CV.
