# Attempt at a general-purpose GraphBLAS locator.
#
# Known to work on:
# * macOS with `brew install suite-sparse` (laptop and GitHub Actions macos)
# * Ubuntu with `sudo apt-get install -y libsuitesparse-dev` (GitHub Actions)
#

# Attempt to use FindGraphBLAS
cmake_policy(SET CMP0074 NEW)
find_package(GraphBLAS QUIET MODULE)

# See if GraphBLAS is installed system-wide already
if (NOT GraphBLAS_FOUND)
    check_cxx_source_compiles("
    #include <GraphBLAS.h>
    int main(void) { return 0; }
    " GRAPHBLAS_H_AVAILABLE)

    if (GRAPHBLAS_H_AVAILABLE)
        set(GraphBLAS_FOUND TRUE)
        set(GRAPHBLAS_INCLUDE_DIR "")
        set(GRAPHBLAS_LIBRARIES "graphblas")
    #    set(GRAPHBLAS_STATIC "")
    #    set(GRAPHBLAS_LIBRARY "")
    endif()
endif()

# See if brew is installed, and if so check for suite-sparse
if (NOT GraphBLAS_FOUND AND ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    find_program(BREW "brew")
    if (BREW)
        execute_process(COMMAND brew --prefix suite-sparse OUTPUT_VARIABLE PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
        if (NOT ("${PREFIX}" STREQUAL "" ))
            set(GraphBLAS_ROOT "${PREFIX}/lib/cmake/SuiteSparse")
            message("suite-sparse found: ${PREFIX}")
            message("checking: ${GraphBLAS_ROOT}")
            set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${GraphBLAS_ROOT}")
            find_package(GraphBLAS QUIET MODULE)
        else()
            message("Install GraphBLAS with 'brew install suite-sparse'")
        endif()
    endif()
endif()