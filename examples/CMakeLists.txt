cmake_minimum_required(VERSION 3.11...3.25)
project(fmm_example VERSION 1.0.0)

# include fast_matrix_market
add_subdirectory(fast_matrix_market)

# simple example
add_executable(simple1 simple1.cpp)
target_link_libraries(simple1 fast_matrix_market::fast_matrix_market)

# matrix market generator
add_executable(generate_matrix_market generate_matrix_market.cpp)
target_link_libraries(generate_matrix_market fast_matrix_market::fast_matrix_market)

# file sorter
add_executable(sort_matrix_market sort_matrix_market.cpp)
target_link_libraries(sort_matrix_market fast_matrix_market::fast_matrix_market)
