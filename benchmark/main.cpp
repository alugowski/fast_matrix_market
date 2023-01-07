// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#include "fmm_bench.hpp"

#include <numeric>

const std::array<std::string, 3> kLines = {
    "123456 234567 333.323",
    "1 234567 333.323",
    "1 2 3",
};

const std::array<std::string, 2> kIntStrings = {
    "123456",
    "1",
};

const std::array<std::string, 3> kDoubleStrings = {
    "123456",
    "1",
    "333.323",
};

/**
 * Constructs a large string block composed of repeated lines from kLines.
 *
 * @param byte_target size in bytes of the result
 */
std::string ConstructManyLines(std::size_t byte_target) {
    std::vector<char> chunk;
    for (const auto& line : kLines) {
        std::copy(std::begin(line), std::end(line), std::back_inserter(chunk));
        chunk.emplace_back('\n');
    }

    std::vector<char> result;
    result.reserve(byte_target + chunk.size() + 1);

    while (result.size() < byte_target) {
        std::copy(std::begin(chunk), std::end(chunk), std::back_inserter(result));
    }
    result.emplace_back(0);

    return {result.data()};
}

/**
 * Large string with many lines.
 */
const std::string kLineBlock = ConstructManyLines(50u << 20u);

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

BENCHMARK_MAIN();
