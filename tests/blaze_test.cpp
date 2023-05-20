// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
// SPDX-License-Identifier: BSD-2-Clause

#include <algorithm>
#include <fstream>
#include <sstream>

#include "fmm_tests.hpp"

#if defined(__clang__)
// Disable warnings from Blaze headers.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

#include <fast_matrix_market/app/Blaze.hpp>

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#if defined(__clang__)
// for TYPED_TEST_SUITE
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

using ValueTypes = ::testing::Types<int64_t, float, double, std::complex<double>, long double>;

template <typename MatType>
MatType read_mtx(const std::string& source, typename MatType::ElementType pattern_value = 1) {
    fast_matrix_market::read_options options;
    options.chunk_size_bytes = 1;

    std::istringstream iss(source);
    MatType ret;
    fast_matrix_market::read_matrix_market_blaze(iss, ret, options, pattern_value);
    return ret;
}


template <typename MatType>
std::string write_mtx(MatType& mat, bool pattern_only=false) {
    fast_matrix_market::write_options options;
    options.chunk_size_values = 1;

    fast_matrix_market::matrix_market_header header{};
    if (pattern_only) header.field = fast_matrix_market::pattern;

    std::ostringstream oss;
    fast_matrix_market::write_matrix_market_blaze(oss, mat, options, header);
    return oss.str();
}

/**
 * Equality check that prints the matrices if they're unequal. Useful for debugging when remote tests fail.
 */
template <typename LHS, typename RHS, typename std::enable_if<blaze::IsMatrix_v<LHS>, int>::type = 0>
bool is_equal(const LHS& lhs, const RHS& rhs) {
    if (lhs.rows() != rhs.rows() || lhs.columns() != rhs.columns()) {
        return false;
    }

    bool equal = true;
    for (size_t row = 0; row < rhs.rows(); ++row) {
        for (size_t col = 0; col < rhs.columns(); ++col) {
            if (rhs(row, col) != lhs(row, col)) {
                equal = false;
            }
        }
    }

    if (!equal) {
        std::cout << "Left:" << std::endl << lhs.rows() << "-by-" << lhs.columns() << std::endl << lhs << std::endl;
        std::cout << "Right:" << std::endl << rhs.rows() << "-by-" << rhs.columns() << std::endl << rhs << std::endl
                  << std::endl;
    }

    return equal;
}

/**
 * Equality check that prints the vectors if they're unequal. Useful for debugging when remote tests fail.
 */
template <typename LHS, typename RHS, typename std::enable_if<blaze::IsVector_v<LHS>, int>::type = 0>
bool is_equal(const LHS& lhs, const RHS& rhs) {
    bool equal = true;
    if (lhs.size() != rhs.size()) {
        equal = false;
    }

    if (equal) {
        for (size_t row = 0; row < rhs.size(); ++row) {
            if (rhs[row] != lhs[row]) {
                equal = false;
            }
        }
    }

    if (!equal) {
        std::cout << "Left:" << std::endl << "len=" << lhs.size() << " sparse=" << blaze::IsSparseVector_v<LHS> << std::endl << lhs << std::endl;
        std::cout << "Right:" << std::endl << "len=" << rhs.size() << " sparse=" << blaze::IsSparseVector_v<RHS> << std::endl << rhs << std::endl << std::endl;
    }

    return equal;
}

TYPED_TEST_SUITE(BlazeMatrixTest, ValueTypes);

template <typename VT>
class BlazeMatrixTest : public testing::Test {
public:
    void load(const std::string& param) {
        std::string load_path = kTestMatrixDir + "/" + param;

        {
            std::ifstream f(load_path);
            fast_matrix_market::read_matrix_market_blaze(f, col_major);
        }
        {
            std::ifstream f(load_path);
            fast_matrix_market::read_matrix_market_blaze(f, row_major);
        }
        {
            std::ifstream f(load_path);
            fast_matrix_market::read_matrix_market_blaze(f, dense_col_major);
        }
        {
            std::ifstream f(load_path);
            fast_matrix_market::read_matrix_market_blaze(f, dense_row_major);
        }

    }

    protected:
    blaze::CompressedMatrix<VT, blaze::columnMajor> col_major;
    blaze::CompressedMatrix<VT, blaze::rowMajor> row_major;

    blaze::DynamicMatrix<VT, blaze::columnMajor> dense_col_major;
    blaze::DynamicMatrix<VT, blaze::rowMajor> dense_row_major;
};

