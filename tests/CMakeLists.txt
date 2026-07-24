# ─────────────────────────────────────────────────────────────────────────────
# UltraBacktest — Tests
# ─────────────────────────────────────────────────────────────────────────────

# We use Google Test which is fetched in the root CMakeLists.txt

add_executable(ultrabacktest_test_event test_event.cpp)
target_link_libraries(ultrabacktest_test_event PRIVATE ultrabacktest gtest_main)
add_test(NAME test_event COMMAND ultrabacktest_test_event)

add_executable(ultrabacktest_test_exchange_sim test_exchange_simulator.cpp)
target_link_libraries(ultrabacktest_test_exchange_sim PRIVATE ultrabacktest gtest_main)
add_test(NAME test_exchange_sim COMMAND ultrabacktest_test_exchange_sim)

add_executable(ultrabacktest_test_portfolio test_portfolio.cpp)
target_link_libraries(ultrabacktest_test_portfolio PRIVATE ultrabacktest gtest_main)
add_test(NAME test_portfolio COMMAND ultrabacktest_test_portfolio)

add_executable(ultrabacktest_test_risk test_risk.cpp)
target_link_libraries(ultrabacktest_test_risk PRIVATE ultrabacktest gtest_main)
add_test(NAME test_risk COMMAND ultrabacktest_test_risk)
