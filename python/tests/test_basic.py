# Copyright (C) 2022-2023 Adam Lugowski. All rights reserved.
# Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
# SPDX-License-Identifier: BSD-2-Clause

import unittest

import fast_matrix_market as fmm

try:
    import threadpoolctl
except ImportError:
    threadpoolctl = None


class TestModule(unittest.TestCase):
    def test_version(self):
        self.assertTrue(fmm.__version__)

    def test_doc(self):
        self.assertTrue(fmm.__doc__)

    @unittest.skipIf(not threadpoolctl or not hasattr(threadpoolctl, "register"),
                     reason="no threadpoolctl or version too old")
    def test_threadpoolctl(self):
        with threadpoolctl.threadpool_limits(limits=2, user_api='fast_matrix_market'):
            self.assertEqual(fmm.PARALLELISM, 2)
        with threadpoolctl.threadpool_limits(limits=4):
            self.assertEqual(fmm.PARALLELISM, 4)


if __name__ == '__main__':
    unittest.main()
