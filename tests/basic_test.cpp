// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#include <filesystem>
#include <numeric>
#include <fstream>

#include "fmm_tests.hpp"

// for TYPED_TEST_SUITE
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"


template <typename TRIPLET>
void read_triplet_file(const std::string& matrix_filename, TRIPLET& triplet) {
    std::ifstream f(kTestMatrixDir + "/" + matrix_filename);
    fast_matrix_market::read_options options{};
    options.chunk_size_bytes = 1;

    fast_matrix_market::matrix_market_header header;
    fast_matrix_market::read_matrix_market_triplet(f, header, triplet.rows, triplet.cols, triplet.vals, options);
    triplet.nrows = header.nrows;
    triplet.ncols = header.ncols;
}

template <typename ARRAY>
void read_array_file(const std::string& matrix_filename, ARRAY& array) {
    std::ifstream f(kTestMatrixDir + "/" + matrix_filename);
    fast_matrix_market::read_options options{};
    options.chunk_size_bytes = 1;

    fast_matrix_market::matrix_market_header header;
    fast_matrix_market::read_matrix_market_array(f, header, array.vals, options);
    array.nrows = header.nrows;
    array.ncols = header.ncols;
}

//template <typename VECTOR>
//void read_vector_file(const std::string& matrix_filename, VECTOR& vec) {
//    std::ifstream f(kTestMatrixDir + "/" + matrix_filename);
//    fast_matrix_market::read_options options{};
//    options.chunk_size_bytes = 1;
//
//    fast_matrix_market::matrix_market_header header;
//    fast_matrix_market::read_matrix_market_vector(f, header, vec.indices, vec.vals, options);
//    vec.length = header.vector_length;
//}

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
bool expected(const VECTOR& mat, int64_t nrows, int64_t ncols, int64_t index_sum, typename VECTOR::value_type value_sum) {
    if (mat.nrows != nrows) {
        return false;
    }
    if (mat.ncols != ncols) {
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
        const std::string kInvalidMatrixDir = kTestMatrixDir + "invalid/";

        std::vector<std::string> ret;
        for (const auto & mtx : std::filesystem::directory_iterator(kInvalidMatrixDir)) {
            ret.emplace_back(mtx.path().filename());
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
}

/**
 * Very basic tests: Array
 */
template <typename MAT>
class PlainArraySuite : public testing::Test {
    MAT ignored;
};

using PlainArrayTypes = ::testing::Types<
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
//    EXPECT_EQ(array, array2);


    sparse_vector<int64_t, TypeParam> vec;
//    EXPECT_THROW(read_vector_file("eye3.mtx", vec), fast_matrix_market::invalid_mm);
}

TEST(PlainVectorSuite, Complex) {
}

/**
 * Very basic tests: Patterns
 */
class PatternSuite : public testing::TestWithParam<std::string> {

};

/**
 * Very basic tests: Symmetries
 */
class SymmetrySuite : public testing::TestWithParam<std::string> {
    // Test that symmetry is reported in header
    // Test that symmetry is generalized
    // Test non-dupe loader (no duplicate main diagonal elements)

    // Test against expected general version
};


#pragma clang diagnostic pop