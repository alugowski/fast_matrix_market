cmake_minimum_required(VERSION 3.19)
project(fast_matrix_market VERSION 1.0.0 LANGUAGES CXX)
SET(CMAKE_CXX_STANDARD 17)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-g -O0")

# Main
add_library(fast_matrix_market INTERFACE include/fast_matrix_market/fast_matrix_market.hpp include/fast_matrix_market/sequential_read.hpp include/fast_matrix_market/interface_triplet.hpp)
# add alias so the project can be used with add_subdirectory
add_library(fast_matrix_market::fast_matrix_market ALIAS fast_matrix_market)

target_include_directories(
        fast_matrix_market INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_compile_features(fast_matrix_market INTERFACE cxx_std_17)

# Test for float version of std::from_chars.
include(cmake/from_chars_double.cmake)

# Get fast_float if it's required.
if (NOT float_from_chars_supported)
    message("Fetching fast_float.")

    include(FetchContent)

    # Add fast_float library
    include(cmake/fast_float.cmake)

    target_link_libraries(fast_matrix_market INTERFACE fast_float)
endif()

# test driver
add_executable(driver include/fast_matrix_market/driver.cpp)
target_link_libraries(driver PUBLIC fast_matrix_market::fast_matrix_market)

# enable OpenMP
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    message("OpenMP found")
    add_definitions(-DENABLE_OPENMP)
    target_link_libraries(fast_matrix_market INTERFACE OpenMP::OpenMP_CXX)
else()
    message("OpenMP not found, parallel benchmarks disabled.")
endif()
