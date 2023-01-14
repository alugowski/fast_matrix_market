# Copyright (C) 2022-2023 Adam Lugowski. All rights reserved.
# Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
import numpy
import os
from pathlib import Path

from . import _core
from ._core import __doc__, __version__, header

__all__ = ["__doc__", "__version__", "header"]

_field_to_dtype = {
    "integer": "int64",
    "real": "float64",
    "complex": "complex",
    "pattern": "float64",

    "long-integer": "int64",
    "long-real": "longdouble",
    "long-complex": "longcomplex",
    "long-pattern": "float64",
}

PARALLELISM = 0
"""
Number of threads to use. 0 means number of threads in the system.
"""


def _read_body_array(cursor, long_type):
    import numpy as np
    vals = np.zeros(cursor.header.shape, dtype=_field_to_dtype.get(("long-" if long_type else "")+cursor.header.field))
    _core.read_body_array(cursor, vals)
    return vals


def _read_body_triplet(cursor, long_type, generalize_symmetry=True):
    import numpy as np

    index_dtype = "int32"  # SciPy uses this size
    if cursor.header.nrows > 2**31 or cursor.header.ncols > 2**31:
        # Dimensions are too large to fit in int32
        index_dtype = "int64"

    i = np.zeros(cursor.header.nnz, dtype=index_dtype)
    j = np.zeros(cursor.header.nnz, dtype=index_dtype)
    data = np.zeros(cursor.header.nnz, dtype=_field_to_dtype.get(("long-" if long_type else "")+cursor.header.field))

    _core.read_body_triplet(cursor, i, j, data)

    if generalize_symmetry and cursor.header.symmetry != "general":
        off_diagonal_mask = (i != j)
        off_diagonal_rows = i[off_diagonal_mask]
        off_diagonal_cols = j[off_diagonal_mask]
        off_diagonal_data = data[off_diagonal_mask]

        if cursor.header.symmetry == "skew-symmetric":
            off_diagonal_data *= -1
        elif cursor.header.symmetry == "hermitian":
            off_diagonal_data = off_diagonal_data.conjugate()

        i = np.concatenate((i, off_diagonal_cols))
        j = np.concatenate((j, off_diagonal_rows))
        data = np.concatenate((data, off_diagonal_data))

    return (data, (i, j)), cursor.header.shape


def _get_read_cursor(source, parallelism=None):
    if parallelism is None:
        parallelism = PARALLELISM

    try:
        source = os.fspath(source)
    except TypeError:
        # Stream object. Not supported yet.
        raise

    if isinstance(source, str) and (source.startswith("%%MatrixMarket") or source.startswith("%MatrixMarket")):
        return _core.open_read_string(str(source), parallelism)

    return _core.open_read_file(str(source), parallelism)


def _get_write_cursor(source, h=None, comment=None, parallelism=None):
    if parallelism is None:
        parallelism = PARALLELISM

    if not h:
        if comment is not None:
            h = header(comment=comment)
        else:
            h = header()

    if not source:
        # string
        # TODO: REMOVE
        return _core.open_write_string(h, parallelism)

    try:
        source = os.fspath(source)
    except TypeError:
        # Stream object. Not supported yet.
        raise

    return _core.open_write_file(str(source), comment, parallelism)


def read_header(source=None) -> header:
    """
    Read a Matrix Market header from a file or from a string.

    :param source: filename or string
    :return: parsed header object
    """
    return _get_read_cursor(source, 1).header


def write_header(h: header, target=None):
    """
    Write a Matrix Market header to a file or a string.

    :param h: header to write
    :param target: if not None, write to this file.
    :return: if target is None then returns a string containing h as if it was written to a file
    """

    if target:
        try:
            target = os.fspath(target)
        except TypeError:
            # Stream object. Not supported yet.
            raise
        _core.write_header_file(h, str(target))
    else:
        cursor = _get_write_cursor(None, h, parallelism=1)
        _core.write_header_only(cursor)
        return cursor.get_string()


def read_array(source, parallelism=None, long_type=False) -> numpy.ndarray:
    """
    Read MatrixMarket file into dense NumPy Array, regardless if the file is sparse or dense.

    :param source: path to MatrixMarket file
    :param parallelism: number of threads to use. 0 means auto.
    :param long_type: Use long floating point datatypes (if available in your NumPy).
    This means longdouble and longcomplex instead of float64 and complex64.
    :return: numpy array
    """

    return _read_body_array(_get_read_cursor(source, parallelism), long_type=long_type)


