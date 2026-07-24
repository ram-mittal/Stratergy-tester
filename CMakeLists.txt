# ─────────────────────────────────────────────────────────────────────────────
# UltraBacktest — Production-Quality Tick-Level Backtesting Engine
#
# Built on UltraLOB (matching engine, order book, memory pools).
# Designed for low-latency, cache-efficient, deterministic replay of
# hundreds of millions of historical market events.
# ─────────────────────────────────────────────────────────────────────────────

cmake_minimum_required(VERSION 3.28)

project(UltraBacktest
    VERSION   0.1.0
    LANGUAGES CXX C
)

# ─────────────────────────────────────────────────────────────────────────────
# C++20, strict, no extensions
# ─────────────────────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ─────────────────────────────────────────────────────────────────────────────
# Compiler flags — tuned for throughput and cache efficiency
#
# Rationale:
#   -O3              : aggressive inlining, loop vectorisation
#   -mavx2 -mfma     : SIMD for batch event processing, statistics
#   -fno-rtti        : save vtable pointer overhead (we use CRTP, not virtual)
#   -march=native    : tune for the build machine (drop for portable builds)
# ─────────────────────────────────────────────────────────────────────────────
if(MSVC)
    add_compile_options(
        /W4
        /O2 /Oi /Ot
        /arch:AVX2
        /EHsc
        /D_CRT_SECURE_NO_WARNINGS
        /DNOMINMAX
        /wd4100   # unused parameter
        /wd4702   # unreachable code
        /wd4127   # conditional expression is constant
    )
else()
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -O3
        -mavx2 -mfma
        -Wno-unused-parameter
    )
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(
            -Wno-missing-field-initializers
            -include stdlib.h
        )
    endif()
endif()

# ─────────────────────────────────────────────────────────────────────────────
# Options
# ─────────────────────────────────────────────────────────────────────────────
option(ULTRABACKTEST_BUILD_TESTS     "Build unit tests"          ON)
option(ULTRABACKTEST_BUILD_BENCHMARKS "Build benchmark suite"    ON)
option(ULTRABACKTEST_BUILD_EXAMPLES  "Build example strategies"  ON)
option(ULTRABACKTEST_PROFILING       "Enable trace profiling"    OFF)

# ─────────────────────────────────────────────────────────────────────────────
# Dependencies via FetchContent
# ─────────────────────────────────────────────────────────────────────────────
include(FetchContent)

# ── UltraLOB ─────────────────────────────────────────────────────────────────
# Prefer local checkout for development; fall back to GitHub for CI.
#
# The local path expects UltraLOB at ../system (sibling directory).
# Override with -DULTRALOB_SOURCE_DIR=/path/to/UltraLOB
# ─────────────────────────────────────────────────────────────────────────────
set(ULTRALOB_SOURCE_DIR "" CACHE PATH "Path to local UltraLOB source tree")

if(ULTRALOB_SOURCE_DIR AND EXISTS "${ULTRALOB_SOURCE_DIR}/CMakeLists.txt")
    message(STATUS "UltraLOB: using local source at ${ULTRALOB_SOURCE_DIR}")
    add_subdirectory("${ULTRALOB_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/ultralob" EXCLUDE_FROM_ALL)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../system/CMakeLists.txt")
    message(STATUS "UltraLOB: using sibling directory ../system")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../system" "${CMAKE_BINARY_DIR}/ultralob" EXCLUDE_FROM_ALL)
else()
    message(STATUS "UltraLOB: fetching from GitHub")
    FetchContent_Declare(
        ultralob
        GIT_REPOSITORY https://github.com/ram-mittal/UltraLOB.git
        GIT_TAG        main
    )
    FetchContent_MakeAvailable(ultralob)
endif()

# ── fmt ──────────────────────────────────────────────────────────────────────
FetchContent_Declare(
    fmt
    URL https://github.com/fmtlib/fmt/archive/refs/tags/11.1.4.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(fmt)

# ── spdlog (header-only mode, uses our fmt) ──────────────────────────────────
FetchContent_Declare(
    spdlog
    URL https://github.com/gabime/spdlog/archive/refs/tags/v1.15.3.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(spdlog)

# ── CLI11 ────────────────────────────────────────────────────────────────────
FetchContent_Declare(
    cli11
    URL https://github.com/CLIUtils/CLI11/archive/refs/tags/v2.4.2.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(CLI11_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CLI11_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(cli11)

# ── Google Test ──────────────────────────────────────────────────────────────
if(ULTRABACKTEST_BUILD_TESTS)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)
    FetchContent_MakeAvailable(googletest)
endif()

# ── Google Benchmark ─────────────────────────────────────────────────────────
if(ULTRABACKTEST_BUILD_BENCHMARKS)
    FetchContent_Declare(
        benchmark
        URL https://github.com/google/benchmark/archive/refs/tags/v1.9.1.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(BENCHMARK_ENABLE_TESTING   OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL   OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_EXCEPTIONS ON  CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(benchmark)
endif()

# ─────────────────────────────────────────────────────────────────────────────
# UltraBacktest static library
# ─────────────────────────────────────────────────────────────────────────────
add_library(ultrabacktest STATIC
    src/replay_engine.cpp
    src/exchange_simulator.cpp
    src/portfolio.cpp
    src/risk_engine.cpp
    src/performance.cpp
)

target_include_directories(ultrabacktest PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(ultrabacktest PUBLIC
    ultralob
    fmt::fmt
    spdlog::spdlog
)

target_compile_definitions(ultrabacktest PUBLIC
    ULTRABACKTEST_VERSION="${PROJECT_VERSION}"
)

if(ULTRABACKTEST_PROFILING)
    target_compile_definitions(ultrabacktest PUBLIC ULTRABACKTEST_PROFILING=1)
endif()

if(WIN32)
    target_compile_definitions(ultrabacktest PUBLIC WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0601)
endif()

# ─────────────────────────────────────────────────────────────────────────────
# Subdirectories
# ─────────────────────────────────────────────────────────────────────────────
if(ULTRABACKTEST_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(ULTRABACKTEST_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

if(ULTRABACKTEST_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Python bindings
option(ULTRABACKTEST_BUILD_PYTHON "Build Python bindings via pybind11" ON)
if(ULTRABACKTEST_BUILD_PYTHON)
    add_subdirectory(bindings/python)
endif()
