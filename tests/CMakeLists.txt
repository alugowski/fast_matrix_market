# FetchContent requires cmake >=3.11
cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

include(FetchContent)

# Add Google Benchmark
include(../cmake/GoogleTest.cmake)

enable_testing()

add_executable(chunking_test chunking_test.cpp)
target_link_libraries(chunking_test GTest::gtest_main fast_matrix_market::fast_matrix_market)

include(GoogleTest)
gtest_discover_tests(chunking_test)
