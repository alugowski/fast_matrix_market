# Copyright (C) 2022-2023 Adam Lugowski. All rights reserved.
# Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
# SPDX-License-Identifier: BSD-2-Clause

import warnings
from io import BytesIO, StringIO
from pathlib import Path
import tempfile
import unittest

try:
    import numpy as np
except ImportError:
    np = None

try:
    import scipy
    import scipy.io  # for Python 3.7
    import scipy.sparse  # for Python 3.7
except ImportError:
    scipy = None

try:
    import bz2
except ImportError:
    bz2 = None

import fast_matrix_market as fmm

matrices = Path("matrices")
cpp_matrices = matrices / ".." / ".." / ".." / "tests" / "matrices"


@unittest.skipIf(scipy is None, "SciPy not installed")
class TestSciPy(unittest.TestCase):
    """
    Test compatibility with SciPy
    """

    def assertMatrixEqual(self, lhs, rhs, types=True):
        """
        Assert matrices are equal. Can be dense numpy arrays or sparse scipy matrices.
        """
        self.assertEqual(lhs.shape, rhs.shape)
        if isinstance(lhs, np.ndarray):
            self.assertEqual(type(lhs), type(rhs))
            if types:
                if np.dtype('intp') == np.dtype('int32'):
                    # 32-bit system. scipy.io._mmio will load integer arrays as int32
                    if lhs.dtype == np.dtype('int32'):
                        lhs = lhs.astype('int64')
                    if rhs.dtype == np.dtype('int32'):
                        rhs = rhs.astype('int64')

                self.assertEqual(lhs.dtype, rhs.dtype)
            np.testing.assert_almost_equal(lhs, rhs)
            return

        # Sparse.
        # Avoid failing on different ordering by using a CSC with sorted indices
        lhs_csc = lhs.tocsc().sorted_indices()
        rhs_csc = rhs.tocsc().sorted_indices()

        if types:
            if np.dtype('intp') == np.dtype('int32'):
                # 32-bit system. scipy.io._mmio will load integer arrays as int32
                if lhs_csc.data.dtype == np.dtype('int32'):
                    lhs_csc.data = lhs_csc.data.astype('int64')
                if rhs_csc.data.dtype == np.dtype('int32'):
                    rhs_csc.data = rhs_csc.data.astype('int64')

            self.assertEqual(lhs_csc.indptr.dtype, rhs_csc.indptr.dtype)
            self.assertEqual(lhs_csc.indices.dtype, rhs_csc.indices.dtype)
            self.assertEqual(lhs_csc.data.dtype, rhs_csc.data.dtype)
        np.testing.assert_almost_equal(lhs_csc.indptr, rhs_csc.indptr, err_msg="indptr")
        np.testing.assert_almost_equal(lhs_csc.indices, rhs_csc.indices, err_msg="indices")
        np.testing.assert_almost_equal(lhs_csc.data, rhs_csc.data, err_msg="data")

    def test_mminfo(self):
        for mtx in sorted(list(matrices.glob("*.mtx*"))):
            if str(mtx).endswith(".bz2") and bz2 is None:
                continue

            with self.subTest(msg=mtx.stem):
                scipy_info = scipy.io.mminfo(mtx)
                fmm_info = fmm.mminfo(mtx)
                self.assertEqual(scipy_info, fmm_info)

    def test_mminfo_docstring(self):
        text = '''%%MatrixMarket matrix coordinate real general
         5 5 7
         2 3 1.0
         3 4 2.0
         3 5 3.0
         4 1 4.0
         4 2 5.0
         4 3 6.0
         4 4 7.0
        '''
        info = fmm.mminfo(StringIO(text))
        self.assertEqual(info, (5, 5, 7, 'coordinate', 'real', 'general'))

    def test_read(self):
        for mtx in sorted(list(matrices.glob("*.mtx*"))):
            if str(mtx).endswith(".bz2") and bz2 is None:
                continue

            with self.subTest(msg=mtx.stem):
                m = scipy.io.mmread(mtx)
                header = fmm.read_header(mtx)
                m_fmm = fmm.read_scipy(mtx)
                self.assertEqual(m.shape, header.shape)
                self.assertEqual(m.shape, m_fmm.shape)

                self.assertMatrixEqual(m, m_fmm)

    def test_read_array_or_coo(self):
        for mtx in sorted(list(matrices.glob("*.mtx*"))):
            if str(mtx).endswith(".bz2") and bz2 is None:
                continue

            with self.subTest(msg=mtx.stem):
                m = scipy.io.mmread(mtx)
                header = fmm.read_header(mtx)
                dense_or_sparse, shape = fmm.read_array_or_coo(mtx)
                if isinstance(dense_or_sparse, np.ndarray):
                    m_fmm = dense_or_sparse
                else:
                    m_fmm = scipy.sparse.coo_matrix(dense_or_sparse, shape)
                self.assertEqual(m.shape, header.shape)
                self.assertEqual(m.shape, m_fmm.shape)

                self.assertMatrixEqual(m, m_fmm)

    @unittest.skipIf(scipy is None or not scipy.__version__.startswith("1.11"),
                     "SciPy 1.12 ships FMM so this no longer crashes.")
    def test_scipy_crashes(self):
        for mtx in sorted(list((matrices / "scipy_crashes").glob("*.mtx*"))):
            with self.subTest(msg=mtx.stem):
                # Verify SciPy has not been updated to handle this file
                # noinspection PyTypeChecker
                with self.assertRaises((ValueError, TypeError, OverflowError)), warnings.catch_warnings():
                    warnings.filterwarnings('ignore')
                    _ = scipy.io.mmread(mtx)

                # Verify fast_matrix_market can read the file
                m_fmm = fmm.read_scipy(mtx)
                self.assertGreater(m_fmm.shape[0], 0)
                self.assertGreater(m_fmm.shape[1], 0)

    @unittest.skipIf(not cpp_matrices.exists(), "Matrices from C++ code not available.")
    def test_read_windows_lineendings(self):
        path = cpp_matrices / "permissive" / "windows_lineendings_nist_ex1_more_freeformat.mtx"

        fmm.mmread(path)
        # scipy.io._mmio.mmread() cannot read CRLF files on Unix

    def test_write(self):
        for mtx in sorted(list(matrices.glob("*.mtx"))):
            mtx_header = fmm.read_header(mtx)
            with self.subTest(msg=mtx.stem):
                m = scipy.io.mmread(mtx)
                m_fmm = fmm.read_scipy(mtx)

                bio = BytesIO()
                fmm.mmwrite(bio, m_fmm, field=("pattern" if mtx_header.field == "pattern" else None),
                            symmetry="general")
                fmms = bio.getvalue().decode()

                if mtx_header.field == "pattern":
                    # Make sure pattern header is written
                    self.assertIn("pattern", fmms)

                m2 = scipy.io.mmread(StringIO(fmms))

                self.assertMatrixEqual(m, m2)

                if mtx_header.field in ["integer", "pattern"]:
                    # should match byte for byte, except for reals
                    try:
                        bio = BytesIO()
                        scipy.io.mmwrite(bio, m_fmm, field=("pattern" if mtx_header.field == "pattern" else None),
                                         symmetry="general")
                        scipy_s = bio.getvalue().decode()
                        self.assertEqual(fmms, scipy_s)
                    except OverflowError:
                        # Some matrices overflow on 32-bit platforms (scipy.io._mmio only).
                        pass

    def test_write_formats(self):
        for mtx in sorted(list(matrices.glob("*.mtx"))):
            mtx_header = fmm.read_header(mtx)
            if mtx_header.format != "coordinate":
                continue

            m = scipy.io.mmread(mtx)
            m_fmm = fmm.read_scipy(mtx)

            formats = {
                "coo": m_fmm.tocoo(),
                "csc": m_fmm.tocsc(),
                "csr": m_fmm.tocsr(),
                "bsr": m_fmm.tobsr(),
                "dok": m_fmm.todok(),
                "dia": m_fmm.todia(),
                "lil": m_fmm.tolil(),
            }
            for name, m_fmm in formats.items():
                with self.subTest(msg=f"{mtx.stem} - {name}"):
                    bio = BytesIO()
                    fmm.mmwrite(bio, m_fmm, field=("pattern" if mtx_header.field == "pattern" else None))
                    fmms = bio.getvalue().decode()

                    if mtx_header.field == "pattern":
                        # Make sure pattern header is written
                        self.assertIn("pattern", fmms)

                    m2 = scipy.io.mmread(StringIO(fmms))

                    self.assertMatrixEqual(m, m2)

    def test_write_array_formats(self):
        try:
            from scipy.sparse import coo_array
        except ImportError:
            self.skipTest("No scipy.sparse.coo_array")
            return

        rows = [0, 1, 2]
        cols = [0, 1, 2]
        data = [1.0, 2.0, 3.0]
        coo = coo_array((data, (rows, cols)), shape=(3, 3))

        formats = {
            "coo": coo.tocoo(),
            "csc": coo.tocsc(),
            "csr": coo.tocsr(),
            "bsr": coo.tobsr(),
            "dok": coo.todok(),
            "dia": coo.todia(),
            "lil": coo.tolil(),
        }

        for name, a in formats.items():
            with self.subTest(msg=f"{name}"):
                bio = BytesIO()
                fmm.mmwrite(bio, a)
                fmms = bio.getvalue().decode()

                m2 = scipy.io.mmread(StringIO(fmms))

                self.assertMatrixEqual(a.toarray(), m2.toarray())

    def test_write_fields(self):
        for mtx in sorted(list(matrices.glob("*.mtx"))):
            mtx_header = fmm.read_header(mtx)
            mat = scipy.io.mmread(mtx)

            for field in ["integer", "real", "complex", "pattern"]:
                if mtx_header.format == "array" and field == "pattern":
                    continue

                with self.subTest(msg=f"{mtx.stem} to field={field}"), warnings.catch_warnings():
                    # Converting complex to real raises a warning
                    warnings.simplefilter('ignore', np.ComplexWarning)

                    # write scipy with this field
                    try:
                        sio = BytesIO()
                        scipy.io.mmwrite(sio, mat, field=field)
                        scipys = sio.getvalue().decode("latin1")
                    except OverflowError as e:
                        if mtx_header.field != "integer" and field == "integer":
                            continue
                        else:
                            raise e

                    # Write FMM with this field
                    bio = BytesIO()
                    fmm.mmwrite(bio, mat, field=field)
                    fmms = bio.getvalue().decode()

                    # verify the reads come up with the same types
                    m = scipy.io.mmread(StringIO(scipys))

                    m2 = fmm.mmread(StringIO(fmms))

                    self.assertMatrixEqual(m, m2)

    def test_long_types(self):
        # verify that this platform's numpy is built with longdouble and longcomplex support
        long_val = "1E310"  # Value just slightly out of range of a 64-bit float
        with warnings.catch_warnings():
            warnings.filterwarnings('ignore')
            d = np.array([long_val], dtype="double")
            ld = np.array([long_val], dtype="longdouble")

        if d[0] == ld[0]:
            self.skipTest("Numpy does not have longdouble support on this platform.")

        for mtx in sorted(list((matrices / "long_double").glob("*.mtx*"))):
            # assert no exception
            # fast_matrix_market throws if value is out of range
            _ = fmm.mmread(mtx, long_type=True)

    def test_invalid(self):
        # use the invalid matrices from the C++ tests
        for mtx in sorted(list((cpp_matrices / "invalid").glob("*.mtx"))):
            with self.subTest(msg=mtx.stem):
                with self.assertRaises(ValueError):
                    fmm.mmread(mtx)

    def test_symmetry_read(self):
        # use the symmetry matrices from the C++ tests
        paths = list((cpp_matrices / "symmetry").glob("*.mtx")) + list((cpp_matrices / "symmetry_array").glob("*.mtx"))
        for mtx in sorted(paths):
            if "_general" in mtx.stem:
                continue
            mtx_general = str(mtx).replace(".mtx", "_general.mtx")

            with self.subTest(msg=mtx.stem):
                m = scipy.io.mmread(mtx)
                m_gen = scipy.io.mmread(mtx_general)
                m_fmm = fmm.mmread(mtx)
                m_fmm_gen = fmm.mmread(mtx_general)

                self.assertMatrixEqual(m, m_gen)
                self.assertMatrixEqual(m, m_fmm)
                self.assertMatrixEqual(m_gen, m_fmm_gen)
                self.assertMatrixEqual(m_fmm, m_fmm_gen)

    def test_symmetry_write(self):
        # use the symmetry matrices from the C++ tests
        paths = list((cpp_matrices / "symmetry").glob("*.mtx")) + list((cpp_matrices / "symmetry_array").glob("*.mtx"))
        for mtx in sorted(paths):
            if "_general" in mtx.stem:
                continue
            mtx_general = str(mtx).replace(".mtx", "_general.mtx")
            mtx_header = fmm.read_header(mtx)

            with self.subTest(msg=mtx.stem):
                m = scipy.io.mmread(mtx)

                bio = BytesIO()
                fmm.mmwrite(bio, m, symmetry=mtx_header.symmetry)
                fmms = bio.getvalue().decode()

                self.assertIn(mtx_header.symmetry, fmms)

                m2_fmm = fmm.mmread(StringIO(fmms))
                m2_scipy = scipy.io.mmread(mtx)
                self.assertMatrixEqual(m, m2_fmm)
                self.assertMatrixEqual(m, m2_scipy)

                general = scipy.io.mmread(mtx_general)
                self.assertMatrixEqual(m2_fmm, general)

    def test_find_symmetry(self):
        # noinspection PyProtectedMember
        if not fmm._has_find_symmetry():
            self.skipTest("find_symmetry not available.")

        # use the symmetry matrices from the C++ tests
        paths = list((cpp_matrices / "symmetry").glob("*.mtx")) + list((cpp_matrices / "symmetry_array").glob("*.mtx"))
        for mtx in sorted(paths):
            if "_general" in mtx.stem:
                continue
            mtx_header = fmm.read_header(mtx)

            with self.subTest(msg=mtx.stem):
                m = scipy.io.mmread(mtx)

                bio = BytesIO()
                fmm.mmwrite(bio, m, find_symmetry=True)
                fmms = bio.getvalue().decode()

                expected_symmetry = mtx_header.symmetry
                if mtx_header.field == "real" and mtx_header.symmetry == "hermitian":
                    expected_symmetry = "symmetric"
                self.assertIn(expected_symmetry, fmms)

    def test_precision(self):
        test_values = [np.pi] + [10**i for i in range(0, -10, -1)]
        test_precisions = range(0, 10)
        for value in test_values:
            for precision in test_precisions:
                with self.subTest(msg=f"value={value}, precision={precision}"):
                    m = np.array([[value]])

                    bio = BytesIO()
                    fmm.mmwrite(bio, m, precision=precision, comment="comment")
                    fmms = bio.getvalue().decode()

                    # Pull out the line that contains the value
                    value_str = fmms.splitlines()[3]
                    self.assertAlmostEqual(float(value_str), float('%%.%dg' % precision % value), places=precision)

                    m2 = fmm.mmread(StringIO(fmms))
                    self.assertAlmostEqual(m2[0][0], float('%%.%dg' % precision % value))

    @unittest.skipIf(not cpp_matrices.exists(), "Matrices from C++ code not available.")
    def test_value_overflow(self):
        with self.subTest("integer"):
            with self.assertRaises(OverflowError):
                # SciPy throws OverflowError for integer values larger than 64-bit
                fmm.mmread(cpp_matrices / "overflow" / "overflow_value_gt_int64.mtx")

            # values larger than 32-bit do not throw
            fmm.mmread(cpp_matrices / "overflow" / "overflow_value_gt_int32.mtx")

        with self.subTest("float"):
            # Python floating-point returns closest match on overflow, matching strtod() behavior if ERANGE is ignored
            m = fmm.mmread(cpp_matrices / "overflow" / "overflow_value_gt_float128.mtx")
            self.assertEqual(m.data[0], float("inf"))

        with self.subTest("complex"):
            m = fmm.mmread(cpp_matrices / "overflow" / "overflow_value_gt_complex128.mtx")
            self.assertEqual(m.data[-1].real, float("inf"))
            self.assertEqual(m.data[-1].imag, float("inf"))

    def test_write_file(self):
        rows = [0, 1, 2]
        cols = [0, 1, 2]
        data = [1.1, 2.2, 3.3]
        mtx = scipy.sparse.coo_matrix((data, (rows, cols)), shape=(3, 3))

        with tempfile.TemporaryDirectory() as temp_dir:
            mtx_path = Path(temp_dir) / "matrix.mtx"

            # Write with filename, read with filename
            fmm.mmwrite(mtx_path, mtx)
            a = fmm.mmread(mtx_path)
            self.assertMatrixEqual(a, mtx)

            # Write with filename, read with file object
            fmm.mmwrite(mtx_path, mtx)
            with open(mtx_path, 'r') as f:
                a = fmm.mmread(f)
            self.assertMatrixEqual(a, mtx)

            # Write with file object (binary), read with filename
            with open(mtx_path, 'wb') as f:
                fmm.mmwrite(f, mtx)
            a = fmm.mmread(mtx_path)
            self.assertMatrixEqual(a, mtx)

            # Write with file object (text)
            # Not allowed. scipy.io._mmio.mmwrite() also throws TypeError here
            with open(mtx_path, 'w') as f:
                with self.assertRaises(TypeError):
                    fmm.mmwrite(f, mtx)

            # Write with file object (binary), read with file object (text)
            with open(mtx_path, 'wb') as f:
                fmm.mmwrite(f, mtx)
            with open(mtx_path, 'r') as f:
                a = fmm.mmread(f)
            self.assertMatrixEqual(a, mtx)

            # Write with file object (binary), read with file object (binary)
            with open(mtx_path, 'wb') as f:
                fmm.mmwrite(f, mtx)
            with open(mtx_path, 'rb') as f:
                a = fmm.mmread(f)
            self.assertMatrixEqual(a, mtx)

    def test_bz2(self):
        try:
            import bz2
        except ImportError:
            self.skipTest(reason="no bz2 module")
            return

        rows = [0, 1, 2]
        cols = [0, 1, 2]
        data = [1.1, 2.2, 3.3]
        mtx = scipy.sparse.coo_matrix((data, (rows, cols)), shape=(3, 3))

        with tempfile.TemporaryDirectory() as temp_dir:
            temp_path = Path(temp_dir)
            mtx_path = temp_path / "matrix.mtx"
            bz2_path = temp_path / "matrix.mtx.bz2"

            fmm.mmwrite(mtx_path, mtx)

            # create bz2
            with open(mtx_path, 'rb') as f_in, bz2.BZ2File(bz2_path, 'wb') as f_out:
                f_out.write(f_in.read())

            a = fmm.mmread(bz2_path)
            self.assertMatrixEqual(a, mtx)

    def test_gzip(self):
        try:
            import gzip
        except ImportError:
            self.skipTest(reason="no gzip module")
            return

        rows = [0, 1, 2]
        cols = [0, 1, 2]
        data = [1.1, 2.2, 3.3]
        mtx = scipy.sparse.coo_matrix((data, (rows, cols)), shape=(3, 3))

        with tempfile.TemporaryDirectory() as temp_dir:
            temp_path = Path(temp_dir)
            mtx_path = temp_path / "matrix.mtx"
            gzip_path = temp_path / "matrix.mtx.gz"

            fmm.mmwrite(mtx_path, mtx)

            # create bz2
            with open(mtx_path, 'rb') as f_in, gzip.GzipFile(gzip_path, 'wb') as f_out:
                f_out.write(f_in.read())

            a = fmm.mmread(gzip_path)
            self.assertMatrixEqual(a, mtx)


if __name__ == '__main__':
    unittest.main()
