cmake_minimum_required(VERSION 3.15...3.25)

project(${SKBUILD_PROJECT_NAME} VERSION ${SKBUILD_PROJECT_VERSION})

find_package(pybind11 CONFIG REQUIRED)

# Sets -fPIC. Necessary because Dragonbox builds a static library. Any static library loaded to Python needs -fPIC.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Optionally disable dependencies.
# Using environment variable to be able to set this when using pip install.
# Useful to speed up builds in testing, but also the Windows CI tools successfully build the dependencies then throw a
# permission denied when trying to use them. Therefore this is necessary to get a Windows CI build to finish.
# Also enabling it for Windows in general until someone with a Windows box can confirm that the sdist can build
# correctly without it.
if(DEFINED ENV{FMM_PYTHON_NO_DEPS} OR WIN32)
    set(FMM_USE_FAST_FLOAT OFF CACHE BOOL "" FORCE)
    set(FMM_USE_DRAGONBOX OFF CACHE BOOL "" FORCE)
endif()

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

pybind11_add_module(_core MODULE src/fast_matrix_market/core.cpp)
target_compile_definitions(_core PRIVATE VERSION_INFO=${PROJECT_VERSION})
target_link_libraries(_core PRIVATE fast_matrix_market::fast_matrix_market)

install(TARGETS _core DESTINATION fast_matrix_market)
