cmake_minimum_required(VERSION 3.19)
project(fast_matrix_market VERSION 1.0.0 LANGUAGES CXX)

# These options get switched on if compiler support demands it.
# You may force their use to override compiler-provided routines.
option(FMM_USE_FAST_FLOAT "Whether or not to use the fast_float library" OFF)
option(FMM_USE_DRAGONBOX "Whether or not to use the Dragonbox library" OFF)

# Main
add_library(fast_matrix_market INTERFACE)
# add alias so the project can be used with add_subdirectory
add_library(fast_matrix_market::fast_matrix_market ALIAS fast_matrix_market)

target_include_directories(
        fast_matrix_market INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_compile_features(fast_matrix_market INTERFACE cxx_std_17)

############################################
# Test for available versions of std::from_chars.
include(cmake/from_chars_tests.cmake)
include(cmake/to_chars_tests.cmake)

if (from_chars_int_supported)
    message("std::from_chars<int> detected.")
    target_compile_definitions(fast_matrix_market INTERFACE FMM_FROM_CHARS_INT_SUPPORTED)
else()
    message("std::from_chars<int> not detected.")
endif()

if (from_chars_double_supported)
    target_compile_definitions(fast_matrix_market INTERFACE FMM_FROM_CHARS_DOUBLE_SUPPORTED)
else()
    message("std::from_chars<double> not detected. Need fast_float.")
    set(FMM_USE_FAST_FLOAT ON)
endif()

if (from_chars_long_double_supported)
    target_compile_definitions(fast_matrix_market INTERFACE FMM_FROM_CHARS_LONG_DOUBLE_SUPPORTED)
else()
    message("std::from_chars<long double> not detected. Using std::strtold() fallback.")
endif()

###############################################
# Test for available versions of std::to_chars.

if (to_chars_int_supported)
    message("std::to_chars<int> detected.")
    target_compile_definitions(fast_matrix_market INTERFACE FMM_TO_CHARS_INT_SUPPORTED)
else()
    message("std::to_chars<int> not detected. Using std::to_string (write parallelism will suffer).")
endif()

if (to_chars_double_supported)
    message("std::to_chars<double> detected.")
    target_compile_definitions(fast_matrix_market INTERFACE FMM_TO_CHARS_DOUBLE_SUPPORTED)
else()
    message("std::to_chars<double> not detected. Need Dragonbox.")
    set(FMM_USE_DRAGONBOX ON)
endif()

if (to_chars_long_double_supported)
    message("std::to_chars<long double> detected.")
    target_compile_definitions(fast_matrix_market INTERFACE FMM_TO_CHARS_LONG_DOUBLE_SUPPORTED)
else()
    message("std::to_chars<long double> not detected. Using std::to_string (write parallelism will suffer).")
endif()

###############################################
# Get fast_float if it's required.
if (FMM_USE_FAST_FLOAT)
    message("Fetching fast_float.")

    # Add fast_float library
    include(cmake/fast_float.cmake)

    target_compile_definitions(fast_matrix_market INTERFACE FMM_USE_FAST_FLOAT)
    target_link_libraries(fast_matrix_market INTERFACE fast_float)
endif()

# Get Dragonbox if floating-point std::to_chars are not available.
if (FMM_USE_DRAGONBOX)
    message("Fetching Dragonbox.")

    include(cmake/Dragonbox.cmake)

    target_compile_definitions(fast_matrix_market INTERFACE FMM_USE_DRAGONBOX)
    target_link_libraries(fast_matrix_market INTERFACE dragonbox::dragonbox_to_chars)
endif()

###############################################

# Tests
option(FAST_MATRIX_MARKET_TEST "Enable tests" OFF)
if(FAST_MATRIX_MARKET_TEST)
    enable_testing()
    add_subdirectory(tests)
endif(FAST_MATRIX_MARKET_TEST)

# Benchmarks
option(FAST_MATRIX_MARKET_BENCH "Enable benchmarks" OFF)
if(FAST_MATRIX_MARKET_BENCH)
    add_subdirectory(benchmark)
endif(FAST_MATRIX_MARKET_BENCH)
