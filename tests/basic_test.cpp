// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#include <filesystem>
#include <numeric>
#include <fstream>
#include <regex>

#include "fmm_tests.hpp"

// for TYPED_TEST_SUITE
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"


template <typename TRIPLET>
void read_triplet_file(const std::string& matrix_filename, TRIPLET& triplet, fast_matrix_market::read_options options = {}) {
    std::ifstream f(kTestMatrixDir + "/" + matrix_filename);
    options.chunk_size_bytes = 1;

    fast_matrix_market::read_matrix_market_triplet(f, triplet.nrows, triplet.ncols, triplet.rows, triplet.cols, triplet.vals, options);
}

template <typename ARRAY>
void read_array_file(const std::string& matrix_filename, ARRAY& array, fast_matrix_market::read_options options = {}) {
    std::ifstream f(kTestMatrixDir + "/" + matrix_filename);
    options.chunk_size_bytes = 1;

    fast_matrix_market::read_matrix_market_array(f, array.nrows, array.ncols, array.vals, options);
}

template <typename VECTOR>
void read_vector_file(const std::string& matrix_filename, VECTOR& vec) {
    std::ifstream f(kTestMatrixDir + "/" + matrix_filename);
    fast_matrix_market::read_options options{};
    options.chunk_size_bytes = 1;

    fast_matrix_market::read_matrix_market_doublet(f, vec.length, vec.indices, vec.vals, options);
}

template <typename VECTOR>
void read_vector_string(const std::string& str, VECTOR& vec, fast_matrix_market::matrix_market_header& header) {
    std::istringstream f(str);
    fast_matrix_market::read_options options{};
    options.chunk_size_bytes = 1;

    fast_matrix_market::read_matrix_market_doublet(f, header, vec.indices, vec.vals, options);
    vec.length = header.vector_length;
}

template <typename VECTOR>
std::string write_vector_string(VECTOR& vec, fast_matrix_market::matrix_market_header header = {}) {
    std::ostringstream f;

    header.vector_length = vec.length;
    fast_matrix_market::write_matrix_market_doublet(f, header, vec.indices, vec.vals);
    vec.length = header.vector_length;

    return f.str();
}

template <typename TRIPLET>
bool expected(const TRIPLET& mat, int64_t nrows, int64_t ncols, int64_t rows_sum, int64_t cols_sum, typename TRIPLET::value_type value_sum) {
    if (mat.nrows != nrows) {
        return false;
    }
    if (mat.ncols != ncols) {
        return false;
    }

    if (std::accumulate(std::begin(mat.rows), std::end(mat.rows), 0L) != rows_sum) {
        return false;
    }

    if (std::accumulate(std::begin(mat.cols), std::end(mat.cols), 0L) != cols_sum) {
        return false;
    }

    if (std::accumulate(std::begin(mat.vals), std::end(mat.vals), typename TRIPLET::value_type{}) != value_sum) {
        return false;
    }

    return true;
}

template <typename VECTOR>
bool expected_vec(const VECTOR& mat, int64_t length, int64_t index_sum, typename VECTOR::value_type value_sum) {
    if (mat.length != length) {
        return false;
    }

    if (std::accumulate(std::begin(mat.indices), std::end(mat.indices), 0L) != index_sum) {
        return false;
    }

    if (std::accumulate(std::begin(mat.vals), std::end(mat.vals), typename VECTOR::value_type{}) != value_sum) {
        return false;
    }

    return true;
}

template <typename ARRAY>
bool expected(const ARRAY& mat, int64_t nrows, int64_t ncols, typename ARRAY::value_type value_sum) {
    if (mat.nrows != nrows) {
        return false;
    }
    if (mat.ncols != ncols) {
        return false;
    }

    if (std::accumulate(std::begin(mat.vals), std::end(mat.vals), typename ARRAY::value_type{}) != value_sum) {
        return false;
    }

    return true;
}

class InvalidSuite : public testing::TestWithParam<std::string> {
public:
    static std::vector<std::string> get_invalid_matrix_files() {
        const std::string kInvalidSubdir = "invalid/";
        const std::string kInvalidMatrixDir = kTestMatrixDir + kInvalidSubdir;

        std::vector<std::string> ret;
        for (const auto & mtx : std::filesystem::directory_iterator(kInvalidMatrixDir)) {
            ret.push_back(kInvalidSubdir + mtx.path().filename().string());
        }

        std::sort(ret.begin(), ret.end());
        return ret;
    }

