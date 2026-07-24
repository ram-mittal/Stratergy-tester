#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// UltraBacktest — Event Types (Payloads)
//
// These are the raw POD payloads for the tagged Event union.
// Design constraint: Max payload size is 48 bytes to keep the full Event
// exactly 64 bytes (one cache line).
// ─────────────────────────────────────────────────────────────────────────────

#include <ultrabacktest/common/types.hpp>

namespace ultrabacktest {

// ─────────────────────────────────────────────────────────────────────────────
// Market Data Payloads
// ─────────────────────────────────────────────────────────────────────────────

struct TickData {
    Price best_bid;    // 8B
    Price best_ask;    // 8B
    Qty   bid_qty;     // 8B
    Qty   ask_qty;     // 8B
    Price last_price;  // 8B
    Qty   last_qty;    // 8B
}; // Total: 48B

struct TradeData {
    Price   price;           // 8B
    Qty     qty;             // 8B
    Side    aggressor_side;  // 1B
    uint8_t _pad[7];         // 7B
}; // Total: 24B

struct QuoteData {
    Price best_bid;    // 8B
    Price best_ask;    // 8B
    Qty   bid_qty;     // 8B
    Qty   ask_qty;     // 8B
}; // Total: 32B

// ─────────────────────────────────────────────────────────────────────────────
// L3 Order Book Payloads
// ─────────────────────────────────────────────────────────────────────────────

struct OrderAddData {
    OrderId order_id;  // 8B
    Price   price;     // 8B
    Qty     qty;       // 8B
    Side    side;      // 1B
    uint8_t _pad[7];   // 7B
}; // Total: 32B

struct OrderCancelData {
    OrderId order_id;  // 8B
}; // Total: 8B

struct OrderModifyData {
    OrderId order_id;  // 8B
    Price   new_price; // 8B (0 = keep existing)
    Qty     new_qty;   // 8B (0 = keep existing)
}; // Total: 24B

struct OrderExecutionData {
    OrderId order_id;    // 8B
    Price   exec_price;  // 8B
    Qty     exec_qty;    // 8B
    Qty     leaves_qty;  // 8B
    Side    side;        // 1B
    uint8_t _pad[7];     // 7B
}; // Total: 40B

// ─────────────────────────────────────────────────────────────────────────────
// Strategy / System Payloads
// ─────────────────────────────────────────────────────────────────────────────

struct TimerData {
    StrategyId strategy_id;  // 4B
    uint32_t   timer_id;     // 4B
}; // Total: 8B

} // namespace ultrabacktest
