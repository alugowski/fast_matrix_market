# Copyright (C) 2022-2023 Adam Lugowski. All rights reserved.
# Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
# SPDX-License-Identifier: BSD-2-Clause

from io import BytesIO, StringIO
from pathlib import Path
import tempfile

import numpy as np
import unittest

try:
    import scipy
    HAVE_SCIPY = True
except ImportError:
    HAVE_SCIPY = False

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
                m_fmm = fmm.read_array(mtx)
                if not HAVE_SCIPY:
                    continue

                m = scipy.io.mmread(mtx)
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

    def test_write_file(self):
        mtx = np.array([[11, 0, 0], [0, 22, 0], [0, 0, 33]])

        with tempfile.TemporaryDirectory() as temp_dir:
            mtx_path = Path(temp_dir) / "matrix.mtx"

            # Write with filename, read with filename
            fmm.write_array(mtx_path, mtx)
            a = fmm.read_array(mtx_path)
            self.assertMatrixEqual(a, mtx)

            # Write with filename, read with file object
            fmm.write_array(mtx_path, mtx)
            with open(mtx_path, 'r') as f:
                a = fmm.read_array(f)
            self.assertMatrixEqual(a, mtx)

            # Write with file object (binary), read with filename
            with open(mtx_path, 'wb') as f:
                fmm.write_array(f, mtx)
            a = fmm.read_array(mtx_path)
            self.assertMatrixEqual(a, mtx)

            # Write with file object (text)
            # Not allowed. scipy.io._mmio.mmwrite() also throws TypeError here
            with open(mtx_path, 'w') as f:
                with self.assertRaises(TypeError):
                    fmm.write_array(f, mtx)

            # Write with file object (binary), read with file object (text)
            with open(mtx_path, 'wb') as f:
                fmm.write_array(f, mtx)
            with open(mtx_path, 'r') as f:
                a = fmm.read_array(f)
            self.assertMatrixEqual(a, mtx)

            # Write with file object (binary), read with file object (binary)
            with open(mtx_path, 'wb') as f:
                fmm.write_array(f, mtx)
            with open(mtx_path, 'rb') as f:
                a = fmm.read_array(f)
            self.assertMatrixEqual(a, mtx)

    def test_bz2(self):
        try:
            import bz2
        except ImportError:
            self.skipTest(reason="no bz2 module")
            return

        mtx = np.array([[11, 0, 0], [0, 22, 0], [0, 0, 33]])

        with tempfile.TemporaryDirectory() as temp_dir:
            temp_path = Path(temp_dir)
            mtx_path = temp_path / "matrix.mtx"
            bz2_path = temp_path / "matrix.mtx.bz2"

            fmm.write_array(mtx_path, mtx)

            # create bz2
            with open(mtx_path, 'rb') as f_in, bz2.BZ2File(bz2_path, 'wb') as f_out:
                f_out.write(f_in.read())

            a = fmm.read_array(bz2_path)
            self.assertMatrixEqual(a, mtx)

    def test_gzip(self):
        try:
            import gzip
        except ImportError:
            self.skipTest(reason="no gzip module")
            return

        mtx = np.array([[11, 0, 0], [0, 22, 0], [0, 0, 33]])

        with tempfile.TemporaryDirectory() as temp_dir:
            temp_path = Path(temp_dir)
            mtx_path = temp_path / "matrix.mtx"
            gzip_path = temp_path / "matrix.mtx.gz"

            fmm.write_array(mtx_path, mtx)

            # create bz2
            with open(mtx_path, 'rb') as f_in, gzip.GzipFile(gzip_path, 'wb') as f_out:
                f_out.write(f_in.read())

            a = fmm.read_array(gzip_path)
            self.assertMatrixEqual(a, mtx)


if __name__ == '__main__':
    unittest.main()
