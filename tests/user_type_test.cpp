// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
// SPDX-License-Identifier: BSD-2-Clause

#include <fstream>

// Need some types from fast_matrix_market but cannot include the main header yet.
// Include just the types so they can be used.
#include <fast_matrix_market/types.hpp>

// Test arbitrary types as the value type of the matrix.
// Use T=std::string.
//
// Must declare read, write, and manipulation methods.
// These must be declared before they're used, so before #include <fast_matrix_market/fast_matrix_market.hpp>
namespace fast_matrix_market {
    /**
     * (Needed for read) Parse a value.
     *
     * This method must find the end of the value, too. Likely the next newline or end of string is the end.
     *
     * @param pos starting character
     * @param end end of string. Do not dereference >end.
     * @param out out parameter of parsed value
     * @return a pointer to the next character following the value. Likely the newline.
     */
    const char *read_value(const char *pos, const char *end, std::string &out, [[maybe_unused]] const read_options& options = {}) {
        while (pos != end && *pos != '\n') {
            out += *pos;
            ++pos;
        }
        return pos;
    }

    /**
     * (Needed for read) Used to handle skew-symmetric files. Must be defined regardless.
     */
    std::string negate(const std::string& o) {
        return "-" + o;
    }

    /**
     * (Needed for read) Default value to set for patterns (if option selected). Must be defined regardless.
     */
    std::string pattern_default_value([[maybe_unused]] const std::string* type) {
        return "1";
    }

    // If using default dense array loader, type must also work with std::plus<>.

    /**
     * (Needed for write) Used to determine what field type to set in the MatrixMarket header.
     */
    field_type get_field_type([[maybe_unused]] const std::string* type) {
        return real;
    }

    /**
     * (Needed for write) Write a value.
     */
    std::string value_to_string(const std::string& value, [[maybe_unused]] int precision) {
        return value;
    }
}

// Now we can include the main header.
#include <fast_matrix_market/fast_matrix_market.hpp>

#include "fmm_tests.hpp"


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
StrMat read_triplet(const std::string& mm) {
    std::istringstream f(mm);

    StrMat triplet;
    fast_matrix_market::read_matrix_market_triplet(f, triplet.nrows, triplet.ncols, triplet.rows, triplet.cols, triplet.vals);
    return triplet;
}

/**
 * Write a triplet to MatrixMarket.
 *
 * @param triplet matrix to write
 * @return string contents of MatrixMarket file
 */
std::string write_triplet(const StrMat& triplet) {
    std::ostringstream f;
    fast_matrix_market::write_matrix_market_triplet(f, {triplet.nrows, triplet.ncols}, triplet.rows, triplet.cols, triplet.vals);
    return f.str();
}

TEST(UserTypeTest, String) {
    StrMat m = read_triplet(read_file("eye3_str.mtx"));
    EXPECT_EQ(m.vals.size(), 3);

    EXPECT_EQ(m.vals[0], "1");
    EXPECT_EQ(m.vals[1], "1.0");
    EXPECT_EQ(m.vals[2], "1E0");

    // Test writing.
    std::string out = write_triplet(m);
    EXPECT_EQ(m, read_triplet(out));
}