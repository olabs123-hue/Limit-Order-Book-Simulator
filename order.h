#pragma once
#include <cstdint>

enum class Side { BUY, SELL };

struct Order {
    uint64_t id;
    Side side;
    double price;
    int quantity;      // remaining quantity (shrinks as it gets filled)
    uint64_t timestamp; // used to enforce time priority at the same price
};
