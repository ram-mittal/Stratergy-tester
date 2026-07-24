#include <ultrabacktest/portfolio/portfolio.hpp>
#include <algorithm>

namespace ultrabacktest {

void Portfolio::on_execution(const ExecutionReport& rpt) {
    if (!rpt.is_fill()) {
        return;
    }

    Position& pos = positions_[rpt.symbol];
    
    // Fee calculation (assuming taker for now, would need resting order flag for maker)
    Price fee = config_.fee_model.compute(rpt.exec_qty, false);
    cash_ -= fee;

    const int64_t qty = static_cast<int64_t>(rpt.exec_qty);
    const Price exec_price = rpt.exec_price;

    if (rpt.side == Side::Buy) {
        if (pos.net_qty >= 0) {
            // Increasing long position
            Price total_cost = (pos.avg_entry_price * pos.net_qty) + (exec_price * qty);
            pos.net_qty += qty;
            pos.avg_entry_price = total_cost / pos.net_qty;
        } else {
            // Closing short position
            int64_t closing_qty = std::min(qty, -pos.net_qty);
            // PnL = (entry - exit) * qty
            Price realized = (pos.avg_entry_price - exec_price) * closing_qty;
            pos.realized_pnl += realized;
            cash_ += realized;
            
            pos.net_qty += qty; // might flip to long
            if (pos.net_qty > 0) {
                pos.avg_entry_price = exec_price;
            } else if (pos.net_qty == 0) {
                pos.avg_entry_price = 0;
            }
        }
    } else {
        if (pos.net_qty <= 0) {
            // Increasing short position
            Price total_value = (pos.avg_entry_price * -pos.net_qty) + (exec_price * qty);
            pos.net_qty -= qty;
            pos.avg_entry_price = total_value / -pos.net_qty;
        } else {
            // Closing long position
            int64_t closing_qty = std::min(qty, pos.net_qty);
            // PnL = (exit - entry) * qty
            Price realized = (exec_price - pos.avg_entry_price) * closing_qty;
            pos.realized_pnl += realized;
            cash_ += realized;
            
            pos.net_qty -= qty; // might flip to short
            if (pos.net_qty < 0) {
                pos.avg_entry_price = exec_price;
            } else if (pos.net_qty == 0) {
                pos.avg_entry_price = 0;
            }
        }
    }

    pos.total_traded += qty;
    pos.num_trades++;
    pos.last_price = exec_price;
    update_m2m(rpt.symbol, exec_price);
}

void Portfolio::update_m2m(Symbol sym, Price last_price) {
    Position& pos = positions_[sym];
    pos.last_price = last_price;
    
    if (pos.net_qty > 0) {
        pos.unrealized_pnl = (last_price - pos.avg_entry_price) * pos.net_qty;
    } else if (pos.net_qty < 0) {
        pos.unrealized_pnl = (pos.avg_entry_price - last_price) * -pos.net_qty;
    } else {
        pos.unrealized_pnl = 0;
    }
}

Price Portfolio::total_pnl() const noexcept {
    Price total = 0;
    for (Symbol i = 0; i < MAX_SYMBOLS; ++i) {
        if (positions_[i].num_trades > 0 || positions_[i].net_qty != 0) {
            total += positions_[i].realized_pnl + positions_[i].unrealized_pnl;
        }
    }
    return total;
}

} // namespace ultrabacktest
