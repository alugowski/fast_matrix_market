// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include <numeric>

#if defined(__clang__)
// Disable pedantic GTest warnings
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"
#endif

#include <gtest/gtest.h>

#include <fast_matrix_market/fast_matrix_market.hpp>

static const std::string kTestMatrixDir = TEST_MATRIX_DIR;  // configured in CMake

template <typename IT, typename VT>
struct csc_matrix {
    int64_t nrows = 0, ncols = 0;
    std::vector<IT> indptr;
    std::vector<IT> indices;
    std::vector<VT> vals;
};

template <typename IT, typename VT>
struct triplet_matrix {
    using value_type = VT;

    int64_t nrows = 0, ncols = 0;
    std::vector<IT> rows;
    std::vector<IT> cols;
    std::vector<VT> vals;
};

template <typename IT, typename VT>
struct sparse_vector {
    using value_type = VT;

    int64_t length = 0;
    std::vector<IT> indices;
    std::vector<VT> vals;
};

template <typename VT>
struct array_matrix {
    using value_type = VT;

    int64_t nrows = 0, ncols = 0;
    std::vector<VT> vals;
    fast_matrix_market::storage_order order = fast_matrix_market::row_major;

    typename std::vector<VT>::reference operator()(int64_t row, int64_t col) {
        if (order == fast_matrix_market::row_major) {
            return vals[row * ncols + col];
        } else {
            return vals[col * nrows + row];
        }
    }

    typename std::vector<VT>::const_reference operator()(int64_t row, int64_t col) const {
        if (order == fast_matrix_market::row_major) {
            return vals[row * ncols + col];
        } else {
            return vals[col * nrows + row];
        }
    }
};

template <typename FT>
std::ostream& operator<<(std::ostream& os, const std::complex<FT>& c) {
    os << c.real();
    if (c.imag() != 0) {
        os << "+" << c.imag() << "i";
    }
    return os;
}

template <typename VT>
std::ostream& operator<<(std::ostream& os, const array_matrix<VT>& arr) {
    std::string order = (arr.order == fast_matrix_market::row_major ? "row_major" : "col_major");
    os << arr.nrows << "-by-" << arr.ncols << " " << order << " dense array" << std::endl;

    for (int64_t row = 0; row < arr.nrows; ++row) {
        for (int64_t col = 0; col < arr.ncols; ++col) {
            os << " " << std::setw(5) << arr(row, col);
        }
        os << std::endl;
    }
    return os;
}

/**
 * Sort the elements of a triplet matrix for consistent equality checks.
 */
template <typename TRIPLET>
TRIPLET sorted_triplet(const TRIPLET& triplet) {
    TRIPLET ret;
    ret.nrows = triplet.nrows;
    ret.ncols = triplet.ncols;
    ret.rows.resize(triplet.rows.size());
    ret.cols.resize(triplet.cols.size());
    ret.vals.resize(triplet.vals.size());

    // Find sort permutation
    std::vector<std::size_t> p(triplet.rows.size());
    std::iota(p.begin(), p.end(), 0);
    std::sort(p.begin(), p.end(),
              [&](std::size_t i, std::size_t j) {
        if (triplet.rows[i] != triplet.rows[j])
            return triplet.rows[i] < triplet.rows[j];
        if (triplet.cols[i] != triplet.cols[j])
            return triplet.cols[i] < triplet.cols[j];

        // do not compare values because std::complex does not have an operator<
        return false;
    });

    // Permute rows, cols, vals
    std::transform(p.begin(), p.end(), ret.rows.begin(), [&](std::size_t i){ return triplet.rows[i]; });
    std::transform(p.begin(), p.end(), ret.cols.begin(), [&](std::size_t i){ return triplet.cols[i]; });
    std::transform(p.begin(), p.end(), ret.vals.begin(), [&](std::size_t i){ return triplet.vals[i]; });

    return ret;
}

template <typename VEC>
void print_vec(const VEC& vec, const std::string& label) {
    std::cout << label << " size=" << vec.size();
    for (const auto& v : vec) {
        std::cout << v << std::endl;
    }
}

template <typename IT, typename VT>
bool operator==(const triplet_matrix<IT, VT>& a, const triplet_matrix<IT, VT>& b) {
    if (a.nrows != b.nrows) {
        std::cout << "nrows mismatch" << std::endl;
        return false;
    }
    if (a.ncols != b.ncols) {
        std::cout << "ncols mismatch" << std::endl;
        return false;
    }

    auto a_sorted = sorted_triplet(a);
    auto b_sorted = sorted_triplet(b);

    if (a_sorted.rows != b_sorted.rows) {
        std::cout << "row indices mismatch" << std::endl;
        print_vec(a_sorted.rows, "a (sorted)");
        print_vec(b_sorted.rows, "b (sorted)");
        return false;
    }

    if (a_sorted.cols != b_sorted.cols) {
        std::cout << "col indices mismatch" << std::endl;
        print_vec(a_sorted.cols, "a (sorted)");
        print_vec(b_sorted.cols, "b (sorted)");
        return false;
    }

    if (a_sorted.vals != b_sorted.vals) {
        std::cout << "values mismatch" << std::endl;
        print_vec(a_sorted.vals, "a (sorted)");
        print_vec(b_sorted.vals, "b (sorted)");
        return false;
    }

    return true;
}

template <typename IT, typename VT>
bool operator==(const sparse_vector<IT, VT>& a, const sparse_vector<IT, VT>& b) {
    if (a.length != b.length) {
        std::cout << "length mismatch" << std::endl;
        return false;
    }

    if (a.indices != b.indices) {
        std::cout << "indices mismatch" << std::endl;
        print_vec(a.indices, "a");
        print_vec(b.indices, "b");
        return false;
    }

    if (a.vals != b.vals) {
        std::cout << "vals mismatch" << std::endl;
        print_vec(a.vals, "a");
        print_vec(b.vals, "b");
        return false;
    }

    return true;
}

template <typename VT>
bool operator==(const array_matrix<VT>& a, const array_matrix<VT>& b) {
    if (a.nrows != b.nrows) {
        std::cout << "nrows mismatch" << std::endl;
        return false;
    }
    if (a.ncols != b.ncols) {
        std::cout << "ncols mismatch" << std::endl;
        return false;
    }

    if (a.vals != b.vals) {
        std::cout << "vals mismatch" << std::endl;
        print_vec(a.vals, "a");
        print_vec(b.vals, "b");
        return false;
    }

    return true;
}

template <typename VT>
bool operator!=(const array_matrix<VT>& a, const array_matrix<VT>& b) {
    return (a.nrows != b.nrows || a.ncols != b.ncols || a.vals != b.vals);
}
