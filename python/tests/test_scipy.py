# Copyright (C) 2022-2023 Adam Lugowski. All rights reserved.
# Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
from io import BytesIO, StringIO
from pathlib import Path
import unittest

import numpy as np
import scipy.io

import fast_matrix_market as fmm

matrices = Path("matrices")


class TestSciPy(unittest.TestCase):
    """
    Test compatibility with SciPy
    """
    def test_read_types(self):
        for mtx in sorted(list(matrices.glob("*.mtx*"))):
            mtx_name = str(mtx.stem)
            with self.subTest(msg=mtx_name):
                if str(mtx).endswith(".gz"):
                    # TODO: fmm does not handle GZip files yet
                    # import gzip
                    # stream = gzip.open(filespec, mode)
                    continue
                    # self.skipTest("no gz")
                if str(mtx).endswith(".bz2"):
                    # TODO: fmm does not handle BZ2 files yet
                    # import bz2
                    # stream = bz2.BZ2File(filespec, 'rb')
                    continue
                    # self.skipTest("no bz2")

                m = scipy.io.mmread(mtx)
                header = fmm.read_header(mtx)
                m_fmm = fmm.read_scipy(mtx)
                self.assertEqual(m.shape, header.shape)
                self.assertEqual(m.shape, m_fmm.shape)

                if "array" in mtx_name:
                    np.testing.assert_almost_equal(m, m_fmm)
                else:
                    self.assertEqual(m.shape, m_fmm.shape)
                    np.testing.assert_almost_equal(m.row, m_fmm.row)
                    np.testing.assert_almost_equal(m.col, m_fmm.col)
                    np.testing.assert_almost_equal(m.data, m_fmm.data)

    def test_scipy_crashes(self):
        for mtx in sorted(list((matrices / "scipy_crashes").glob("*.mtx*"))):
            mtx_name = str(mtx.stem)

            with self.subTest(msg=mtx_name):
                # Verify SciPy has not been updated to handle this file
                # noinspection PyTypeChecker
                with self.assertRaises((ValueError, TypeError)):
                    _ = scipy.io.mmread(mtx)

                # Verify fast_matrix_market can read the file
                m_fmm = fmm.read_scipy(mtx)
                self.assertGreater(m_fmm.shape[0], 0)
                self.assertGreater(m_fmm.shape[1], 0)

    def test_scipy_write(self):
        for mtx in sorted(list(matrices.glob("*.mtx"))):
            mtx_name = str(mtx.stem)
            with self.subTest(msg=mtx_name):
                m = scipy.io.mmread(mtx)
                m_fmm = fmm.read_scipy(mtx)
                # m_fmm = m_fmm.tocsc()
                fmms = fmm.write_scipy(None, m_fmm, field=("pattern" if "pattern" in mtx_name else None))

                if "pattern" in mtx_name:
                    # Make sure pattern header is written
                    self.assertIn("pattern", fmms)

                m2 = scipy.io.mmread(StringIO(fmms))

                if "array" in mtx.stem:
                    np.testing.assert_almost_equal(m, m2)
                else:
                    self.assertEqual(m.shape, m2.shape)
                    self.assertEqual(m.data.dtype, m2.data.dtype)
                    np.testing.assert_almost_equal(m.row, m2.row)
                    np.testing.assert_almost_equal(m.col, m2.col)
                    np.testing.assert_almost_equal(m.data, m2.data)
        # TODO: writing symmetry
        # TODO: casting types to particular field
        # TODO: write CSC/CSR tests
        # TODO: write other scipy matrix types


if __name__ == '__main__':
    unittest.main()
