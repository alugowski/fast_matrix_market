// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#pragma once

#include <charconv>
#include <complex>
#include <type_traits>
#include <fast_float/fast_float.h>

#include "fast_matrix_market.hpp"

namespace fast_matrix_market {

    template <typename VT>
    class row_major_parse_handler {
    public:
        using coordinate_type = int64_t;
        using value_type = VT;

        explicit row_major_parse_handler(std::vector<VT> &values, int64_t ncols) : values(values), ncols(ncols) {}

        void handle(const coordinate_type row, const coordinate_type col, const value_type value) {
            values[col * ncols + row] = value;
        }

    protected:
        std::vector<VT>& values;
        int64_t ncols;
    };

    template <typename VT>
    void read_matrix_market_array(std::istream &instream,
                                  matrix_market_header& header,
                                  std::vector<VT>& row_major_values,
                                  const read_options& options = {}) {
        read_header(instream, header);

        row_major_values.resize(header.nrows * header.ncols);

        auto handler = row_major_parse_handler(row_major_values, header.ncols);
        read_matrix_market_body_with_pattern(instream, header, handler, 1, options);
    }

    template<typename VT>
    class row_major_array_formatter {
    public:
        explicit row_major_array_formatter(const std::vector<VT> &values, int64_t ncols) : values(values), ncols(ncols), nrows(values.size() / ncols) {}

        std::string next_chunk(const write_options& options) {
            std::string chunk;

            if (cur_column >= ncols) {
                return chunk;
            }

            // chunk by column
            chunk.reserve(ncols*15);

            for (int64_t i = 0; i < nrows; ++i) {
                chunk += value_to_string(values[(i * ncols) + cur_column]);
                chunk += kNewline;
            }
            ++cur_column;

            return chunk;
        }

    protected:
        const std::vector<VT>& values;
        int64_t nrows, ncols;
        int64_t cur_column = 0;
    };


    template <typename VT>
    void write_matrix_market_array(std::ostream &os,
                                   matrix_market_header header,
                                   const std::vector<VT>& row_major_array,
                                   const write_options& options = {}) {
        if (header.nrows * header.ncols != row_major_array.size()) {
            throw invalid_argument("Array length does not match matrix dimensions.");
        }

        header.nnz = row_major_array.size();

        header.object = matrix;
        header.field = get_field_type(VT());
        header.format = array;
        header.symmetry = general;

        write_header(os, header);

        auto formatter = row_major_array_formatter(row_major_array, header.ncols);
        write_body(os, header, formatter, options);
    }
}