#pragma once
#include "order.h"
#include <map>
#include <list>
#include <unordered_map>
#include <vector>
#include <functional>
#include <iostream>

struct Trade {
    uint64_t buy_order_id;
    uint64_t sell_order_id;
    double price;
    int quantity;
};

class OrderBook {
public:
    // Add a new limit order. Attempts to match it against the book first;
    // whatever quantity isn't filled gets rested in the book.
    // Returns the trades generated.
    std::vector<Trade> addOrder(Order order);

    // Cancel a resting order by id. O(1) average case.
    bool cancelOrder(uint64_t order_id);

    void printBook() const;

private:
    // Buy side: highest price first  -> descending order
    // std::map is sorted ascending by default, so we use std::greater<double>
    std::map<double, std::list<Order>, std::greater<double>> buy_levels_;

    // Sell side: lowest price first -> ascending order (map's default)
    std::map<double, std::list<Order>> sell_levels_;

    // Fast lookup for cancellation: order_id -> (side, price, iterator into the list)
    struct OrderLocation {
        Side side;
        double price;
        std::list<Order>::iterator it;
    };
    std::unordered_map<uint64_t, OrderLocation> order_index_;

    std::vector<Trade> matchBuy(Order& incoming);
    std::vector<Trade> matchSell(Order& incoming);
};
