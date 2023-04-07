# Blaze (blaze-lib) Matrix Market

`fast_matrix_market` provides read/write methods for [Blaze](https://bitbucket.org/blaze-lib/blaze) sparse and dense matrices and vectors.

# Usage

```c++
#include <fast_matrix_market/app/Blaze.hpp>
```

```c++
blaze::CompressedMatrix<double> A;
// or
blaze::DynamicMatrix<double> A;
// or
blaze::CompressedVector<double> A;
// or
blaze::DynamicVector<double> A;
```

Not restricted to `double` matrices. Any type supported by Blaze that makes sense in Matrix Market is also supported, such as `int64_t`, `float`, `std::complex<double>`, etc.

### Reading
```c++
std::ifstream f("input.mtx");

fast_matrix_market::read_matrix_market_blaze(f, A);
```

**Note:** if `A` is a Vector type then the Matrix Market file must be either a vector or a
1-by-N row or M-by-1 column matrix. Any other matrix will cause an exception to be thrown.

### Writing

```c++
std::ofstream f("output.mtx");

fast_matrix_market::write_matrix_market_blaze(f, A);
```


**Compatibility Note:** Blaze Vector objects are written as vector Matrix Market files.
Many MatrixMarket loaders cannot read vector files. If this is a problem write the vector
as a 1-by-N or M-by-1 matrix.