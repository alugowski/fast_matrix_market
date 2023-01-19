[![PyPI version](https://badge.fury.io/py/fast_matrix_market.svg)](https://badge.fury.io/py/fast_matrix_market)

Fast and full-featured Matrix Market file I/O package for Python.

Fastest way to read and write any Matrix Market `.mtx` file into a dense ndarray, sparse coordinate (triplet) arrays, or SciPy sparse matrix.

Implemented as a Python binding of the C++ [fast_matrix_market](https://github.com/alugowski/fast_matrix_market) library.

```shell
pip install fast_matrix_market
```

# Usage
```python
import fast_matrix_market as fmm
```

#### Read as scipy sparse matrix
```python
>>> a = fmm.mmread("eye3.mtx")
>>> a
<3x3 sparse matrix of type '<class 'numpy.float64'>'
        with 3 stored elements in COOrdinate format>
>>> print(a)
(0, 0)	1.0
(1, 1)	1.0
(2, 2)	1.0
```
#### Read as raw coordinate/triplet arrays
```python
>>> (data, (rows, cols)), shape = fmm.read_coo("eye3.mtx")
>>> rows, cols, data
(array([0, 1, 2], dtype=int32), array([0, 1, 2], dtype=int32), array([1., 1., 1.]))
```
#### Read as dense ndarray
```python
>>> a = fmm.read_array("eye3.mtx")
>>> a
array([[1., 0., 0.],
       [0., 1., 0.],
       [0., 0., 1.]])
```
#### Write any of the above to a file
```python
>>> fmm.mmwrite("matrix_out.mtx", a)
```
#### Write to streams (read from streams too)
```python
>>> bio = io.BytesIO()
>>> fmm.mmwrite(bio, a)
```
#### Read only the header
```python
>>> header = fmm.read_header("eye3.mtx")
header(shape=(3, 3), nnz=3, comment="3-by-3 identity matrix", object="matrix", format="coordinate", field="real", symmetry="general")

>>> header.shape
(3, 3)

>>> header.to_dict()
{'shape': (3, 3), 'nnz': 3, 'comment': '3-by-3 identity matrix', 'object': 'matrix', 'format': 'coordinate', 'field': 'real', 'symmetry': 'general'}
```

**Note:** SciPy is only a runtime dependency for the `mmread` and `mmwrite` methods. All others depend only on NumPy.

# Compared to scipy.io.mmread()

The `fast_matrix_market.mmread()` and `mmwrite()` methods are direct replacements for their respective SciPy versions.
Compared to SciPy v1.10.0:

* **Significant performance boost**

    ![read](https://raw.githubusercontent.com/alugowski/fast_matrix_market/main/benchmark_plots/parallel-scaling-python-read.svg)
![write](https://raw.githubusercontent.com/alugowski/fast_matrix_market/main/benchmark_plots/parallel-scaling-python-write.svg)

    All cores on the system are used by default, use the `parallelism` argument to override. SciPy's routines are single-threaded.

* **64-bit indices**, but only if the matrix dimensions require it.

    `scipy.io.mmread()` crashes on large matrices (dimensions > 2<sup>31</sup>) because it uses 32-bit indices on most platforms.

* **Directly write CSC/CSR matrices**  with no COO intermediary.

* **longdouble**  
Read and write `longdouble`/`longcomplex` values for more floating-point precision on platforms that support it (e.g. 80-bit floats).

    Just pass `long_type=True` argument to any read method to use `longdouble` arrays. SciPy can write `longdouble` matrices but reads use `double` precision.

    **Note:** Many platforms do not offer any precision greater than `double` even if the `longdouble` type exists.
On those platforms `longdouble == double` so check your Numpy for support. As of writing only Linux tends to have `longdouble > double`.

* **Vector files**  
Read 1D vector files. `scipy.io.mmread()` throws a `ValueError`.

### Differences

* If no symmetry is specified `scipy.io.mmwrite` will search the matrix for one. 
This is a very slow process that significantly impacts writing time for all matrices, including non-symmetric ones.
It can be disabled by setting `symmetry="general"`, but that is easily forgotten.
`fast_matrix_market.mmwrite()` only looks for symmetries if the `find_symmetry=True` argument is passed.
* `precision` argument to `mmwrite()` is currently ignored. Floats may be written with more precision than desired.

### Quick way to try

Replace `scipy.io.mmread` with `fast_matrix_market.mmread` to quickly see if your scripts would benefit from a refactor:

```python
import scipy.io
import fast_matrix_market as fmm

scipy.io.mmread = fmm.mmread
scipy.io.mmwrite = fmm.mmwrite
```


# Dependencies

* No dependencies to read/write MatrixMarket headers.
* `numpy` to read/write arrays (i.e. `read_array()` and `read_coo()`). SciPy is **not** required.
* `scipy` to read/write `scipy.sparse` sparse matrices (i.e. `read_scipy()` and `mmread()`).

# Development

This Python binding is implemented using pybind11 and built with [scikit-build](https://scikit-build.readthedocs.io/en/latest/).

All code is in the [python/](https://github.com/alugowski/fast_matrix_market/tree/main/python) directory. If you make any changes simply install the package directory to build it:

```shell
pip install python/ -v
```