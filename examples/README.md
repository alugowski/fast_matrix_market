Complete example of how to use `fast_matrix_market`.

This directory is explicitly *not* part of the `fast_matrix_market` build system, because neither is your project.

The [fast_matrix_market](fast_matrix_market) subdirectory simulates cloning the repo into a subdirectory of your project. "Simulates" because this directory just contains symlinks into the main repo so that the examples are always up-to-date. The symlinks also demonstrate the parts of the main repo that are necessary to use FMM.

The example [CMakeLists.txt](CMakeLists.txt) uses `add_subdirectory()`, but of course any of the methods described in the main Installation section of the [README](../README.md) will work.

## Build and Run

Build using standard CMake commands:

```bash
cmake -S . -B cmake-build-debug/ -D CMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-debug/

cmake-build-debug/simple1
```

### simple1

Defines simple sparse and dense matrices built on `std::vector`, writes and reads both using `fast_matrix_market` triplet and array methods.

### generate_matrix_market

Generates a random Matrix Market file that has a file size approximately equal to a user-specified target.

Demonstrates FMM's generator functionality.

### sort_matrix_market

Sorts the elements of a Matrix Market file, first by row then by column.

Demonstrates FMM's ability to read values into `std::string`, useful here because the sorted values are never changed by a parse/format cycle, no matter what their type is.

This may be a useful pre-processing utility for cases where the same matrix is loaded multiple times. Some loads can be significantly faster if the elements in the Matrix Market file are already sorted.
