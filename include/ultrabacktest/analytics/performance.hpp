#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// UltraBacktest — Performance Analytics
//
// Online (streaming) computation of backtest performance metrics.
// Computes Sharpe, Drawdown, and return statistics incrementally without
// storing the entire equity curve in memory.
// ─────────────────────────────────────────────────────────────────────────────

#include <ultrabacktest/common/types.hpp>
#include <ultrabacktest/portfolio/portfolio.hpp>

namespace ultrabacktest {

class PerformanceAnalytics {
public:
    PerformanceAnalytics() = default;

    /// Update metrics. Call this periodically (e.g., daily or per trade)
    void update(Nanos ts, const Portfolio& port);

    [[nodiscard]] double sharpe_ratio(double risk_free_rate = 0.0) const noexcept;
    [[nodiscard]] double max_drawdown() const noexcept { return max_dd_; }
    [[nodiscard]] double max_drawdown_pct() const noexcept { return max_dd_pct_; }
    [[nodiscard]] double total_return() const noexcept;
    [[nodiscard]] double annualized_return(Nanos duration_ns) const noexcept;

private:
    double initial_equity_ = -1.0;
    double last_equity_ = 0.0;
    
    double peak_equity_ = 0.0;
    double max_dd_ = 0.0;
    double max_dd_pct_ = 0.0;
    
    // Welford's online algorithm for variance of returns
    size_t count_ = 0;
    double mean_return_ = 0.0;
    double m2_ = 0.0;
};

} // namespace ultrabacktest
