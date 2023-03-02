# FetchContent requires cmake >=3.11
cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Wextra -pedantic")
    set(CMAKE_CXX_FLAGS_RELEASE "-g -O3 -DNDEBUG")
elseif (MSVC)
    add_compile_options(/bigobj)
endif ()

include(FetchContent)

# Add Google Benchmark
include(../cmake/GoogleBenchmark.cmake)

add_executable(fmm_bench bench_chunking.cpp bench_array.cpp bench_iostream.cpp bench_triplet.cpp main.cpp fmm_bench.hpp)
target_link_libraries(fmm_bench benchmark::benchmark fast_matrix_market::fast_matrix_market)
