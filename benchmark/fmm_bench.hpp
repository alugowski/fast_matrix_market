// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include <benchmark/benchmark.h>

#include <array>
#include <vector>

#include <fast_matrix_market/fast_matrix_market.hpp>

template <typename IT, typename VT>
struct triplet_matrix {
    int64_t nrows = 0, ncols = 0;
    std::vector<IT> rows;
    std::vector<IT> cols;
    std::vector<VT> vals;

    [[nodiscard]] size_t size_bytes() const {
        return sizeof(IT)*rows.size() + sizeof(IT)*cols.size() + sizeof(VT)*vals.size();
    }
};

template <typename IT, typename VT>
struct csc_matrix {
    int64_t nrows = 0, ncols = 0;
    std::vector<IT> indptr;
    std::vector<IT> indices;
    std::vector<VT> vals;

    [[nodiscard]] size_t size_bytes() const {
        return sizeof(IT)*indptr.size() + sizeof(IT)*indices.size() + sizeof(VT)*vals.size();
    }
};

template <typename VT>
struct array_matrix {
    int64_t nrows = 0, ncols = 0;
    std::vector<VT> vals;

    [[nodiscard]] size_t size_bytes() const {
        return sizeof(VT)*vals.size();
    }
};

/**
 * Construct a test matrix.
 * @param byte_target How big the matrix tuples should be
 */
template <typename IT, typename VT>
triplet_matrix<IT, VT> construct_triplet(std::size_t byte_target) {
    int64_t num_elements = byte_target / (2*sizeof(IT) + sizeof(VT));

    triplet_matrix<IT, VT> ret;

    ret.nrows = num_elements;
    ret.ncols = num_elements;

    ret.rows.reserve(num_elements);
    ret.cols.reserve(num_elements);
    ret.vals.reserve(num_elements);

    for (auto i = 0; i < num_elements; ++i) {
        ret.rows.emplace_back(i);
        ret.cols.emplace_back(i);
        ret.vals.emplace_back(static_cast<VT>(i) / 100);
    }

    return ret;
}

/**
 * Construct a test matrix.
 * @param byte_target How big the matrix should be
 */
template <typename IT, typename VT>
csc_matrix<IT, VT> construct_csc(std::size_t byte_target, int ncols = 1000) {
    int64_t num_elements = (byte_target - ((ncols+1) * sizeof(IT))) / (2*sizeof(IT) + sizeof(VT));

    csc_matrix<IT, VT> ret;

    ret.nrows = num_elements;
    ret.ncols = ncols;

    double num_elements_per_column = ((double)num_elements) / ncols;

    ret.indptr.reserve(ncols+1);
    for (auto i = 0; i < ncols; ++i) {
        ret.indptr.emplace_back(static_cast<IT>(i * num_elements_per_column));
    }
    ret.indptr.emplace_back(num_elements);

    ret.indices.reserve(num_elements);
    ret.vals.reserve(num_elements);
    for (auto i = 0; i < num_elements; ++i) {
        ret.indices.emplace_back(i);
        ret.vals.emplace_back(static_cast<VT>(i) / 100);
    }

    return ret;
}

/**
 * Construct a test matrix.
 * @param byte_target How big the matrix tuples should be
 */
template <typename VT>
array_matrix<VT> construct_array(std::size_t byte_target) {
    int64_t num_elements = byte_target / (sizeof(VT));
    auto n = (int64_t)std::sqrt(num_elements);

    array_matrix<VT> ret;

    ret.nrows = n;
    ret.ncols = n;
    ret.vals.reserve(n*n);

    for (auto i = 0; i < n*n; ++i) {
        ret.vals.emplace_back(static_cast<VT>(i) / 100);
    }
    return ret;
}

/**
 * Constructs a large string block composed of repeated lines from kLines.
 *
 * @param byte_target size in bytes of the result
 */
std::string construct_large_coord_string(std::size_t byte_target);

// In-memory byte targets.
// MatrixMarket files will be larger.
constexpr int64_t kArrayTargetBytes = 128 * 2 << 20;
constexpr int64_t kCoordTargetBytes = 256 * 2 << 20;

/**
 * Set thread count benchmark arguments.
 */
void NumThreadsArgument(benchmark::internal::Benchmark* b);
