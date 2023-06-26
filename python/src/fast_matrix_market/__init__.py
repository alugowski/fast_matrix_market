# Copyright (C) 2022-2023 Adam Lugowski. All rights reserved.
# Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
# SPDX-License-Identifier: BSD-2-Clause
"""
Read and write Matrix Market files.

Supports sparse coo/triplet matrices, sparse scipy matrices, and numpy array dense matrices.
"""
import io
import os

from . import _core  # type: ignore

__version__ = _core.__version__
header = _core.header

__all__ = [
    "read_header", "write_header",
    "read_array", "write_array", "read_coo", "write_coo", "read_array_or_coo",
    "mminfo", "mmread", "mmwrite", "read_scipy", "write_scipy"]

PARALLELISM = 0
"""
Default value for the parallelism argument to mmread() and mmwrite().
0 means number of CPUs in the system.
"""

ALWAYS_FIND_SYMMETRY = False
"""
Whether mmwrite() with symmetry='AUTO' will always search for symmetry inside the matrix.
This is scipy.io._mmio.mmwrite()'s default behavior, but has a significant performance cost on large matrices.
"""

_field_to_dtype = {
    "integer": "int64",
    "unsigned-integer": "uint64",
    "real": "float64",
    "complex": "complex",
    "pattern": "float64",

    "long-integer": "int64",
    "long-unsigned-integer": "uint64",
    "long-real": "longdouble",
    "long-complex": "longcomplex",
    "long-pattern": "longdouble",
}


class _TextToBytesWrapper(io.BufferedReader):
    """
    Convert a TextIOBase string stream to a byte stream.
    """

    def __init__(self, text_io_buffer, encoding=None, errors=None, **kwargs):
        super(_TextToBytesWrapper, self).__init__(text_io_buffer, **kwargs)
        self.encoding = encoding or text_io_buffer.encoding or 'utf-8'
        self.errors = errors or text_io_buffer.errors or 'strict'

    def __del__(self):
        # do not close the wrapped stream
        self.detach()

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

    def seek(self, offset, whence=0):
        # Random seeks are not allowed because of non-trivial conversion between byte and character offsets,
        # with the possibility of a byte offset landing within a character.
        if offset == 0 and whence == 0 or \
           offset == 0 and whence == 2:
            # seek to start or end is ok
            super(_TextToBytesWrapper, self).seek(offset, whence)
        else:
            # Drop any other seek
            # In this application this may happen when pystreambuf seeks during sync(), which can happen when closing
            # a partially-read stream. Ex. when mminfo() only reads the header then exits.
            pass


def _read_body_array(cursor, long_type):
    import numpy as np
    vals = np.zeros(cursor.header.shape, dtype=_field_to_dtype.get(("long-" if long_type else "")+cursor.header.field))
    _core.read_body_array(cursor, vals)
    return vals


def _read_body_coo(cursor, long_type, generalize_symmetry=True):
    import numpy as np

    index_dtype = "int32"
    if cursor.header.nrows >= 2**31 or cursor.header.ncols >= 2**31:
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
            source = gzip.GzipFile(path, 'r')
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
        if isinstance(target, io.TextIOBase):
            raise TypeError("target stream must be open in binary mode")
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
    cursor = _get_read_cursor(source, 1)
    h = cursor.header
    cursor.close()
    return h


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
    :param long_type: Whether to use 'longdouble' and 'longcomplex' extended-precision floating-point number types.
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
    _core.write_body_array(cursor, a)


def read_coo(source, parallelism=None, long_type=False, generalize_symmetry=True):
    """
    Read MatrixMarket file to a (data, (i, j)) triplet, regardless if the file is sparse or dense.

    :param source: path to MatrixMarket file or open file-like object
    :param parallelism: number of threads to use. 0 means auto.
    :param long_type: Whether to use 'longdouble' and 'longcomplex' extended-precision floating-point number types.
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
    _core.write_body_coo(cursor, shape, row, col, data)


def read_array_or_coo(source, parallelism=None, long_type=False, generalize_symmetry=True):
    """
    Read MatrixMarket file. If the file is dense, return a 2D numpy array. Else return a coordinate matrix.

    This is the same as read_array() if the file is dense, and read_coo() if the file is sparse.

    :param source: path to MatrixMarket file or open file-like object
    :param parallelism: number of threads to use. 0 means auto.
    :param long_type: Whether to use 'longdouble' and 'longcomplex' extended-precision floating-point number types.
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


