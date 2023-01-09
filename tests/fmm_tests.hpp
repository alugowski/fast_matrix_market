// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
#pragma once

// Disable pedantic GTest warnings
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"
#include <gtest/gtest.h>
#pragma clang diagnostic pop

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
};

template <typename IT, typename VT>
bool operator==(const triplet_matrix<IT, VT>& a, const triplet_matrix<IT, VT>& b) {
    if (a.nrows != b.nrows) {
        return false;
    }
    if (a.ncols != b.ncols) {
        return false;
    }

    if (a.rows != b.rows) {
        return false;
    }

    if (a.cols != b.cols) {
        return false;
    }

    if (a.vals != b.vals) {
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