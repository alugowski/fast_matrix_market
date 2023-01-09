A full-featured and fast Matrix Market loader.

[Matrix Market](https://math.nist.gov/MatrixMarket/formats.html) is a simple, human-readable, and widely used sparse matrix file format. Most sparse matrix libraries include Matrix Market read and write routines.

However, included routines are typically slow and/or are missing format features. Use this library to fix those shortcomings.

## Fast

Loaders using `iostreams` or `fscanf` are both slow and do not parallelize. See [parse_bench](https://github.com/alugowski/parse-bench) for a demonstration.

`fast_matrix_market` takes the fastest available sequential methods for a 10x sequential speed improvement over `iostreams`.

The majority of the improvement comes from using C++17's `std::from_chars` and `std::to_chars`.
Also:
* Parse floating-point using [`fast_float`](https://github.com/fastfloat/fast_float). Compiler support for floating-point `std::from_chars` varies.
* Write floating-point using [`Dragonbox`](https://github.com/jk-jeon/dragonbox). Compiler support for floating-point `std::to_chars` varies.

We include standard library fallbacks for the above libraries, but both sequential and parallel performance suffers without them.

## Parallel

We support parallel reading and writing using C++11 threads.

This lets us reach **>1GB/s** read and write speeds on a laptop (about 25x improvement over `iostreams`).

![read](benchmark_plots/parallel-scaling-read.svg)
![write](benchmark_plots/parallel-scaling-write.svg)

Note: IOStreams benchmark is sequential. IOstreams get *slower* with additional parallelism due to internal locking on the locale.

## Full Featured

`matrix` and `vector`.

`coordinate` and `array`, readable into either sparse or dense structures.

All field types supported, with appropriate C++ types:
`integer`, `real`, `double`, `complex`, `pattern`.

Support integer, `float`, `double`, `long double`.

Automatic sensible conversions. For example, `integer` files can be read into `complex<>` arrays, `pattern` can be expanded to any type.

Read and write all symmetries.

Optional (on by default) automatic symmetry generalization if your code cannot make use of the symmetries. 
Matrix Market format spec says, for any symmetries other than general, only entries in the lower triangular portion need be supplied.
* **`symmetric`:** for `(row, column, value)`, also emit `(column, row, value)`
* **`skew-symmetric`:** for `(row, column, value)`, also emit `(column, row, -value)`
* **`hermitian`:** for `(row, column, value)`, also emit `(column, row, complex_conjugate(value))`


## Easy

Header-only. Use CMake to fetch the library, copy and use `add_subdirectory`, or just copy into your project's `include` directory.
Note: If you make a copy, be sure to also include the [`fast_float`](https://github.com/fastfloat/fast_float) library. You can omit it if you know your compiler implements `std::from_chars<double>` (e.g. GCC 12+).
Same with [`Dragonbox`](https://github.com/jk-jeon/dragonbox).

## Bundled CXSparse and Eigen Integrations

See [CXSparse](README.CXSparse.md) and [Eigen](README.Eigen.md).

## Flexible

Support reading/writing to any datastructure. Simply provide single-method `parse_handler` and `formatter` implementations to support any datastructure.


## Other 3rd Party Libraries Used
Thread pool implementation from [thread-pool](https://github.com/bshoshany/thread-pool).