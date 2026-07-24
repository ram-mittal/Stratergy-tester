#include <ultrabacktest/exchange/exchange_simulator.hpp>
#include <ultrabacktest/portfolio/portfolio.hpp>

namespace ultrabacktest {

void ExchangeSimulator::initialize() {
    // Register all symbols from the config with the underlying matching engine.
    // We pass `this` as the context pointer so the static C callback can route
    // back to this simulator instance.
    for (Symbol sym : config_.symbols) {
        engine_.add_symbol(sym, &ExchangeSimulator::on_execution_cb, this);
    }
}

void ExchangeSimulator::submit_order(const OrderRequest& req) {
    OrderRequest adjusted_req = req;
    
    // Stamp with current simulated time if not provided by strategy
    if (adjusted_req.timestamp == 0) {
        adjusted_req.timestamp = current_time_;
    }
    
    // Apply simulated network/processing latency
    adjusted_req.timestamp += config_.simulated_latency;
    
    // Submit synchronously to the matching engine.
    // This maintains deterministic ordering within the same tick.
    engine_.submit(adjusted_req);
}

bool ExchangeSimulator::cancel_order(Symbol sym, OrderId id) {
    return engine_.cancel(sym, id);
}

bool ExchangeSimulator::modify_order(Symbol sym, const ModifyRequest& req) {
    return engine_.modify(sym, req);
}

void ExchangeSimulator::on_execution_cb(const ExecutionReport& rpt, void* ctx) {
    auto* self = static_cast<ExchangeSimulator*>(ctx);
    self->handle_execution(rpt);
}

void ExchangeSimulator::handle_execution(const ExecutionReport& rpt) {
    // 1. Update Portfolio state (positions, PnL, cash)
    if (portfolio_) {
        portfolio_->on_execution(rpt);
    }
    
    // 2. The strategy receives the execution report via the ReplayEngine's
    //    event dispatcher (we would enqueue an ExecutionReport event here
    //    or call a callback, but for v1 we let the Strategy pull it or
    //    we can add a direct callback).
    // For now, the portfolio state is synchronously updated.
}

} // namespace ultrabacktest