TYPED_TEST(BlazeMatrixTest, SmallMatrices) {
    auto filenames = std::vector{
        "eye3.mtx"
        , "row_3by4.mtx"
        , "kepner_gilbert_graph.mtx"
        , "vector_array.mtx"
        , "vector_coordinate.mtx"
        , "eye3_pattern.mtx"
    };
    if (fast_matrix_market::is_complex<TypeParam>::value) {
        filenames.emplace_back("eye3_complex.mtx");
        filenames.emplace_back("vector_coordinate_complex.mtx");
    }
    for (auto param : filenames) {
        EXPECT_NO_THROW(this->load(param));

        auto& col_major = this->col_major;
        auto& row_major = this->row_major;
        auto& dense_col_major = this->dense_col_major;
        auto& dense_row_major = this->dense_row_major;

        EXPECT_TRUE(is_equal(col_major, row_major));

        EXPECT_EQ(col_major.nonZeros(), row_major.nonZeros());

        EXPECT_TRUE(is_equal(dense_col_major, dense_row_major));
        EXPECT_TRUE(is_equal(col_major, dense_col_major));

        // Read/Write into a copy
        using Dense = blaze::DynamicMatrix<TypeParam, blaze::rowMajor>;
        using Sparse = blaze::CompressedMatrix<TypeParam, blaze::columnMajor>;
        EXPECT_TRUE(is_equal(col_major, read_mtx<Sparse>(write_mtx(row_major))));
        EXPECT_TRUE(is_equal(col_major, read_mtx<Sparse>(write_mtx(col_major))));
        EXPECT_TRUE(is_equal(dense_row_major, read_mtx<Dense>(write_mtx(row_major))));
        EXPECT_TRUE(is_equal(dense_row_major, read_mtx<Dense>(write_mtx(dense_row_major))));
        EXPECT_TRUE(is_equal(dense_row_major, read_mtx<Dense>(write_mtx(dense_col_major))));

        // write pattern
        std::string pattern_mtx = write_mtx(col_major, true);
        EXPECT_TRUE(pattern_mtx.find("pattern") > 0); // pattern should appear in the header

        auto sparse_pat = read_mtx<Sparse>(pattern_mtx, 1);
        auto dense_pat = read_mtx<Dense>(pattern_mtx, 1);
        EXPECT_TRUE(is_equal(dense_pat, sparse_pat));
    }
}

/////////////////////////////////////////////////////////
///// Vectors
/////////////////////////////////////////////////////////

TYPED_TEST_SUITE(BlazeVectorTest, ValueTypes);

template <typename VT>
class BlazeVectorTest : public testing::Test {
public:
    void load(const std::string& param) {
        std::string load_path = kTestMatrixDir + "/" + param;

        {
            std::ifstream f(load_path);
            fast_matrix_market::read_matrix_market_blaze(f, sparse);
        }
        {
            std::ifstream f(load_path);
            fast_matrix_market::read_matrix_market_blaze(f, dense);
        }
    }

protected:
    blaze::CompressedVector<VT> sparse;
    blaze::DynamicVector<VT> dense;
};

TYPED_TEST(BlazeVectorTest, SmallVectors) {
    auto filenames = std::vector{
        "vector_array.mtx"
        , "vector_coordinate.mtx"
    };
    if (fast_matrix_market::is_complex<TypeParam>::value) {
        filenames.emplace_back("vector_coordinate_complex.mtx");
    }
    for (auto param : filenames) {
        EXPECT_NO_THROW(this->load(param));

        auto& sparse = this->sparse;
        auto& dense = this->dense;

        EXPECT_TRUE(is_equal(sparse, dense));

        EXPECT_GT(sparse.nonZeros(), 0);

        // Read/Write into a copy
        using Dense = blaze::DynamicVector<TypeParam>;
        using Sparse = blaze::CompressedVector<TypeParam>;
        EXPECT_TRUE(is_equal(sparse, read_mtx<Sparse>(write_mtx(sparse))));
        EXPECT_TRUE(is_equal(sparse, read_mtx<Sparse>(write_mtx(dense))));
        EXPECT_TRUE(is_equal(dense, read_mtx<Dense>(write_mtx(sparse))));
        EXPECT_TRUE(is_equal(dense, read_mtx<Dense>(write_mtx(dense))));

        // write pattern
        std::string pattern_mtx = write_mtx(sparse, true);
        EXPECT_TRUE(pattern_mtx.find("pattern") > 0); // pattern should appear in the header

        auto sparse_pat = read_mtx<Sparse>(pattern_mtx, 1);
        auto dense_pat = read_mtx<Dense>(pattern_mtx, 1);
        EXPECT_TRUE(is_equal(dense_pat, sparse_pat));
    }
}
