// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#pragma once

#include "../fast_matrix_market.hpp"

namespace fast_matrix_market {
    /**
     * Read a Matrix Market file into a triplet (i.e. row, column, value vectors).
     */
    template <typename IT, typename VT>
    void read_matrix_market_triplet(std::istream &instream,
                                    matrix_market_header& header,
                                    std::vector<IT>& rows, std::vector<IT>& cols, std::vector<VT>& values,
                                    const read_options& options = {}) {
        read_header(instream, header);

        rows.resize(get_storage_nnz(header, options));
        cols.resize(get_storage_nnz(header, options));
        values.resize(get_storage_nnz(header, options));

        auto handler = triplet_parse_handler(rows.begin(), cols.begin(), values.begin());
        read_matrix_market_body(instream, header, handler, 1, options);
    }

    /**
     * Convenience method that omits the header requirement if the user only cares about the dimensions.
     */
    template <typename IT, typename VT, typename DIM>
    void read_matrix_market_triplet(std::istream &instream,
                                    DIM& nrows, DIM& ncols,
                                    std::vector<IT>& rows, std::vector<IT>& cols, std::vector<VT>& values,
                                    const read_options& options = {}) {
        matrix_market_header header;
        read_matrix_market_triplet(instream, header, rows, cols, values, options);
        nrows = header.nrows;
        ncols = header.ncols;
    }

    /**
     * Write triplets to a Matrix Market file.
     */
    template <typename IT, typename VT>
    void write_matrix_market_triplet(std::ostream &os,
                                     matrix_market_header header,
                                     const std::vector<IT>& rows,
                                     const std::vector<IT>& cols,
                                     const std::vector<VT>& values,
                                     const write_options& options = {}) {
        header.nnz = values.size();

        header.object = matrix;
        header.field = get_field_type::value<VT>();
        header.format = coordinate;

        write_header(os, header);

        auto formatter = triplet_formatter(rows.begin(), rows.end(),
                                           cols.begin(), cols.end(),
                                           values.begin(), header.field == pattern ? values.begin() : values.end());
        write_body(os, formatter, options);
    }
}