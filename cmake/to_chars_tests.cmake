# Test for std::to_chars.
# The floating point versions are specified in C++17, but compiler support varies.
include(CheckCXXSourceCompiles)

# CMP0067: Honor language standard in try_compile() source-file signature.
# https://cmake.org/cmake/help/latest/policy/CMP0067.html
cmake_policy(SET CMP0067 NEW)
set(CMAKE_CXX_STANDARD 17)

# Check for int support
check_cxx_source_compiles("
#include <charconv>
int main(void) {
    int value = 0;
    char ptr[10];
    std::to_chars(ptr, ptr+10, value);
    return 0;
}
" to_chars_int_supported)

# Check for double support
check_cxx_source_compiles("
#include <charconv>
int main(void) {
    double value = 0;
    char ptr[10];
    std::to_chars(ptr, ptr+10, value);
    return 0;
}
" to_chars_double_supported)

# Check for long double support
check_cxx_source_compiles("
#include <charconv>
int main(void) {
    long double value = 0;
    char ptr[10];
    std::to_chars(ptr, ptr+10, value);
    return 0;
}
" to_chars_long_double_supported)
