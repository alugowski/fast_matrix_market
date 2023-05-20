// Copyright (C) 2022-2023 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
// SPDX-License-Identifier: BSD-2-Clause

#include <algorithm>
#include <fstream>
#include <sstream>

#include "fmm_tests.hpp"

#if defined(__clang__)
// Disable warnings from GraphBLAS headers. They must be included with extern "C" but the complex types are defined as
// std::complex.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
#endif

#include <fast_matrix_market/app/GraphBLAS.hpp>

#if defined(__clang__)
#pragma clang diagnostic pop
#endif


/**
 * GraphBLAS needs GrB_init() to be called before any other methods, else you get a GrB_PANIC.
 * Use the GoogleTest Environment feature to call the setup and finalizers appropriately.
 */
class GraphBLASEnvironment : public ::testing::Environment {
public:
    ~GraphBLASEnvironment() override = default;

    // Override this to define how to set up the environment.
    void SetUp() override {
        GrB_init(GrB_BLOCKING);
    }

    // Override this to define how to tear down the environment.
    void TearDown() override {
        GrB_finalize();
    }
};

[[maybe_unused]] testing::Environment* const graphblas_env =
        ::testing::AddGlobalTestEnvironment(new GraphBLASEnvironment);


GrB_Matrix read_mtx(const std::string& source) {
    fast_matrix_market::read_options options;
    options.chunk_size_bytes = 1;

    std::istringstream iss(source);
    GrB_Matrix ret;
    fast_matrix_market::read_matrix_market_graphblas(iss, &ret, options);
    return ret;
}

GrB_Vector read_mtx_vec(const std::string& source) {
    fast_matrix_market::read_options options;
    options.chunk_size_bytes = 1;

    std::istringstream iss(source);
    GrB_Vector ret;
    fast_matrix_market::read_matrix_market_graphblas(iss, &ret, options);
    return ret;
}

template <typename MAT_OR_VEC>
std::string write_mtx(MAT_OR_VEC mat, bool pattern_only=false) {
    fast_matrix_market::write_options options;
    options.chunk_size_values = 1;

    fast_matrix_market::matrix_market_header header{};
    if (pattern_only) header.field = fast_matrix_market::pattern;

    std::ostringstream oss;
    fast_matrix_market::write_matrix_market_graphblas(oss, mat, options, header);
    return oss.str();
}

template <typename ARRAY>
void read_array_file(const std::string& matrix_filename, ARRAY& array, fast_matrix_market::read_options options = {}) {
    std::ifstream f(matrix_filename);
    options.chunk_size_bytes = 1;

    fast_matrix_market::read_matrix_market_array(f, array.nrows, array.ncols, array.vals, array.order, options);
}

GrB_Index nrows(const GrB_Matrix& m) {
    GrB_Index ret;
    GrB_Matrix_nrows(&ret, m);
    return ret;
}

GrB_Index ncols(const GrB_Matrix& m) {
    GrB_Index ret;
    GrB_Matrix_ncols(&ret, m);
    return ret;
}

GrB_Index nnz(const GrB_Matrix& m) {
    GrB_Index ret;
    GrB_Matrix_nvals(&ret, m);
    return ret;
}

GrB_Index nnz(const GrB_Vector& m) {
    GrB_Index ret;
    GrB_Vector_nvals(&ret, m);
    return ret;
}

GrB_Index size(const GrB_Vector& m) {
    GrB_Index ret;
    GrB_Vector_size(&ret, m);
    return ret;
}

bool element_equals(const GrB_Matrix& lhs, const GrB_Matrix& rhs, GrB_Index row, GrB_Index col) {
    // every tested type can be converted to std::complex<double>
    std::complex<double> l, r;
    auto l_ret = GxB_Matrix_extractElement_FC64(&l, lhs, row, col);
    auto r_ret = GxB_Matrix_extractElement_FC64(&r, rhs, row, col);
    if (l_ret != r_ret) {
        return false;
    }

    if (l_ret == GrB_NO_VALUE) {
        return true;
    }

    if (l_ret == GrB_SUCCESS) {
        return l == r;
    }

    return false;
}

bool element_equals(const GrB_Vector& lhs, const GrB_Vector& rhs, GrB_Index row) {
    // every tested type can be converted to std::complex<double>
    std::complex<double> l, r;
    auto l_ret = GxB_Vector_extractElement_FC64(&l, lhs, row);
    auto r_ret = GxB_Vector_extractElement_FC64(&r, rhs, row);
    if (l_ret != r_ret) {
        return false;
    }

    if (l_ret == GrB_NO_VALUE) {
        return true;
    }

    if (l_ret == GrB_SUCCESS) {
        return l == r;
    }

    return false;
}

