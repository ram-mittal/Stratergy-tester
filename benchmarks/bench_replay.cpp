#include <benchmark/benchmark.h>
#include <ultrabacktest/engine/replay_engine.hpp>
#include <ultrabacktest/strategy/strategy.hpp>
#include <ultrabacktest/data/data_reader.hpp>
#include <vector>

using namespace ultrabacktest;

// A mock reader that iterates over an in-memory vector of events to isolate 
// the replay engine overhead from disk I/O.
class MemoryReader : public DataReader<MemoryReader> {
public:
    explicit MemoryReader(const std::vector<Event>& events) 
        : events_(events), idx_(0) {}

    bool do_open(const std::string&) { return true; }

    bool do_read_next(Event& out_event) {
        if (idx_ >= events_.size()) return false;
        out_event = events_[idx_++];
        return true;
    }

    bool do_has_next() const noexcept {
        return idx_ < events_.size();
    }

    void do_reset() {
        idx_ = 0;
    }

private:
    const std::vector<Event>& events_;
    size_t idx_;
};

// A dummy strategy that does nothing but count ticks
class DummyStrategy : public Strategy<DummyStrategy> {
public:
    using Strategy::Strategy;

    void on_trade(Nanos ts, Symbol sym, const TradeData& trade) {
        benchmark::DoNotOptimize(ts);
        benchmark::DoNotOptimize(trade.price);
    }
};

static void BM_ReplayEngine_Overhead(benchmark::State& state) {
    // Generate some dummy events
    const size_t num_events = state.range(0);
    std::vector<Event> events;
    events.reserve(num_events);
    for (size_t i = 0; i < num_events; ++i) {
        events.push_back(Event::make_trade(1000 + i, 1, from_double(150.0), 100, Side::Buy));
    }

    BacktestConfig config;
    Portfolio port(config);
    ExchangeSimulator sim(config, &port);
    DummyStrategy strategy(&sim, &port);

    for (auto _ : state) {
        MemoryReader reader(events);
        ReplayEngine<MemoryReader, DummyStrategy> engine(reader, strategy, config);
        
        engine.run();
        
        benchmark::DoNotOptimize(engine.events_processed());
    }
    
    state.SetItemsProcessed(state.iterations() * num_events);
}

// Benchmark 10M events to see the millions-of-events-per-second (MEPS) throughput
BENCHMARK(BM_ReplayEngine_Overhead)->Arg(10'000'000);

BENCHMARK_MAIN();
