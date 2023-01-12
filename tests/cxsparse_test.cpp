// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#include <filesystem>
#include <fstream>
#include <sstream>

#include "fmm_tests.hpp"

#include <fast_matrix_market/app/CXSparse.hpp>
#include "fake_cxsparse/cs.hpp"

using std::filesystem::directory_iterator;

int count_lines(std::string s) {
    return (int)std::count(s.begin(), s.end(), '\n');
}

cs_dl *read_mtx(const std::string& path) {
    cs_dl *A;

    std::ifstream f(path);

    fast_matrix_market::read_matrix_market_cxsparse(f, &A, cs_dl_spalloc);

    // ignore the memory leak
    return A;
}

TEST(CXSparse, ReadSmall) {
    EXPECT_NO_THROW(read_mtx(kTestMatrixDir + "/eye3.mtx"));
    EXPECT_EQ(3, read_mtx(kTestMatrixDir + "/eye3.mtx")->nzmax);
}

TEST(CXSparse, ReadPattern) {
    EXPECT_NO_THROW(read_mtx(kTestMatrixDir + "/eye3_pattern.mtx"));
    EXPECT_EQ(3, read_mtx(kTestMatrixDir + "/eye3_pattern.mtx")->nzmax);
    EXPECT_EQ(nullptr, read_mtx(kTestMatrixDir + "/eye3_pattern.mtx")->x);
}

std::string write_mtx(cs_dl *A) {
    std::ostringstream oss;

    fast_matrix_market::write_matrix_market_cxsparse(oss, A);

    return oss.str();
}

TEST(CXSparse, WriteSmall) {
    EXPECT_NO_THROW(write_mtx(read_mtx(kTestMatrixDir + "/eye3.mtx")));
    EXPECT_EQ(5, count_lines(write_mtx(read_mtx(kTestMatrixDir + "/eye3.mtx"))));
}

TEST(CXSparse, WritePattern) {
    EXPECT_NO_THROW(write_mtx(read_mtx(kTestMatrixDir + "/eye3_pattern.mtx")));
    EXPECT_EQ(5, count_lines(write_mtx(read_mtx(kTestMatrixDir + "/eye3_pattern.mtx"))));
    EXPECT_NE(-1, write_mtx(read_mtx(kTestMatrixDir + "/eye3_pattern.mtx")).find("pattern"));
}


class CSCTest : public ::testing::TestWithParam<std::string> {
protected:
    void load(const std::string& fname) {
        triplet = read_mtx(kTestMatrixDir + fname);
        csc = cs_dl_compress(triplet);
    }

    cs_dl* triplet = nullptr;
    cs_dl* csc = nullptr;
};

TEST_P(CSCTest, ReadSmall) {
    load(GetParam());

    EXPECT_EQ(-1, csc->nz);
    EXPECT_GT(csc->nzmax, 0);

    EXPECT_EQ(write_mtx(triplet), write_mtx(csc));
}

INSTANTIATE_TEST_SUITE_P(
        CSCTest,
        CSCTest,
        ::testing::Values("eye3.mtx"
                          , "kepner_gilbert_graph.mtx"
                          , "vector_array.mtx"
                          , "vector_coordinate.mtx"
                          , "vector_array.mtx"
                          , "vector_coordinate.mtx"
                          , "eye3_pattern.mtx"
        ));