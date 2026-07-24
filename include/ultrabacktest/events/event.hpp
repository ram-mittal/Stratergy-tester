#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// UltraBacktest — Core Event Structure
//
// A single 64-byte aligned struct representing any event in the system.
// By fitting exactly into one x86 cache line, we guarantee peak memory 
// bandwidth during sequential batch processing in the Replay Engine.
// ─────────────────────────────────────────────────────────────────────────────

#include "event_types.hpp"

namespace ultrabacktest {

struct alignas(64) Event {
    // ── Header (16 bytes) ───────────────────────────────────────────────────
    Nanos     timestamp;    // 8B - sort key, always first
    Symbol    symbol;       // 4B
    EventType type;         // 1B
    uint8_t   _pad[3];      // 3B alignment padding
    
    // ── Payload (48 bytes) ──────────────────────────────────────────────────
    union {
        TickData           tick;
        TradeData          trade;
        QuoteData          quote;
        OrderAddData       order_add;
        OrderCancelData    order_cancel;
        OrderModifyData    order_modify;
        OrderExecutionData order_execution;
        TimerData          timer;
    };

    // ─────────────────────────────────────────────────────────────────────────
    // Helper Constructors
    // ─────────────────────────────────────────────────────────────────────────

    constexpr Event() noexcept
        : timestamp(0), symbol(0), type(EventType::EndOfData), _pad{0,0,0}, tick{} {}

    // Factory for EndOfData sentinel
    [[nodiscard]] static constexpr Event make_end_of_data() noexcept {
        return Event();
    }

    // Example factory for a Trade event
    [[nodiscard]] static constexpr Event make_trade(Nanos ts, Symbol sym, Price p, Qty q, Side s) noexcept {
        Event e{};
        e.timestamp = ts;
        e.symbol = sym;
        e.type = EventType::Trade;
        e.trade.price = p;
        e.trade.qty = q;
        e.trade.aggressor_side = s;
        return e;
    }
};

// ── Static Assertions ────────────────────────────────────────────────────────
// Guarantee layout and size for determinism and cache locality.
static_assert(sizeof(Event) == 64, "Event must be exactly 64 bytes (1 cache line)");
static_assert(alignof(Event) == 64, "Event must be 64-byte aligned");

} // namespace ultrabacktest
