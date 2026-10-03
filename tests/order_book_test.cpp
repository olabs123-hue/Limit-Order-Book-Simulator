#include "order_book.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <random>
#include <unordered_map>
#include <vector>

namespace {

Order makeOrder(uint64_t id, Side side, double price, int qty) {
    return Order{id, side, price, qty, id};  // timestamp == id keeps arrival order obvious
}

int totalQty(const std::vector<Trade>& trades) {
    int sum = 0;
    for (const auto& t : trades) sum += t.quantity;
    return sum;
}

}  // namespace

// ---------------------------------------------------------------- basic matching

TEST(OrderBookMatching, NonCrossingOrdersDoNotTrade) {
    OrderBook book;
    EXPECT_TRUE(book.addOrder(makeOrder(1, Side::BUY, 99.0, 10)).empty());
    EXPECT_TRUE(book.addOrder(makeOrder(2, Side::SELL, 100.0, 10)).empty());
}

TEST(OrderBookMatching, CrossingOrderTradesAtRestingPrice) {
    OrderBook book;
    book.addOrder(makeOrder(1, Side::SELL, 100.0, 10));

    // Buyer is willing to pay 101, but the trade happens at the resting price.
    auto trades = book.addOrder(makeOrder(2, Side::BUY, 101.0, 10));

    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].buy_order_id, 2u);
    EXPECT_EQ(trades[0].sell_order_id, 1u);
    EXPECT_DOUBLE_EQ(trades[0].price, 100.0);
    EXPECT_EQ(trades[0].quantity, 10);
}

TEST(OrderBookMatching, SellSideTradesAtRestingBidPrice) {
    OrderBook book;
    book.addOrder(makeOrder(1, Side::BUY, 100.0, 10));

    auto trades = book.addOrder(makeOrder(2, Side::SELL, 99.0, 10));

    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].buy_order_id, 1u);
    EXPECT_EQ(trades[0].sell_order_id, 2u);
    EXPECT_DOUBLE_EQ(trades[0].price, 100.0);
}

TEST(OrderBookMatching, EqualPricesCross) {
    OrderBook book;
    book.addOrder(makeOrder(1, Side::SELL, 100.0, 5));
    auto trades = book.addOrder(makeOrder(2, Side::BUY, 100.0, 5));
    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].quantity, 5);
}

// ---------------------------------------------------------------- partial fills

TEST(OrderBookPartialFills, IncomingLargerThanRestingRestsTheRemainder) {
    OrderBook book;
    book.addOrder(makeOrder(1, Side::SELL, 100.0, 10));

    auto first = book.addOrder(makeOrder(2, Side::BUY, 101.0, 25));
    ASSERT_EQ(first.size(), 1u);
    EXPECT_EQ(first[0].quantity, 10);

    // The remaining 15 should now rest as a bid at 101 and match a later seller
    // at the resting (bid) price.
    auto second = book.addOrder(makeOrder(3, Side::SELL, 100.0, 15));
    ASSERT_EQ(second.size(), 1u);
    EXPECT_EQ(second[0].buy_order_id, 2u);
    EXPECT_EQ(second[0].sell_order_id, 3u);
    EXPECT_EQ(second[0].quantity, 15);
    EXPECT_DOUBLE_EQ(second[0].price, 101.0);
}

TEST(OrderBookPartialFills, RestingLargerThanIncomingKeepsTheRemainder) {
    OrderBook book;
    book.addOrder(makeOrder(1, Side::SELL, 100.0, 50));

    auto a = book.addOrder(makeOrder(2, Side::BUY, 100.0, 20));
    auto b = book.addOrder(makeOrder(3, Side::BUY, 100.0, 30));
    auto c = book.addOrder(makeOrder(4, Side::BUY, 100.0, 1));  // nothing left to hit

    ASSERT_EQ(a.size(), 1u);
    EXPECT_EQ(a[0].quantity, 20);
    ASSERT_EQ(b.size(), 1u);
    EXPECT_EQ(b[0].quantity, 30);
    EXPECT_TRUE(c.empty());
}

