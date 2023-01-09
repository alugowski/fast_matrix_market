# Test for float version of std::from_chars.
# The method is specified in C++17, but compiler support varies.
include(CheckSourceCompiles)

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
