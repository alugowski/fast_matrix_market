# Eigen integration

`fast_matrix_market` provides read/write methods for Eigen sparse matrices, dense matrices, and dense vectors.

# Usage

```c++
#include <fast_matrix_market/app/Eigen.hpp>
```

### Reading
Sparse:
```c++
std::ifstream f("input.mtx");

Eigen::SparseMatrix<double> mat;
fast_matrix_market::read_matrix_market_eigen(f, mat);
```

Dense Matrix/Vector is the same, using `read_matrix_market_eigen_dense`:
```c++
std::ifstream f("input.mtx");

Eigen::VectorXd vec;
fast_matrix_market::read_matrix_market_eigen_dense(f, vec);
```

### Writing

```c++
std::ofstream f("output.mtx");

Eigen::SparseMatrix<double> mat;
fast_matrix_market::write_matrix_market_eigen(f, mat);
```

Again, for dense matrices/vectors use `write_matrix_market_eigen_dense`.

Note that Vectors are written with object type = `matrix` for compatibility reasons. Eigen's `saveMarketVector()` does the same. As of writing Eigen's loader crashes on `vector` files.