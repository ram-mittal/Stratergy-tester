#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <ultrabacktest/common/types.hpp>
#include <ultrabacktest/common/config.hpp>
#include <ultrabacktest/events/event.hpp>
#include <ultrabacktest/engine/replay_engine.hpp>
#include <ultrabacktest/strategy/strategy.hpp>
#include <ultrabacktest/data/binary_reader.hpp>
#include <ultrabacktest/portfolio/portfolio.hpp>

namespace py = pybind11;
using namespace ultrabacktest;

// ─────────────────────────────────────────────────────────────────────────────
// Strategy Bridge (Virtualizes CRTP for Python)
// ─────────────────────────────────────────────────────────────────────────────

// 1. A virtual base class that Python users will inherit from
class PyStrategyBase {
public:
    virtual ~PyStrategyBase() = default;

    virtual void on_trade(Nanos ts, Symbol sym, const TradeData& trade) {}
    virtual void on_tick(Nanos ts, Symbol sym, const TickData& tick) {}
};

// 2. Pybind11 trampoline to allow Python subclasses to override C++ virtual methods
class PyStrategyTrampoline : public PyStrategyBase {
public:
    using PyStrategyBase::PyStrategyBase;

    void on_trade(Nanos ts, Symbol sym, const TradeData& trade) override {
        PYBIND11_OVERRIDE(void, PyStrategyBase, on_trade, ts, sym, trade);
    }
    
    void on_tick(Nanos ts, Symbol sym, const TickData& tick) override {
        PYBIND11_OVERRIDE(void, PyStrategyBase, on_tick, ts, sym, tick);
    }
};

// 3. The CRTP Strategy that the C++ Engine uses, which delegates to the Python object
class CppBridgeStrategy : public Strategy<CppBridgeStrategy> {
public:
    CppBridgeStrategy(ExchangeSimulator* sim, Portfolio* port, PyStrategyBase* py_strat)
        : Strategy<CppBridgeStrategy>(sim, port), py_strat_(py_strat) {}

    void on_trade(Nanos ts, Symbol sym, const TradeData& trade) {
        if (py_strat_) py_strat_->on_trade(ts, sym, trade);
    }
    
    void on_tick(Nanos ts, Symbol sym, const TickData& tick) {
        if (py_strat_) py_strat_->on_tick(ts, sym, tick);
    }

private:
    PyStrategyBase* py_strat_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Engine Runner (Simplifies the templated ReplayEngine for Python)
// ─────────────────────────────────────────────────────────────────────────────
class PythonBacktestRunner {
public:
    PythonBacktestRunner(const BacktestConfig& config, PyStrategyBase* py_strat)
        : config_(config), port_(config), sim_(config, &port_), 
          strat_(&sim_, &port_, py_strat) 
    {}

    void run(const std::string& data_file) {
        BinaryReader reader;
        if (!reader.open(data_file)) {
            throw std::runtime_error("Failed to open data file");
        }
        
        sim_.initialize();
        
        ReplayEngine<BinaryReader, CppBridgeStrategy> engine(reader, strat_, config_);
        engine.run();
    }
    
    double total_pnl() const { return to_double(port_.total_pnl()); }

private:
    BacktestConfig config_;
    Portfolio port_;
    ExchangeSimulator sim_;
    CppBridgeStrategy strat_;
};


// ─────────────────────────────────────────────────────────────────────────────
// Module Definition
// ─────────────────────────────────────────────────────────────────────────────
PYBIND11_MODULE(ultrabacktest_py, m) {
    m.doc() = "UltraBacktest Python Bindings";

    // Common Types
    py::enum_<Side>(m, "Side")
        .value("Buy", Side::Buy)
        .value("Sell", Side::Sell)
        .export_values();
        
    py::class_<TradeData>(m, "TradeData")
        .def_readonly("price", &TradeData::price)
        .def_readonly("qty", &TradeData::qty)
        .def_readonly("aggressor_side", &TradeData::aggressor_side);
        
    py::class_<TickData>(m, "TickData")
        .def_readonly("best_bid", &TickData::best_bid)
        .def_readonly("best_ask", &TickData::best_ask)
        .def_readonly("bid_qty", &TickData::bid_qty)
        .def_readonly("ask_qty", &TickData::ask_qty);

    // Config
    py::class_<BacktestConfig>(m, "BacktestConfig")
        .def(py::init<>())
        .def_readwrite("initial_cash", &BacktestConfig::initial_cash)
        .def_readwrite("symbols", &BacktestConfig::symbols);

    // Strategy Base
    py::class_<PyStrategyBase, PyStrategyTrampoline>(m, "Strategy")
        .def(py::init<>())
        .def("on_trade", &PyStrategyBase::on_trade)
        .def("on_tick", &PyStrategyBase::on_tick);

    // Runner
    py::class_<PythonBacktestRunner>(m, "BacktestRunner")
        .def(py::init<const BacktestConfig&, PyStrategyBase*>())
        .def("run", &PythonBacktestRunner::run)
        .def("total_pnl", &PythonBacktestRunner::total_pnl);
}