def mmread(source, parallelism=None, long_type=False):
    """
    Read MatrixMarket file. If the file is dense, return a 2D numpy array. Else return a SciPy sparse matrix.

    Interchangeable with scipy.io.mmread() but faster and supports longdouble.

    :param source: path to MatrixMarket file or open file-like object
    :param parallelism: number of threads to use. 0 means auto.
    :param long_type: Whether to use 'longdouble' and 'longcomplex' extended-precision floating-point number types.
    :return: an ndarray if the MatrixMarket file is dense, a scipy.sparse.coo_matrix if the MatrixMarket file is sparse.
    """
    cursor = _get_read_cursor(source, parallelism)

    if cursor.header.format == "array":
        return _read_body_array(cursor, long_type=long_type)
    else:
        from scipy.sparse import coo_matrix
        triplet, shape = _read_body_coo(cursor, long_type=long_type, generalize_symmetry=True)
        return coo_matrix(triplet, shape=shape)


def mmwrite(target, a, comment=None, field=None, precision=None, symmetry="AUTO",
            parallelism=None, find_symmetry=False):
    """
    Write a matrix to a MatrixMarket file or file-like object.

    Interchangeable with scipy.io.mmwrite() but faster.

    :param target: path to MatrixMarket file or open file-like object
    :param a: a 2D ndarray (or an array convertible to one) or a scipy.sparse matrix
    :param comment: comment to include in the MatrixMarket header
    :param field: convert matrix values to this MatrixMarket field. "pattern" means write
    only the nonzero structure and no values.
    :param precision: floating-point precision to use. If None then use shortest representation.
    :param symmetry: if not None then the matrix is written as having this MatrixMarket symmetry.
    :param parallelism: number of threads to use. 0 means auto.
    :param find_symmetry: autodetect what symmetry the matrix contains and set the `symmetry` field accordingly. This
    can be slow. scipy.io.mmwrite always does this if symmetry is not set, but it is very slow on large matrices.
    """
    import numpy as np
    import scipy.sparse

    if isinstance(a, list) or isinstance(a, tuple) or hasattr(a, "__array__"):
        a = np.asarray(a)

    if symmetry == "AUTO":
        if ALWAYS_FIND_SYMMETRY or (hasattr(a, "shape") and max(a.shape) < 100):
            symmetry = None
        else:
            symmetry = "general"

    if symmetry is None or find_symmetry:
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
        _core.write_body_array(cursor, a)
        return

    # handle both scipy.sparse.*_matrix and scipy.sparse.*_array
    # Both have the same interface as far as this method is concerned, so let duck typing do its thing.
    # Support for these types varies between scipy versions, so attempt to support all possibilities.
    is_sparse = False
    is_compressed = False
    coo_type = None
    csr_types = []

    # check for *_matrix
    try:
        if scipy.sparse.isspmatrix(a):
            is_sparse = True
            from scipy.sparse import coo_matrix
            coo_type = coo_matrix
            # CSC and CSR have specialized writers.
            is_compressed = (isinstance(a, scipy.sparse.csc_matrix) or isinstance(a, scipy.sparse.csr_matrix))
            csr_types.append(scipy.sparse.csr_matrix)
    except ImportError:
        pass

    # check for *_array
    try:
        if scipy.sparse.issparse(a):
            is_sparse = True
            from scipy.sparse import coo_array
            coo_type = coo_array
            # CSC and CSR have specialized writers. The type may already be a cs*_matrix.
            is_compressed = is_compressed or \
                            (isinstance(a, scipy.sparse.csc_array) or isinstance(a, scipy.sparse.csr_array))
            csr_types.append(scipy.sparse.csr_array)
    except ImportError:
        pass

    if is_sparse:
        # Write sparse scipy matrices
        if symmetry is not None and symmetry != "general":
            # A symmetric matrix only specifies the elements below the diagonal.
            # Ensure that the matrix satisfies this requirement.

            a = a.tocoo()
            lower_triangle_mask = a.row >= a.col
            a = coo_type((a.data[lower_triangle_mask],
                          (a.row[lower_triangle_mask],
                           a.col[lower_triangle_mask])), shape=a.shape)
            is_compressed = False

        if not is_compressed:
            # convert everything except CSC/CSR to coo
            a = a.tocoo()

        data = _apply_field(a.data, field)

        if is_compressed:
            # CSC and CSR can be written directly
            is_csr = any([isinstance(a, t) for t in csr_types])
            _core.write_body_csc(cursor, a.shape, a.indptr, a.indices, data, is_csr)
        else:
            _core.write_body_coo(cursor, a.shape, a.row, a.col, data)
        return

    raise ValueError("unknown matrix type: %s" % type(a))


def mminfo(source):
    """
    Same as scipy.io.mminfo()

    :param source: a Matrix Market file path or an open file-like object
    :return: a tuple of (# of rows, #of columns, #of entries, "coordinate" or "array", field type, symmetry type)
    """
    h = read_header(source)
    return h.nrows, h.ncols, h.nnz, h.format, h.field, h.symmetry


read_scipy = mmread
write_scipy = mmwrite
