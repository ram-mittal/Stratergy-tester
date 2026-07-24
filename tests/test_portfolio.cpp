#include <ultrabacktest/portfolio/portfolio.hpp>
#include <gtest/gtest.h>

using namespace ultrabacktest;

TEST(PortfolioTest, InitialState) {
    BacktestConfig config;
    config.initial_cash = from_double(100000.0);
    Portfolio p(config);
    
    EXPECT_EQ(p.cash(), config.initial_cash);
    EXPECT_EQ(p.total_pnl(), 0);
    EXPECT_EQ(p.get_position(1).net_qty, 0);
}

TEST(PortfolioTest, LongPositionPnL) {
    BacktestConfig config;
    Portfolio p(config);
    
    ExecutionReport fill{};
    fill.symbol = 1;
    fill.status = OrderStatus::Filled;
    fill.side = Side::Buy;
    fill.exec_qty = 10;
    fill.exec_price = from_double(100.0);
    
    p.on_execution(fill);
    
    const auto& pos = p.get_position(1);
    EXPECT_EQ(pos.net_qty, 10);
    EXPECT_EQ(pos.avg_entry_price, from_double(100.0));
    
    p.update_m2m(1, from_double(110.0));
    EXPECT_EQ(pos.unrealized_pnl, from_double(10.0 * 10)); // 100 profit
    EXPECT_EQ(p.total_pnl(), from_double(100.0));
    
    // Close position
    fill.side = Side::Sell;
    fill.exec_price = from_double(110.0);
    p.on_execution(fill);
    
    EXPECT_EQ(pos.net_qty, 0);
    EXPECT_EQ(pos.realized_pnl, from_double(100.0));
    EXPECT_EQ(pos.unrealized_pnl, 0);
    EXPECT_EQ(p.total_pnl(), from_double(100.0));
}

TEST(PortfolioTest, ShortPositionPnL) {
    BacktestConfig config;
    Portfolio p(config);
    
    ExecutionReport fill{};
    fill.symbol = 1;
    fill.status = OrderStatus::Filled;
    fill.side = Side::Sell;
    fill.exec_qty = 5;
    fill.exec_price = from_double(100.0);
    
    p.on_execution(fill);
    
    const auto& pos = p.get_position(1);
    EXPECT_EQ(pos.net_qty, -5);
    EXPECT_EQ(pos.avg_entry_price, from_double(100.0));
    
    p.update_m2m(1, from_double(90.0));
    EXPECT_EQ(pos.unrealized_pnl, from_double(10.0 * 5)); // 50 profit
    EXPECT_EQ(p.total_pnl(), from_double(50.0));
    
    // Close position
    fill.side = Side::Buy;
    fill.exec_price = from_double(90.0);
    p.on_execution(fill);
    
    EXPECT_EQ(pos.net_qty, 0);
    EXPECT_EQ(pos.realized_pnl, from_double(50.0));
    EXPECT_EQ(pos.unrealized_pnl, 0);
    EXPECT_EQ(p.total_pnl(), from_double(50.0));
}
