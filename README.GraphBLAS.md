# GraphBLAS Matrix Market

`fast_matrix_market` provides read and write methods for GraphBLAS GrB_Matrix and GrB_Vector objects.

Tested with [SuiteSparse:GraphBLAS](https://github.com/DrTimothyAldenDavis/GraphBLAS), but should work with any GraphBLAS implementation.

# Usage

```c++
#include <fast_matrix_market/app/GraphBLAS.hpp>
```

```c++
// Initialize GraphBLAS.
// GrB_init() has to be called before any other GraphBLAS methods else you get a GrB_PANIC.
GrB_init(GrB_NONBLOCKING); // or GrB_BLOCKING
```

```c++
GrB_Matrix A;
// or
GrB_Vector A;
```
### Reading
```c++
std::ifstream f("input.mtx");

fast_matrix_market::read_matrix_market_graphblas(f, &A);
```

**A's GrB_Type** is determined by the first non-null condition:
* Explicit `desired_type` argument to `read*()`. `A` will be read as this type.
* `%%GraphBLAS type <ctype>` structured comment.
* MatrixMarket header `field`.

User types are not supported at this time. If you have a use case for user types stored in Matrix Market please get in touch.

**Duplicate values** are summed using `GrB_PLUS_*`.

**Note:** if `A` is a `GrB_Vector` then the Matrix Market file must be either a `vector` or a
1-by-N row or M-by-1 column matrix. Any other matrix will cause an exception to be thrown.

### Writing

```c++
std::ofstream f("output.mtx", std::ios_base::binary);

fast_matrix_market::write_matrix_market_graphblas(f, A);
```

**Compatibility Note:** `GrB_Vector` objects are written as `vector` Matrix Market files.
Many MatrixMarket loaders cannot read vector files. If this is a problem write the vector
as a 1-by-N or M-by-1 matrix.

# GxB Extensions Used

The following GraphBLAS extensions are used. All are individually switchable. All are present in SuiteSparse:GraphBLAS.
* `GxB_FC32`/`GxB_FC64` complex types. Standard GraphBLAS does not include complex number types.
  * Define `FMM_GXB_COMPLEX=0` to disable.
* `GxB_Matrix_build_Scalar` is used to build iso-valued matrices from `pattern` files.
  * Define `FMM_GXB_BUILD_SCALAR=0` to disable.
* `GxB_Matrix_pack_FullC` is used to efficiently read `array` files.
  * Define `FMM_GXB_PACK_UNPACK=0` to disable
* `GxB_Iterator` for zero-copy writes.
  * Define `FMM_GXB_ITERATORS=0` to disable.
* `GxB_Matrix_type_name` to determine matrix type.
  * Define `FMM_GXB_TYPE_NAME=0` to disable.

All extensions can be disabled by defining `FMM_NO_GXB`.

