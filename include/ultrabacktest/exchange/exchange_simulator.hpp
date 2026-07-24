#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// UltraBacktest — Exchange Simulator
//
// Wraps the UltraLOB MatchingEngine for deterministic backtesting.
// Handles order submission, latency simulation, and routes ExecutionReports
// to the Portfolio and Risk engines.
// ─────────────────────────────────────────────────────────────────────────────

#include <ultrabacktest/common/types.hpp>
#include <ultrabacktest/common/config.hpp>
#include <ultralob/matching_engine.hpp>

namespace ultrabacktest {

class Portfolio;

class ExchangeSimulator {
public:
    ExchangeSimulator(const BacktestConfig& config, Portfolio* portfolio)
        : engine_(config.order_pool_capacity)
        , config_(config)
        , portfolio_(portfolio)
    {}

    /// Initialize the engine (register symbols, setup callbacks)
    void initialize();

    /// Advance the simulator's internal clock
    void advance_to(Nanos timestamp) {
        current_time_ = timestamp;
        // In a more complex simulator, this is where we would flush the
        // simulated latency queue of pending orders. For v1, orders are 
        // submitted synchronously.
    }

    /// Submit an order to the simulated exchange
    void submit_order(const OrderRequest& req);

    /// Cancel a resting order
    bool cancel_order(Symbol sym, OrderId id);

    /// Modify a resting order (price/qty)
    bool modify_order(Symbol sym, const ModifyRequest& req);

    /// Get direct access to the underlying matching engine (e.g. for book snapshots)
    [[nodiscard]] const ultralob::MatchingEngine& engine() const noexcept {
        return engine_;
    }

private:
    // C-style callback for UltraLOB ExecutionReports
    static void on_execution_cb(const ExecutionReport& rpt, void* ctx);
    
    // Internal handler
    void handle_execution(const ExecutionReport& rpt);

    ultralob::MatchingEngine engine_;
    const BacktestConfig& config_;
    Portfolio* portfolio_;
    Nanos current_time_ = 0;
};

} // namespace ultrabacktest
