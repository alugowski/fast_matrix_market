# Copyright (C) 2022-2023 Adam Lugowski. All rights reserved.
# Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
"""
The ultimate Matrix Market I/O library. Read and write MatrixMarket files.

Supports sparse coo/triplet matrices, sparse scipy matrices, and numpy array dense matrices.
"""
import io
import os

from . import _core
from ._core import __version__, header

PARALLELISM = 0
"""
Number of threads to use. 0 means number of threads in the system.
"""

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


class _TextToBytesWrapper(io.BufferedReader):
    """
    Convert a TextIOBase string stream to a byte stream.
    """

    def __init__(self, text_io_buffer, encoding=None, errors=None, **kwargs):
        super(_TextToBytesWrapper, self).__init__(text_io_buffer, **kwargs)
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


def _read_body_coo(cursor, long_type, generalize_symmetry=True):
    import numpy as np

    index_dtype = "int32"  # SciPy uses this size
    if cursor.header.nrows > 2**31 or cursor.header.ncols > 2**31:
        # Dimensions are too large to fit in int32
        index_dtype = "int64"

    i = np.zeros(cursor.header.nnz, dtype=index_dtype)
    j = np.zeros(cursor.header.nnz, dtype=index_dtype)
    data = np.zeros(cursor.header.nnz, dtype=_field_to_dtype.get(("long-" if long_type else "")+cursor.header.field))

    _core.read_body_coo(cursor, i, j, data)

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
    """
    Open file for reading.
    """
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
            source = _TextToBytesWrapper(source)
        return _core.open_read_stream(source, parallelism)
    else:
        raise TypeError("Unknown source type")


def _get_write_cursor(target, h=None, comment=None, parallelism=None, symmetry="general", precision=None):
    """
    Open file for writing.
    """
    if parallelism is None:
        parallelism = PARALLELISM
    if comment is None:
        comment = ''
    if symmetry is None:
        symmetry = "general"
    if precision is None:
        precision = -1

    if not h:
        h = header(comment=comment, symmetry=symmetry)

    try:
        target = os.fspath(target)
        # It's a file path
        return _core.open_write_file(str(target), h, parallelism, precision)
    except TypeError:
        pass

    if hasattr(target, "write"):
        # Stream object.
        return _core.open_write_stream(target, h, parallelism, precision)
    else:
        raise TypeError("Unknown source object")


def _apply_field(data, field, no_pattern=False):
    """
    Ensure that numpy array is of the type specified by the field
    :param data: array to check
    :param field: MatrixMarket field
    :param no_pattern:
    :return: data if no conversion necessary, or a converted version
    """
    import numpy as np

    if field is None:
        return data
    if field == "pattern":
        if no_pattern:
            return data
        else:
            return np.zeros(0)

    import numpy as np
    dtype = _field_to_dtype.get(field, None)
    if dtype is None:
        raise ValueError("Invalid field.")

    return np.asarray(data, dtype=dtype)


def _validate_symmetry(symmetry):
    """
    Sanitize symmetry parameter.
    """
    if symmetry is None:
        return "general"

    symmetry = str(symmetry).lower()
    symmetries = ["general", "symmetric", "skew-symmetric", "hermitian"]
    if symmetry not in symmetries:
        raise ValueError("Invalid symmetry. Must be one of: " + ", ".join(symmetries))

    return symmetry


def _has_find_symmetry():
    """
    The find_symmetry method borrows scipy's method. This method is private, however, so it may not
    be reliably available. Skip testing it if it's not available on that platform.
    """
    import numpy as np
    # Attempt to use scipy's method for finding matrix symmetry
    import scipy.io
    try:
        # noinspection PyProtectedMember
        _ = scipy.io._mmio.MMFile()._get_symmetry(np.zeros(shape=(1, 1)))
    except AttributeError:
        return False
    return True


def read_header(source) -> header:
    """
    Read a Matrix Market header from a file or open file-like object.

    :param source: filename or open file-like object
    :return: parsed header object
    """
    return _get_read_cursor(source, 1).header


def write_header(target, h: header):
    """
    Write a Matrix Market header to a file or open file-like object.

    :param target: filename or open file-like object.
    :param h: header to write
    """
    _core.write_header_only(_get_write_cursor(target, h, parallelism=1))


def read_array(source, parallelism=None, long_type=False):
    """
    Read MatrixMarket file into dense NumPy Array, regardless if the file is sparse or dense.

    :param source: path to MatrixMarket file or open file-like object
    :param parallelism: number of threads to use. 0 means auto.
    :param long_type: Use long floating point datatypes (if available in your NumPy).
    This means longdouble and longcomplex instead of float64 and complex64.
    :return: numpy array
    """
    return _read_body_array(_get_read_cursor(source, parallelism), long_type=long_type)


def write_array(target, a, comment=None, parallelism=None):
    """
    Write 2D array to a MatrixMarket file (or file-like object).

    :param target: path to MatrixMarket file or open file-like object
    :param a: convertible to a Numpy array.
    :param comment: comment to include in the MatrixMarket header
    :param parallelism: number of threads to use, None means auto.
    """
    import numpy as np
    a = np.asarray(a)
    cursor = _get_write_cursor(target, comment=comment, parallelism=parallelism)
    _core.write_array(cursor, a)


def read_coo(source, parallelism=None, long_type=False, generalize_symmetry=True):
    """
    Read MatrixMarket file to a (data, (i, j)) triplet, regardless if the file is sparse or dense.

    :param source: path to MatrixMarket file or open file-like object
    :param parallelism: number of threads to use. 0 means auto.
    :param long_type: Use long floating point datatypes (if available in your NumPy).
    This means longdouble and longcomplex instead of float64 and complex64.
    :param generalize_symmetry: if the MatrixMarket file specifies a symmetry, emit the symmetric entries too.
    :return: (data, (row_indices, column_indices)) (same as scipy.io.mmread)
    """
    (data, (rows, cols)), shape = _read_body_coo(_get_read_cursor(source, parallelism),
                                                 long_type=long_type, generalize_symmetry=generalize_symmetry)
    return (data, (rows, cols)), shape


