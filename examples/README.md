## Build and Run

Build using the standard CMake commands:

```bash
cmake -S . -B cmake-build-debug/ -D CMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-debug/

cmake-build-debug/simple1
```

### simple1

Defines custom sparse and dense matrices, writes and reads both.