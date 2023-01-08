// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#include "fmm_bench.hpp"

#include <numeric>

/**
 * Set thread count benchmark arguments.
 */
void NumThreadsArgument(benchmark::internal::Benchmark* b) {
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

/**
 * Constructs a large string block composed of repeated lines from kLines.
 *
 * @param byte_target size in bytes of the result
 */
std::string construct_large_coord_string(std::size_t byte_target) {
    const std::string chunk =
            "123456 234567 333.323\n"
            "1 234567 333.323\n"
            "1 2 3\n";

    std::string result;

    auto num_copies = byte_target / chunk.size();
    result.reserve(chunk.size() * num_copies);

    for (std::size_t i = 0; i < num_copies; ++i) {
        result += chunk;
    }

    return result;
}

BENCHMARK_MAIN();
