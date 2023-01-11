// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#pragma once

#include "../fast_matrix_market.hpp"

namespace fast_matrix_market {
    template <typename VT>
    void read_matrix_market_array(std::istream &instream,
                                  matrix_market_header& header,
                                  std::vector<VT>& row_major_values,
                                  const read_options& options = {}) {
        read_header(instream, header);

        row_major_values.resize(header.nrows * header.ncols);

        auto handler = dense_row_major_adding_parse_handler(row_major_values.begin(), header.nrows);
        read_matrix_market_body(instream, header, handler, 1, options);
    }

    /**
     * Convenience method that omits the header requirement if the user only cares about the dimensions.
     */
    template <typename VT, typename DIM>
    void read_matrix_market_array(std::istream &instream,
                                  DIM& nrows, DIM& ncols,
                                  std::vector<VT>& row_major_values,
                                  const read_options& options = {}) {
        matrix_market_header header;
        read_matrix_market_array(instream, header, row_major_values, options);
        nrows = header.nrows;
        ncols = header.ncols;
    }

    template <typename VT>
    void write_matrix_market_array(std::ostream &os,
                                   matrix_market_header header,
                                   const std::vector<VT>& row_major_array,
                                   const write_options& options = {}) {
        if (header.nrows * header.ncols != (int64_t)row_major_array.size()) {
            throw invalid_argument("Array length does not match matrix dimensions.");
        }

        header.nnz = row_major_array.size();

        header.object = matrix;
        header.field = get_field_type::value<VT>();
        header.format = array;
        header.symmetry = general;

        write_header(os, header);

        auto formatter = row_major_array_formatter(row_major_array, header.nrows);
        write_body(os, formatter, options);
    }
}