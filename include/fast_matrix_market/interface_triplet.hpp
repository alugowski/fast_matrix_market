// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#pragma once

#include <charconv>
#include <complex>
#include <type_traits>

#include "fast_matrix_market.hpp"

namespace fast_matrix_market {
    template <typename IT, typename VT>
    void read_matrix_market_triplet(std::istream &instream,
                                    matrix_market_header& header,
                                    std::vector<IT>& rows, std::vector<IT>& cols, std::vector<VT>& values,
                                    const read_options& options = {}) {
        read_header(instream, header);

        rows.reserve(header.nnz);
        cols.reserve(header.nnz);
        values.reserve(header.nnz);

        auto handler = triplet_parse_handler(rows, cols, values);
        read_matrix_market_body(instream, header, handler, 1, options);
    }

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
                                           values.begin(), values.end());
        write_body(os, formatter, options);
    }
}