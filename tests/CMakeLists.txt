# FetchContent requires cmake >=3.11
cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

include(FetchContent)

# Add Google Test
include(../cmake/GoogleTest.cmake)

enable_testing()

add_definitions(-DTEST_MATRIX_DIR="${CMAKE_CURRENT_SOURCE_DIR}/matrices/")

add_executable(chunking_test chunking_test.cpp)
target_link_libraries(chunking_test GTest::gtest_main fast_matrix_market::fast_matrix_market)

add_executable(invalid_test invalid_test.cpp)
target_link_libraries(invalid_test GTest::gtest_main fast_matrix_market::fast_matrix_market)

add_executable(cxsparse_test cxsparse_test.cpp fake_cxsparse/cs.hpp)
target_link_libraries(cxsparse_test GTest::gtest_main fast_matrix_market::fast_matrix_market)

# find Eigen
include(../cmake/Eigen.cmake)
find_package (Eigen3 3.4 REQUIRED NO_MODULE)

add_executable(eigen_test eigen_test.cpp)
target_link_libraries(eigen_test GTest::gtest_main fast_matrix_market::fast_matrix_market Eigen3::Eigen)

include(GoogleTest)
gtest_discover_tests(chunking_test)
gtest_discover_tests(invalid_test)
gtest_discover_tests(cxsparse_test)
gtest_discover_tests(eigen_test)
