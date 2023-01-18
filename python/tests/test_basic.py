# Copyright (C) 2022-2023 Adam Lugowski. All rights reserved.
# Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

import unittest
import fast_matrix_market as fmm


class TestModule(unittest.TestCase):
    def test_version(self):
        self.assertTrue(fmm.__version__)

    def test_doc(self):
        self.assertTrue(fmm.__doc__)


if __name__ == '__main__':
    unittest.main()
