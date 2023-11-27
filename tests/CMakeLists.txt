# FetchContent requires cmake >=3.11
cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

if (WIN32)
    # Need bigobj because some tests use lots of templates
    if (MSVC)
        add_compile_options("/bigobj")
    elseif(MINGW)
        # Also add -Os to reduce maximum filesize
        add_compile_options("-Wa,-mbig-obj" "-Os")
    endif()
else()
    if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        add_compile_options("-g" "-Wall" "-Wextra" "-pedantic")
    endif()
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

include(GoogleTest)

add_executable(chunking_test chunking_test.cpp)
target_link_libraries(chunking_test GTest::gtest_main fast_matrix_market::fast_matrix_market)
gtest_discover_tests(chunking_test)

add_executable(field_conv_test field_conv_test.cpp)
target_link_libraries(field_conv_test GTest::gtest_main fast_matrix_market::fast_matrix_market)
gtest_discover_tests(field_conv_test)

add_executable(basic_test basic_test.cpp)
target_link_libraries(basic_test GTest::gtest_main fast_matrix_market::fast_matrix_market)
if (NOT ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9))
    # GCC 8 has an interaction with GTest
    gtest_discover_tests(basic_test)
endif ()

add_executable(disabled_features_test disabled_features_test.cpp)
target_link_libraries(disabled_features_test GTest::gtest_main fast_matrix_market::fast_matrix_market)
target_compile_definitions(disabled_features_test PUBLIC FMM_NO_VECTOR)

add_executable(user_type_test user_type_test.cpp)
target_link_libraries(user_type_test GTest::gtest_main fast_matrix_market::fast_matrix_market)
gtest_discover_tests(user_type_test)

add_executable(array_test array_test.cpp)
target_link_libraries(array_test GTest::gtest_main fast_matrix_market::fast_matrix_market)
gtest_discover_tests(array_test)

add_executable(csc_test csc_test.cpp)
target_link_libraries(csc_test GTest::gtest_main fast_matrix_market::fast_matrix_market)
gtest_discover_tests(csc_test)

add_executable(triplet_test triplet_test.cpp)
target_link_libraries(triplet_test GTest::gtest_main fast_matrix_market::fast_matrix_market)
gtest_discover_tests(triplet_test)

add_executable(cxsparse_test cxsparse_test.cpp fake_cxsparse/cs.hpp)
target_link_libraries(cxsparse_test GTest::gtest_main fast_matrix_market::fast_matrix_market)
gtest_discover_tests(cxsparse_test)

if ("cxx_std_23" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
    include(stdfloat_tests.cmake)

    if (have_stdfloat)
        message("Including test for C++23 fixed-width floating point types")
        add_executable(cpp23_test cpp23_test.cpp)
        target_link_libraries(cpp23_test GTest::gtest_main fast_matrix_market::fast_matrix_market)
        target_compile_features(cpp23_test PUBLIC cxx_std_23)
        gtest_discover_tests(cpp23_test)
    endif()
endif()


option(FAST_MATRIX_MARKET_TEST_EXTERNAL_APPS "Enable tests for bindings (e.g. Blaze, Eigen, etc.)" ON)
if (FAST_MATRIX_MARKET_TEST_EXTERNAL_APPS)
    # Eigen
    include(../cmake/Eigen.cmake)
    find_package (Eigen3 3.4 REQUIRED NO_MODULE)

    add_executable(eigen_test eigen_test.cpp)
    target_link_libraries(eigen_test GTest::gtest_main fast_matrix_market::fast_matrix_market Eigen3::Eigen)

    gtest_discover_tests(eigen_test)

    # Blaze
    if(MINGW)
        # Blaze platform detection doesn't like the MinGW installed in tests.yml
        message("Skipping Blaze test on MinGW")
    else()
        set(BLAZE_BLAS_MODE OFF CACHE BOOL "" FORCE)  # No BLAS dependencies
        set(USE_LAPACK OFF CACHE BOOL "" FORCE)  # No LAPACK (and transitively, BLAS) dependencies
        include(../cmake/Blaze.cmake)
        FetchContent_GetProperties(blaze SOURCE_DIR BLAZE_INCLUDE_DIR)
        add_executable(blaze_test blaze_test.cpp)
        target_link_libraries(blaze_test GTest::gtest_main fast_matrix_market::fast_matrix_market)
        target_include_directories(blaze_test PUBLIC ${BLAZE_INCLUDE_DIR})

        gtest_discover_tests(blaze_test)
    endif()

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

    # Armadillo
    # Available since cmake 3.18: https://cmake.org/cmake/help/latest/module/FindArmadillo.html
    find_package(Armadillo)
    if (ARMADILLO_FOUND)
        message("Found Armadillo ${ARMADILLO_VERSION_STRING}")
        add_executable(armadillo_test armadillo_test.cpp)
        target_include_directories(armadillo_test PUBLIC ${ARMADILLO_INCLUDE_DIRS})
        target_link_libraries(armadillo_test GTest::gtest_main fast_matrix_market::fast_matrix_market)
        target_link_libraries(armadillo_test ${ARMADILLO_LIBRARIES})

        gtest_discover_tests(armadillo_test)
    else()
        message("Armadillo library not found. Skipping Armadillo binding tests.")
    endif()
endif()
