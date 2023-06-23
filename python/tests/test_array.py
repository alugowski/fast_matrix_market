# Copyright (C) 2022-2023 Adam Lugowski. All rights reserved.
# Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
# SPDX-License-Identifier: BSD-2-Clause

from io import BytesIO, StringIO
from pathlib import Path

import numpy as np
import unittest
import scipy

import fast_matrix_market as fmm

matrices = Path("matrices")

try:
    import bz2
    HAVE_BZ2 = True
except ImportError:
    HAVE_BZ2 = False


class TestArray(unittest.TestCase):
    def assertMatrixEqual(self, lhs, rhs, types=True):
        """
        Assert dense numpy matrices are equal.
        """
        self.assertEqual(lhs.shape, rhs.shape)
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

    def test_read(self):
        for mtx in sorted(list(matrices.glob("*.mtx*"))):
            if str(mtx).endswith(".bz2") and not HAVE_BZ2:
                continue
            mtx_header = fmm.read_header(mtx)
            if mtx_header.format != "array":
                continue

            with self.subTest(msg=mtx.stem):
                m = scipy.io.mmread(mtx)
                m_fmm = fmm.read_array(mtx)
                self.assertMatrixEqual(m, m_fmm)

    def test_write(self):
        for mtx in sorted(list(matrices.glob("*.mtx*"))):
            if str(mtx).endswith(".bz2") and not HAVE_BZ2:
                continue
            mtx_header = fmm.read_header(mtx)
            if mtx_header.format != "array":
                continue

            with self.subTest(msg=mtx.stem):
                m_fmm = fmm.read_array(mtx)

                bio = BytesIO()
                fmm.write_array(bio, m_fmm)
                fmms = bio.getvalue().decode()

                m = fmm.read_array(StringIO(fmms))

                self.assertMatrixEqual(m, m_fmm)


if __name__ == '__main__':
    unittest.main()
