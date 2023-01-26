# Test for float version of std::from_chars.
# The method is specified in C++17, but compiler support varies.
include(CheckSourceCompiles)

# CMP0067: Honor language standard in try_compile() source-file signature.
# https://cmake.org/cmake/help/latest/policy/CMP0067.html
cmake_policy(SET CMP0067 NEW)
set(CMAKE_CXX_STANDARD 17)

# Check for int support
check_source_compiles(CXX "
#include <charconv>
int main(void) {
    int value = 0;
    const char* ptr;
    std::from_chars_result result = std::from_chars(ptr, ptr, value);
    return 0;
}
" from_chars_int_supported)

# Check for double support
check_source_compiles(CXX "
#include <charconv>
int main(void) {
    double value = 0;
    const char* ptr;
    std::from_chars_result result = std::from_chars(ptr, ptr, value, std::chars_format::general);
    return 0;
}
" from_chars_double_supported)

# Check for long double support
check_source_compiles(CXX "
#include <charconv>
int main(void) {
    long double value = 0;
    const char* ptr;
    std::from_chars_result result = std::from_chars(ptr, ptr, value, std::chars_format::general);
    return 0;
}
" from_chars_long_double_supported)
