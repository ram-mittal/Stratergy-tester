#include <ultrabacktest/risk/risk_engine.hpp>
#include <gtest/gtest.h>

using namespace ultrabacktest;

TEST(RiskEngineTest, MaxOrderSize) {
    RiskLimits limits;
    limits.max_order_size = 100;
    
    BacktestConfig config;
    Portfolio port(config);
    RiskEngine risk(limits, &port);
    
    OrderRequest req;
    req.qty = 50;
    EXPECT_EQ(risk.check_pre_trade(req), RiskRejectReason::None);
    
    req.qty = 150;
    EXPECT_EQ(risk.check_pre_trade(req), RiskRejectReason::MaxOrderSize);
}

TEST(RiskEngineTest, MaxPosition) {
    RiskLimits limits;
    limits.max_position_per_symbol = 200;
    
    BacktestConfig config;
    Portfolio port(config);
    RiskEngine risk(limits, &port);
    
    // Simulate an existing long position of 150
    ExecutionReport fill{};
    fill.symbol = 1;
    fill.status = OrderStatus::Filled;
    fill.side = Side::Buy;
    fill.exec_qty = 150;
    fill.exec_price = from_double(100.0);
    port.on_execution(fill);
    
    OrderRequest req;
    req.symbol = 1;
    req.qty = 100;
    req.side = Side::Buy;
    
    // 150 + 100 = 250 > 200 (limit)
    EXPECT_EQ(risk.check_pre_trade(req), RiskRejectReason::MaxPosition);
    
    // 150 - 100 = 50 <= 200 (limit)
    req.side = Side::Sell;
    EXPECT_EQ(risk.check_pre_trade(req), RiskRejectReason::None);
}

TEST(RiskEngineTest, MaxLossKillSwitch) {
    RiskLimits limits;
    limits.max_loss = from_double(1000.0);
    
    BacktestConfig config;
    Portfolio port(config);
    RiskEngine risk(limits, &port);
    
    // Fake a loss
    ExecutionReport fill{};
    fill.symbol = 1;
    fill.status = OrderStatus::Filled;
    fill.side = Side::Buy;
    fill.exec_qty = 100;
    fill.exec_price = from_double(100.0);
    port.on_execution(fill);
    
    port.update_m2m(1, from_double(80.0)); // Loss of 20 * 100 = 2000
    
    OrderRequest req;
    req.qty = 10;
    
    EXPECT_EQ(risk.check_pre_trade(req), RiskRejectReason::MaxLoss);
    EXPECT_TRUE(risk.is_killed());
    
    // Subsequent checks should return KillSwitch
    EXPECT_EQ(risk.check_pre_trade(req), RiskRejectReason::KillSwitch);
}
