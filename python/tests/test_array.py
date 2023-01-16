# Copyright (C) 2022-2023 Adam Lugowski. All rights reserved.
# Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
from io import BytesIO, StringIO
from pathlib import Path
import numpy as np
import unittest
import scipy

import fast_matrix_market as fmm

matrices = Path("matrices")


class TestArray(unittest.TestCase):
    def assertMatrixEqual(self, lhs, rhs, types=True):
        """
        Assert dense numpy matrices are equal.
        """
        self.assertEqual(lhs.shape, rhs.shape)
        self.assertEqual(type(lhs), type(rhs))
        if types:
            self.assertEqual(lhs.dtype, rhs.dtype)
        np.testing.assert_almost_equal(lhs, rhs)

    def test_read(self):
        for mtx in sorted(list(matrices.glob("*.mtx*"))):
            mtx_header = fmm.read_header(mtx)
            if mtx_header.format != "array":
                continue

            with self.subTest(msg=mtx.stem):
                m = scipy.io.mmread(mtx)
                m_fmm = fmm.read_array(mtx)
                self.assertMatrixEqual(m, m_fmm)

    def test_write(self):
        for mtx in sorted(list(matrices.glob("*.mtx*"))):
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
