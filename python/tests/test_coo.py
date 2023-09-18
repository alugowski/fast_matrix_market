# Copyright (C) 2022-2023 Adam Lugowski. All rights reserved.
# Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
# SPDX-License-Identifier: BSD-2-Clause

from io import BytesIO, StringIO
from pathlib import Path
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


@unittest.skipIf(np is None, reason="no numpy")
class TestTriplet(unittest.TestCase):
    def assertMatrixEqual(self, lhs, rhs, types=True):
        """
        Assert sparse triplet matrices are equal.
        """
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

    def test_read(self):
        for mtx in sorted(list(matrices.glob("*.mtx*"))):
            if str(mtx).endswith(".bz2") and bz2 is None:
                continue
            mtx_header = fmm.read_header(mtx)
            if mtx_header.format != "coordinate":
                continue

            with self.subTest(msg=mtx.stem):
                triplet, shape = fmm.read_coo(mtx)
                if scipy is None:
                    continue

                m = scipy.io.mmread(mtx)
                fmm_scipy = scipy.sparse.coo_matrix(triplet, shape=shape)
                self.assertMatrixEqual(m, fmm_scipy)

    def test_write(self):
        for mtx in sorted(list(matrices.glob("*.mtx*"))):
            if str(mtx).endswith(".bz2") and bz2 is None:
                continue
            mtx_header = fmm.read_header(mtx)
            if mtx_header.format != "coordinate":
                continue

            with self.subTest(msg=mtx.stem):
                triplet, shape = fmm.read_coo(mtx)

                bio = BytesIO()
                fmm.write_coo(bio, triplet, shape=shape)
                fmms = bio.getvalue().decode()

                triplet2, shape2 = fmm.read_coo(StringIO(fmms))

                self.assertEqual(shape, shape2)

                if scipy is None:
                    continue

                fmm_scipy = scipy.sparse.coo_matrix(triplet, shape=shape)
                fmm_scipy2 = scipy.sparse.coo_matrix(triplet2, shape=shape2)
                self.assertMatrixEqual(fmm_scipy, fmm_scipy2)

    def test_list(self):
        i = [0, 1, 2]
        j = [0, 1, 2]
        data = [1, 1, 1]

        bio = BytesIO()
        fmm.write_coo(bio, (data, (i, j)), shape=(3, 3))
        fmms = bio.getvalue().decode()

        lists_fmm_triplet, lists_fmm_shape = fmm.read_coo(StringIO(fmms))

        if scipy is None:
            return

        lists = scipy.sparse.coo_matrix((data, (i, j)), shape=(3, 3))
        lists_fmm = scipy.sparse.coo_matrix(lists_fmm_triplet, shape=lists_fmm_shape)
        self.assertMatrixEqual(lists, lists_fmm, types=False)

    @unittest.skipIf(not cpp_matrices.exists(), "Matrices from C++ code not available.")
    def test_index_overflow(self):
        with self.assertRaises(OverflowError):
            fmm.read_coo(cpp_matrices / "overflow" / "overflow_index_gt_int64.mtx")


if __name__ == '__main__':
    unittest.main()