bool element_equals(const array_matrix<std::complex<double>>& lhs, const GrB_Matrix& rhs, GrB_Index row, GrB_Index col) {
    // every tested type can be converted to std::complex<double>
    std::complex<double> l, r;
    l = lhs((int64_t)row, (int64_t)col);
    auto r_ret = GxB_Matrix_extractElement_FC64(&r, rhs, row, col);

    if (r_ret == GrB_NO_VALUE && l.real() == 0) {
        return true;
    }

    if (r_ret == GrB_SUCCESS) {
        return l == r;
    }

    return false;
}

/**
 * Equality check that prints the matrices if they're unequal. Useful for debugging when remote tests fail.
 */
bool is_equal(const GrB_Matrix& lhs, const GrB_Matrix& rhs) {
    if (nrows(lhs) != nrows(rhs) || ncols(lhs) != ncols(rhs)) {
        return false;
    }

    bool equal = true;
    for (GrB_Index row = 0; row < nrows(rhs); ++row) {
        for (GrB_Index col = 0; col < ncols(rhs); ++col) {
            if (!element_equals(lhs, rhs, row, col)) {
                equal = false;
            }
        }
    }

    if (!equal) {
        GxB_Matrix_fprint(lhs, "Left", GxB_COMPLETE, nullptr);
        GxB_Matrix_fprint(rhs, "Right", GxB_COMPLETE, nullptr);
        std::cout << std::endl << std::endl;
    }

    return equal;
}

/**
 * Equality check that prints the matrices if they're unequal. Useful for debugging when remote tests fail.
 */
bool is_equal(const array_matrix<std::complex<double>>& lhs, const GrB_Matrix& rhs) {
    if (lhs.nrows != (int64_t)nrows(rhs) || lhs.ncols != (int64_t)ncols(rhs)) {
        return false;
    }

    bool equal = true;
    for (GrB_Index row = 0; row < nrows(rhs); ++row) {
        for (GrB_Index col = 0; col < ncols(rhs); ++col) {
            if (!element_equals(lhs, rhs, row, col)) {
                equal = false;
            }
        }
    }

    if (!equal) {
        std::cout << "Left:" << std::endl << lhs << std::endl;
        GxB_Matrix_fprint(rhs, "Right", GxB_COMPLETE, nullptr);
        std::cout << std::endl << std::endl;
    }

    return equal;
}

/**
 * Equality check that prints the vectors if they're unequal. Useful for debugging when remote tests fail.
 */
bool is_equal(const GrB_Vector& lhs, const GrB_Vector& rhs) {
    bool equal = true;
    if (size(lhs) != size(rhs)) {
        equal = false;
    }

    if (equal) {
        for (GrB_Index row = 0; row < size(rhs); ++row) {
            if (!element_equals(lhs, rhs, row)) {
                equal = false;
            }
        }
    }

    if (!equal) {
        GxB_Vector_fprint(lhs, "Left", GxB_COMPLETE, nullptr);
        GxB_Vector_fprint(rhs, "Right", GxB_COMPLETE, nullptr);
        std::cout << std::endl << std::endl;
    }

    return equal;
}


bool element_equals(const array_matrix<std::complex<double>>& lhs, const GrB_Vector& rhs, GrB_Index row) {
    // every tested type can be converted to std::complex<double>
    std::complex<double> l, r;
    l = lhs((int64_t)row, (int64_t)0);
    auto r_ret = GxB_Vector_extractElement_FC64(&r, rhs, row);

    if (r_ret == GrB_NO_VALUE && l.real() == 0) {
        return true;
    }

    if (r_ret == GrB_SUCCESS) {
        return l == r;
    }

    return false;
}

/**
 * Equality check that prints the vectors if they're unequal. Useful for debugging when remote tests fail.
 */
bool is_equal(const array_matrix<std::complex<double>>& lhs, const GrB_Vector& rhs) {
    bool equal = true;
    if (lhs.nrows != (int64_t)size(rhs)) {
        equal = false;
    }

    if (equal) {
        for (GrB_Index row = 0; row < size(rhs); ++row) {
            if (!element_equals(lhs, rhs, row)) {
                equal = false;
            }
        }
    }

    if (!equal) {
        std::cout << "Left:" << std::endl << lhs << std::endl;
        GxB_Vector_fprint(rhs, "Right", GxB_COMPLETE, nullptr);
        std::cout << std::endl << std::endl;
    }

    return equal;
}

class GraphBLASMatrixTest : public testing::TestWithParam<std::string> {
public:
    ~GraphBLASMatrixTest() override {
        GrB_Matrix_free(&full);
        GrB_Matrix_free(&bitmap_col);
        GrB_Matrix_free(&bitmap_row);
        GrB_Matrix_free(&csc);
        GrB_Matrix_free(&csr);
        GrB_Matrix_free(&dcsc);
        GrB_Matrix_free(&dcsr);
    }

