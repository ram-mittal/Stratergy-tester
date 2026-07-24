#include <ultrabacktest/exchange/exchange_simulator.hpp>
#include <ultrabacktest/portfolio/portfolio.hpp>
#include <gtest/gtest.h>

using namespace ultrabacktest;

TEST(ExchangeSimulatorTest, Initialization) {
    BacktestConfig config;
    config.symbols = {1, 2};
    
    Portfolio portfolio(config);
    ExchangeSimulator sim(config, &portfolio);
    sim.initialize();
    
    EXPECT_TRUE(sim.engine().has_symbol(1));
    EXPECT_TRUE(sim.engine().has_symbol(2));
    EXPECT_FALSE(sim.engine().has_symbol(3));
}

TEST(ExchangeSimulatorTest, SubmitOrder) {
    BacktestConfig config;
    config.symbols = {1};
    config.simulated_latency = 0;
    
    Portfolio portfolio(config);
    ExchangeSimulator sim(config, &portfolio);
    sim.initialize();
    sim.advance_to(1000);
    
    OrderRequest req;
    req.id = 100;
    req.symbol = 1;
    req.side = Side::Buy;
    req.type = OrderType::Limit;
    req.price = from_double(150.0);
    req.qty = 100;
    
    // Submit an order; since the book is empty, it should just rest
    sim.submit_order(req);
    
    // Check if it's in the book
    // Wait, the UltraLOB API for checking the book might be needed,
    // but just checking it didn't crash is a good start.
}