// ---------------------------------------------------------------- priority rules

TEST(OrderBookPriority, BestPriceIsMatchedFirstAcrossLevels) {
    OrderBook book;
    book.addOrder(makeOrder(1, Side::SELL, 102.0, 10));
    book.addOrder(makeOrder(2, Side::SELL, 100.0, 10));
    book.addOrder(makeOrder(3, Side::SELL, 101.0, 10));

    auto trades = book.addOrder(makeOrder(4, Side::BUY, 102.0, 30));

    ASSERT_EQ(trades.size(), 3u);
    EXPECT_EQ(trades[0].sell_order_id, 2u);  // 100
    EXPECT_EQ(trades[1].sell_order_id, 3u);  // 101
    EXPECT_EQ(trades[2].sell_order_id, 1u);  // 102
    EXPECT_DOUBLE_EQ(trades[0].price, 100.0);
    EXPECT_DOUBLE_EQ(trades[1].price, 101.0);
    EXPECT_DOUBLE_EQ(trades[2].price, 102.0);
}

TEST(OrderBookPriority, HighestBidIsMatchedFirst) {
    OrderBook book;
    book.addOrder(makeOrder(1, Side::BUY, 98.0, 10));
    book.addOrder(makeOrder(2, Side::BUY, 100.0, 10));
    book.addOrder(makeOrder(3, Side::BUY, 99.0, 10));

    auto trades = book.addOrder(makeOrder(4, Side::SELL, 98.0, 30));

    ASSERT_EQ(trades.size(), 3u);
    EXPECT_EQ(trades[0].buy_order_id, 2u);  // 100
    EXPECT_EQ(trades[1].buy_order_id, 3u);  // 99
    EXPECT_EQ(trades[2].buy_order_id, 1u);  // 98
}

TEST(OrderBookPriority, SamePriceIsFirstInFirstOut) {
    OrderBook book;
    book.addOrder(makeOrder(1, Side::SELL, 100.0, 5));
    book.addOrder(makeOrder(2, Side::SELL, 100.0, 5));
    book.addOrder(makeOrder(3, Side::SELL, 100.0, 5));

    auto trades = book.addOrder(makeOrder(4, Side::BUY, 100.0, 12));

    ASSERT_EQ(trades.size(), 3u);
    EXPECT_EQ(trades[0].sell_order_id, 1u);
    EXPECT_EQ(trades[1].sell_order_id, 2u);
    EXPECT_EQ(trades[2].sell_order_id, 3u);
    EXPECT_EQ(trades[2].quantity, 2);  // last order only partially consumed
}

TEST(OrderBookPriority, PartiallyFilledOrderKeepsItsQueuePosition) {
    OrderBook book;
    book.addOrder(makeOrder(1, Side::SELL, 100.0, 10));
    book.addOrder(makeOrder(2, Side::SELL, 100.0, 10));

    book.addOrder(makeOrder(3, Side::BUY, 100.0, 4));   // partially fills order 1
    auto trades = book.addOrder(makeOrder(4, Side::BUY, 100.0, 8));

    ASSERT_EQ(trades.size(), 2u);
    EXPECT_EQ(trades[0].sell_order_id, 1u);  // still first in line
    EXPECT_EQ(trades[0].quantity, 6);
    EXPECT_EQ(trades[1].sell_order_id, 2u);
    EXPECT_EQ(trades[1].quantity, 2);
}

// ---------------------------------------------------------------- cancellation

TEST(OrderBookCancel, CancelRestingOrderSucceedsOnceThenFails) {
    OrderBook book;
    book.addOrder(makeOrder(1, Side::SELL, 100.0, 10));

    EXPECT_TRUE(book.cancelOrder(1));
    EXPECT_FALSE(book.cancelOrder(1));
}

