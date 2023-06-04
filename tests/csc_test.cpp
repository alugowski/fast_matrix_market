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
 */
template <typename IT, typename VT>
void construct_csc(csc_matrix<IT, VT>& ret, triplet_matrix<IT, VT>& triplet, int num_elements, int ncols = 1000) {
    if (num_elements < ncols) {
        ncols = num_elements;
    }
    ret.nrows = num_elements;
    ret.ncols = ncols;

    double num_elements_per_column = ((double)num_elements) / ncols;

    ret.indptr.resize(ncols+1);
    for (auto i = 0; i < ncols; ++i) {
        ret.indptr[i] = static_cast<IT>(i * num_elements_per_column);
    }
    ret.indptr[ncols] = num_elements;

    ret.indices.resize(num_elements);
    ret.vals.resize(num_elements);
    for (auto i = 0; i < num_elements; ++i) {
        ret.indices[i] = i;
        ret.vals[i] = static_cast<VT>(i);
    }

    // setup the triplet
    triplet.nrows = ret.nrows;
    triplet.ncols = ret.ncols;
    triplet.rows = ret.indices;
    triplet.vals = ret.vals;

    triplet.cols.clear();
    triplet.cols.reserve(num_elements);
    for (IT col = 0; col < ncols; ++col) {
        auto colsize = ret.indptr[col + 1] - ret.indptr[col];
        for (auto i = 0; i < colsize; ++i) {
            triplet.cols.emplace_back(col);
        }
    }
}

template <typename MatType>
std::string write_mtx(MatType& csc, const fast_matrix_market::write_options& options) {
    std::ostringstream oss;

    fast_matrix_market::write_matrix_market_csc(oss,
                                                {csc.nrows, csc.ncols},
                                                csc.indptr, csc.indices, csc.vals,
                                                false,
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
class CSCTest : public ::testing::Test {
public:
    using Mat = csc_matrix<int64_t, T>;

    void load(int nnz, int chunk_size, int p) {
        construct_csc(mat, triplet, nnz);

        roptions.chunk_size_bytes = chunk_size;
        woptions.chunk_size_values = chunk_size;
        roptions.num_threads = p;
        woptions.num_threads = p;
    }

protected:
    Mat mat;
    triplet_matrix<int64_t, T> triplet;
    fast_matrix_market::read_options roptions;
    fast_matrix_market::write_options woptions;
};

using MyTypes = ::testing::Types<float, double, std::complex<double>, int64_t, long double>;
TYPED_TEST_SUITE(CSCTest, MyTypes);

TYPED_TEST(CSCTest, Generated) {
    using ReadMat = triplet_matrix<int64_t, TypeParam>;

    for (int nnz : {0, 10, 1000}) {
        for (int chunk_size : {1, 15, 203, 1 << 10, 1 << 20}) {
            for (int p : {1, 4}) {
                this->load(nnz, chunk_size, p);

                auto b = read_mtx<ReadMat>(write_mtx(this->mat, this->woptions), this->roptions);
                EXPECT_EQ(this->triplet, b);
            }
        }
    }
}
