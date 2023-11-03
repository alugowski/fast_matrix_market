// Copyright (C) 2023 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
// SPDX-License-Identifier: BSD-2-Clause

#include <filesystem>
#include <fstream>
#include <iostream>
#include <numeric>
#include <fast_matrix_market/fast_matrix_market.hpp>

namespace fmm = fast_matrix_market;

template <typename IT, typename VT>
void sort_file(const std::filesystem::path& in_path, const std::filesystem::path& out_path) {
    std::vector<IT> original_rows, sorted_rows;
    std::vector<IT> original_cols, sorted_cols;
    std::vector<VT> original_vals, sorted_vals;

    fmm::matrix_market_header header;
    
    // Load
    {
        fmm::read_options options;
        options.generalize_symmetry = false;
        std::ifstream f(in_path);
        fmm::read_matrix_market_triplet(f, header, original_rows, original_cols, original_vals, options);
    }

    // Find sort permutation
    std::vector<std::size_t> perm(original_rows.size());
    std::iota(perm.begin(), perm.end(), 0);
    std::sort(perm.begin(), perm.end(),
              [&](std::size_t i, std::size_t j) {
                  if (original_rows[i] != original_rows[j])
                      return original_rows[i] < original_rows[j];
                  if (original_cols[i] != original_cols[j])
                      return original_cols[i] < original_cols[j];

                  return false;
              });

    // Apply permutation
    sorted_rows.reserve(original_rows.size());
    std::transform(perm.begin(), perm.end(), std::back_inserter(sorted_rows), [&](auto i) { return original_rows[i]; });
    original_rows = std::vector<IT>();

    sorted_cols.reserve(original_cols.size());
    std::transform(perm.begin(), perm.end(), std::back_inserter(sorted_cols), [&](auto i) { return original_cols[i]; });
    original_cols = std::vector<IT>();

    sorted_vals.reserve(original_vals.size());
    std::transform(perm.begin(), perm.end(), std::back_inserter(sorted_vals), [&](auto i) { return original_vals[i]; });
    original_vals = std::vector<VT>();

    // Write
    {
        fmm::write_options options;
        options.fill_header_field_type = false;
        std::ofstream f(out_path);
        fmm::write_matrix_market_triplet(f, header, sorted_rows, sorted_cols, sorted_vals, options);
    }
}


int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "Sort the elements of a .mtx file by coordinate (row, column)." << std::endl;
        std::cout << std::endl;
        std::cout << "Usage:" << std::endl;
        std::cout << argv[0] << " <file>.mtx" << std::endl;
        std::cout << std::endl;
        std::cout << "will create a file named '<file>.sorted.mtx' in the current working directory." << std::endl;
        return 0;
    }

    std::filesystem::path in_path{argv[1]};
    std::filesystem::path out_path{argv[1]};
    out_path.replace_extension(".sorted.mtx");

    // find the type
    fmm::matrix_market_header header;
    {
        std::ifstream f(in_path);
        fmm::read_header(f, header);
    }

    if (header.format == fmm::array) {
        std::cout << "Array .mtx file is already sorted." << std::endl;
        return 0;
    }

    sort_file<int64_t, std::string>(in_path, out_path);

    return 0;
}
