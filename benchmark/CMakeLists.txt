# FetchContent requires cmake >=3.11
cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

include(FetchContent)

# Add Google Benchmark
include(../cmake/GoogleBenchmark.cmake)

add_executable(fmm_bench bench_array.cpp bench_chunking.cpp bench_triplet.cpp main.cpp fmm_bench.hpp)
target_link_libraries(fmm_bench benchmark::benchmark fast_matrix_market::fast_matrix_market)
