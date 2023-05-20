// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
// SPDX-License-Identifier: BSD-2-Clause

#include <filesystem>
#include <fstream>
#include <regex>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244) // '=': conversion from '__int64' to '_Ty', possible loss of data
#endif

#include "fmm_tests.hpp"

#if defined(__clang__)
// for TYPED_TEST_SUITE
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif


fast_matrix_market::matrix_market_header read_header_file(const std::string& matrix_filename) {
    std::ifstream f(kTestMatrixDir + "/" + matrix_filename);

    fast_matrix_market::matrix_market_header header;
    fast_matrix_market::read_header(f, header);
    return header;
}

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

    fast_matrix_market::read_matrix_market_array(f, array.nrows, array.ncols, array.vals, array.order, options);
}

template <typename ARRAY>
void read_array_string(const std::string& s, ARRAY& array, fast_matrix_market::read_options options = {}) {
    std::istringstream f(s);
    options.chunk_size_bytes = 1;

    fast_matrix_market::read_matrix_market_array(f, array.nrows, array.ncols, array.vals, array.order, options);
}

template <typename ARRAY>
std::string write_array_string(ARRAY& array, fast_matrix_market::write_options options = {}) {
    std::ostringstream f;
    options.chunk_size_values = 1;

    fast_matrix_market::write_matrix_market_array(f, {array.nrows, array.ncols}, array.vals, array.order, options);
    return f.str();
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

/**
 * Utilities
 */
TEST(Utils, misc) {
    EXPECT_FALSE(fast_matrix_market::ends_with("foo", "bar"));
    EXPECT_TRUE(fast_matrix_market::ends_with("foobar", "bar"));
    EXPECT_FALSE(fast_matrix_market::ends_with("", "bar"));

    EXPECT_FALSE(fast_matrix_market::starts_with("foo", "bar"));
    EXPECT_TRUE(fast_matrix_market::starts_with("foobar", "foo"));
    EXPECT_FALSE(fast_matrix_market::starts_with("", "bar"));

    EXPECT_EQ(fast_matrix_market::trim("foo"), "foo");
    EXPECT_EQ(fast_matrix_market::trim(" foo"), "foo");
    EXPECT_EQ(fast_matrix_market::trim(" \nfoo"), "foo");
    EXPECT_EQ(fast_matrix_market::trim("foo "), "foo");
    EXPECT_EQ(fast_matrix_market::trim("foo\n  \n"), "foo");

    std::string msg("error");
    EXPECT_EQ(fast_matrix_market::fmm_error(msg).what(), msg);
}

/**
 * Invalid matrices
 */
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
    triplet_matrix<int64_t, long double> triplet_lld;
};

TEST_P(InvalidSuite, Small) {
    fast_matrix_market::read_options options{};
    options.parallel_ok = true;
    EXPECT_THROW(read_triplet_file(GetParam(), triplet_ld, options), fast_matrix_market::invalid_mm);

    // Also verify sequential
    options.parallel_ok = false;
    EXPECT_THROW(read_triplet_file(GetParam(), triplet_lc, options), fast_matrix_market::invalid_mm);
    EXPECT_THROW(read_triplet_file(GetParam(), triplet_lld, options), fast_matrix_market::invalid_mm);
}

INSTANTIATE_TEST_SUITE_P(Invalid, InvalidSuite, testing::ValuesIn(InvalidSuite::get_invalid_matrix_files()));

/**
 * Permissive matrices
 *
 * These are technically invalid, but can still be read.
 */
TEST(PermissiveSuite, Small) {
    const std::string kPermissiveMatrixSubDir = "permissive/";

    triplet_matrix<int64_t, double> eye3;
    read_triplet_file("eye3.mtx", eye3);

    {
        triplet_matrix<int64_t, double> triplet_ld;
        read_triplet_file(kPermissiveMatrixSubDir + "permissive_banner_one_percent_eye3.mtx", triplet_ld);
        EXPECT_EQ(eye3, triplet_ld);
    }
    {
        triplet_matrix<int64_t, double> triplet_ld;
        read_triplet_file(kPermissiveMatrixSubDir + "permissive_banner_leading_spaces_eye3.mtx", triplet_ld);
        EXPECT_EQ(eye3, triplet_ld);
    }
}

/**
 * Overflow
 */
