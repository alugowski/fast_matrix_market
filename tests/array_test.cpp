// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#include <algorithm>
#include <cmath>

#include "fmm_tests.hpp"

// for TYPED_TEST_SUITE
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"


/**
 * Construct a test matrix.
 * @param byte_target How big the matrix tuples should be
 */
template <typename VT>
void construct_array(array_matrix<VT>& ret, int64_t num_elements) {
    ret.nrows = std::max(1LL, (int64_t)std::ceil(std::sqrt(num_elements) / 2));
    ret.ncols = num_elements / ret.nrows;

    num_elements = ret.nrows * ret.ncols;

    ret.vals.resize(num_elements);
    for (auto i = 0; i < num_elements; ++i) {
        ret.vals[i] = static_cast<VT>(i);
    }
}

template <typename MatType>
std::string write_mtx(MatType& array, const fast_matrix_market::write_options& options) {
    std::ostringstream oss;
    fast_matrix_market::write_matrix_market_array(oss,
                                                  fast_matrix_market::matrix_market_header(array.nrows, array.ncols),
                                                  array.vals,
                                                  options);
    return oss.str();
}

template <typename MatType>
MatType read_mtx(const std::string& source, const fast_matrix_market::read_options& options) {
    std::istringstream iss(source);
    MatType array;
    fast_matrix_market::read_matrix_market_array(iss, array.nrows, array.ncols, array.vals, options);
    return array;
}


template <typename T>
class ArrayTest : public ::testing::Test {
public:
    using Mat = array_matrix<T>;

    void load(int nnz, int chunk_size, int p) {
        construct_array(mat, nnz);

        roptions.chunk_size_bytes = chunk_size;
        roptions.num_threads = p;
        woptions.num_threads = p;
    }

protected:
    Mat mat;
    fast_matrix_market::read_options roptions;
    fast_matrix_market::write_options woptions;
};

using MyTypes = ::testing::Types<float, double, std::complex<double>, int64_t, long double>;
TYPED_TEST_SUITE(ArrayTest, MyTypes);

TYPED_TEST(ArrayTest, Generated) {
    using Mat = array_matrix<TypeParam>;

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

#pragma clang diagnostic pop