    void load(const std::string& param) {
        std::string load_path = kTestMatrixDir + "/" + param;

        {
            std::ifstream f(load_path);
            fast_matrix_market::read_header(f, header);
        }

#if FMM_GXB_COMPLEX
#else
        if (header.field == fast_matrix_market::complex) {
            // no complex value support in this version of GraphBLAS.
            return;
        }
#endif
        // Load dense array for correctness tests
        read_array_file(load_path, array);

        {
            std::ifstream f(load_path);
            fast_matrix_market::read_matrix_market_graphblas(f, &full);
            GxB_Matrix_Option_set(full, GxB_SPARSITY_CONTROL, GxB_FULL);
        }
        {
            std::ifstream f(load_path);
            fast_matrix_market::read_matrix_market_graphblas(f, &bitmap_col);
            GxB_Matrix_Option_set(bitmap_col, GxB_SPARSITY_CONTROL, GxB_BITMAP);
            GxB_Matrix_Option_set(bitmap_col, GxB_FORMAT, GxB_BY_COL);
        }
        {
            std::ifstream f(load_path);
            fast_matrix_market::read_matrix_market_graphblas(f, &bitmap_row);
            GxB_Matrix_Option_set(bitmap_row, GxB_SPARSITY_CONTROL, GxB_BITMAP);
            GxB_Matrix_Option_set(bitmap_row, GxB_FORMAT, GxB_BY_ROW);
        }
        {
            std::ifstream f(load_path);
            fast_matrix_market::read_matrix_market_graphblas(f, &csc);
            GxB_Matrix_Option_set(csc, GxB_SPARSITY_CONTROL, GxB_SPARSE);
            GxB_Matrix_Option_set(csc, GxB_FORMAT, GxB_BY_COL);
        }
        {
            std::ifstream f(load_path);
            fast_matrix_market::read_matrix_market_graphblas(f, &csr);
            GxB_Matrix_Option_set(csr, GxB_SPARSITY_CONTROL, GxB_SPARSE);
            GxB_Matrix_Option_set(csr, GxB_FORMAT, GxB_BY_ROW);
        }
        {
            std::ifstream f(load_path);
            fast_matrix_market::read_matrix_market_graphblas(f, &dcsc);
            GxB_Matrix_Option_set(dcsc, GxB_SPARSITY_CONTROL, GxB_HYPERSPARSE);
            GxB_Matrix_Option_set(dcsc, GxB_FORMAT, GxB_BY_COL);
        }
        {
            std::ifstream f(load_path);
            fast_matrix_market::read_matrix_market_graphblas(f, &dcsr);
            GxB_Matrix_Option_set(dcsr, GxB_SPARSITY_CONTROL, GxB_HYPERSPARSE);
            GxB_Matrix_Option_set(dcsr, GxB_FORMAT, GxB_BY_ROW);
        }
    }

    protected:
    fast_matrix_market::matrix_market_header header;
    array_matrix<std::complex<double>> array;

    GrB_Matrix full = nullptr;
    GrB_Matrix bitmap_col = nullptr;
    GrB_Matrix bitmap_row = nullptr;
    GrB_Matrix csc = nullptr;
    GrB_Matrix csr = nullptr;
    GrB_Matrix dcsc = nullptr;
    GrB_Matrix dcsr = nullptr;

    std::map<std::string, GrB_Matrix&> sparse_mats = {
            {"full", full},
            {"bitmap_col", bitmap_col},
            {"bitmap_row", bitmap_row},
            {"csc", csc},
            {"csr", csr},
            {"dcsc", dcsc},
            {"dcsr", dcsr},
    };
};

TEST_P(GraphBLASMatrixTest, SmallMatrices) {
    EXPECT_NO_THROW(load(GetParam()));

#if FMM_GXB_COMPLEX
#else
    if (header.field == fast_matrix_market::complex) {
        GTEST_SKIP() << "No complex value support in this GraphBLAS.";
    }
#endif

    EXPECT_TRUE(is_equal(csc, csr));

    // Make sure all the variants are loaded correctly.
    for (const auto& [label, mat] : sparse_mats) {
        EXPECT_EQ(header.nrows, nrows(mat));
        EXPECT_EQ(header.ncols, ncols(mat));
        EXPECT_EQ(header.nnz, nnz(mat));
        EXPECT_TRUE(is_equal(full, mat));

        // compare to non-GraphBLAS array
        EXPECT_TRUE(is_equal(array, mat));

        // Test write
        std::string s = write_mtx(mat);
        auto loaded = read_mtx(s);
        EXPECT_TRUE(is_equal(mat, loaded));
    }

    // write pattern
    std::string pattern_mtx = write_mtx(csc, true);
    GrB_Matrix pat_mat = nullptr;
    EXPECT_TRUE(pattern_mtx.find("pattern") > 0); // pattern should appear in the header
    {
        std::istringstream pat_iss(pattern_mtx);
        fast_matrix_market::read_matrix_market_array(pat_iss, array.nrows, array.ncols, array.vals, array.order);
    }
    {
        std::istringstream pat_iss(pattern_mtx);
        fast_matrix_market::read_matrix_market_graphblas(pat_iss, &pat_mat);
    }

    EXPECT_TRUE(is_equal(array, pat_mat));
    GrB_Matrix_free(&pat_mat);
}

