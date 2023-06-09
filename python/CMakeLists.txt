cmake_minimum_required(VERSION 3.15...3.25)

project(${SKBUILD_PROJECT_NAME} VERSION ${SKBUILD_PROJECT_VERSION})

find_package(pybind11 CONFIG REQUIRED)

# Sets -fPIC. Necessary because Dragonbox builds a static library. Any static library loaded to Python needs -fPIC.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Accept a pointer to the directory to include fast_matrix_market from.
# This is needed in some build systems where pip may relocate this directory but not the parent,
# thus breaking the symbolic links in ./fast_matrix_market/
if(DEFINED ENV{FMM_PYTHON_DIR})
    set(FMM_CWD "$ENV{FMM_PYTHON_DIR}/python")
else()
    set(FMM_CWD ".")
endif()

set(FMM_DIR "${FMM_CWD}/fast_matrix_market")
message("Including fast_matrix_market from: ${FMM_DIR}")
add_subdirectory(${FMM_DIR} cmake-build)

pybind11_add_module(_core MODULE
        src/fast_matrix_market/_core.cpp
        src/fast_matrix_market/_core_read_array.cpp
        src/fast_matrix_market/_core_read_coo.cpp
        src/fast_matrix_market/_core_write_array.cpp
        src/fast_matrix_market/_core_write_coo_32.cpp
        src/fast_matrix_market/_core_write_coo_64.cpp
        src/fast_matrix_market/_core_write_csc_32.cpp
        src/fast_matrix_market/_core_write_csc_64.cpp
        )
target_compile_definitions(_core PRIVATE VERSION_INFO=${PROJECT_VERSION})
target_link_libraries(_core PRIVATE fast_matrix_market::fast_matrix_market)

install(TARGETS _core DESTINATION fast_matrix_market)
