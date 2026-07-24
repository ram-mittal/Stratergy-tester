#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// UltraBacktest — Strategy CRTP Base
//
// Base class for all user strategies. By using CRTP, the EventDispatcher
// can call derived class methods without virtual vtable lookups. If a derived
// class does not implement an event handler, it falls back to the empty
// inline definitions here, which the compiler optimizes away entirely.
// ─────────────────────────────────────────────────────────────────────────────

#include <ultrabacktest/common/types.hpp>
#include <ultrabacktest/exchange/exchange_simulator.hpp>
#include <ultrabacktest/portfolio/portfolio.hpp>
#include <ultrabacktest/events/event_types.hpp>

namespace ultrabacktest {

template <typename Derived>
class Strategy {
public:
    Strategy(ExchangeSimulator* sim, Portfolio* port) 
        : sim_(sim), portfolio_(port) {}

    // ─────────────────────────────────────────────────────────────────────────
    // Default Event Handlers (optimized away if unused, hidden if implemented
    // in Derived class).
    // ─────────────────────────────────────────────────────────────────────────
    inline void on_tick(Nanos ts, Symbol sym, const TickData& tick) {}
    inline void on_trade(Nanos ts, Symbol sym, const TradeData& trade) {}
    inline void on_quote(Nanos ts, Symbol sym, const QuoteData& quote) {}
    
    inline void on_order_add(Nanos ts, Symbol sym, const OrderAddData& order_add) {}
    inline void on_order_cancel(Nanos ts, Symbol sym, const OrderCancelData& order_cancel) {}
    inline void on_order_modify(Nanos ts, Symbol sym, const OrderModifyData& order_modify) {}
    inline void on_order_execution(Nanos ts, Symbol sym, const OrderExecutionData& exec) {}
    
    inline void on_timer(Nanos ts, const TimerData& timer) {}
    inline void on_session_start(Nanos ts) {}
    inline void on_session_end(Nanos ts) {}

protected:
    // ─────────────────────────────────────────────────────────────────────────
    // Actions (Order Routing)
    // ─────────────────────────────────────────────────────────────────────────
    void send_order(const OrderRequest& req) {
        sim_->submit_order(req);
    }
    
    bool cancel_order(Symbol sym, OrderId id) {
        return sim_->cancel_order(sym, id);
    }

    bool modify_order(Symbol sym, const ModifyRequest& req) {
        return sim_->modify_order(sym, req);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Context
    // ─────────────────────────────────────────────────────────────────────────
    [[nodiscard]] const Portfolio* portfolio() const noexcept { return portfolio_; }
    [[nodiscard]] const ExchangeSimulator* simulator() const noexcept { return sim_; }

private:
    ExchangeSimulator* sim_;
    Portfolio* portfolio_;
};

} // namespace ultrabacktest
