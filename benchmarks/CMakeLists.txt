add_executable(bench_replay bench_replay.cpp)
target_link_libraries(bench_replay PRIVATE ultrabacktest benchmark::benchmark)
