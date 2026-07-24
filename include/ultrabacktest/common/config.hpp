#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// UltraBacktest — Configuration
//
// All configuration for a backtest run is captured in BacktestConfig.
// Designed as a POD-like struct that can be serialized to/from JSON or CLI.
//
// Design decisions:
//   • Strings for file paths only — everything else is numeric/enum.
//   • Default values are sensible for equity backtesting.
//   • FeeModel and SlippageConfig are value types (no heap allocation).
// ─────────────────────────────────────────────────────────────────────────────

#include "types.hpp"

#include <string>
#include <vector>

namespace ultrabacktest {

// ─────────────────────────────────────────────────────────────────────────────
// Data format enumeration
// ─────────────────────────────────────────────────────────────────────────────

enum class DataFormat : uint8_t {
    CSV         = 0,
    BinaryTick  = 1,
    CompressedBinary = 2,
    MemoryMapped     = 3,
};

[[nodiscard]] inline constexpr std::string_view to_string(DataFormat f) noexcept {
    switch (f) {
        case DataFormat::CSV:              return "CSV";
        case DataFormat::BinaryTick:       return "BinaryTick";
        case DataFormat::CompressedBinary: return "CompressedBinary";
        case DataFormat::MemoryMapped:     return "MemoryMapped";
    }
    return "Unknown";
}

// ─────────────────────────────────────────────────────────────────────────────
// Replay speed mode
// ─────────────────────────────────────────────────────────────────────────────

enum class ReplaySpeed : uint8_t {
    MaxSpeed  = 0,   // as fast as possible (benchmarking mode)
    RealTime  = 1,   // 1x real-time (simulated wallclock)
    Scaled    = 2,   // N× real-time
    StepByStep = 3,  // manual advance (debugging)
};

// ─────────────────────────────────────────────────────────────────────────────
// Backtest configuration
// ─────────────────────────────────────────────────────────────────────────────

struct BacktestConfig {
    // ── Data ────────────────────────────────────────────────────────────────
    std::vector<std::string> data_paths;        // tick data files
    DataFormat               data_format = DataFormat::CSV;
    std::vector<Symbol>      symbols;           // instruments to replay

    // ── Time range ──────────────────────────────────────────────────────────
    Nanos start_time = 0;                       // 0 = beginning of data
    Nanos end_time   = 0;                       // 0 = end of data

    // ── Replay ──────────────────────────────────────────────────────────────
    ReplaySpeed replay_speed     = ReplaySpeed::MaxSpeed;
    double      speed_multiplier = 1.0;         // used when ReplaySpeed::Scaled

    // ── Exchange simulation ─────────────────────────────────────────────────
    Nanos   simulated_latency  = 0;             // artificial order-to-fill delay (ns)
    size_t  order_pool_capacity = 1'000'000;    // UltraLOB OrderPool size

    // ── Fees and slippage ───────────────────────────────────────────────────
    FeeModel      fee_model;
    SlippageConfig slippage_config;

    // ── Portfolio ───────────────────────────────────────────────────────────
    Price initial_cash = 1'000'000 * PRICE_SCALE;  // $1M default

    // ── Output ──────────────────────────────────────────────────────────────
    std::string output_dir;                     // results output directory
    bool        enable_profiling = false;       // chrome trace output
    std::string trace_output;                   // trace file path

    // ── Logging ─────────────────────────────────────────────────────────────
    std::string log_level = "info";             // spdlog level
};

} // namespace ultrabacktest
