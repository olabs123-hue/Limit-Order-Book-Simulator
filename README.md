# Limit Order Book Simulator

A price-time priority limit order book matching engine in C++.

## Design
- `buy_levels_`: `std::map<double, std::list<Order>, std::greater<double>>` — sorted descending (best bid first). O(log N) insertion of a new price level.
- `sell_levels_`: `std::map<double, std::list<Order>>` — sorted ascending (best ask first).
- `order_index_`: `std::unordered_map<order_id, iterator>` — enables O(1) average-case order cancellation by holding a direct iterator into the resting order's position in its price level's list.
- Empty price levels are erased after fills and cancels.

## Build & run
```
g++ -std=c++17 -Wall -Wextra -O2 -o lob_demo main.cpp order_book.cpp
./lob_demo

g++ -std=c++17 -Wall -Wextra -O2 -o benchmark benchmark.cpp order_book.cpp
./benchmark
```

## Tests
```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

22 GoogleTest tests cover crossing and non-crossing orders, trade price at the resting
order's price, partial fills on both sides, price priority, FIFO time priority within a level,
cancellation, pruning of empty price levels, and randomized invariants. CI runs them on every push.

## Benchmark results
1,000,000 random orders, single thread, `g++ -O2`. Timings include per-call `steady_clock`
overhead. Only order adds (with matching) are timed; cancellations are not benchmarked.

| Run | Throughput | p50 | p99 | p99.9 |
|---|---|---|---|---|
| Author's machine | ~3.97M orders/sec | 166 ns | 605 ns | 2053 ns |
| Cloud VM, 2.1 GHz Xeon vCPU (3 runs) | 3.3-3.8M orders/sec | 162-183 ns | 650-763 ns | 2.2-2.9 µs |

Numbers vary with hardware, so rerun before quoting.
