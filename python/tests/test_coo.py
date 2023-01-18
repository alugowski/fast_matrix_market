# Copyright (C) 2022-2023 Adam Lugowski. All rights reserved.
# Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
from io import BytesIO, StringIO
from pathlib import Path
import numpy as np
import unittest
import scipy

import fast_matrix_market as fmm

matrices = Path("matrices")


class TestTriplet(unittest.TestCase):
    def assertMatrixEqual(self, lhs, rhs, types=True):
        """
        Assert sparse triplet matrices are equal.
        """
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

    def test_read(self):
        for mtx in sorted(list(matrices.glob("*.mtx*"))):
            mtx_header = fmm.read_header(mtx)
            if mtx_header.format != "coordinate":
                continue

            with self.subTest(msg=mtx.stem):
                m = scipy.io.mmread(mtx)
                triplet, shape = fmm.read_coo(mtx)
                fmm_scipy = scipy.sparse.coo_matrix(triplet, shape=shape)
                self.assertMatrixEqual(m, fmm_scipy)

    def test_write(self):
        for mtx in sorted(list(matrices.glob("*.mtx*"))):
            mtx_header = fmm.read_header(mtx)
            if mtx_header.format != "coordinate":
                continue

            with self.subTest(msg=mtx.stem):
                triplet, shape = fmm.read_coo(mtx)
                print(triplet)

                bio = BytesIO()
                fmm.write_coo(bio, triplet, shape=shape)
                fmms = bio.getvalue().decode()

                triplet2, shape2 = fmm.read_coo(StringIO(fmms))

                self.assertEqual(shape, shape2)
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

        lists = scipy.sparse.coo_matrix((data, (i, j)), shape=(3, 3))
        lists_fmm = scipy.sparse.coo_matrix(lists_fmm_triplet, shape=lists_fmm_shape)
        self.assertMatrixEqual(lists, lists_fmm, types=False)


if __name__ == '__main__':
    unittest.main()
