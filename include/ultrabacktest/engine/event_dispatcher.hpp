#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// UltraBacktest — Event Dispatcher
//
// Routes raw Events to Strategy callbacks using CRTP.
// Zero virtual dispatch overhead, fully inlinable at compile time.
// ─────────────────────────────────────────────────────────────────────────────

#include <ultrabacktest/events/event.hpp>

namespace ultrabacktest {

template <typename StrategyImpl>
class EventDispatcher {
public:
    explicit EventDispatcher(StrategyImpl* strategy) : strategy_(strategy) {}

    inline void dispatch(const Event& e) {
        switch (e.type) {
            case EventType::MarketTick:
                strategy_->on_tick(e.timestamp, e.symbol, e.tick);
                break;
            case EventType::Trade:
                strategy_->on_trade(e.timestamp, e.symbol, e.trade);
                break;
            case EventType::Quote:
                strategy_->on_quote(e.timestamp, e.symbol, e.quote);
                break;
            case EventType::OrderAdd:
                strategy_->on_order_add(e.timestamp, e.symbol, e.order_add);
                break;
            case EventType::OrderCancel:
                strategy_->on_order_cancel(e.timestamp, e.symbol, e.order_cancel);
                break;
            case EventType::OrderModify:
                strategy_->on_order_modify(e.timestamp, e.symbol, e.order_modify);
                break;
            case EventType::OrderExecution:
                strategy_->on_order_execution(e.timestamp, e.symbol, e.order_execution);
                break;
            case EventType::StrategyTimer:
                strategy_->on_timer(e.timestamp, e.timer);
                break;
            case EventType::SessionStart:
                strategy_->on_session_start(e.timestamp);
                break;
            case EventType::SessionEnd:
                strategy_->on_session_end(e.timestamp);
                break;
            case EventType::EndOfData:
            default:
                break;
        }
    }

private:
    StrategyImpl* strategy_;
};

} // namespace ultrabacktest
