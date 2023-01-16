cmake_minimum_required(VERSION 3.15...3.25)

project(${SKBUILD_PROJECT_NAME} VERSION ${SKBUILD_PROJECT_VERSION})

find_package(pybind11 CONFIG REQUIRED)

add_subdirectory(fast_matrix_market)

pybind11_add_module(_core MODULE src/fast_matrix_market/core.cpp)
target_compile_definitions(_core PRIVATE VERSION_INFO=${PROJECT_VERSION})
target_link_libraries(_core PRIVATE fast_matrix_market::fast_matrix_market)

install(TARGETS _core DESTINATION fast_matrix_market)