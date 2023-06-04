// Copyright (C) 2023 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
// SPDX-License-Identifier: BSD-2-Clause

#include <sstream>

#include "fmm_bench.hpp"

using VT = double;
static int num_iterations = 3;

auto csc_to_write = construct_csc<int64_t, VT>(kCoordTargetBytes, 1000);

/**
 * Write CSC.
 */
static void csc_write(benchmark::State& state) {


    std::size_t num_bytes = 0;

    fast_matrix_market::write_options options;
    options.parallel_ok = true;
    options.num_threads = (int)state.range(0);

    for ([[maybe_unused]] auto _ : state) {

        std::ostringstream oss;

        fast_matrix_market::write_matrix_market_csc(oss,
                                                    {csc_to_write.nrows, csc_to_write.ncols},
                                                    csc_to_write.indptr, csc_to_write.indices, csc_to_write.vals,
                                                    false,
                                                    options);

        num_bytes += oss.str().size();
        benchmark::ClobberMemory();
    }

    state.SetBytesProcessed((int64_t)num_bytes);
}

BENCHMARK(csc_write)->Name("op:write/matrix:CSC/impl:FMM/lang:C++")->UseRealTime()->Iterations(num_iterations)->Apply(NumThreadsArgument);