def write_coo(target, a, shape, comment=None, parallelism=None):
    """
    Write a (data, (i, j)) triplet into a MatrixMarket file or file-like object.

    :param target: path to MatrixMarket file or open file-like object
    :param a: (data, (i, j)) triplet of arrays
    :param shape: tuple of (nrows, ncols)
    :param comment: any comment to include in the MatrixMarket header
    :param parallelism: number of threads to use. 0 means auto.
    """
    if len(shape) != 2:
        raise ValueError("shape needs to be: (# of rows, # of columns)")

    # unpack a
    data, (row, col) = a

    cursor = _get_write_cursor(target, comment=comment, parallelism=parallelism)
    _core.write_coo(cursor, shape, row, col, data)


def read_array_or_coo(source, parallelism=None, long_type=False, generalize_symmetry=True):
    """
    Read MatrixMarket file. If the file is dense, return a 2D numpy array. Else return a coordinate matrix.

    This is the same as read_array() if the file is dense, and read_coo() if the file is sparse.

    :param source: path to MatrixMarket file or open file-like object
    :param parallelism: number of threads to use. 0 means auto.
    :param long_type: Use long floating point datatypes (if available in your NumPy).
    This means longdouble and longcomplex instead of float64 and complex64.
    :param generalize_symmetry: if the MatrixMarket file specifies a symmetry, emit the symmetric entries too.
    Always True for dense files.
    :return: a tuple of (matrix, shape), where matrix is an ndarray if the MatrixMarket file is dense,
    a triplet tuple if the MatrixMarket file is sparse.
    """
    cursor = _get_read_cursor(source, parallelism)

    if cursor.header.format == "array":
        arr = _read_body_array(cursor, long_type=long_type)
        return arr, arr.shape
    else:
        (data, (rows, cols)), shape = _read_body_coo(cursor, long_type=long_type,
                                                     generalize_symmetry=generalize_symmetry)
        return (data, (rows, cols)), shape


def read_scipy(source, parallelism=None, long_type=False):
    """
    Read MatrixMarket file. If the file is dense, return a 2D numpy array. Else return a SciPy sparse matrix.

    Interchangeable with scipy.io.mmread() but faster and supports longdouble.

    :param source: path to MatrixMarket file or open file-like object
    :param parallelism: number of threads to use. 0 means auto.
    :param long_type: Use long floating point datatypes (if available in your NumPy).
    This means longdouble and longcomplex instead of float64 and complex64.
    :return: an ndarray if the MatrixMarket file is dense, a scipy.sparse.coo_matrix if the MatrixMarket file is sparse.
    """
    cursor = _get_read_cursor(source, parallelism)

    if cursor.header.format == "array":
        return _read_body_array(cursor, long_type=long_type)
    else:
        from scipy.sparse import coo_matrix
        triplet, shape = _read_body_coo(cursor, long_type=long_type, generalize_symmetry=True)
        return coo_matrix(triplet, shape=shape)


def write_scipy(target, a, comment=None, field=None, precision=None, symmetry=None,
                parallelism=None, find_symmetry=False):
    """
    Write a matrix to a MatrixMarket file or file-like object.

    Interchangeable with scipy.io.mmwrite() but faster.

    :param target: path to MatrixMarket file or open file-like object
    :param a: a 2D ndarray (or an array convertible to one) or a scipy.sparse matrix
    :param comment: comment to include in the MatrixMarket header
    :param field: convert matrix values to this MatrixMarket field
    :param precision: ignored
    :param symmetry: if not None then the matrix is written as having this MatrixMarket symmetry. "pattern" means write
    only the nonzero structure and no values.
    :param parallelism: number of threads to use. 0 means auto.
    :param find_symmetry: autodetect what symmetry the matrix contains and set the `symmetry` field accordingly. This
    can be slow. scipy.io.mmwrite always does this if symmetry is not set, but it is very slow on large matrices.
    """
    import numpy as np
    import scipy.sparse

    if isinstance(a, list) or isinstance(a, tuple) or hasattr(a, '__array__'):
        a = np.asarray(a)

    if find_symmetry:
        # Attempt to use scipy's method for finding matrix symmetry
        import scipy.io
        try:
            # noinspection PyProtectedMember
            symmetry = scipy.io._mmio.MMFile()._get_symmetry(a)
        except AttributeError:
            symmetry = "general"

    symmetry = _validate_symmetry(symmetry)
    cursor = _get_write_cursor(target, comment=comment, parallelism=parallelism, precision=precision, symmetry=symmetry)

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
            # convert everything except CSC/CSR to coo
            a = a.tocoo()

        data = _apply_field(a.data, field)

        if is_compressed:
            # CSC and CSR can be written directly
            is_csr = isinstance(a, scipy.sparse.csr_matrix)
            _core.write_csc(cursor, a.shape, a.indptr, a.indices, data, is_csr)
        else:
            _core.write_coo(cursor, a.shape, a.row, a.col, data)
        return

    raise ValueError("unknown matrix type: %s" % type(a))


mmread = read_scipy
mmwrite = write_scipy


def mminfo(source):
    """
    Same as scipy.io.mminfo()

    :param source: a Matrix Market file path or an open file-like object
    :return: a tuple of (# of rows, #of columns, #of entries, "coordinate" or "array", field type, symmetry type)
    """
    h = read_header(source)
    return h.nrows, h.ncols, h.nnz, h.format, h.field, h.symmetry
