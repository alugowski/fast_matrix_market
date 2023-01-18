// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#include <sstream>

#include "fmm_bench.hpp"

using VT = double;
static int num_iterations = 3;

static std::string generate_read_string() {
    // Generate a big matrix in-memory.
    auto array = construct_array<VT>(kArrayTargetReadBytes);

    std::ostringstream oss;
    fast_matrix_market::write_matrix_market_array(oss, {array.nrows, array.ncols}, array.vals);
    return oss.str();
}

static std::string string_to_read = generate_read_string();
static auto array_to_write = construct_array<VT>(kArrayTargetReadBytes);

/**
 * Read dense array.
 */
static void array_read(benchmark::State& state) {
    // read options
    fast_matrix_market::read_options options{};
    options.parallel_ok = true;
    options.num_threads = (int)state.range(0);

    std::size_t num_bytes = 0;

    for ([[maybe_unused]] auto _ : state) {
        fast_matrix_market::matrix_market_header header;
        array_matrix<VT> array;

        std::istringstream iss(string_to_read);
        fast_matrix_market::read_matrix_market_array(iss, header, array.vals, fast_matrix_market::row_major, options);
        num_bytes += string_to_read.size();
        benchmark::ClobberMemory();
    }

    state.SetBytesProcessed((int64_t)num_bytes);
}

BENCHMARK(array_read)->Name("op:read/matrix:Array/impl:FMM/lang:C++")->UseRealTime()->Iterations(num_iterations)->Apply(NumThreadsArgument);


/**
 * Write dense array.
 */
static void array_write(benchmark::State& state) {
    std::size_t num_bytes = 0;

    fast_matrix_market::write_options options;
    options.parallel_ok = true;
    options.num_threads = (int)state.range(0);

    for ([[maybe_unused]] auto _ : state) {

        std::ostringstream oss;

        fast_matrix_market::write_matrix_market_array(oss,
                                                      {array_to_write.nrows, array_to_write.ncols},
                                                      array_to_write.vals,
                                                      fast_matrix_market::row_major,
                                                      options);

        num_bytes += oss.str().size();
        benchmark::ClobberMemory();
    }

    state.SetBytesProcessed((int64_t)num_bytes);
}

BENCHMARK(array_write)->Name("op:write/matrix:Array/impl:FMM/lang:C++")->UseRealTime()->Iterations(num_iterations)->Apply(NumThreadsArgument);
