# Copyright (C) 2022-2023 Adam Lugowski. All rights reserved.
# Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
"""
Run the actual SciPy test suite for scipy.io.mm* methods against fast_matrix_market.

This is intended to be run in development only as SciPy may change their suite at any time.
"""

# replace scipy.io.mm* methods with the fast_matrix_market equivalents
import scipy.io
import fast_matrix_market as fmm
scipy.io.mmread = fmm.mmread
scipy.io.mmwrite = fmm.mmwrite
scipy.io.mmwinfo = fmm.mminfo

# match scipy.io.mmwrite()'s behavior of always looking for symmetry
fmm.ALWAYS_FIND_SYMMETRY = True

# Import all tests so PyTest can pick them up
# noinspection PyUnresolvedReferences
from scipy.io.tests.test_mmio import *

# To verify that the tests are actually run against fast_matrix_market modify read_scipy() to return None and
# verify that the tests all fail.
