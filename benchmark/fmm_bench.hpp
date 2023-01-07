// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

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

constexpr int64_t kInMemoryByteTargetRead = 500 * 2 << 20;
constexpr int64_t kInMemoryByteTargetWrite = 100 * 2 << 20;

/**
 * Set thread count benchmark arguments.
 */
static void NumThreadsArgument(benchmark::internal::Benchmark* b) {
    if (std::thread::hardware_concurrency() == 0) {
        // Not defined.
        b->ArgName("p")->Arg(1);
        return;
    }

    unsigned int p = 1;

    // Do every thread count initially.
    for (; p < 8 && p < std::thread::hardware_concurrency(); ++p) {
        b->ArgName("p")->Arg(p);
    }

    // For larger thread counts do powers of two and the value in-between.
    for (; p < std::thread::hardware_concurrency(); p *= 2) {
        b->ArgName("p")->Arg(p);

        auto half_step = p + p / 2;
        if (half_step < std::thread::hardware_concurrency()) {
            b->ArgName("p")->Arg(half_step);
        }
    }

    // Always do the max threads available.
    b->ArgName("p")->Arg(std::thread::hardware_concurrency());
}
