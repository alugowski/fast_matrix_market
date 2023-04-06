// Copyright (C) 2023 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#include <algorithm>
#include <sstream>

#include "fmm_tests.hpp"

#include <fast_matrix_market/app/Armadillo.hpp>

#include <armadillo>

#if defined(__clang__)
// for TYPED_TEST_SUITE
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

namespace fmm = fast_matrix_market;

template <typename MatType>
std::string write_mtx(MatType& mat, bool pattern_only=false) {
    fast_matrix_market::write_options options;
    options.chunk_size_values = 1;

    fast_matrix_market::matrix_market_header header{};
    if (pattern_only) header.field = fast_matrix_market::pattern;

    std::ostringstream oss;
    fast_matrix_market::write_matrix_market_arma(oss, mat, options, header);
    return oss.str();
}

template <typename VT, typename ARMA_MAT>
bool operator==(const array_matrix<VT>& a, const ARMA_MAT& b) {
    if (a.nrows != (int64_t)b.n_rows) {
        std::cout << "nrows mismatch" << std::endl;
        return false;
    }
    if (a.ncols != (int64_t)b.n_cols) {
        std::cout << "ncols mismatch" << std::endl;
        return false;
    }

    for (int64_t row = 0; row < a.nrows; ++row) {
        for (int64_t col = 0; col < a.ncols; ++col) {
            if (a(row, col) != b(row, col)) {
                std::cout << "value mismatch at (" << row << ", " << col << ") : " << a(row, col) << " != " << b(row, col) << std::endl;
                return false;
            }
        }
    }

    return true;
}


using ValueTypes = ::testing::Types<int64_t, float, double, std::complex<double>>;
TYPED_TEST_SUITE(ArmadilloTest, ValueTypes);

template <typename VT>
class ArmadilloTest : public testing::Test {
public:
    std::ifstream get_ifstream(const std::string& param) {
        return std::ifstream(kTestMatrixDir + "/" + param);
    }
};

TYPED_TEST(ArmadilloTest, SmallMatrices) {
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
        std::ifstream f = this->get_ifstream(param);
        array_matrix<TypeParam> array;
        fmm::read_matrix_market_array(f, array.nrows, array.ncols, array.vals);

        std::ifstream f2 = this->get_ifstream(param);
        arma::Mat<TypeParam> dense;
        fmm::read_matrix_market_arma(f2, dense);

        std::ifstream f3 = this->get_ifstream(param);
        arma::SpMat<TypeParam> sparse;
        fmm::read_matrix_market_arma(f3, sparse);

        EXPECT_EQ(array, dense);
        EXPECT_EQ(array, sparse);

        // test writes
        {
            std::stringstream iss(write_mtx(dense));
            arma::Mat<TypeParam> dense2;
            fmm::read_matrix_market_arma(iss, dense2);
            EXPECT_EQ(array, dense2);
        }

        {
            std::stringstream iss(write_mtx(sparse));
            arma::SpMat<TypeParam> sparse2;
            fmm::read_matrix_market_arma(iss, sparse2);
            EXPECT_EQ(array, sparse2);
        }

        // test write sparse matrix pattern
        {
            std::string pattern_mtx = write_mtx(sparse, true);
            EXPECT_TRUE(pattern_mtx.find("pattern") > 0); // pattern should appear in the header
        }
    }
}
