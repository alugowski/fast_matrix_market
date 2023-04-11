# CXSparse Matrix Market

`fast_matrix_market` provides read/write methods for CXSparse `cs_xx` structs.

# Usage

```c++
#include <fast_matrix_market/app/CXSparse.hpp>
```

### Reading
```c++
cs_dl *A;

std::ifstream f("input.mtx");
fast_matrix_market::read_matrix_market_cxsparse(f, &A, cs_dl_spalloc);
```
Note the last argument. It is the `cs_*_spalloc` routine that matches the type
of `A`. It must be specified explicitly because it is impractical to autodetect due to the way CXSparse
implements multiple index and value types. All CXSparse types are supported, such as `cs_dl`, `cs_ci`, `cs_cl`, etc.

`read_matrix_market_cxsparse` creates a triplet version of the matrix structure. For CSC, follow up with
CXSparse's `cs_compress()`:
```c++
cs_dl *csc_A = cs_dl_compress(A);
```
### Writing

```c++
cs_dl *A;

std::ofstream f("output.mtx");
fast_matrix_market::write_matrix_market_cxsparse(f, A);
```

The write method supports both triplet and compressed matrices.