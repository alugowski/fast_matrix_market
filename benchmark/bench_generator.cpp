// Copyright (C) 2023 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
// SPDX-License-Identifier: BSD-2-Clause

#include <sstream>

#include "fmm_bench.hpp"
#include <fast_matrix_market/app/generator.hpp>

using VT = double;
static int num_iterations = 3;

/**
 * Write a generated identity matrix.
 */
static void generate_eye(benchmark::State& state) {
    const int64_t eye_rank = 1 << 22;

    std::size_t num_bytes = 0;

    fast_matrix_market::write_options options;
    options.parallel_ok = true;
    options.num_threads = (int)state.range(0);

    for ([[maybe_unused]] auto _ : state) {
        std::ostringstream oss;
        fast_matrix_market::write_matrix_market_generated_triplet<int64_t, VT>(
            oss, {eye_rank, eye_rank}, eye_rank,
            [](auto coo_index, auto& row, auto& col, auto& value) {
                row = coo_index;
                col = coo_index;
                value = 1;
            }, options);

        num_bytes += oss.str().size();
        benchmark::ClobberMemory();
    }

    state.SetBytesProcessed((int64_t)num_bytes);
}

BENCHMARK(generate_eye)->Name("op:write/matrix:generated_eye/impl:FMM/lang:C++")->UseRealTime()->Iterations(num_iterations)->Apply(NumThreadsArgument);
