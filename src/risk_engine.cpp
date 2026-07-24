#include <ultrabacktest/risk/risk_engine.hpp>
#include <cmath>

namespace ultrabacktest {

RiskRejectReason RiskEngine::check_pre_trade(const OrderRequest& req) {
    if (killed_) {
        return RiskRejectReason::KillSwitch;
    }

    // 1. Max Order Size check
    if (limits_.max_order_size > 0 && req.qty > limits_.max_order_size) {
        return RiskRejectReason::MaxOrderSize;
    }

    // 2. Max Loss check (trigger kill switch if breached)
    if (limits_.max_loss > 0 && portfolio_) {
        if (portfolio_->total_pnl() < -limits_.max_loss) {
            killed_ = true;
            return RiskRejectReason::MaxLoss;
        }
    }

    // 3. Max Position check
    if (limits_.max_position_per_symbol > 0 && portfolio_) {
        const Position& pos = portfolio_->get_position(req.symbol);
        
        // Calculate expected position if this order is fully filled
        // (Note: this is a simple check that doesn't account for other resting orders)
        int64_t expected_pos = pos.net_qty;
        if (req.side == Side::Buy) {
            expected_pos += req.qty;
        } else {
            expected_pos -= req.qty;
        }
        
        if (std::abs(expected_pos) > limits_.max_position_per_symbol) {
            return RiskRejectReason::MaxPosition;
        }
    }

    return RiskRejectReason::None;
}

} // namespace ultrabacktest
