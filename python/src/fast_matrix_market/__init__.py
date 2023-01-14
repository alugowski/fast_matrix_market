# Copyright (C) 2022-2023 Adam Lugowski. All rights reserved.
# Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
import numpy
import os
from pathlib import Path

from . import _core
from ._core import __doc__, __version__, header

__all__ = ["__doc__", "__version__", "header"]

_dtypes_by_field = {
    "integer": "intp",
    "real": "float",
    "double": "float",
    "complex": "D",
    "pattern": "float",
}

PARALLELISM = 0
"""
Number of threads to use. 0 means number of threads in the system.
"""


def _read_body_array(cursor):
    import numpy as np
    vals = np.zeros(cursor.header.shape, dtype=_dtypes_by_field.get(cursor.header.field))
    _core.read_body_array(cursor, vals)
    return vals


def _read_body_triplet(cursor, generalize_symmetry=True):
    import numpy as np

    index_dtype = "int32"  # SciPy uses this size
    if cursor.header.nrows > 2 ** 31 or cursor.header.ncols > 2 ** 31:
        # Dimensions are too large to fit in int32
        index_dtype = "int64"

    i = np.zeros(cursor.header.nnz, dtype=index_dtype)
    j = np.zeros(cursor.header.nnz, dtype=index_dtype)
    data = np.zeros(cursor.header.nnz, dtype=_dtypes_by_field.get(cursor.header.field))

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


def read_array(source, parallelism=None) -> numpy.ndarray:
    """
    Read MatrixMarket file into dense NumPy Array, regardless if the file is sparse or dense.

    :param source: path to MatrixMarket file
    :param parallelism: number of threads to use. 0 means auto.
    :return: numpy array
    """

    return _read_body_array(_get_read_cursor(source, parallelism))


def read_triplet(source, parallelism=None):
    return _read_body_triplet(_get_read_cursor(source, parallelism))


def read_scipy(source, parallelism=None):
    cursor = _get_read_cursor(source, parallelism)

    if cursor.header.format == "array":
        return _read_body_array(cursor)
    else:
        from scipy.sparse import coo_matrix
        triplet, shape = _read_body_triplet(cursor)
        return coo_matrix(triplet, shape=shape)


def write_scipy(target, a, comment='', field=None, precision=None, symmetry=None, parallelism=None):
    import numpy as np
    import scipy.sparse

    cursor = _get_write_cursor(target, comment=comment, parallelism=parallelism)

    if isinstance(a, np.ndarray):
        _core.write_array(cursor, a)
        # TODO: remove this:
        if target is None:
            return cursor.get_string()

    if isinstance(a, scipy.sparse.coo_matrix):
        raise


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
