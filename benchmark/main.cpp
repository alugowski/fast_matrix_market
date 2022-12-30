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

BENCHMARK_MAIN();
