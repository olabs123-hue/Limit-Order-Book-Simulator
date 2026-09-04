#include "order_book.h"
#include <iostream>

int main() {
    OrderBook book;
    uint64_t ts = 0;

    std::cout << "=== Resting some sell orders ===\n";
    book.addOrder({1, Side::SELL, 101.5, 50, ts++});
    book.addOrder({2, Side::SELL, 101.0, 30, ts++});
    book.addOrder({3, Side::SELL, 102.0, 20, ts++});
    book.printBook();

    std::cout << "\n=== Resting some buy orders (no cross yet) ===\n";
    book.addOrder({4, Side::BUY, 99.0, 40, ts++});
    book.addOrder({5, Side::BUY, 98.5, 25, ts++});
    book.printBook();

    std::cout << "\n=== Cancel order 2 (sell 30 @ 101.0) ===\n";
    bool cancelled = book.cancelOrder(2);
    std::cout << "cancelOrder(2) returned: " << std::boolalpha << cancelled << "\n";
    book.printBook();

    std::cout << "\n=== Incoming BUY order that crosses the book: BUY 60 @ 102.0 ===\n";
    auto trades = book.addOrder({6, Side::BUY, 102.0, 60, ts++});
    std::cout << "Trades generated: " << trades.size() << "\n";
    for (auto& t : trades) {
        std::cout << "  buy_id=" << t.buy_order_id
                  << " sell_id=" << t.sell_order_id
                  << " price=" << t.price
                  << " qty=" << t.quantity << "\n";
    }
    book.printBook();

    std::cout << "\n=== Cancel an order that no longer exists (order 1, fully filled) ===\n";
    cancelled = book.cancelOrder(1);
    std::cout << "cancelOrder(1) returned: " << std::boolalpha << cancelled << "\n";

    return 0;
}