def write_array(target, a, comment='', parallelism=None):
    import numpy as np
    a = np.asarray(a)
    cursor = _get_write_cursor(target, comment=comment, parallelism=parallelism)
    _core.write_array(cursor, a)


def read_triplet(source, parallelism=None, long_type=False, generalize_symmetry=True):
    return _read_body_triplet(_get_read_cursor(source, parallelism),
                              long_type=long_type, generalize_symmetry=generalize_symmetry)


def write_triplet(target, a, shape, comment='', parallelism=None):
    data, row, col = a
    cursor = _get_write_cursor(target, comment=comment, parallelism=parallelism)
    _core.write_triplet(cursor, shape, row, col, data)


def read_scipy(source, parallelism=None, long_type=False):
    cursor = _get_read_cursor(source, parallelism)

    if cursor.header.format == "array":
        return _read_body_array(cursor, long_type=long_type)
    else:
        from scipy.sparse import coo_matrix
        triplet, shape = _read_body_triplet(cursor, long_type=long_type, generalize_symmetry=True)
        return coo_matrix(triplet, shape=shape)


def _apply_field(data, field, no_pattern=False):
    import numpy as np

    if field is None:
        return data
    if field == "pattern":
        if no_pattern:
            return data
        else:
            return np.zeros(0)
    if field == "integer" or field == "unsigned-integer":
        return data.astype('int64')
    if field == "real" or field == "double":
        return data.astype('float64')
    if field == "complex":
        return data.astype('complex')

    raise ValueError("Invalid field name")


def write_scipy(target, a, comment='', field=None, precision=None, symmetry=None, parallelism=None):
    import numpy as np
    import scipy.sparse

    cursor = _get_write_cursor(target, comment=comment, parallelism=parallelism)

    if isinstance(a, list) or isinstance(a, tuple) or hasattr(a, '__array__'):
        a = np.asarray(a)

    if isinstance(a, np.ndarray):
        # Write dense numpy arrays
        a = _apply_field(a, field, no_pattern=True)
        _core.write_array(cursor, a)
        # TODO: remove this:
        if target is None:
            return cursor.get_string()
        return

    if scipy.sparse.isspmatrix(a):
        # Write sparse scipy matrices

        is_compressed = (isinstance(a, scipy.sparse.csc_matrix) or isinstance(a, scipy.sparse.csr_matrix))

        if not is_compressed:
            # convert everything except CSC/CSR to triplet
            a = a.tocoo()

        data = _apply_field(a.data, field)

        if is_compressed:
            # CSC and CSR can be written directly
            is_csr = isinstance(a, scipy.sparse.csr_matrix)
            _core.write_csc(cursor, a.shape, a.indptr, a.indices, data, is_csr)
        else:
            _core.write_triplet(cursor, a.shape, a.row, a.col, data)

        # TODO: remove this:
        if target is None:
            return cursor.get_string()
        return

    raise ValueError("unknown matrix type: %s" % type(a))


mmread = read_scipy
mmwrite = write_scipy

_scipy_mmread = None
_scipy_mmwrite = None


def patch_scipy():
    """
    Replace scipy.io.mmread and mmwrite with fast_matrix_market versions.
    Do this to speed up your SciPy code without changing it.
    """
    import scipy.io

    global _scipy_mmread
    global _scipy_mmwrite

    # save original SciPy methods
    if not _scipy_mmread:
        _scipy_mmread = scipy.io.mmread
    if not _scipy_mmwrite:
        _scipy_mmwrite = scipy.io.mmwrite

    # replace with fast_matrix_market methods
    scipy.io.mmread = read_scipy
    scipy.io.mmwrite = write_scipy


def unpatch_scipy():
    """
    Undo patch_scipy()
    """
    import scipy.io

    global _scipy_mmread
    global _scipy_mmwrite

    if _scipy_mmread:
        scipy.io.mmread = _scipy_mmread
        _scipy_mmread = None
    if _scipy_mmwrite:
        scipy.io.mmwrite = _scipy_mmwrite
        _scipy_mmwrite = None
