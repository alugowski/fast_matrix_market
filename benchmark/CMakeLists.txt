# FetchContent requires cmake >=3.11
# see https://cmake.org/cmake/help/v3.11/module/FetchContent.html
cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

include(FetchContent)

# Add Google Benchmark
include(../cmake/GoogleBenchmark.cmake)

add_executable(fmm_bench bench_sequential_triplet.cpp main.cpp fmm_bench.hpp)
target_link_libraries(fmm_bench benchmark::benchmark fast_matrix_market::fast_matrix_market)
