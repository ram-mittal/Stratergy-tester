#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// UltraBacktest — Report Generator
//
// Formats performance metrics into console summaries or exports to CSV/JSON.
// ─────────────────────────────────────────────────────────────────────────────

#include <ultrabacktest/analytics/performance.hpp>
#include <fmt/core.h>
#include <fmt/color.h>
#include <string>

namespace ultrabacktest {

class ReportGenerator {
public:
    static std::string generate_console_summary(const PerformanceAnalytics& perf, Nanos duration_ns) {
        double ret_pct = perf.total_return() * 100.0;
        double ann_ret = perf.annualized_return(duration_ns) * 100.0;
        double dd_pct  = perf.max_drawdown_pct() * 100.0;
        double sharpe  = perf.sharpe_ratio(0.0);

        return fmt::format(
            "────────────────────────────────────────\n"
            "         UltraBacktest Results          \n"
            "────────────────────────────────────────\n"
            "Total Return:       {:.2f}%\n"
            "Annualized Return:  {:.2f}%\n"
            "Max Drawdown:       {:.2f}%\n"
            "Sharpe Ratio:       {:.2f}\n"
            "────────────────────────────────────────\n",
            ret_pct, ann_ret, dd_pct, sharpe
        );
    }
};

} // namespace ultrabacktest
