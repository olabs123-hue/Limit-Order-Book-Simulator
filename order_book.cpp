#include "order_book.h"

std::vector<Trade> OrderBook::addOrder(Order order) {
    std::vector<Trade> trades;

    if (order.side == Side::BUY) {
        trades = matchBuy(order);
    } else {
        trades = matchSell(order);
    }

    // If quantity remains after matching, rest the order in the book.
    if (order.quantity > 0) {
        if (order.side == Side::BUY) {
            auto& dq = buy_levels_[order.price];
            dq.push_back(order);
            auto it = std::prev(dq.end());
            order_index_[order.id] = {Side::BUY, order.price, it};
        } else {
            auto& dq = sell_levels_[order.price];
            dq.push_back(order);
            auto it = std::prev(dq.end());
            order_index_[order.id] = {Side::SELL, order.price, it};
        }
    }

    return trades;
}

std::vector<Trade> OrderBook::matchBuy(Order& incoming) {
    std::vector<Trade> trades;

    // Walk the sell side from lowest price upward, while incoming buy
    // price is >= the best ask (i.e. a match is possible), and while
    // the incoming order still has quantity left to fill.
    auto level_it = sell_levels_.begin();
    while (incoming.quantity > 0 && level_it != sell_levels_.end()
           && incoming.price >= level_it->first) {

        auto& orders_at_level = level_it->second;

        while (incoming.quantity > 0 && !orders_at_level.empty()) {
            Order& resting = orders_at_level.front();
            int fill_qty = std::min(incoming.quantity, resting.quantity);

            trades.push_back({incoming.id, resting.id, resting.price, fill_qty});

            incoming.quantity -= fill_qty;
            resting.quantity -= fill_qty;

            if (resting.quantity == 0) {
                order_index_.erase(resting.id);
                orders_at_level.pop_front();
            }
        }

        if (orders_at_level.empty()) {
            level_it = sell_levels_.erase(level_it); // remove empty price level
        } else {
            ++level_it;
        }
    }

    return trades;
}

std::vector<Trade> OrderBook::matchSell(Order& incoming) {
    std::vector<Trade> trades;

    auto level_it = buy_levels_.begin();
    while (incoming.quantity > 0 && level_it != buy_levels_.end()
           && incoming.price <= level_it->first) {

        auto& orders_at_level = level_it->second;

        while (incoming.quantity > 0 && !orders_at_level.empty()) {
            Order& resting = orders_at_level.front();
            int fill_qty = std::min(incoming.quantity, resting.quantity);

            trades.push_back({resting.id, incoming.id, resting.price, fill_qty});

            incoming.quantity -= fill_qty;
            resting.quantity -= fill_qty;

            if (resting.quantity == 0) {
                order_index_.erase(resting.id);
                orders_at_level.pop_front();
            }
        }

        if (orders_at_level.empty()) {
            level_it = buy_levels_.erase(level_it);
        } else {
            ++level_it;
        }
    }

    return trades;
}

bool OrderBook::cancelOrder(uint64_t order_id) {
    auto found = order_index_.find(order_id);
    if (found == order_index_.end()) {
        return false; // order not found (already filled or never existed)
    }

    const OrderLocation& loc = found->second;

    if (loc.side == Side::BUY) {
        auto level_it = buy_levels_.find(loc.price);
        level_it->second.erase(loc.it);
        if (level_it->second.empty()) {
            buy_levels_.erase(level_it);
        }
    } else {
        auto level_it = sell_levels_.find(loc.price);
        level_it->second.erase(loc.it);
        if (level_it->second.empty()) {
            sell_levels_.erase(level_it);
        }
    }

    order_index_.erase(found);
    return true;
}

void OrderBook::printBook() const {
    std::cout << "----- SELL (asks) -----\n";
    for (auto it = sell_levels_.rbegin(); it != sell_levels_.rend(); ++it) {
        int total = 0;
        for (auto& o : it->second) total += o.quantity;
        std::cout << "  " << it->first << " x " << total << "\n";
    }
    std::cout << "----- BUY (bids) -----\n";
    for (auto& [price, orders] : buy_levels_) {
        int total = 0;
        for (auto& o : orders) total += o.quantity;
        std::cout << "  " << price << " x " << total << "\n";
    }
    std::cout << "------------------------\n";
}
