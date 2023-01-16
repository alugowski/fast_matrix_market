# Copyright (C) 2022-2023 Adam Lugowski. All rights reserved.
# Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
import io
import os

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


class TextToBytesWrapper(io.BufferedReader):
    """Convert a TextIOBase string stream to a byte stream."""

    def __init__(self, text_io_buffer, encoding=None, errors=None, **kwargs):
        super(TextToBytesWrapper, self).__init__(text_io_buffer, **kwargs)
        self.encoding = encoding or text_io_buffer.encoding or 'utf-8'
        self.errors = errors or text_io_buffer.errors or 'strict'

    def _encoding_call(self, method_name, *args, **kwargs):
        raw_method = getattr(self.raw, method_name)
        val = raw_method(*args, **kwargs)
        return val.encode(self.encoding, errors=self.errors)

    def read(self, size=-1):
        return self._encoding_call('read', size)

    def read1(self, size=-1):
        return self._encoding_call('read1', size)

    def peek(self, size=-1):
        return self._encoding_call('peek', size)


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
        # It's a file path
        is_path = True
    except TypeError:
        is_path = False

    if is_path:
        path = str(source)
        if path.endswith('.gz'):
            import gzip
            source = gzip.open(path, 'r')
        elif path.endswith('.bz2'):
            import bz2
            source = bz2.BZ2File(path, 'rb')
        else:
            return _core.open_read_file(path, parallelism)

    # Stream object.
    if hasattr(source, "read"):
        if isinstance(source, io.TextIOBase):
            source = TextToBytesWrapper(source)
        return _core.open_read_stream(source, parallelism)
    else:
        raise TypeError("Unknown source type")


def _get_write_cursor(target, h=None, comment=None, parallelism=None, symmetry="general"):
    if parallelism is None:
        parallelism = PARALLELISM

    if not h:
        h = header(comment=comment, symmetry=symmetry)

    try:
        target = os.fspath(target)
        # It's a file path
        return _core.open_write_file(str(target), h, parallelism)
    except TypeError:
        pass

    if hasattr(target, "write"):
        # Stream object.
        return _core.open_write_stream(target, h, parallelism)
    else:
        raise TypeError("Unknown source object")


def read_header(source) -> header:
    """
    Read a Matrix Market header from a file or from a string.

    :param source: filename or string
    :return: parsed header object
    """
    return _get_read_cursor(source, 1).header


def write_header(target, h: header):
    """
    Write a Matrix Market header to a file or a string.

    :param h: header to write
    :param target: if not None, write to this file.
    :return: if target is None then returns a string containing h as if it was written to a file
    """

    _core.write_header_only(_get_write_cursor(target, h, parallelism=1))


def read_array(source, parallelism=None, long_type=False):
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


def _validate_symmetry(symmetry):
    if symmetry is None:
        return "general"

    symmetry = str(symmetry).lower()
    symmetries = ["general", "symmetric", "skew-symmetric", "hermitian"]
    if symmetry not in symmetries:
        raise ValueError("Invalid symmetry. Must be one of: " + ", ".join(symmetries))

    return symmetry


def write_scipy(target, a, comment='', field=None, precision=None, symmetry=None,
                parallelism=None, find_symmetry=False):
    import numpy as np
    import scipy.sparse

    if isinstance(a, list) or isinstance(a, tuple) or hasattr(a, '__array__'):
        a = np.asarray(a)

    if find_symmetry:
        import scipy.io
        try:
            file = scipy.io._mmio.MMFile()
            # noinspection PyProtectedMember
            symmetry = file._get_symmetry(a)
        except AttributeError:
            symmetry = "general"

    symmetry = _validate_symmetry(symmetry)
    cursor = _get_write_cursor(target, comment=comment, parallelism=parallelism, symmetry=symmetry)

    if isinstance(a, np.ndarray):
        # Write dense numpy arrays
        a = _apply_field(a, field, no_pattern=True)
        _core.write_array(cursor, a)
        return

    if scipy.sparse.isspmatrix(a):
        # Write sparse scipy matrices
        if symmetry is not None and symmetry != "general":
            # A symmetric matrix only specifies the elements below the diagonal.
            # Ensure that the matrix satisfies this requirement.
            from scipy.sparse import coo_matrix
            a = a.tocoo()
            lower_triangle_mask = a.row >= a.col
            a = coo_matrix((a.data[lower_triangle_mask],
                            (a.row[lower_triangle_mask],
                             a.col[lower_triangle_mask])), shape=a.shape)

        # CSC and CSR have specialized writers.
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
        return

    raise ValueError("unknown matrix type: %s" % type(a))


mmread = read_scipy
mmwrite = write_scipy


def mminfo(source):
    h = read_header(source)
    return h.nrows, h.ncols, h.nnz, h.format, h.field, h.symmetry


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
