// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#include <sstream>

#include "fmm_bench.hpp"


/**
 * Read dense array.
 */
static void array_read(benchmark::State& state) {
    // Generate a big matrix in-memory.
    auto array = construct_array<double>(kInMemoryByteTargetRead);

    std::ostringstream oss;
    fast_matrix_market::matrix_market_header header(array.nrows, array.ncols);
    fast_matrix_market::write_matrix_market_array(oss, header, array.vals);
    std::string mm = oss.str();

    // read options
    fast_matrix_market::read_options options{};
    options.parallel_ok = true;
    options.num_threads = (int)state.range(0);

    std::size_t num_bytes = 0;

    for ([[maybe_unused]] auto _ : state) {
        array.vals.resize(0);

        std::istringstream iss(mm);
        fast_matrix_market::read_matrix_market_array(iss, header, array.vals, options);
        num_bytes += mm.size();
    }

    state.SetBytesProcessed((int64_t)num_bytes);
}

BENCHMARK(array_read)->Name("Array read")->Apply(NumThreadsArgument);


/**
 * Write dense array.
 */
static void array_write(benchmark::State& state) {
    auto array = construct_array<double>(kInMemoryByteTargetRead);

    std::size_t num_bytes = 0;

    fast_matrix_market::write_options options;
    options.parallel_ok = true;
    options.num_threads = (int)state.range(0);

    for ([[maybe_unused]] auto _ : state) {

        std::ostringstream oss;

        fast_matrix_market::write_matrix_market_array(oss,
                                                      fast_matrix_market::matrix_market_header(array.nrows, array.ncols),
                                                      array.vals,
                                                        options);

        num_bytes += oss.str().size();
    }

    state.SetBytesProcessed((int64_t)num_bytes);
}

BENCHMARK(array_write)->Name("Array write")->Apply(NumThreadsArgument);
