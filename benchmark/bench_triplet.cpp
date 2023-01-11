// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#include <sstream>

#include "fmm_bench.hpp"

using VT = double;
static int num_iterations = 3;

static std::string generate_read_string() {
    // Generate a big matrix in-memory.
    auto triplet = construct_triplet<int64_t, VT>(kInMemoryByteTargetRead);

    std::ostringstream oss;
    fast_matrix_market::matrix_market_header header(triplet.nrows, triplet.ncols);
    fast_matrix_market::write_matrix_market_triplet(oss, header, triplet.rows, triplet.cols, triplet.vals);
    return oss.str();
}

static std::string string_to_read = generate_read_string();
static auto triplet_to_write = construct_triplet<int64_t, VT>(kInMemoryByteTargetWrite);

/**
 * Read triplets.
 */
static void triplet_read(benchmark::State& state) {
    // read options
    fast_matrix_market::read_options options{};
    options.parallel_ok = true;
    options.num_threads = (int)state.range(0);

    std::size_t num_bytes = 0;

    for ([[maybe_unused]] auto _ : state) {
        fast_matrix_market::matrix_market_header header;
        triplet_matrix<int64_t, VT> triplet;

        std::istringstream iss(string_to_read);
        fast_matrix_market::read_matrix_market_triplet(iss, header, triplet.rows, triplet.cols, triplet.vals, options);
        num_bytes += string_to_read.size();
        benchmark::ClobberMemory();
    }

    state.SetBytesProcessed((int64_t)num_bytes);
}

BENCHMARK(triplet_read)->Name("Triplet read")->UseRealTime()->Iterations(num_iterations)->Apply(NumThreadsArgument);


/**
 * Write triplets.
 */
static void triplet_write(benchmark::State& state) {


    std::size_t num_bytes = 0;

    fast_matrix_market::write_options options;
    options.parallel_ok = true;
    options.num_threads = (int)state.range(0);

    for ([[maybe_unused]] auto _ : state) {

        std::ostringstream oss;

        fast_matrix_market::write_matrix_market_triplet(oss,
                fast_matrix_market::matrix_market_header(triplet_to_write.nrows, triplet_to_write.ncols),
                triplet_to_write.rows, triplet_to_write.cols, triplet_to_write.vals,
                options);

        num_bytes += oss.str().size();
        benchmark::ClobberMemory();
    }

    state.SetBytesProcessed((int64_t)num_bytes);
}

BENCHMARK(triplet_write)->Name("Triplet write")->UseRealTime()->Iterations(num_iterations)->Apply(NumThreadsArgument);
