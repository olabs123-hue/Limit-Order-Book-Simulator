# Limit Order Book Simulator

A price-time priority limit order book matching engine in C++.

## Design
- `buy_levels_`: `std::map<double, std::list<Order>, std::greater<double>>` — sorted descending (best bid first). O(log N) insertion of a new price level.
- `sell_levels_`: `std::map<double, std::list<Order>>` — sorted ascending (best ask first).
- `order_index_`: `std::unordered_map<order_id, iterator>` — enables O(1) average-case order cancellation by holding a direct iterator into the resting order's position in its price level's list.

## Build & run
```
g++ -std=c++17 -Wall -Wextra -O2 -o lob_demo main.cpp order_book.cpp
./lob_demo

g++ -std=c++17 -Wall -Wextra -O2 -o benchmark benchmark.cpp order_book.cpp
./benchmark
```

## Benchmark results (1,000,000 random orders, single core)
- Throughput: ~3.97M orders/sec
- Latency p50: 166 ns
- Latency p99: 605 ns
- Latency p99.9: 2053 ns

(Your numbers may vary depending on hardware — rerun before quoting.)
