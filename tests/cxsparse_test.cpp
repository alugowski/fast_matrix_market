// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#include <filesystem>
#include <fstream>
#include <sstream>

#include <gtest/gtest.h>

#include <fast_matrix_market/fast_matrix_market.hpp>
#include <fast_matrix_market/extras/CXSparse.hpp>
#include "fake_cxsparse/cs.hpp"

using std::filesystem::directory_iterator;

static const std::string kTestMatrixDir = TEST_MATRIX_DIR;  // configured in CMake

int count_lines(std::string s) {
    return (int)std::count(s.begin(), s.end(), '\n');
}

cs_dl *read_mtx(const std::string& path) {
    cs_dl *A;

    std::ifstream f(path);

    fast_matrix_market::matrix_market_header header;
    fast_matrix_market::read_matrix_market_cxsparse(f, &A, cs_dl_spalloc, cs_dl_entry, cs_dl_spfree);

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

    fast_matrix_market::matrix_market_header header(A->m, A->n);
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