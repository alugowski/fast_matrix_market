// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#include <sstream>

#include "fmm_bench.hpp"

/**
 * IO streams are slow so use a small target.
 */
constexpr int64_t kIOStreamTargetRead = 10 * 2 << 20;

/**
 * Read triplets using iostreams.
 *
 * Don't bother with parallelism because iostreams only get slower due to internal locking on the locale.
 */
static void triplet_read_iostream(benchmark::State& state) {
    std::string large = construct_large_coord_string(kIOStreamTargetRead);

    std::size_t num_bytes = 0;

    for ([[maybe_unused]] auto _ : state) {
        std::istringstream iss(large);

        int64_t row, col;
        double value;

        while (!iss.eof()) {
            iss >> row >> col >> value;

            benchmark::DoNotOptimize(row);
            benchmark::DoNotOptimize(col);
            benchmark::DoNotOptimize(value);
        }

        num_bytes += large.size();
    }

    state.SetBytesProcessed((int64_t)num_bytes);
}

BENCHMARK(triplet_read_iostream)->Name("IOStream read");


/**
 * Write triplets using iostreams.
 *
 * Don't bother with parallelism because iostreams only get slower due to internal locking on the locale.
 */
static void triplet_write_iostream(benchmark::State& state) {
    auto triplet = construct_triplet<int64_t, double>(kIOStreamTargetRead);

    std::size_t num_bytes = 0;

    for ([[maybe_unused]] auto _ : state) {
        std::ostringstream oss;

        auto row = std::begin(triplet.rows);
        auto col = std::begin(triplet.cols);
        auto val = std::begin(triplet.vals);

        while (row != std::end(triplet.rows)) {
            oss << *row << " " << *col << " " << *val << "\n";
            ++row;
            ++col;
            ++val;
        }

        num_bytes += oss.str().size();
    }

    state.SetBytesProcessed((int64_t)num_bytes);
}

BENCHMARK(triplet_write_iostream)->Name("IOStream write");
