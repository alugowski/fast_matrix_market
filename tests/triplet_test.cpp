// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#include <algorithm>
#include <sstream>

#include "fmm_tests.hpp"

// for TYPED_TEST_SUITE
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"

template <typename IT, typename VT>
struct triplet_matrix {
    int64_t nrows = 0, ncols = 0;
    std::vector<IT> rows;
    std::vector<IT> cols;
    std::vector<VT> vals;
};

template <typename IT, typename VT>
bool operator==(const triplet_matrix<IT, VT>& a, const triplet_matrix<IT, VT>& b) {
    if (a.nrows != b.nrows) {
        return false;
    }
    if (a.ncols != b.ncols) {
        return false;
    }

    if (a.rows != b.rows) {
        return false;
    }

    if (a.cols != b.cols) {
        return false;
    }

    if (a.vals != b.vals) {
        return false;
    }

    return true;
}

/**
 * Construct a test matrix.
 * @param byte_target How big the matrix tuples should be
 */
template <typename IT, typename VT>
void construct_triplet(triplet_matrix<IT, VT>& ret, int64_t num_elements) {
    ret.nrows = num_elements;
    ret.ncols = num_elements;

    ret.rows.reserve(num_elements);
    ret.cols.reserve(num_elements);
    ret.vals.reserve(num_elements);

    for (auto i = 0; i < num_elements; ++i) {
        ret.rows.emplace_back(i);
        ret.cols.emplace_back(i);
        ret.vals.emplace_back(static_cast<VT>(i));
    }
}

template <typename MatType>
std::string write_mtx(MatType& triplet, const fast_matrix_market::write_options& options) {
    std::ostringstream oss;
    fast_matrix_market::matrix_market_header header(triplet.nrows, triplet.ncols);
    fast_matrix_market::write_matrix_market_triplet(oss, header,
                                                    triplet.rows, triplet.cols, triplet.vals,
                                                    options);
    return oss.str();
}

template <typename MatType>
MatType read_mtx(const std::string& source, const fast_matrix_market::read_options& options) {
    std::istringstream iss(source);
    MatType triplet;
    fast_matrix_market::matrix_market_header header;
    fast_matrix_market::read_matrix_market_triplet(iss, header, triplet.rows, triplet.cols, triplet.vals, options);
    triplet.nrows = header.nrows;
    triplet.ncols = header.ncols;
    return triplet;
}



struct Param {
    int nnz;
    int chunk_size;
    int p;
};

std::ostream& operator<<(std::ostream& os, const Param& p) {
    os << "nnz=" << p.nnz << " chunk_size=" << p.chunk_size << " p=" << p.p;
    return os;
}

template <typename T>
class TripletTest : public ::testing::Test {
public:
    using Mat = triplet_matrix<int64_t, T>;

    void load(int nnz, int chunk_size, int p) {
        construct_triplet(mat, nnz);

        roptions.chunk_size_bytes = chunk_size;
        woptions.num_threads = p;
    }

protected:
    Mat mat;
    fast_matrix_market::read_options roptions;
    fast_matrix_market::write_options woptions;
};

using MyTypes = ::testing::Types<float, double, std::complex<double>, int64_t, long double>;
TYPED_TEST_SUITE(TripletTest, MyTypes);

TYPED_TEST(TripletTest, Generated) {
    for (int nnz : {0, 10, 1000, 10000}) {
        for (int chunk_size : {1, 15, 203, 1 << 10, 1 << 20}) {
            for (int p : {1, 4}) {
                this->load(nnz, chunk_size, p);

                using Mat = triplet_matrix<int64_t, TypeParam>;

                Mat b = read_mtx<Mat>(write_mtx(this->mat, this->woptions), this->roptions);
                EXPECT_EQ(this->mat, b);
            }
        }
    }
}

#pragma clang diagnostic pop