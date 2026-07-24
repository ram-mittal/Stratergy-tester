#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// UltraBacktest — Portfolio Engine
//
// Tracks positions, cash, and PnL based on ExecutionReports.
// Uses a flat array for O(1) symbol lookups. Avoids heap allocations on the 
// hot path. PnL is computed using fixed-point arithmetic to avoid FP hazards.
// ─────────────────────────────────────────────────────────────────────────────

#include <ultrabacktest/common/types.hpp>
#include <ultrabacktest/common/config.hpp>

#include <vector>

namespace ultrabacktest {

struct Position {
    Symbol   symbol          = 0;
    int64_t  net_qty         = 0;   // Positive = long, Negative = short
    Price    avg_entry_price = 0;   // Volume-weighted average entry price
    Price    realized_pnl    = 0;
    Price    unrealized_pnl  = 0;
    Qty      total_traded    = 0;
    uint32_t num_trades      = 0;
    Price    last_price      = 0;   // For M2M calculation
};

class Portfolio {
public:
    explicit Portfolio(const BacktestConfig& config)
        : config_(config), cash_(config.initial_cash) 
    {
        positions_.resize(MAX_SYMBOLS);
        for (Symbol i = 0; i < MAX_SYMBOLS; ++i) {
            positions_[i].symbol = i;
        }
    }

    /// Process a fill from the exchange simulator
    void on_execution(const ExecutionReport& rpt);

    /// Mark to market based on latest price (tick or trade)
    void update_m2m(Symbol sym, Price last_price);

    [[nodiscard]] const Position& get_position(Symbol sym) const noexcept {
        return positions_[sym];
    }
    
    [[nodiscard]] Price cash() const noexcept { return cash_; }
    
    /// Total PnL (Realized + Unrealized) across all symbols
    [[nodiscard]] Price total_pnl() const noexcept;

    /// Total portfolio value (Cash + M2M value of positions)
    [[nodiscard]] Price total_equity() const noexcept {
        return cash_ + total_pnl();
    }

private:
    const BacktestConfig& config_;
    Price cash_;
    std::vector<Position> positions_; // Fixed size array, indexed by Symbol
};

} // namespace ultrabacktest