TEST(OrderBookCancel, CancelUnknownIdFails) {
    OrderBook book;
    EXPECT_FALSE(book.cancelOrder(12345));
}

TEST(OrderBookCancel, CancelledOrderCannotBeMatched) {
    OrderBook book;
    book.addOrder(makeOrder(1, Side::SELL, 100.0, 10));
    ASSERT_TRUE(book.cancelOrder(1));

    EXPECT_TRUE(book.addOrder(makeOrder(2, Side::BUY, 100.0, 10)).empty());
}

TEST(OrderBookCancel, FullyFilledOrderCannotBeCancelled) {
    OrderBook book;
    book.addOrder(makeOrder(1, Side::SELL, 100.0, 10));
    book.addOrder(makeOrder(2, Side::BUY, 100.0, 10));

    EXPECT_FALSE(book.cancelOrder(1));
    EXPECT_FALSE(book.cancelOrder(2));
}

TEST(OrderBookCancel, CancellingOneOrderLeavesOthersAtTheLevelIntact) {
    OrderBook book;
    book.addOrder(makeOrder(1, Side::SELL, 100.0, 5));
    book.addOrder(makeOrder(2, Side::SELL, 100.0, 5));
    book.addOrder(makeOrder(3, Side::SELL, 100.0, 5));
    ASSERT_TRUE(book.cancelOrder(2));

    auto trades = book.addOrder(makeOrder(4, Side::BUY, 100.0, 10));

    ASSERT_EQ(trades.size(), 2u);
    EXPECT_EQ(trades[0].sell_order_id, 1u);
    EXPECT_EQ(trades[1].sell_order_id, 3u);
}

TEST(OrderBookCancel, EmptiedLevelDoesNotLeaveAStaleBestPrice) {
    OrderBook book;
    book.addOrder(makeOrder(1, Side::SELL, 100.0, 10));
    book.addOrder(makeOrder(2, Side::SELL, 101.0, 10));
    ASSERT_TRUE(book.cancelOrder(1));  // level 100 is now empty and must be pruned

    auto trades = book.addOrder(makeOrder(3, Side::BUY, 101.0, 10));

    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].sell_order_id, 2u);
    EXPECT_DOUBLE_EQ(trades[0].price, 101.0);
}

TEST(OrderBookCancel, PartiallyFilledRestingOrderCanBeCancelled) {
    OrderBook book;
    book.addOrder(makeOrder(1, Side::SELL, 100.0, 10));
    book.addOrder(makeOrder(2, Side::BUY, 100.0, 4));

    EXPECT_TRUE(book.cancelOrder(1));
    EXPECT_TRUE(book.addOrder(makeOrder(3, Side::BUY, 100.0, 6)).empty());
}

// ---------------------------------------------------------------- bookkeeping

TEST(OrderBookBookkeeping, EmptyPriceLevelsArePrunedOnCancel) {
    OrderBook book;
    book.addOrder(makeOrder(1, Side::SELL, 100.0, 5));
    book.addOrder(makeOrder(2, Side::SELL, 100.0, 5));
    book.addOrder(makeOrder(3, Side::SELL, 101.0, 5));
    ASSERT_EQ(book.askLevelCount(), 2u);

    book.cancelOrder(1);
    EXPECT_EQ(book.askLevelCount(), 2u);  // level 100 still holds order 2
    book.cancelOrder(2);
    EXPECT_EQ(book.askLevelCount(), 1u);  // level 100 is now gone
    book.cancelOrder(3);
    EXPECT_EQ(book.askLevelCount(), 0u);
    EXPECT_EQ(book.restingOrderCount(), 0u);
}