    triplet_matrix<int64_t, double> triplet_ld;
    triplet_matrix<int64_t, std::complex<double>> triplet_lc;
};

TEST_P(InvalidSuite, Small) {
    EXPECT_THROW(read_triplet_file(GetParam(), triplet_ld), fast_matrix_market::invalid_mm);
    EXPECT_THROW(read_triplet_file(GetParam(), triplet_lc), fast_matrix_market::invalid_mm);
}

INSTANTIATE_TEST_SUITE_P(Invalid, InvalidSuite, testing::ValuesIn(InvalidSuite::get_invalid_matrix_files()));

/**
 * Very basic tests: Triplet
 */
template <typename MAT>
class PlainTripletSuite : public testing::Test {
    MAT ignored;
};

using PlainTripletTypes = ::testing::Types<
        triplet_matrix<int64_t, bool>,
        triplet_matrix<int64_t, int32_t>,
        triplet_matrix<int64_t, float>,
        triplet_matrix<int64_t, double>,
        triplet_matrix<int64_t, std::complex<double>>,
        triplet_matrix<int64_t, long double>
        >;
TYPED_TEST_SUITE(PlainTripletSuite, PlainTripletTypes);


TYPED_TEST(PlainTripletSuite, Basic) {
    TypeParam triplet, triplet2;
    read_triplet_file("eye3.mtx", triplet);
    EXPECT_TRUE(expected(triplet, 3, 3, 3, 3, 3));

    read_triplet_file("eye3_pattern.mtx", triplet2);
    EXPECT_EQ(triplet, triplet2);
}

TEST(PlainTripletSuite, Complex) {
    triplet_matrix<int64_t, std::complex<double>> triplet, triplet2;
    read_triplet_file("eye3.mtx", triplet);

    read_triplet_file("eye3_complex.mtx", triplet2);
    EXPECT_EQ(triplet, triplet2);

    triplet_matrix<int64_t, double> non_complex;
    EXPECT_THROW(read_triplet_file("eye3_complex.mtx", non_complex), fast_matrix_market::complex_incompatible);
}

/**
 * Very basic tests: Array
 */
template <typename MAT>
class PlainArraySuite : public testing::Test {
    MAT ignored;
};

using PlainArrayTypes = ::testing::Types<
        array_matrix<bool>,
        array_matrix<int32_t>,
        array_matrix<float>,
        array_matrix<double>,
        array_matrix<std::complex<double>>,
        array_matrix<long double>
>;
TYPED_TEST_SUITE(PlainArraySuite, PlainArrayTypes);


TYPED_TEST(PlainArraySuite, Basic) {
    TypeParam array, array2, array3;
    read_array_file("eye3.mtx", array);
    EXPECT_TRUE(expected(array, 3, 3, 3));

    read_array_file("eye3_pattern.mtx", array2);
    EXPECT_EQ(array, array2);

    read_array_file("eye3_array.mtx", array3);
    EXPECT_EQ(array, array3);
}

TEST(PlainArraySuite, Complex) {
    array_matrix<std::complex<double>> array, array2;
    read_array_file("eye3.mtx", array);
    read_array_file("eye3_complex.mtx", array2);

    EXPECT_EQ(array, array2);

    array_matrix<double> non_complex;
    EXPECT_THROW(read_array_file("eye3_complex.mtx", non_complex), fast_matrix_market::complex_incompatible);
}

/**
 * Very basic tests: Patterns
 */
template <typename VT>
class PlainVectorSuite : public testing::Test {
    VT ignored{};
};

using PlainVectorTypes = ::testing::Types<int32_t, float, double, std::complex<double>, long double>;
TYPED_TEST_SUITE(PlainVectorSuite, PlainVectorTypes);


TYPED_TEST(PlainVectorSuite, Basic) {
    // Read a vector file into a triplet
    triplet_matrix<int64_t, TypeParam> triplet;
    read_triplet_file("vector_coordinate.mtx", triplet);
    EXPECT_TRUE(expected(triplet, 4, 1, 4, 0, 707));

    // Read a vector file into an array
    array_matrix<TypeParam> array, array2;
    read_array_file("vector_coordinate.mtx", array);
    read_array_file("vector_array.mtx", array2);
    EXPECT_TRUE(expected(array, 4, 1, 707));
    EXPECT_EQ(array, array2);

    // Read vectors
    sparse_vector<int64_t, TypeParam> vec;
    read_vector_file("vector_coordinate.mtx", vec);
    EXPECT_TRUE(expected_vec(vec, 4, 4, 707));

    sparse_vector<int64_t, TypeParam> vec_from_triplet;
    vec_from_triplet.length = triplet.nrows;
    vec_from_triplet.indices = triplet.rows;
    vec_from_triplet.vals = triplet.vals;
    EXPECT_EQ(vec, vec_from_triplet);

    // write out the vector
    fast_matrix_market::matrix_market_header header;
    std::string vec_str = write_vector_string(vec);
    sparse_vector<int64_t, TypeParam> vec_from_string;
    read_vector_string(vec_str, vec_from_string, header);
    EXPECT_EQ(vec, vec_from_string);
}

