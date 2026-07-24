#include <ultrabacktest/analytics/performance.hpp>
#include <cmath>

namespace ultrabacktest {

void PerformanceAnalytics::update(Nanos ts, const Portfolio& port) {
    const double equity = to_double(port.total_equity());
    
    if (initial_equity_ < 0.0) {
        initial_equity_ = equity;
        last_equity_ = equity;
        peak_equity_ = equity;
        return;
    }

    // 1. Drawdown calculation
    if (equity > peak_equity_) {
        peak_equity_ = equity;
    } else {
        const double dd = peak_equity_ - equity;
        const double dd_pct = dd / peak_equity_;
        if (dd > max_dd_) max_dd_ = dd;
        if (dd_pct > max_dd_pct_) max_dd_pct_ = dd_pct;
    }

    // 2. Return statistics (Welford's online algorithm for variance)
    if (last_equity_ > 0.0) {
        const double ret = (equity - last_equity_) / last_equity_;
        count_++;
        const double delta = ret - mean_return_;
        mean_return_ += delta / count_;
        const double delta2 = ret - mean_return_;
        m2_ += delta * delta2;
    }

    last_equity_ = equity;
}

double PerformanceAnalytics::sharpe_ratio(double risk_free_rate) const noexcept {
    if (count_ < 2 || m2_ == 0.0) return 0.0;
    const double variance = m2_ / (count_ - 1);
    const double stddev = std::sqrt(variance);
    // Note: Returns Sharpe for the update period frequency.
    // Annualization requires multiplying by sqrt(periods_per_year).
    return (mean_return_ - risk_free_rate) / stddev;
}

double PerformanceAnalytics::total_return() const noexcept {
    if (initial_equity_ <= 0.0) return 0.0;
    return (last_equity_ - initial_equity_) / initial_equity_;
}

double PerformanceAnalytics::annualized_return(Nanos duration_ns) const noexcept {
    if (duration_ns == 0 || initial_equity_ <= 0.0) return 0.0;
    const double years = static_cast<double>(duration_ns) / (365.25 * 24.0 * 60.0 * 60.0 * 1e9);
    if (years <= 0.0) return 0.0;
    return std::pow(last_equity_ / initial_equity_, 1.0 / years) - 1.0;
}

} // namespace ultrabacktest
