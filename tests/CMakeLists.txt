# FetchContent requires cmake >=3.11
cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Wextra -pedantic")
elseif (MSVC)
    # MSVC needs this flag if using lots of templates
    add_compile_options("/bigobj")
endif()

# Code Coverage
option(FAST_MATRIX_MARKET_TEST_COVERAGE "Code-coverage" OFF)
if(FAST_MATRIX_MARKET_TEST_COVERAGE)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        add_compile_options("--coverage")
        add_link_options("-lgcov" "--coverage")
    endif()
endif()

include(FetchContent)

# Add Google Test
include(../cmake/GoogleTest.cmake)

enable_testing()

set(MATRIX_DIR "${CMAKE_CURRENT_LIST_DIR}/matrices/")
message("Test Matrix directory: ${MATRIX_DIR}")
add_definitions(-DTEST_MATRIX_DIR="${MATRIX_DIR}")

add_executable(chunking_test chunking_test.cpp)
target_link_libraries(chunking_test GTest::gtest_main fast_matrix_market::fast_matrix_market)

add_executable(field_conv_test field_conv_test.cpp)
target_link_libraries(field_conv_test GTest::gtest_main fast_matrix_market::fast_matrix_market)

add_executable(basic_test basic_test.cpp)
target_link_libraries(basic_test GTest::gtest_main fast_matrix_market::fast_matrix_market)

add_executable(user_type_test user_type_test.cpp)
target_link_libraries(user_type_test GTest::gtest_main fast_matrix_market::fast_matrix_market)

add_executable(array_test array_test.cpp)
target_link_libraries(array_test GTest::gtest_main fast_matrix_market::fast_matrix_market)

add_executable(triplet_test triplet_test.cpp)
target_link_libraries(triplet_test GTest::gtest_main fast_matrix_market::fast_matrix_market)

add_executable(cxsparse_test cxsparse_test.cpp fake_cxsparse/cs.hpp)
target_link_libraries(cxsparse_test GTest::gtest_main fast_matrix_market::fast_matrix_market)

include(GoogleTest)
gtest_discover_tests(chunking_test)
gtest_discover_tests(field_conv_test)
gtest_discover_tests(basic_test)
gtest_discover_tests(user_type_test)
gtest_discover_tests(array_test)
gtest_discover_tests(triplet_test)
gtest_discover_tests(cxsparse_test)


option(FAST_MATRIX_MARKET_TEST_EXTERNAL_APPS "Enable tests for bindings (e.g. Blaze, Eigen, etc.)" ON)
if (FAST_MATRIX_MARKET_TEST_EXTERNAL_APPS)
    # Eigen
    include(../cmake/Eigen.cmake)
    find_package (Eigen3 3.4 REQUIRED NO_MODULE)

    add_executable(eigen_test eigen_test.cpp)
    target_link_libraries(eigen_test GTest::gtest_main fast_matrix_market::fast_matrix_market Eigen3::Eigen)

    gtest_discover_tests(eigen_test)

    # Blaze
    set(BLAZE_BLAS_MODE OFF CACHE BOOL "" FORCE)  # No BLAS dependencies
    set(USE_LAPACK OFF CACHE BOOL "" FORCE)  # No LAPACK (and transitively, BLAS) dependencies
    include(../cmake/Blaze.cmake)
    FetchContent_GetProperties(blaze SOURCE_DIR BLAZE_INCLUDE_DIR)
    add_executable(blaze_test blaze_test.cpp)
    target_link_libraries(blaze_test GTest::gtest_main fast_matrix_market::fast_matrix_market)
    target_include_directories(blaze_test PUBLIC ${BLAZE_INCLUDE_DIR})

    gtest_discover_tests(blaze_test)

    # GraphBLAS
    option(FAST_MATRIX_MARKET_TEST_GRAPHBLAS "Enable tests for GraphBLAS. Requires GraphBLAS installed on system such that find_package will find it." ON)
    if (FAST_MATRIX_MARKET_TEST_GRAPHBLAS)
        include(../cmake/GraphBLAS.cmake)

        if (GraphBLAS_FOUND)
            message("GraphBLAS found.")
            message("GRAPHBLAS_INCLUDE_DIR: ${GRAPHBLAS_INCLUDE_DIR}")
            message("GRAPHBLAS_LIBRARIES: ${GRAPHBLAS_LIBRARIES}")
            message("GRAPHBLAS_STATIC: ${GRAPHBLAS_STATIC}")
            message("GRAPHBLAS_LIBRARY: ${GRAPHBLAS_LIBRARY}")

            add_executable(graphblas_test graphblas_test.cpp)

            if (NOT ("${GRAPHBLAS_INCLUDE_DIR}" STREQUAL "" ))
                target_include_directories(graphblas_test PUBLIC ${GRAPHBLAS_INCLUDE_DIR})
            endif()
            target_link_libraries(graphblas_test GTest::gtest_main fast_matrix_market::fast_matrix_market ${GRAPHBLAS_LIBRARIES})

            option(FMM_NO_GXB "Disable GxB_* extensions" OFF)
            if (FMM_NO_GXB)
                target_compile_definitions(graphblas_test PUBLIC FMM_NO_GXB)
            endif()

            gtest_discover_tests(graphblas_test)
        else()
            message("GraphBLAS not found, skipping GraphBLAS tests.")
        endif()
    endif()
endif()