TEST(OverflowSuite, Small) {
    triplet_matrix<int8_t, double> triplet_8d;
    EXPECT_THROW(read_triplet_file("overflow/overflow_index_gt_int8.mtx", triplet_8d), fast_matrix_market::out_of_range);

    fast_matrix_market::read_options best_match_options{};
    fast_matrix_market::read_options throw_options{};
    throw_options.float_out_of_range_behavior = fast_matrix_market::ThrowOutOfRange;
    triplet_matrix<int64_t, float> triplet_lf;
    EXPECT_THROW(read_triplet_file("overflow/overflow_value_gt_float64.mtx", triplet_lf, throw_options), fast_matrix_market::out_of_range);
    EXPECT_NO_THROW(read_triplet_file("overflow/overflow_value_gt_float64.mtx", triplet_lf, best_match_options));

    triplet_matrix<int64_t, double> triplet_ld;
    EXPECT_THROW(read_triplet_file("overflow/overflow_value_gt_float64.mtx", triplet_ld, throw_options), fast_matrix_market::out_of_range);
    EXPECT_NO_THROW(read_triplet_file("overflow/overflow_value_gt_float64.mtx", triplet_ld, best_match_options));

    triplet_matrix<int64_t, long double> triplet_l_ld;
    EXPECT_THROW(read_triplet_file("overflow/overflow_value_gt_float128.mtx", triplet_l_ld, throw_options), fast_matrix_market::out_of_range);
    EXPECT_NO_THROW(read_triplet_file("overflow/overflow_value_gt_float128.mtx", triplet_l_ld, best_match_options));

    triplet_matrix<int64_t, std::complex<double>> triplet_lc;
    EXPECT_THROW(read_triplet_file("overflow/overflow_value_gt_complex128.mtx", triplet_lc, throw_options), fast_matrix_market::out_of_range);
    EXPECT_NO_THROW(read_triplet_file("overflow/overflow_value_gt_complex128.mtx", triplet_lc, best_match_options));

    triplet_matrix<int64_t, int64_t> triplet_l64;
    EXPECT_THROW(read_triplet_file("overflow/overflow_value_gt_int64.mtx", triplet_l64), fast_matrix_market::out_of_range);

    triplet_matrix<int64_t, int32_t> triplet_l32;
    EXPECT_THROW(read_triplet_file("overflow/overflow_value_gt_int32.mtx", triplet_l32), fast_matrix_market::out_of_range);

    triplet_matrix<int64_t, int8_t> triplet_l8;
    EXPECT_THROW(read_triplet_file("overflow/overflow_value_gt_int32.mtx", triplet_l8), fast_matrix_market::out_of_range);
}

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
    EXPECT_TRUE(expected(triplet, 3, 3, 3, 3, static_cast<typename TypeParam::value_type>(3)));

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
public:
    MAT swap_storage_order(MAT& src) {
        MAT dst;
        dst.nrows = src.nrows;
        dst.ncols = src.ncols;
        dst.vals.resize(src.vals.size());

        dst.order = (src.order == fast_matrix_market::row_major ? fast_matrix_market::col_major : fast_matrix_market::row_major);

        for (int64_t row = 0; row < dst.nrows; row++) {
            for (int64_t col = 0; col < dst.ncols; col++) {
                dst(row, col) = src(row, col);
            }
        }

        return dst;
    }
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
    EXPECT_TRUE(expected(array, 3, 3, static_cast<typename TypeParam::value_type>(3)));

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

TYPED_TEST(PlainArraySuite, StorageOrder) {
    TypeParam array_rm, array_cm, array;
    array_rm.order = fast_matrix_market::row_major;
    array_cm.order = fast_matrix_market::col_major;
    read_array_file("row_3by4.mtx", array_rm);
    read_array_file("row_3by4.mtx", array_cm);
    EXPECT_TRUE(expected(array_rm, 3, 4, static_cast<typename TypeParam::value_type>(10)));
    EXPECT_TRUE(expected(array_cm, 3, 4, static_cast<typename TypeParam::value_type>(10)));

    // ensure transposed values work as expected
    TypeParam rm_swapped = PlainArraySuite<TypeParam>::swap_storage_order(array_cm);
    EXPECT_EQ(array_rm, rm_swapped);
    TypeParam cm_swapped = PlainArraySuite<TypeParam>::swap_storage_order(array_rm);
    EXPECT_EQ(array_cm, cm_swapped);

    // ensure read/write works as expected
    array.order = fast_matrix_market::row_major;
    read_array_string(write_array_string(array_rm), array);
    EXPECT_EQ(array_rm, array);
    read_array_string(write_array_string(array_cm), array);
    EXPECT_EQ(array_rm, array);

    array.order = fast_matrix_market::col_major;
    read_array_string(write_array_string(array_cm), array);
    EXPECT_EQ(array_cm, array);
    read_array_string(write_array_string(array_rm), array);
    EXPECT_EQ(array_cm, array);
}

/**
 * Very basic tests: Patterns
 */
template <typename VT>
class PlainVectorSuite : public testing::Test {
    VT ignored{};
};

using PlainVectorTypes = ::testing::Types<int64_t, float, double, std::complex<double>, long double>;
TYPED_TEST_SUITE(PlainVectorSuite, PlainVectorTypes);


TYPED_TEST(PlainVectorSuite, Basic) {
    // Read a vector file into a triplet
    triplet_matrix<int64_t, TypeParam> triplet;
    read_triplet_file("vector_coordinate.mtx", triplet);
    EXPECT_TRUE(expected(triplet, 4, 1, 4, 0, static_cast<TypeParam>(707)));

    // Read a vector file into an array
    array_matrix<TypeParam> array, array2;
    read_array_file("vector_coordinate.mtx", array);
    read_array_file("vector_array.mtx", array2);
    EXPECT_TRUE(expected(array, 4, 1, static_cast<TypeParam>(707)));
    EXPECT_EQ(array, array2);

    // Read vectors
    sparse_vector<int64_t, TypeParam> vec;
    read_vector_file("vector_coordinate.mtx", vec);
    EXPECT_TRUE(expected_vec(vec, 4, 4, static_cast<TypeParam>(707)));

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

    static std::vector<symmetry_problem> get_array_symmetry_problems() {
        const std::string kSymmetrySubdir = "symmetry_array/";
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
            ret.push_back(p);
        }

        std::sort(ret.begin(), ret.end());
        return ret;
    }
};

