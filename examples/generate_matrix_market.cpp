// Copyright (C) 2023 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
// SPDX-License-Identifier: BSD-2-Clause

#include <fstream>
#include <iostream>
#include <random>
#include <fast_matrix_market/app/generator.hpp>

constexpr int64_t index_max = 10000000;
constexpr int64_t index_min = index_max / 10;

void generate_tuple([[maybe_unused]] int64_t coo_index, int64_t &row, int64_t &col, double& value) {
    static thread_local std::mt19937 generator{std::random_device{}()};
    std::uniform_int_distribution<int64_t> index_distribution(index_min,index_max - 1);
    std::uniform_real_distribution<double> distribution(0,1);

    row = index_distribution(generator);
    col = index_distribution(generator);
    value = distribution(generator);
}


int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "Generate a random coordinate .mtx of the given target file size." << std::endl;
        std::cout << std::endl;
        std::cout << "Usage:" << std::endl;
        std::cout << argv[0] << " <matrix_market_file_size_in_megabytes>" << std::endl;
        std::cout << std::endl;
        std::cout << "will create a file named '<filesize>MiB.mtx' in the current working directory with the specified file size." << std::endl;
        return 0;
    }

    int64_t megabytes = std::strtoll(argv[1], nullptr, 10);
    int64_t bytes = megabytes << 20;

    // approximately 25 characters per nnz
    int64_t nnz = bytes / 25;
    fast_matrix_market::write_options options;
    options.precision = 6;

    std::ofstream f{std::to_string(megabytes) + "MiB.mtx", std::ios_base::binary};
    fast_matrix_market::write_matrix_market_generated_triplet<int64_t, double>(
        f, {index_max, index_max}, nnz, generate_tuple, options);

    return 0;
}
