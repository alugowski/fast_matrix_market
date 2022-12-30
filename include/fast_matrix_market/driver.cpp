// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

/**
 * Code to call methods so they get compiled.
 *
 * TODO: get rid of this.
 */

#include <iostream>
#include <vector>
#include "fast_matrix_market.hpp"

int main() {
    fast_matrix_market::matrix_market_header header;

    header.nrows = 4;
    header.ncols = 4;
    header.comment = "test!\nnewline";

    std::vector<int64_t> rows = {1, 2, 3, 3};
    std::vector<int64_t> cols = {0, 1, 2, 3};
    std::vector<std::complex<float>> values = {std::complex<float>(1.0, 1.0), 1, 1, 1};
    header.symmetry = fast_matrix_market::hermitian;

    std::string mm;
    {
        std::ostringstream oss;
        fast_matrix_market::write_matrix_market_triplet(oss, header, rows, cols, values);
        mm = oss.str();
        std::cout << mm << std::endl << std::endl;

        rows.clear();
        cols.clear();
        values.clear();
        header.comment = "";
        std::istringstream iss(mm);
        fast_matrix_market::read_matrix_market_triplet(iss, header, rows, cols, values);
    }

    // write again
    {
        std::ostringstream oss;
        fast_matrix_market::write_matrix_market_triplet(oss, header, rows, cols, values);
        mm = oss.str();
        std::cout << mm << std::endl << std::endl;
    }

    // read array
    std::vector<std::complex<long double>> array;
    {
        header.comment = "";
        std::istringstream iss(mm);
        fast_matrix_market::read_matrix_market_array(iss, header, array);
    }

    // write array
    {
        std::ostringstream oss;
        fast_matrix_market::write_matrix_market_array(oss, header, array);
        mm = oss.str();
        std::cout << mm << std::endl << std::endl;
    }

    return 0;
}