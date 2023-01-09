// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#include <algorithm>
#include <sstream>

#include "fmm_tests.hpp"

#include <fast_matrix_market/app/Eigen.hpp>

// Disable some pedantic warnings from Eigen headers.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-but-set-variable"
#include <unsupported/Eigen/SparseExtra>
#pragma clang diagnostic pop


using SpColMajor = Eigen::SparseMatrix<double, Eigen::ColMajor>;
using SpRowMajor = Eigen::SparseMatrix<double, Eigen::RowMajor>;
using Dense = Eigen::MatrixXd;

template <typename MatType>
MatType read_mtx(const std::string& source) {
    std::istringstream iss(source);
    MatType ret;
    fast_matrix_market::read_matrix_market_eigen(iss, ret);
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
std::string write_mtx(MatType& mat) {
    std::ostringstream oss;
    fast_matrix_market::write_matrix_market_eigen(oss, mat);
    return oss.str();
}

template <>
std::string write_mtx(Dense& mat) {
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

                Eigen::VectorXd vec;
                fast_matrix_market::read_matrix_market_eigen_dense(f, vec);
                col_major = vec.sparseView();
                row_major = vec.sparseView();
                break;
            }
        }

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
    EXPECT_TRUE(dense.isApprox(read_dense<Dense>(write_mtx(dense)), 1e-6));

    // sparse/dense combinations
    EXPECT_TRUE(col_major.isApprox(dense, 1e-6));
    EXPECT_TRUE(col_major.isApprox(read_mtx<SpColMajor>(write_mtx(dense)), 1e-6));
    EXPECT_TRUE(dense.isApprox(read_dense<Dense>(write_mtx(col_major)), 1e-6));
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