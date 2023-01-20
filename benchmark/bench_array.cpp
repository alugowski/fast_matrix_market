// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#include <sstream>

#include "fmm_bench.hpp"

using VT = double;
static int num_iterations = 3;

template <typename ARR>
static std::string generate_read_string(const ARR& array) {
    std::ostringstream oss;
    fast_matrix_market::write_matrix_market_array(oss, {array.nrows, array.ncols}, array.vals);
    std::string ret = oss.str();

    // Set a locale for comma separations
    if (std::locale("").name() == "C" || std::locale("").name().empty()) {
        std::cout.imbue(std::locale("en_US"));
    } else {
        std::cout.imbue(std::locale(""));
    }
    std::cout << "Array matrix has " << array.vals.size() << " elements (" << array.size_bytes() << " bytes) for " << ret.size() << " bytes in MatrixMarket format." << std::endl;
    return ret;
}

auto array_to_write = construct_array<VT>(kArrayTargetBytes);
std::string array_string_to_read = generate_read_string(array_to_write);

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

        std::istringstream iss(array_string_to_read);
        fast_matrix_market::read_matrix_market_array(iss,
                                                     header,
                                                     array.vals,
                                                     fast_matrix_market::col_major,
                                                     options);
        num_bytes += array_string_to_read.size();
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
                                                      fast_matrix_market::col_major,
                                                      options);

        num_bytes += oss.str().size();
        benchmark::ClobberMemory();
    }

    state.SetBytesProcessed((int64_t)num_bytes);
}

BENCHMARK(array_write)->Name("op:write/matrix:Array/impl:FMM/lang:C++")->UseRealTime()->Iterations(num_iterations)->Apply(NumThreadsArgument);
