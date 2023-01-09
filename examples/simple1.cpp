// Copyright (C) 2022-2023 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#include <iostream>
#include <vector>

#include <fast_matrix_market/fast_matrix_market.hpp>

/**
 * A simple triplet sparse matrix.
 */
template <typename IT, typename VT>
struct triplet_matrix {
    int64_t nrows = 0, ncols = 0;
    std::vector<IT> rows;
    std::vector<IT> cols;
    std::vector<VT> vals;
};

/**
 * A simple dense matrix.
 */
template <typename VT>
struct array_matrix {
    int64_t nrows = 0, ncols = 0;
    std::vector<VT> vals;
};

int main() {
    // create a matrix
    triplet_matrix<int64_t, double> triplet;

    triplet.nrows = 4;
    triplet.ncols = 4;

    triplet.rows = {1, 2, 3, 3};
    triplet.cols = {0, 1, 2, 3};
    triplet.vals = {1.0, 5, 2E5, 19};

    std::string mm;

    // write to a string
    {
        std::ostringstream oss;

        fast_matrix_market::write_matrix_market_triplet(
                oss,
                fast_matrix_market::matrix_market_header(triplet.nrows, triplet.ncols),
                triplet.rows, triplet.cols, triplet.vals);

        mm = oss.str();
        std::cout << mm << std::endl << std::endl;
    }

    // read into another triplet
    {
        fast_matrix_market::matrix_market_header header;

        triplet_matrix<int64_t, double> triplet2;
        std::istringstream iss(mm);
        fast_matrix_market::read_matrix_market_triplet(iss, header, triplet2.rows, triplet2.cols, triplet2.vals);
        triplet2.nrows = header.nrows;
        triplet2.ncols = header.ncols;

        assert(triplet.nrows == triplet2.nrows &&
               triplet.ncols == triplet2.ncols &&
               triplet.rows == triplet2.rows &&
               triplet.cols == triplet2.cols &&
               triplet.vals == triplet2.vals);
    }

    // read into array
    array_matrix<std::complex<double>> array;
    {
        fast_matrix_market::matrix_market_header header;
        std::istringstream iss(mm);
        fast_matrix_market::read_matrix_market_array(iss, header, array.vals);
        array.nrows = header.nrows;
        array.ncols = header.ncols;
    }

    // write array
    {
        std::ostringstream oss;
        fast_matrix_market::write_matrix_market_array(
                oss,
                fast_matrix_market::matrix_market_header(array.nrows, array.ncols),
                array.vals);

        mm = oss.str();
        std::cout << mm << std::endl << std::endl;
    }

    return 0;
}