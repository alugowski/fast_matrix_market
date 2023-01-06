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
if (to_chars_int_supported)
    add_definitions(-DTO_CHARS_INT_SUPPORTED)
else()
    message("std::to_chars<int> not detected. Using std::to_string (write parallelism will suffer).")
endif()

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
if (to_chars_double_supported)
    add_definitions(-DTO_CHARS_DOUBLE_SUPPORTED)
else()
    message("std::to_chars<double> not detected. Using std::to_string (write parallelism will suffer).")
endif()

# Check for long double support
check_source_compiles(CXX "
#include <charconv>
int main(void) {
    long double value = 0;
    char ptr[10];
    std::to_chars(ptr, ptr+10, value);
    return 0;
}
" to_chars_double_supported)
if (to_chars_double_supported)
    add_definitions(-DTO_CHARS_LONG_DOUBLE_SUPPORTED)
else()
    message("std::to_chars<long double> not detected. Using std::to_string (write parallelism will suffer).")
endif()