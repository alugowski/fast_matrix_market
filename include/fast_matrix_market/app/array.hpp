// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#pragma once

#include "../fast_matrix_market.hpp"

namespace fast_matrix_market {
    /**
     * Read a Matrix Market file into a row-major array.
     */
    template <typename VT>
    void read_matrix_market_array(std::istream &instream,
                                  matrix_market_header& header,
                                  std::vector<VT>& values,
                                  storage_order order = row_major,
                                  const read_options& options = {}) {
        read_header(instream, header);

        if (!values.empty()) {
            values.resize(0);
        }
        values.resize(header.nrows * header.ncols);

        auto handler = dense_adding_parse_handler(values.begin(), order, header.nrows, header.ncols);
        read_matrix_market_body(instream, header, handler, 1, options);
    }

    /**
     * Convenience method that omits the header requirement if the user only cares about the dimensions.
     */
    template <typename VT, typename DIM>
    void read_matrix_market_array(std::istream &instream,
                                  DIM& nrows, DIM& ncols,
                                  std::vector<VT>& values,
                                  storage_order order = row_major,
                                  const read_options& options = {}) {
        matrix_market_header header;
        read_matrix_market_array(instream, header, values, order, options);
        nrows = header.nrows;
        ncols = header.ncols;
    }

    /**
     * Write a row-major array to a Matrix Market file.
     */
    template <typename VT>
    void write_matrix_market_array(std::ostream &os,
                                   matrix_market_header header,
                                   const std::vector<VT>& values,
                                   storage_order order = row_major,
                                   const write_options& options = {}) {
        if (header.nrows * header.ncols != (int64_t)values.size()) {
            throw invalid_argument("Array length does not match matrix dimensions.");
        }

        header.nnz = values.size();

        header.object = matrix;
        header.field = get_field_type((const VT*)nullptr);
        header.format = array;
        header.symmetry = general;

        write_header(os, header);

        auto formatter = array_formatter(values.cbegin(), order, header.nrows, header.ncols);
        write_body(os, formatter, options);
    }
}