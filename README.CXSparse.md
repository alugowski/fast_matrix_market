# CXSparse integration

`fast_matrix_market` provides read/write methods for CXSparse `cs_xx` structs.

# Usage

```c++
#include <fast_matrix_market/extras/CXSparse.hpp>
```

### Reading
```c++
    cs_dl *A;

    std::ifstream f(path_to_mtx_file);
    fast_matrix_market::read_matrix_market_cxsparse(f, &A, cs_dl_spalloc);
```
Note the last argument. It is the `cs_spalloc` routine that matches the type
of `A`. It must be specified explicitly because it is impractical to autodetect due to the way CXSparse
implements multiple index and value types.

`read_matrix_market_cxsparse` creates a triplet version of the matrix structure. For CSC, follow up with
CXSparse's `cs_compress()`:
```c++
    cs_dl *csc_A = cs_dl_compress(A);
```
### Writing

```c++
    cs_dl *A;

    std::ofstream f(path_to_mtx_file);
    fast_matrix_market::write_matrix_market_cxsparse(f, A);
```

The write method supports both triplet and compressed versions.