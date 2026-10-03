
Independent runs on a cloud VM (single 2.1 GHz Xeon vCPU, `g++ -O2`, 3 runs) gave
3.3-3.8M orders/sec, p50 162-183 ns and p99 650-763 ns.

## Tests

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

22 GoogleTest tests cover crossing and non-crossing orders, trade price at the resting
order's price, partial fills on both sides, price priority, FIFO time priority within a level,
cancellation, pruning of empty price levels, and randomized invariants. CI runs them on every push.
