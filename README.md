
## Fast
fast, using `fast_float` to parse floats, and `Dragonbox` for writing floats.

parallel

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

## Flexible

Support reading/writing to any datastructure. Simply provide single-method `parse_handler` and `formatter` implementations to support any datastructure.