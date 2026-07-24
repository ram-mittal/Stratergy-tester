#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// UltraBacktest — Replay Engine
//
// The core tick-by-tick simulation loop.
// Templated on DataReader and Strategy to allow full inlining of the hot path.
// ─────────────────────────────────────────────────────────────────────────────

#include <ultrabacktest/common/config.hpp>
#include <ultrabacktest/engine/event_dispatcher.hpp>
#include <ultrabacktest/data/data_reader.hpp>

// Compiler-specific prefetch for hiding memory latency
#if defined(__GNUC__) || defined(__clang__)
#define ULTRA_PREFETCH(ptr) __builtin_prefetch(ptr, 0, 0) // read, temporal locality 0
#else
#define ULTRA_PREFETCH(ptr)
#endif

namespace ultrabacktest {

template <typename Reader, typename Strategy>
class ReplayEngine {
public:
    ReplayEngine(Reader& reader, Strategy& strategy, const BacktestConfig& config)
        : reader_(reader)
        , dispatcher_(&strategy)
        , config_(config)
        , events_processed_(0)
    {}

    /// Run the backtest simulation loop
    void run() {
        Event current_event;
        Event next_event;
        
        bool has_current = reader_.read_next(current_event);
        bool has_next = false;
        
        if (has_current) {
            has_next = reader_.read_next(next_event);
        }

        while (has_current) {
            // Prefetch next event into cache while processing current
            if (has_next) {
                ULTRA_PREFETCH(&next_event);
            }

            // Time filtering
            if (config_.start_time != 0 && current_event.timestamp < config_.start_time) {
                goto advance;
            }
            if (config_.end_time != 0 && current_event.timestamp > config_.end_time) {
                break; // done
            }

            // TODO: Exchange Simulator hook
            // exchange_sim_.advance_to(current_event.timestamp);

            // Dispatch event directly to strategy callback (fully inlined)
            dispatcher_.dispatch(current_event);
            
            ++events_processed_;

        advance:
            current_event = next_event;
            has_current = has_next;
            if (has_next) {
                has_next = reader_.read_next(next_event);
            }
        }
    }
    
    [[nodiscard]] size_t events_processed() const noexcept { 
        return events_processed_; 
    }

private:
    Reader& reader_;
    EventDispatcher<Strategy> dispatcher_;
    const BacktestConfig& config_;
    size_t events_processed_;
};

} // namespace ultrabacktest
