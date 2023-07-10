// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
// SPDX-License-Identifier: BSD-2-Clause

#include <fstream>

#include <fast_matrix_market/fast_matrix_market.hpp>

#include "fmm_tests.hpp"

// app/user_type_string.hpp defines the std::string user type.
// It is now bundled with the main header for ease of use.
// To define your own user type, define the methods defined in that header and read the note at the top of the file.

using StrMat = triplet_matrix<int64_t, std::string>;

/**
 * Dump a test matrix file to string.
 */
std::string read_file(const std::string& matrix_filename) {
    std::ifstream f(kTestMatrixDir + "/" + matrix_filename);
    std::stringstream buffer;
    buffer << f.rdbuf();
    return buffer.str();
}

/**
 * Read MatrixMarket to a triplet.
 * @param mm string that contains the contents of a MatrixMarket file
 */
std::pair<StrMat, fast_matrix_market::matrix_market_header> read_triplet(const std::string& mm) {
    std::istringstream f(mm);

    StrMat triplet;
    fast_matrix_market::matrix_market_header header;
    fast_matrix_market::read_matrix_market_triplet(f, header, triplet.rows, triplet.cols, triplet.vals);
    triplet.nrows = header.nrows;
    triplet.ncols = header.ncols;
    return std::make_pair(triplet, header);
}

/**
 * Write a triplet to MatrixMarket.
 *
 * @param triplet matrix to write
 * @return string contents of MatrixMarket file
 */
std::string write_triplet(const StrMat& triplet, const fast_matrix_market::matrix_market_header& header) {
    fast_matrix_market::write_options options;
    options.fill_header_field_type = false;
    std::ostringstream f;
    fast_matrix_market::write_matrix_market_triplet(f, header, triplet.rows, triplet.cols, triplet.vals, options);
    return f.str();
}


TEST(UserTypeTest, String) {
    {
        std::string orig = read_file("eye3_str.mtx");
        auto [m, header] = read_triplet(orig);
        EXPECT_EQ(m.vals.size(), 3);

        EXPECT_EQ(m.vals[0], "1");
        EXPECT_EQ(m.vals[1], "1.0");
        EXPECT_EQ(m.vals[2], "1E0");

        // Test writing.
        std::string out = write_triplet(m, header);
        EXPECT_EQ(orig, out);
    }
    {
        std::string orig = read_file("eye3_pattern.mtx");
        auto [m, header] = read_triplet(orig);
        EXPECT_EQ(m.vals.size(), 3);

        EXPECT_EQ(m.vals[0], "");
        EXPECT_EQ(m.vals[1], "");
        EXPECT_EQ(m.vals[2], "");

        // Test writing.
        std::string out = write_triplet(m, header);
        EXPECT_EQ(orig, out);
    }
    {
        std::string orig = read_file("eye3_complex.mtx");
        auto [m, header] = read_triplet(orig);
        EXPECT_EQ(m.vals.size(), 3);

        EXPECT_EQ(m.vals[0], "1 0");
        EXPECT_EQ(m.vals[1], "1 0");
        EXPECT_EQ(m.vals[2], "1 0");

        // Test writing.
        std::string out = write_triplet(m, header);
        EXPECT_EQ(orig, out);
    }
    {
        std::string orig = read_file("vector_array.mtx");
        auto [m, header] = read_triplet(orig);
        EXPECT_EQ(m.vals.size(), 4);

        EXPECT_EQ(m.vals[0], "101");
        EXPECT_EQ(m.vals[1], "202");
        EXPECT_EQ(m.vals[2], "0");
        EXPECT_EQ(m.vals[3], "404");
    }
}