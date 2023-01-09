# Test for std::to_chars.
# The floating point versions are specified in C++17, but compiler support varies.
include(CheckSourceCompiles)

# Check for int support
check_source_compiles(CXX "
#include <charconv>
int main(void) {
    int value = 0;
    char ptr[10];
    std::to_chars(ptr, ptr+10, value);
    return 0;
}
" to_chars_int_supported)

# Check for double support
check_source_compiles(CXX "
#include <charconv>
int main(void) {
    double value = 0;
    char ptr[10];
    std::to_chars(ptr, ptr+10, value);
    return 0;
}
" to_chars_double_supported)

# Check for long double support
check_source_compiles(CXX "
#include <charconv>
int main(void) {
    long double value = 0;
    char ptr[10];
    std::to_chars(ptr, ptr+10, value);
    return 0;
}
" to_chars_long_double_supported)