TEST(OrderBookBookkeeping, EmptyPriceLevelsArePrunedAfterFills) {
    OrderBook book;
    book.addOrder(makeOrder(1, Side::BUY, 99.0, 5));
    book.addOrder(makeOrder(2, Side::BUY, 98.0, 5));
    ASSERT_EQ(book.bidLevelCount(), 2u);

    book.addOrder(makeOrder(3, Side::SELL, 98.0, 10));  // sweeps both bid levels

    EXPECT_EQ(book.bidLevelCount(), 0u);
    EXPECT_EQ(book.askLevelCount(), 0u);
    EXPECT_EQ(book.restingOrderCount(), 0u);
}

TEST(OrderBookBookkeeping, OrderIndexTracksOnlyRestingOrders) {
    OrderBook book;
    book.addOrder(makeOrder(1, Side::SELL, 100.0, 10));
    EXPECT_EQ(book.restingOrderCount(), 1u);

    book.addOrder(makeOrder(2, Side::BUY, 100.0, 4));  // partial fill: order 1 still rests
    EXPECT_EQ(book.restingOrderCount(), 1u);

    book.addOrder(makeOrder(3, Side::BUY, 100.0, 6));  // finishes order 1; nothing left to rest
    EXPECT_EQ(book.restingOrderCount(), 0u);
}

// ---------------------------------------------------------------- invariants

// Randomised check against an independent record of each order's limit price
// and quantity. Fixed seed so failures are reproducible.
TEST(OrderBookInvariants, RandomFlowRespectsLimitsAndQuantities) {
    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> price_ticks(9900, 10100);  // 99.00 .. 101.00
    std::uniform_int_distribution<int> qty_dist(1, 50);
    std::uniform_int_distribution<int> side_dist(0, 1);
    std::uniform_int_distribution<int> cancel_dist(0, 9);

    OrderBook book;
    std::unordered_map<uint64_t, Order> submitted;
    std::unordered_map<uint64_t, int> filled;
    std::vector<uint64_t> ids;

    for (uint64_t id = 1; id <= 20000; ++id) {
        if (!ids.empty() && cancel_dist(rng) == 0) {
            book.cancelOrder(ids[rng() % ids.size()]);  // may or may not still be resting
        }

        Order o = makeOrder(id, side_dist(rng) == 0 ? Side::BUY : Side::SELL,
                            price_ticks(rng) / 100.0, qty_dist(rng));
        submitted[id] = o;
        ids.push_back(id);

        for (const auto& t : book.addOrder(o)) {
            const Order& buy = submitted.at(t.buy_order_id);
            const Order& sell = submitted.at(t.sell_order_id);

            // Orders only trade when their limits cross, and never at a price
            // worse than either side's limit.
            ASSERT_GE(buy.price, sell.price);
            ASSERT_LE(t.price, buy.price);
            ASSERT_GE(t.price, sell.price);
            ASSERT_GT(t.quantity, 0);

            filled[t.buy_order_id] += t.quantity;
            filled[t.sell_order_id] += t.quantity;
        }
    }

    // No order is ever filled for more than its original quantity.
    for (const auto& [id, qty] : filled) {
        EXPECT_LE(qty, submitted.at(id).quantity) << "order " << id << " overfilled";
    }
}

TEST(OrderBookInvariants, BuyAndSellFillsBalance) {
    OrderBook book;
    std::mt19937 rng(7);
    std::uniform_int_distribution<int> qty_dist(1, 20);
    std::uniform_int_distribution<int> side_dist(0, 1);

    int buy_filled = 0;
    int sell_filled = 0;
    std::unordered_map<uint64_t, Side> side_of;

    for (uint64_t id = 1; id <= 5000; ++id) {
        Side s = side_dist(rng) == 0 ? Side::BUY : Side::SELL;
        side_of[id] = s;
        for (const auto& t : book.addOrder(makeOrder(id, s, 100.0, qty_dist(rng)))) {
            buy_filled += t.quantity;
            sell_filled += t.quantity;
            EXPECT_EQ(side_of.at(t.buy_order_id), Side::BUY);
            EXPECT_EQ(side_of.at(t.sell_order_id), Side::SELL);
        }
    }
    EXPECT_EQ(buy_filled, sell_filled);
}
