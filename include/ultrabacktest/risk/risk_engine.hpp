#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// UltraBacktest — Risk Engine
//
// Performs ultra-low latency pre-trade risk checks before orders are routed
// to the ExchangeSimulator. Also monitors portfolio-level limits (max loss)
// to trigger kill switches.
// ─────────────────────────────────────────────────────────────────────────────

#include <ultrabacktest/common/types.hpp>
#include <ultrabacktest/portfolio/portfolio.hpp>

namespace ultrabacktest {

struct RiskLimits {
    Qty   max_position_per_symbol = 100'000; // max abs(position) per symbol
    Price max_loss                = 0;       // portfolio max loss (0 = disabled)
    Qty   max_order_size          = 10'000;  // max qty per order
};

enum class RiskRejectReason : uint8_t {
    None            = 0,
    MaxPosition     = 1,
    MaxLoss         = 2,
    MaxOrderSize    = 3,
    KillSwitch      = 4
};

class RiskEngine {
public:
    RiskEngine(const RiskLimits& limits, Portfolio* portfolio)
        : limits_(limits), portfolio_(portfolio) {}

    /// Pre-trade check. Returns None if approved.
    RiskRejectReason check_pre_trade(const OrderRequest& req);

    /// Manually trip the kill switch
    void trigger_kill_switch() { killed_ = true; }

    /// Has the kill switch been tripped?
    [[nodiscard]] bool is_killed() const noexcept { return killed_; }

private:
    RiskLimits limits_;
    Portfolio* portfolio_;
    bool killed_ = false;
};

} // namespace ultrabacktest
