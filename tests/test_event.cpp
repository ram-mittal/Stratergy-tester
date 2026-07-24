#include <ultrabacktest/events/event.hpp>
#include <gtest/gtest.h>

using namespace ultrabacktest;

TEST(EventTest, MemoryLayout) {
    // Assert 64-byte size and alignment
    EXPECT_EQ(sizeof(Event), 64);
    EXPECT_EQ(alignof(Event), 64);
}

TEST(EventTest, PayloadsFit) {
    // Check that individual payloads don't exceed 48 bytes
    EXPECT_LE(sizeof(TickData), 48);
    EXPECT_LE(sizeof(TradeData), 48);
    EXPECT_LE(sizeof(QuoteData), 48);
    EXPECT_LE(sizeof(OrderAddData), 48);
    EXPECT_LE(sizeof(OrderCancelData), 48);
    EXPECT_LE(sizeof(OrderModifyData), 48);
    EXPECT_LE(sizeof(OrderExecutionData), 48);
    EXPECT_LE(sizeof(TimerData), 48);
}

TEST(EventTest, FieldOffsets) {
    // Ensure header layout matches expectations for efficient memory access
    EXPECT_EQ(offsetof(Event, timestamp), 0);
    EXPECT_EQ(offsetof(Event, symbol), 8);
    EXPECT_EQ(offsetof(Event, type), 12);
    // 13, 14, 15 are padding
    EXPECT_EQ(offsetof(Event, tick), 16);
    EXPECT_EQ(offsetof(Event, trade), 16);
}

TEST(EventTest, MakeTrade) {
    Nanos ts = 1000;
    Symbol sym = 42;
    Price p = from_double(100.50);
    Qty q = 100;
    Side s = Side::Buy;

    Event e = Event::make_trade(ts, sym, p, q, s);

    EXPECT_EQ(e.timestamp, ts);
    EXPECT_EQ(e.symbol, sym);
    EXPECT_EQ(e.type, EventType::Trade);
    EXPECT_EQ(e.trade.price, p);
    EXPECT_EQ(e.trade.qty, q);
    EXPECT_EQ(e.trade.aggressor_side, s);
}
