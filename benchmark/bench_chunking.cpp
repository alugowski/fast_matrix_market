// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#include <sstream>

#include "fmm_bench.hpp"

/**
 * Benchmark chunking.
 *
 * This needs to be fast because it is a sequential operation and can thus be a bottleneck.
 */
static void bench_chunking(benchmark::State& state) {
    std::string large = construct_large_coord_string(kCoordTargetReadBytes);
    // read options
    fast_matrix_market::read_options options{};

    std::size_t num_bytes = 0;

    for ([[maybe_unused]] auto _ : state) {
        std::istringstream iss(large);

        while (iss.good()) {
            std::string chunk = fast_matrix_market::get_next_chunk(iss, options);
            benchmark::DoNotOptimize(chunk);
        }

        num_bytes += large.size();
    }

    state.SetBytesProcessed((int64_t)num_bytes);
}

BENCHMARK(bench_chunking)->Name("op:chunking/impl:FMM/lang:C++")->UseRealTime();