TEST(PlainVectorSuite, Complex) {
}

/**
 * Simple header tests
 */

TEST(Header, Comment) {
    // Create a vector
    sparse_vector<int64_t, double> vec;
    read_vector_file("vector_coordinate.mtx", vec);

    fast_matrix_market::matrix_market_header header, header2;
    header.vector_length = vec.length;

    // set a comment
    header.comment = "multi-line\ncomment";

    // write and read into header2
    std::string vec_str = write_vector_string(vec, header);
    read_vector_string(vec_str, vec, header2);

    EXPECT_EQ(header.comment, header2.comment);
}

/**
 * Describe a symmetry test problem.
 */
struct symmetry_problem {
    std::string symmetric;
    std::string general;
    std::string general_dup;

    bool operator<(const symmetry_problem& rhs) const {
        return symmetric < rhs.symmetric;
    }
};

std::ostream& operator<<(std::ostream& os, const symmetry_problem& p) {
    os << p.symmetric;
    return os;
}

/**
 * Very basic tests: Symmetries
 */
class SymmetrySuite : public testing::TestWithParam<symmetry_problem> {
public:


    static std::vector<symmetry_problem> get_symmetry_problems() {
        const std::string kSymmetrySubdir = "symmetry/";
        const std::string kSymmetryMatrixDir = kTestMatrixDir + kSymmetrySubdir;

        std::vector<symmetry_problem> ret;
        for (const auto & mtx : std::filesystem::directory_iterator(kSymmetryMatrixDir)) {
            auto filename = mtx.path().filename().string();
            if (!fast_matrix_market::ends_with(filename, "_general.mtx")) {
                continue;
            }

            symmetry_problem p{};
            p.symmetric = kSymmetrySubdir + std::regex_replace(filename, std::regex("_general"), "");
            p.general = kSymmetrySubdir + filename;
            p.general_dup = kSymmetrySubdir + std::regex_replace(filename, std::regex("_general"), "_general_dup");
            ret.push_back(p);
        }

        std::sort(ret.begin(), ret.end());
        return ret;
    }
};

using SymMat = triplet_matrix<int64_t, std::complex<double>>;
TEST_P(SymmetrySuite, Small) {
    SymMat symmetric, sym_zero, sym_dup, general_zero, general_dup;

    fast_matrix_market::read_options ro_no_gen{};
    ro_no_gen.generalize_symmetry = false;

    fast_matrix_market::read_options ro_gen_zero{};
    ro_gen_zero.generalize_symmetry = true;
    ro_gen_zero.generalize_coordinate_diagnonal_values = fast_matrix_market::read_options::ExtraZeroElement;

    fast_matrix_market::read_options ro_gen_dup{};
    ro_gen_dup.generalize_symmetry = true;
    ro_gen_dup.generalize_coordinate_diagnonal_values = fast_matrix_market::read_options::DuplicateElement;

    symmetry_problem p = GetParam();
    read_triplet_file(p.symmetric, symmetric, ro_no_gen);
    read_triplet_file(p.symmetric, sym_zero, ro_gen_zero);
    read_triplet_file(p.symmetric, sym_dup, ro_gen_dup);
    read_triplet_file(p.general, general_zero, ro_no_gen);
    read_triplet_file(p.general_dup, general_dup, ro_no_gen);

    EXPECT_EQ(symmetric.nrows, sym_zero.nrows);
    EXPECT_EQ(symmetric.ncols, sym_zero.ncols);
    EXPECT_EQ(symmetric.vals.size() * 2, sym_zero.vals.size());
    EXPECT_EQ(sym_dup.vals.size(), sym_zero.vals.size());
    EXPECT_EQ(sym_zero, general_zero);
    EXPECT_EQ(sym_dup, general_dup);
}

INSTANTIATE_TEST_SUITE_P(SymmetrySuite, SymmetrySuite, testing::ValuesIn(SymmetrySuite::get_symmetry_problems()));


#pragma clang diagnostic pop