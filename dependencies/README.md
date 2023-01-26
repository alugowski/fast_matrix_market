# Dependencies

These optional dependencies speed up floating-point parsing and formatting.
Standard library fallbacks are included but both sequential and parallel performance can suffer without them.

## [fast_float](https://github.com/fastfloat/fast_float)
Floating-point parsing.

Using the header-only single file release version. To upgrade just paste in a new version of `fast_float.h`.

Optional, set `FMM_USE_FAST_FLOAT=OFF` to disable.

## [Dragonbox](https://github.com/jk-jeon/dragonbox)
Floating-point rendering.

Using a pruned copy of the dragonbox repo. The full release includes two large PDFs, subprojects, etc.
Just clone the repo and delete anything not needed to build the library.

Optional, set `FMM_USE_DRAGONBOX=OFF` to disable.

## [Ryu](https://github.com/ulfjack/ryu)
Floating-point rendering with user-specified precision.

Optional, set `FMM_USE_RYU=OFF` to disable.

## [thread-pool](https://github.com/bshoshany/thread-pool)
A lightweight thread pool using C++11 threads. Bundled in `include/`.