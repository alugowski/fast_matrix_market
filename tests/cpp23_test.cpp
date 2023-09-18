// Copyright (C) 2023 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
// SPDX-License-Identifier: BSD-2-Clause

#include <algorithm>
#include <fstream>
#include <stdfloat>

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

template <typename TRIPLET>
std::string write_triplet_string(TRIPLET& triplet, fast_matrix_market::write_options options = {}) {
    std::ostringstream f;
    options.chunk_size_values = 1;

    fast_matrix_market::write_matrix_market_triplet(f, {triplet.nrows, triplet.ncols}, triplet.rows, triplet.cols, triplet.vals, options);
    return f.str();
}

template <typename VT>
std::string eye_cycle() {
    triplet_matrix<int64_t, VT> eye3;
    read_triplet_file("eye3.mtx", eye3);
    return write_triplet_string(eye3);
}

TEST(Cpp23FixedWidthFloats, Eye) {
    std::string expected = eye_cycle<double>();
    std::string expected_complex = eye_cycle<std::complex<double>>();

    // test pattern on floats
    EXPECT_EQ(expected, eye_cycle<float>());
    EXPECT_EQ(expected_complex, eye_cycle<std::complex<float>>());

#if __STDCPP_FLOAT16_T__
    EXPECT_EQ(expected, eye_cycle<std::float16_t>());
    EXPECT_EQ(expected_complex, eye_cycle<std::complex<std::float16_t>>());
#endif

#if __STDCPP_FLOAT32_T__
    EXPECT_EQ(expected, eye_cycle<std::float32_t>());
    EXPECT_EQ(expected_complex, eye_cycle<std::complex<std::float32_t>>());
#endif

#if __STDCPP_FLOAT64_T__
    EXPECT_EQ(expected, eye_cycle<std::float64_t>());
    EXPECT_EQ(expected_complex, eye_cycle<std::complex<std::float64_t>>());
#endif

#if 0 && __STDCPP_FLOAT128_T__
    // stdlib support for float128_t is incomplete. Missing <charconv> and stream overloads.
    EXPECT_EQ(expected, eye_cycle<std::float128_t>());
    EXPECT_EQ(expected_complex, eye_cycle<std::complex<std::float128_t>>());
#endif

#if __STDCPP_BFLOAT16_T__
    EXPECT_EQ(expected, eye_cycle<std::bfloat16_t>());
#endif
}
