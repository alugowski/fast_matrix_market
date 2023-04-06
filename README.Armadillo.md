# Armadillo (blaze-lib) Matrix Market

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

### Reading
```c++
std::ifstream f("input.mtx");

fast_matrix_market::read_matrix_market_arma(f, A);
```

**Note:** Armadillo versions older than 10.5 [did not zero-initialize memory](https://arma.sourceforge.net/docs.html#changelog).
On those versions loading a sparse Matrix Market file into a dense `Mat` is not supported.


### Writing

```c++
std::ofstream f("output.mtx");

fast_matrix_market::write_matrix_market_arma(f, A);
```
