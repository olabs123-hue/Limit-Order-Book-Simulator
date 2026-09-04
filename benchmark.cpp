#include "order_book.h"
#include <chrono>
#include <random>
#include <algorithm>
#include <iostream>
#include <vector>

int main() {
    const int NUM_ORDERS = 1'000'000;

    std::mt19937 rng(42);
    std::uniform_real_distribution<double> price_dist(95.0, 105.0);
    std::uniform_int_distribution<int> qty_dist(1, 100);
    std::uniform_int_distribution<int> side_dist(0, 1);

    OrderBook book;

    // Pre-generate orders so we're only timing the engine, not the RNG.
    std::vector<Order> orders;
    orders.reserve(NUM_ORDERS);
    for (int i = 0; i < NUM_ORDERS; i++) {
        Side s = side_dist(rng) == 0 ? Side::BUY : Side::SELL;
        double p = std::round(price_dist(rng) * 100.0) / 100.0;
        int q = qty_dist(rng);
        orders.push_back({(uint64_t)i, s, p, q, (uint64_t)i});
    }

    std::vector<long long> latencies_ns;
    latencies_ns.reserve(NUM_ORDERS);

    auto start = std::chrono::steady_clock::now();
    for (auto& o : orders) {
        auto t0 = std::chrono::steady_clock::now();
        book.addOrder(o);
        auto t1 = std::chrono::steady_clock::now();
        latencies_ns.push_back(
            std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
    }
    auto end = std::chrono::steady_clock::now();

    double total_seconds = std::chrono::duration<double>(end - start).count();
    double throughput = NUM_ORDERS / total_seconds;

    std::sort(latencies_ns.begin(), latencies_ns.end());
    long long p50 = latencies_ns[latencies_ns.size() * 50 / 100];
    long long p99 = latencies_ns[latencies_ns.size() * 99 / 100];
    long long p999 = latencies_ns[latencies_ns.size() * 999 / 1000];
    long long max_lat = latencies_ns.back();

    std::cout << "Orders processed: " << NUM_ORDERS << "\n";
    std::cout << "Total time: " << total_seconds << " s\n";
    std::cout << "Throughput: " << (long long)throughput << " orders/sec\n";
    std::cout << "Latency p50: " << p50 << " ns\n";
    std::cout << "Latency p99: " << p99 << " ns\n";
    std::cout << "Latency p99.9: " << p999 << " ns\n";
    std::cout << "Latency max: " << max_lat << " ns\n";

    return 0;
}