using SymMat = triplet_matrix<int64_t, std::complex<double>>;
TEST_P(SymmetrySuite, SmallTripletCoordinate) {
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

class SymmetryTripletArraySuite : public SymmetrySuite {};

TEST_P(SymmetryTripletArraySuite, SmallTripletArray) {
    SymMat symmetric, symmetric_no_gen, general;

    fast_matrix_market::read_options options_no_gen{};
    options_no_gen.generalize_symmetry = false;

    fast_matrix_market::read_options options_gen{};
    options_gen.generalize_symmetry = true;

    symmetry_problem p = GetParam();
    fast_matrix_market::matrix_market_header header = read_header_file(p.symmetric);
    read_triplet_file(p.symmetric, symmetric, options_gen);
    read_triplet_file(p.symmetric, symmetric_no_gen, options_no_gen);
    read_triplet_file(p.general, general, options_gen);

    EXPECT_EQ(symmetric.nrows, general.nrows);
    EXPECT_EQ(symmetric.ncols, general.ncols);
    EXPECT_EQ(symmetric.vals.size(), fast_matrix_market::get_storage_nnz(header, options_gen));
    EXPECT_GT(symmetric.vals.size(), symmetric_no_gen.vals.size());

    if (header.symmetry == fast_matrix_market::skew_symmetric) {
        // skew symmetric matrices have zero main diagonals and the loader does not emit these zeros.
        // The general matrix does have them set, however.
        // Manually add them back in so equality can be checked.
        for (int64_t i = 0; i < header.nrows; ++i) {
            symmetric.rows.emplace_back(i);
            symmetric.cols.emplace_back(i);
            symmetric.vals.emplace_back(0);
        }
    }
    EXPECT_EQ(symmetric.vals.size(), general.vals.size());
    EXPECT_EQ(symmetric, general);
}

class SymmetryArraySuite : public SymmetrySuite {};

using SymDenseMat = array_matrix<std::complex<double>>;
TEST_P(SymmetryArraySuite, SmallArray) {
    SymDenseMat symmetric, symmetric_no_gen, general;

    fast_matrix_market::read_options options_no_gen{};
    options_no_gen.generalize_symmetry = false;

    fast_matrix_market::read_options options_gen{};
    options_gen.generalize_symmetry = true;

    symmetry_problem p = GetParam();
    read_array_file(p.symmetric, symmetric, options_gen);
    read_array_file(p.symmetric, symmetric_no_gen, options_no_gen);
    read_array_file(p.general, general, options_gen);

    EXPECT_EQ(symmetric.nrows, general.nrows);
    EXPECT_EQ(symmetric.ncols, general.ncols);
    EXPECT_EQ(symmetric.vals.size(), symmetric_no_gen.vals.size());
    EXPECT_EQ(symmetric.vals.size(), general.vals.size());
    EXPECT_NE(symmetric, symmetric_no_gen);
    EXPECT_EQ(symmetric, general);
}

INSTANTIATE_TEST_SUITE_P(SymmetrySuite, SymmetrySuite, testing::ValuesIn(SymmetrySuite::get_symmetry_problems()));
INSTANTIATE_TEST_SUITE_P(TripletLoadsArray, SymmetryTripletArraySuite, testing::ValuesIn(SymmetrySuite::get_array_symmetry_problems()));
INSTANTIATE_TEST_SUITE_P(Array, SymmetryArraySuite, testing::ValuesIn(SymmetrySuite::get_array_symmetry_problems()));

TEST(SymmetrySuite, TypeValidity) {
    // Cannot load skew-symmetric matrices into an unsigned type.
    triplet_matrix<int64_t, uint64_t> unsigned_triplet;
    array_matrix<uint64_t> unsigned_array;
    EXPECT_THROW(read_triplet_file("symmetry/coordinate_skew_symmetric_row.mtx", unsigned_triplet), fast_matrix_market::invalid_argument);
    EXPECT_THROW(read_array_file("symmetry_array/array_skew-symmetric.mtx", unsigned_array), fast_matrix_market::invalid_argument);
}

TEST(Whitespace, Whitespace) {
    triplet_matrix<int64_t, double> expected, mat;
    read_triplet_file("nist_ex1.mtx", expected);

    for (int chunk_size : {1, 10, 15, 1000}) {
        for (int p : {1, 4}) {
            fast_matrix_market::read_options options;
            options.chunk_size_bytes = chunk_size;
            options.num_threads = p;

            read_triplet_file("nist_ex1_freeformat.mtx", mat, options);
            EXPECT_EQ(mat, expected);

            read_triplet_file("nist_ex1_more_freeformat.mtx", mat, options);
            EXPECT_EQ(mat, expected);
        }
    }
}
