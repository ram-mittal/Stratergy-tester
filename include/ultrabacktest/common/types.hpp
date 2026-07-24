#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// UltraBacktest — Core Types
//
// Re-exports UltraLOB's type system and adds backtester-specific types.
//
// Design decisions:
//   • Reuse UltraLOB Price (int64, 8dp fixed-point) for ALL monetary values
//     including PnL, fees, margin — eliminates FP comparison hazards.
//   • Use strong typedefs (enum class wrappers) where disambiguation matters
//     but raw integers where hot-path arithmetic would suffer from wrapping.
//   • StrategyId and InstrumentId are uint32_t — fits in a register, can be
//     used as array index for O(1) lookup in flat position arrays.
// ─────────────────────────────────────────────────────────────────────────────

#include <ultralob/types.hpp>
#include <ultralob/matching_engine.hpp>  // OrderRequest, ModifyRequest, RejectReason

#include <cstdint>
#include <cstddef>
#include <string_view>
#include <chrono>

namespace ultrabacktest {

// ─────────────────────────────────────────────────────────────────────────────
// Re-export UltraLOB types (avoid forcing downstream to include ultralob/)
// ─────────────────────────────────────────────────────────────────────────────

using ultralob::Price;
using ultralob::Qty;
using ultralob::OrderId;
using ultralob::Symbol;
using ultralob::Nanos;
using ultralob::Side;
using ultralob::OrderType;
using ultralob::OrderStatus;
using ultralob::ExecutionReport;
using ultralob::ExecCallback;
using ultralob::OrderRequest;
using ultralob::ModifyRequest;
using ultralob::RejectReason;
using ultralob::PRICE_SCALE;
using ultralob::PRICE_MAX;
using ultralob::PRICE_MIN;
using ultralob::PRICE_NONE;
using ultralob::INVALID_ORDER_ID;
using ultralob::from_double;
using ultralob::to_double;

// ─────────────────────────────────────────────────────────────────────────────
// Backtester-specific identifiers
// ─────────────────────────────────────────────────────────────────────────────

/// Strategy instance identifier (used as array index → must be dense, small).
using StrategyId = uint32_t;

/// Instrument identifier (maps 1:1 with UltraLOB Symbol).
using InstrumentId = uint32_t;

/// Invalid sentinel values
inline constexpr StrategyId   INVALID_STRATEGY_ID   = 0;
inline constexpr InstrumentId INVALID_INSTRUMENT_ID = 0;

// ─────────────────────────────────────────────────────────────────────────────
// Event type enumeration
//
// Stored as uint8_t in the Event struct — single byte, no padding waste.
// Ordered by expected frequency (ticks > trades > quotes) so branch prediction
// favours the common case when dispatching via switch.
// ─────────────────────────────────────────────────────────────────────────────

enum class EventType : uint8_t {
    // ── Market data events (most frequent first) ────────────────────────────
    MarketTick      = 0,   // generic tick (price + qty update)
    Trade           = 1,   // executed trade on the exchange
    Quote           = 2,   // best bid/ask update
    OrderAdd        = 3,   // L3: new order added to book
    OrderCancel     = 4,   // L3: order cancelled
    OrderModify     = 5,   // L3: order modified
    OrderExecution  = 6,   // L3: order executed (fill)

    // ── Strategy / system events ────────────────────────────────────────────
    StrategyTimer   = 10,  // timer scheduled by strategy
    BarClose        = 11,  // synthetic bar close (1s, 1m, etc.)

    // ── Session events ──────────────────────────────────────────────────────
    SessionStart    = 20,  // trading session open
    SessionEnd      = 21,  // trading session close

    // ── Internal ────────────────────────────────────────────────────────────
    EndOfData       = 255, // sentinel: no more events
};

[[nodiscard]] inline constexpr std::string_view to_string(EventType t) noexcept {
    switch (t) {
        case EventType::MarketTick:      return "MarketTick";
        case EventType::Trade:           return "Trade";
        case EventType::Quote:           return "Quote";
        case EventType::OrderAdd:        return "OrderAdd";
        case EventType::OrderCancel:     return "OrderCancel";
        case EventType::OrderModify:     return "OrderModify";
        case EventType::OrderExecution:  return "OrderExecution";
        case EventType::StrategyTimer:   return "StrategyTimer";
        case EventType::BarClose:        return "BarClose";
        case EventType::SessionStart:    return "SessionStart";
        case EventType::SessionEnd:      return "SessionEnd";
        case EventType::EndOfData:       return "EndOfData";
    }
    return "Unknown";
}

// ─────────────────────────────────────────────────────────────────────────────
// Fee model
//
// All fees in fixed-point Price units (8dp).
// Supports per-share and per-trade fees.
// ─────────────────────────────────────────────────────────────────────────────

struct FeeModel {
    Price per_share_fee   = 0;   // charged per share (e.g. $0.001 = 100'000)
    Price per_trade_fee   = 0;   // flat fee per trade (e.g. $1.00 = 100'000'000)
    Price maker_rebate    = 0;   // rebate for passive fills (negative fee)
    Price taker_fee       = 0;   // fee for aggressive fills

    /// Compute total fee for a fill.
    [[nodiscard]] constexpr Price compute(Qty fill_qty, bool is_maker) const noexcept {
        const Price share_cost = per_share_fee * static_cast<Price>(fill_qty);
        const Price exchange_fee = is_maker ? -maker_rebate : taker_fee;
        const Price exchange_cost = exchange_fee * static_cast<Price>(fill_qty);
        return share_cost + exchange_cost + per_trade_fee;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Slippage model
// ─────────────────────────────────────────────────────────────────────────────

enum class SlippageModel : uint8_t {
    None      = 0,   // no slippage (ideal execution)
    Fixed     = 1,   // fixed tick slippage per trade
    VolumeImpact = 2, // proportional to fill qty / book depth
};

struct SlippageConfig {
    SlippageModel model           = SlippageModel::None;
    Price         fixed_slippage  = 0;        // in price units (fixed model)
    double        impact_factor   = 0.0;      // multiplier (volume impact model)
};

// ─────────────────────────────────────────────────────────────────────────────
// Clock utilities
// ─────────────────────────────────────────────────────────────────────────────

/// High-resolution wall clock (for latency measurement, not event timestamps)
[[nodiscard]] inline Nanos wall_clock_nanos() noexcept {
    using namespace std::chrono;
    return duration_cast<nanoseconds>(
        steady_clock::now().time_since_epoch()).count();
}

// ─────────────────────────────────────────────────────────────────────────────
// Compile-time constants
// ─────────────────────────────────────────────────────────────────────────────

/// Maximum number of symbols supported in a single backtest.
/// Used to size flat arrays (positions, risk limits, book pointers).
/// 256 symbols × sizeof(Position) ≈ 16KB — fits in L1 cache.
inline constexpr size_t MAX_SYMBOLS = 256;

/// Maximum number of concurrent strategies.
inline constexpr size_t MAX_STRATEGIES = 16;

/// Prefetch hint distance (events ahead to prefetch during replay)
inline constexpr size_t PREFETCH_DISTANCE = 4;

} // namespace ultrabacktest
