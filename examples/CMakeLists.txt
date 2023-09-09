cmake_minimum_required(VERSION 3.11...3.25)
project(fmm_example VERSION 1.0.0)

# include fast_matrix_market
add_subdirectory(fast_matrix_market)

add_executable(simple1 simple1.cpp)
target_link_libraries(simple1 fast_matrix_market::fast_matrix_market)
