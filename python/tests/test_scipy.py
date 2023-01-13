# Copyright (C) 2022-2023 Adam Lugowski. All rights reserved.
# Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

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
                if "vector" in mtx_name:
                    # SciPy loader does not support vector files
                    with self.assertRaises((ValueError, TypeError)):
                        _ = scipy.io.mmread(mtx)
                    continue

                m = scipy.io.mmread(mtx)
                if str(mtx).endswith(".gz"):
                    # TODO: fmm does not handle GZip files yet
                    # import gzip
                    # stream = gzip.open(filespec, mode)
                    mtx = str(mtx).rstrip(".gz")
                    continue
                if str(mtx).endswith(".bz2"):
                    # TODO: fmm does not handle GZip files yet
                    # import bz2
                    # stream = bz2.BZ2File(filespec, 'rb')
                    mtx = str(mtx).rstrip(".bz2")
                    continue
                header = fmm.read_header(mtx)
                self.assertEqual(m.shape, header.shape)

                m_ffm = fmm.read_scipy(mtx)

                if "array" in mtx_name:
                    np.testing.assert_almost_equal(m, m_ffm)
                else:
                    self.assertEqual(m.shape, m_ffm.shape)
                    np.testing.assert_almost_equal(m.row, m_ffm.row)
                    np.testing.assert_almost_equal(m.col, m_ffm.col)
                    np.testing.assert_almost_equal(m.data, m_ffm.data)

    def test_scipy_crashes(self):
        for mtx in sorted(list((matrices / "scipy_crashes").glob("*.mtx*"))):
            mtx_name = str(mtx.stem)

            with self.subTest(msg=mtx_name):
                # Verify SciPy has not been updated to handle this file
                with self.assertRaises((ValueError, TypeError)):
                    _ = scipy.io.mmread(mtx)

                # Verify fast_matrix_market can read the file
                m_ffm = fmm.read_scipy(mtx)
                self.assertGreater(m_ffm.shape[0], 0)
                self.assertGreater(m_ffm.shape[1], 0)


if __name__ == '__main__':
    unittest.main()