INSTANTIATE_TEST_SUITE_P(
        GraphBLASMatrixTest,
        GraphBLASMatrixTest,
        ::testing::Values("eye3.mtx"
                , "row_3by4.mtx"
                , "2col_array.mtx"
                , "kepner_gilbert_graph.mtx"
                , "vector_array.mtx"
                , "vector_coordinate.mtx"
                , "eye3_pattern.mtx"
                , "eye3_complex.mtx"
                , "vector_coordinate_complex.mtx"
        ));

// Test %%GraphBLAS type <ctype>
TEST(GraphBLASMatrixTest, TypeComment) {
#if !FMM_GXB_TYPE_NAME
    GTEST_SKIP() << "No type interrogation support in this GraphBLAS.";
#endif

    auto filenames = std::vector{
            "matrix_bool.mtx"
            , "matrix_double.mtx"
            , "matrix_double_complex.mtx"
            , "matrix_float.mtx"
            , "matrix_float_complex.mtx"
            , "matrix_int16.mtx"
            , "matrix_int32.mtx"
            , "matrix_int64.mtx"
            , "matrix_int8.mtx"
            , "matrix_uint16.mtx"
            , "matrix_uint32.mtx"
            , "matrix_uint64.mtx"
            , "matrix_uint8.mtx"
    };
    for (auto param : filenames) {
        std::string load_path = kTestMatrixDir + "graphblas/" + param;
        fast_matrix_market::matrix_market_header header;
        {
            std::ifstream f(load_path);
            fast_matrix_market::read_header(f, header);
        }
#if FMM_GXB_COMPLEX
#else
        if (header.field == fast_matrix_market::complex) {
            // No complex value support in this GraphBLAS.
            continue;
        }
#endif
        // test ctype for matrices
        {
            GrB_Matrix mat = nullptr;

            std::ifstream f(load_path);
            fast_matrix_market::read_matrix_market_graphblas(f, &mat);

            // Test write
            std::string s = write_mtx(mat);
            auto loaded = read_mtx(s);
            EXPECT_TRUE(is_equal(mat, loaded));

            // Verify the written matrix has the same type written
            std::string type_comment = header.comment;
            EXPECT_NE(s.find(type_comment), std::string::npos);
        }
        // test ctype for vectors
        {
            GrB_Vector vec = nullptr;

            std::ifstream f(load_path);
            fast_matrix_market::read_matrix_market_graphblas(f, &vec);

            // Test write
            std::string s = write_mtx(vec);
            auto loaded = read_mtx_vec(s);
            EXPECT_TRUE(is_equal(vec, loaded));

            // Verify the written matrix has the same type written
            std::string type_comment = header.comment;
            EXPECT_NE(s.find(type_comment), std::string::npos);
        }
   }
}


/////////////////////////////////////////////////////////
///// Vectors
/////////////////////////////////////////////////////////

TEST(GraphBLASMatrixTest, Vector) {
    auto filenames = std::vector{
            "vector_array.mtx"
            , "vector_coordinate.mtx"
            , "vector_coordinate_complex.mtx"
    };
    for (auto param : filenames) {
        std::string load_path = kTestMatrixDir + param;
        fast_matrix_market::matrix_market_header header;
        GrB_Vector vec = nullptr;

        {
            std::ifstream f(load_path);
            fast_matrix_market::read_header(f, header);
        }
#if FMM_GXB_COMPLEX
#else
        if (header.field == fast_matrix_market::complex) {
            // No complex value support in this GraphBLAS.
            continue;
        }
#endif
        array_matrix<std::complex<double>> array;
        read_array_file(load_path, array);

        {
            std::ifstream f(load_path);
            fast_matrix_market::read_matrix_market_graphblas(f, &vec);
        }

        // test for correct load against a dense matrix
        EXPECT_TRUE(is_equal(array, vec));

        // Test write
        std::string s = write_mtx(vec);
        auto loaded = read_mtx_vec(s);
        EXPECT_TRUE(is_equal(vec, loaded));
    }
}