// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
#pragma once

#if defined(__clang__)
// Disable pedantic GTest warnings
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"
#endif

#include <gtest/gtest.h>

#include <fast_matrix_market/fast_matrix_market.hpp>

static const std::string kTestMatrixDir = TEST_MATRIX_DIR;  // configured in CMake

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
};

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

template <typename IT, typename VT>
bool operator==(const triplet_matrix<IT, VT>& a, const triplet_matrix<IT, VT>& b) {
    if (a.nrows != b.nrows) {
        return false;
    }
    if (a.ncols != b.ncols) {
        return false;
    }

    auto a_sorted = sorted_triplet(a);
    auto b_sorted = sorted_triplet(b);

    if (a_sorted.rows != b_sorted.rows) {
        return false;
    }

    if (a_sorted.cols != b_sorted.cols) {
        return false;
    }

    if (a_sorted.vals != b_sorted.vals) {
        return false;
    }

    return true;
}

template <typename IT, typename VT>
bool operator==(const sparse_vector<IT, VT>& a, const sparse_vector<IT, VT>& b) {
    if (a.length != b.length) {
        return false;
    }

    if (a.indices != b.indices) {
        return false;
    }

    if (a.vals != b.vals) {
        return false;
    }

    return true;
}

template <typename VT>
bool operator==(const array_matrix<VT>& a, const array_matrix<VT>& b) {
    if (a.nrows != b.nrows) {
        return false;
    }
    if (a.ncols != b.ncols) {
        return false;
    }

    if (a.vals != b.vals) {
        return false;
    }

    return true;
}