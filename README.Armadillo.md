# Armadillo Matrix Market

`fast_matrix_market` provides read/write methods for [Armadillo](https://arma.sourceforge.net/) sparse and dense matrices.

# Usage

```c++
#include <fast_matrix_market/app/Armadillo.hpp>
```

```c++
arma::Mat<double> A;
// or
arma::SpMat<double> A;
```

Not restricted to `double` matrices. Any type supported by Armadillo that makes sense in Matrix Market is also supported, such as `int64_t`, `float`, `std::complex<double>`, etc.

### Reading
```c++
std::ifstream f("input.mtx");

fast_matrix_market::read_matrix_market_arma(f, A);
```

**Note:** Armadillo versions older than 10.5 [did not zero-initialize memory](https://arma.sourceforge.net/docs.html#changelog).
On those versions reading any Matrix Market file into a dense `arma::Mat` is not supported.


### Writing

```c++
std::ofstream f("output.mtx");

fast_matrix_market::write_matrix_market_arma(f, A);
```
