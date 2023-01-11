# FetchContent requires cmake >=3.11
cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Wextra -pedantic")
elseif (MSVC)
    add_compile_options(/bigobj)
endif ()

include(FetchContent)

# Add Google Test
include(../cmake/GoogleTest.cmake)

enable_testing()

set(MATRIX_DIR "${CMAKE_CURRENT_LIST_DIR}/matrices/")
message("Test Matrix directory: ${MATRIX_DIR}")
add_definitions(-DTEST_MATRIX_DIR="${MATRIX_DIR}")

add_executable(chunking_test chunking_test.cpp)
target_link_libraries(chunking_test GTest::gtest_main fast_matrix_market::fast_matrix_market)

add_executable(basic_test basic_test.cpp)
target_link_libraries(basic_test GTest::gtest_main fast_matrix_market::fast_matrix_market)

add_executable(array_test array_test.cpp)
target_link_libraries(array_test GTest::gtest_main fast_matrix_market::fast_matrix_market)

add_executable(triplet_test triplet_test.cpp)
target_link_libraries(triplet_test GTest::gtest_main fast_matrix_market::fast_matrix_market)

add_executable(cxsparse_test cxsparse_test.cpp fake_cxsparse/cs.hpp)
target_link_libraries(cxsparse_test GTest::gtest_main fast_matrix_market::fast_matrix_market)

# find Eigen
include(../cmake/Eigen.cmake)
find_package (Eigen3 3.4 REQUIRED NO_MODULE)

add_executable(eigen_test eigen_test.cpp)
target_link_libraries(eigen_test GTest::gtest_main fast_matrix_market::fast_matrix_market Eigen3::Eigen)

include(GoogleTest)
gtest_discover_tests(chunking_test)
gtest_discover_tests(basic_test)
gtest_discover_tests(array_test)
gtest_discover_tests(triplet_test)
gtest_discover_tests(cxsparse_test)
gtest_discover_tests(eigen_test)
