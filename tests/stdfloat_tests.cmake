# Test for C++23 fixed-width floating point support.
include(CheckSourceCompiles)

# CMP0067: Honor language standard in try_compile() source-file signature.
# https://cmake.org/cmake/help/latest/policy/CMP0067.html
cmake_policy(SET CMP0067 NEW)
set(CMAKE_CXX_STANDARD 23)

# Check for header
check_source_compiles(CXX "
#include <stdfloat>
int main(void) {
    return 0;
}
" have_stdfloat)

set(CMAKE_CXX_STANDARD 17)
