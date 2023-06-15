// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
// SPDX-License-Identifier: BSD-2-Clause

#include <algorithm>

#include "fmm_tests.hpp"

#if defined(__clang__)
// for TYPED_TEST_SUITE
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

/**
 * Construct a test matrix.
 * @param byte_target How big the matrix tuples should be
 */
template <typename IT, typename VT>
void construct_triplet(triplet_matrix<IT, VT>& ret, int64_t num_elements) {
    ret.nrows = num_elements;
    ret.ncols = num_elements;

    ret.rows.resize(num_elements);
    ret.cols.resize(num_elements);
    ret.vals.resize(num_elements);

    for (auto i = 0; i < num_elements; ++i) {
        ret.rows[i] = i;
        ret.cols[i] = i;
        ret.vals[i] = static_cast<VT>(i);
    }
}

template <typename MatType>
std::string write_mtx(MatType& triplet, const fast_matrix_market::write_options& options) {
    std::ostringstream oss;

    fast_matrix_market::write_matrix_market_triplet(oss,
                                                    {triplet.nrows, triplet.ncols},
                                                    triplet.rows, triplet.cols, triplet.vals,
                                                    options);
    return oss.str();
}

template <typename MatType>
MatType read_mtx(const std::string& source, const fast_matrix_market::read_options& options) {
    std::istringstream iss(source);
    MatType triplet;
    fast_matrix_market::read_matrix_market_triplet(iss,
                                                   triplet.nrows, triplet.ncols,
                                                   triplet.rows, triplet.cols, triplet.vals,
                                                   options);
    return triplet;
}


template <typename T>
class TripletTest : public ::testing::Test {
public:
    using Mat = triplet_matrix<int64_t, T>;

    void load(int nnz, int chunk_size, int p) {
        construct_triplet(mat, nnz);

        roptions.chunk_size_bytes = chunk_size;
        woptions.chunk_size_values = chunk_size;
        roptions.num_threads = p;
        woptions.num_threads = p;
    }

protected:
    Mat mat;
    fast_matrix_market::read_options roptions;
    fast_matrix_market::write_options woptions;
};

using MyTypes = ::testing::Types<bool, float, double, std::complex<double>, int64_t, long double>;
TYPED_TEST_SUITE(TripletTest, MyTypes);

TYPED_TEST(TripletTest, Generated) {
    using Mat = triplet_matrix<int64_t, TypeParam>;

    for (int nnz : {0, 10, 1000}) {
        for (int chunk_size : {1, 15, 203, 1 << 10, 1 << 20}) {
            for (int p : {1, 4}) {
                this->load(nnz, chunk_size, p);

                Mat b = read_mtx<Mat>(write_mtx(this->mat, this->woptions), this->roptions);
                EXPECT_EQ(this->mat, b);
            }
        }
    }
}

TEST(TripletTest, BoolRaceConditions) {
    // std::vector<bool> may be specialized such that accessing different elements is not thread safe.
    // Ensure that the protection against this is working.
    using Mat = triplet_matrix<int64_t, bool>;

    Mat mat;
    construct_triplet(mat, 300);
    std::fill(mat.vals.begin(), mat.vals.end(), true);
    std::string mtx = write_mtx(mat, fast_matrix_market::write_options{});
    mat.vals.clear();
    std::string mtx_pattern = write_mtx(mat, fast_matrix_market::write_options{});

    fast_matrix_market::read_options roptions;
    roptions.parallel_ok = true;
    roptions.chunk_size_bytes = 1;
    roptions.num_threads = 8;

    for (int i = 0; i < 1000; ++i) {
        Mat triplet = read_mtx<Mat>(mtx, roptions);
        Mat triplet2 = read_mtx<Mat>(mtx_pattern, roptions);

        EXPECT_EQ(triplet, triplet2);
    }
}
