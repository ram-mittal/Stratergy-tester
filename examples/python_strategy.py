import sys
import os

# Add the build directory to the Python path to find the compiled module
build_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'build', 'bindings', 'python'))
sys.path.append(build_dir)

import ultrabacktest_py as ub

class MyPythonStrategy(ub.Strategy):
    def __init__(self):
        super().__init__()
        self.trades_processed = 0

    def on_trade(self, ts: int, sym: int, trade: ub.TradeData):
        self.trades_processed += 1
        # Example of accessing properties
        # print(f"Trade received: {trade.qty} @ {trade.price}")

if __name__ == "__main__":
    print("Initializing Python Backtest...")
    
    config = ub.BacktestConfig()
    config.initial_cash = 1000000
    config.symbols = [1, 2, 3]

    strat = MyPythonStrategy()
    
    # We create a dummy binary file for the runner, but since we don't have one in the 
    # Python script context, we just print success if we can instantiate everything.
    runner = ub.BacktestRunner(config, strat)
    
    print(f"Instantiated BacktestRunner successfully! Python strategy bound to C++ CRTP.")
    print(f"Total PnL initialized at: {runner.total_pnl()}")
