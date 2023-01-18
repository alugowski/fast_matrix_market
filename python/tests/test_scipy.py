# Copyright (C) 2022-2023 Adam Lugowski. All rights reserved.
# Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
import warnings
from io import BytesIO, StringIO
from pathlib import Path
import unittest

import numpy as np
import scipy.io

import fast_matrix_market as fmm

matrices = Path("matrices")
cpp_matrices = matrices / ".." / ".." / ".." / "tests" / "matrices"


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
                self.assertEqual(lhs.dtype, rhs.dtype)
            np.testing.assert_almost_equal(lhs, rhs)
            return

        # Sparse.
        # Avoid failing on different ordering by using a CSC with sorted indices
        lhs_csc = lhs.tocsc().sorted_indices()
        rhs_csc = rhs.tocsc().sorted_indices()

        if types:
            self.assertEqual(lhs_csc.indptr.dtype, rhs_csc.indptr.dtype)
            self.assertEqual(lhs_csc.indices.dtype, rhs_csc.indices.dtype)
            self.assertEqual(lhs_csc.data.dtype, rhs_csc.data.dtype)
        np.testing.assert_almost_equal(lhs_csc.indptr, rhs_csc.indptr, err_msg="indptr")
        np.testing.assert_almost_equal(lhs_csc.indices, rhs_csc.indices, err_msg="indices")
        np.testing.assert_almost_equal(lhs_csc.data, rhs_csc.data, err_msg="data")

    def test_mminfo(self):
        for mtx in sorted(list(matrices.glob("*.mtx*"))):
            with self.subTest(msg=mtx.stem):
                scipy_info = scipy.io.mminfo(mtx)
                fmm_info = fmm.mminfo(mtx)
                self.assertEqual(scipy_info, fmm_info)

    def test_read(self):
        for mtx in sorted(list(matrices.glob("*.mtx*"))):
            with self.subTest(msg=mtx.stem):
                m = scipy.io.mmread(mtx)
                header = fmm.read_header(mtx)
                m_fmm = fmm.read_scipy(mtx)
                self.assertEqual(m.shape, header.shape)
                self.assertEqual(m.shape, m_fmm.shape)

                self.assertMatrixEqual(m, m_fmm)

    def test_read_array_or_coo(self):
        for mtx in sorted(list(matrices.glob("*.mtx*"))):
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

    def test_write(self):
        for mtx in sorted(list(matrices.glob("*.mtx"))):
            mtx_header = fmm.read_header(mtx)
            with self.subTest(msg=mtx.stem):
                m = scipy.io.mmread(mtx)
                m_fmm = fmm.read_scipy(mtx)

                bio = BytesIO()
                fmm.mmwrite(bio, m_fmm, field=("pattern" if mtx_header.field == "pattern" else None))
                fmms = bio.getvalue().decode()

                if mtx_header.field == "pattern":
                    # Make sure pattern header is written
                    self.assertIn("pattern", fmms)

                m2 = scipy.io.mmread(StringIO(fmms))

                self.assertMatrixEqual(m, m2)

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

    def test_write_fields(self):
        for mtx in sorted(list(matrices.glob("*.mtx"))):
            mtx_header = fmm.read_header(mtx)
            mat = fmm.read_scipy(mtx)

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
                    except OverflowError:
                        if mtx_header.field != "integer" and field == "integer":
                            continue

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


if __name__ == '__main__':
    unittest.main()
