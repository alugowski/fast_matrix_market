// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#include <algorithm>
#include <sstream>

#include "fmm_tests.hpp"

#include <fast_matrix_market/app/Eigen.hpp>

#if defined(__clang__)
// Disable some pedantic warnings from Eigen headers.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-but-set-variable"
#endif
#include <unsupported/Eigen/SparseExtra>

using VT = double;

using SpColMajor = Eigen::SparseMatrix<VT, Eigen::ColMajor>;
using SpRowMajor = Eigen::SparseMatrix<VT, Eigen::RowMajor>;
using Dense = Eigen::Matrix<VT, Eigen::Dynamic, Eigen::Dynamic>;
using DenseVector = Eigen::Vector<VT, Eigen::Dynamic>;

template <typename MatType>
MatType read_mtx(const std::string& source, typename MatType::Scalar pattern_value = 1) {
    std::istringstream iss(source);
    MatType ret;
    fast_matrix_market::read_matrix_market_eigen(iss, ret, fast_matrix_market::read_options{}, pattern_value);
    return ret;
}

template <typename MatType>
MatType read_dense(const std::string& source) {
    std::istringstream iss(source);
    MatType ret;
    fast_matrix_market::read_matrix_market_eigen_dense(iss, ret);
    return ret;
}

template <typename MatType>
std::string write_mtx(MatType& mat, bool pattern_only=false) {
    fast_matrix_market::matrix_market_header header{};
    if (pattern_only) header.field = fast_matrix_market::pattern;

    std::ostringstream oss;
    fast_matrix_market::write_matrix_market_eigen(oss, mat, fast_matrix_market::write_options(), header);
    return oss.str();
}

std::string write_dense(Dense& mat) {
    std::ostringstream oss;
    fast_matrix_market::write_matrix_market_eigen_dense(oss, mat);
    return oss.str();
}

struct Param {
    enum {Load_Eigen, Load_FMM, Load_FMM_Vec} Source;
    std::string filename;
};

std::ostream& operator<<(std::ostream& os, const Param& p) {
    os << p.filename;
    return os;
}

class EigenTest : public ::testing::TestWithParam<Param> {
public:
    void load(const Param& param) {
        std::string load_path = kTestMatrixDir + "/" + param.filename;

        switch (param.Source) {
            case Param::Load_Eigen:
                Eigen::loadMarket(col_major, load_path);
                Eigen::loadMarket(row_major, load_path);
                break;
            case Param::Load_FMM: {
                std::ifstream f(load_path);
                fast_matrix_market::read_matrix_market_eigen(f, col_major);
                std::ifstream f2(load_path);
                fast_matrix_market::read_matrix_market_eigen(f2, row_major);
                break;
            }
            case Param::Load_FMM_Vec: {
                std::ifstream f(load_path);

                DenseVector vec;
                fast_matrix_market::read_matrix_market_eigen_dense(f, vec);
                col_major = vec.sparseView();
                row_major = vec.sparseView();
                break;
            }
        }

    }

    /**
     * Equality check that prints the matrices if they're unequal. Useful for debugging when remote tests fail.
     */
    static bool is_equal(const Dense& lhs, const Dense& rhs) {
        if (lhs.isApprox(rhs, 1e-6)) {
            return true;
        }

        std::cout << "Left:" << std::endl << lhs.rows() << "-by-" << lhs.cols() << std::endl << lhs << std::endl;
        std::cout << "Right:" << std::endl << rhs.rows() << "-by-" << rhs.cols() << std::endl << rhs << std::endl << std::endl;

        return false;
    }

    protected:
    SpColMajor col_major;
    SpRowMajor row_major;
};

TEST_P(EigenTest, Loaded) {
    EXPECT_NO_THROW(load(GetParam()));

    // sparse RowMajor/ColMajor combinations
    EXPECT_TRUE(col_major.isApprox(read_mtx<SpColMajor>(write_mtx(col_major)), 1e-6));
    EXPECT_TRUE(row_major.isApprox(read_mtx<SpColMajor>(write_mtx(col_major)), 1e-6));
    EXPECT_TRUE(col_major.isApprox(read_mtx<SpRowMajor>(write_mtx(row_major)), 1e-6));
    EXPECT_TRUE(row_major.isApprox(read_mtx<SpRowMajor>(write_mtx(row_major)), 1e-6));

    // dense
    Dense dense(col_major);
    EXPECT_TRUE(is_equal(dense, read_dense<Dense>(write_dense(dense))));

    // sparse/dense combinations
    EXPECT_TRUE(col_major.isApprox(dense, 1e-6));
    EXPECT_TRUE(col_major.isApprox(read_mtx<SpColMajor>(write_dense(dense)), 1e-6));
    EXPECT_TRUE(is_equal(dense, read_dense<Dense>(write_mtx(col_major))));

    // write pattern
    std::string pattern_mtx = write_mtx(col_major, true);
    EXPECT_TRUE(pattern_mtx.find("pattern") > 0); // pattern should appear in the header

    auto mat_val_1 = read_mtx<SpColMajor>(pattern_mtx, 1);
    EXPECT_TRUE(mat_val_1.isApprox(read_mtx<SpRowMajor>(pattern_mtx, 1), 1e-6));
    EXPECT_FALSE(mat_val_1.isApprox(read_mtx<SpRowMajor>(pattern_mtx, 2), 1e-6));
}

INSTANTIATE_TEST_SUITE_P(
        EigenTest,
        EigenTest,
        ::testing::Values(Param{Param::Load_Eigen, "eye3.mtx"}
                          , Param{Param::Load_Eigen, "kepner_gilbert_graph.mtx"}
                          , Param{Param::Load_FMM, "vector_array.mtx"}
                          , Param{Param::Load_FMM, "vector_coordinate.mtx"}
                          , Param{Param::Load_FMM_Vec, "vector_array.mtx"}
                          , Param{Param::Load_FMM_Vec, "vector_coordinate.mtx"}
                          , Param{Param::Load_FMM, "eye3_pattern.mtx"}
                          ));