// Copyright (C) 2023 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
// SPDX-License-Identifier: BSD-2-Clause

#include <fstream>
#include <regex>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244) // '=': conversion from '__int64' to '_Ty', possible loss of data
#endif

#include "fmm_tests.hpp"

#if defined(__clang__)
// for TYPED_TEST_SUITE
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

template <typename TRIPLET>
void read_triplet_file(const std::string& matrix_filename, TRIPLET& triplet, fast_matrix_market::read_options options = {}) {
    std::ifstream f(kTestMatrixDir + "/" + matrix_filename);
    options.chunk_size_bytes = 1;

    fast_matrix_market::read_matrix_market_triplet(f, triplet.nrows, triplet.ncols, triplet.rows, triplet.cols, triplet.vals, options);
}

template <typename ARRAY>
void read_array_file(const std::string& matrix_filename, ARRAY& array, fast_matrix_market::read_options options = {}) {
    std::ifstream f(kTestMatrixDir + "/" + matrix_filename);
    options.chunk_size_bytes = 1;

    fast_matrix_market::read_matrix_market_array(f, array.nrows, array.ncols, array.vals, array.order, options);
}


/**
 * Test that disabling vector support via FMM_NO_VECTOR works correctly.
 */
TEST(NoVectorSupport, misc) {
    {
        triplet_matrix<int64_t, double> m;
        EXPECT_THROW(read_triplet_file("vector_coordinate.mtx", m), fast_matrix_market::no_vector_support);
    }
    {
        array_matrix<double> m;
        EXPECT_THROW(read_array_file("vector_array.mtx", m), fast_matrix_market::no_vector_support);
    }
}

