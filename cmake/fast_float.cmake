# Fetch fast_float library
# This includes an implementation of the float version of std::from_chars.
# Native compiler support for this is still spotty at best.
#
# https://github.com/fastfloat/fast_float

FetchContent_Declare(
        fast_float
        GIT_REPOSITORY https://github.com/fastfloat/fast_float.git
        GIT_TAG tags/v3.8.1
        GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(fast_float)