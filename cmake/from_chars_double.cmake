# Test for float version of std::from_chars.
# The method is specified in C++17, but compiler support varies.
include(CheckSourceCompiles)

check_source_compiles(CXX "
#include <charconv>
int main(void) {
    double value = 0;
    const char* ptr;
    std::from_chars_result result = std::from_chars(ptr, ptr, value, std::chars_format::general);
    return 0;
}
" float_from_chars_supported)
if (NOT float_from_chars_supported)
    add_definitions(-DFROM_CHARS_DOUBLE_NOT_SUPPORTED)
    message("std::from_chars<double> not detected. Need fast_float.")
endif()

# Check for long double support
check_source_compiles(CXX "
#include <charconv>
int main(void) {
    long double value = 0;
    const char* ptr;
    std::from_chars_result result = std::from_chars(ptr, ptr, value, std::chars_format::general);
    return 0;
}
" long_double_from_chars_supported)
if (NOT long_double_from_chars_supported)
    add_definitions(-DFROM_CHARS_LONG_DOUBLE_NOT_SUPPORTED)
    message("std::from_chars<long double> not detected. Using std::strtold() fallback.")
endif()