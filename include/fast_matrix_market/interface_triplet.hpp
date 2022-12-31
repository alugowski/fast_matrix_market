// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#pragma once

#include <charconv>
#include <complex>
#include <type_traits>

#include "fast_matrix_market.hpp"

namespace fast_matrix_market {
    template<typename IT, typename VT>
    class triplet_parse_handler {
    public:
        using coordinate_type = IT;
        using value_type = VT;

        explicit triplet_parse_handler(std::vector<coordinate_type> &rows,
                                       std::vector<coordinate_type> &cols,
                                       std::vector<value_type> &values) : rows(rows), cols(cols), values(values) {}

        void handle(const coordinate_type row, const coordinate_type col, const value_type value) {
            rows.emplace_back(row);
            cols.emplace_back(col);
            values.emplace_back(value);
        }

    protected:
        std::vector<coordinate_type>& rows;
        std::vector<coordinate_type>& cols;
        std::vector<value_type>& values;
    };

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

    template<typename IT, typename VT>
    class triplet_formatter {
    public:
        explicit triplet_formatter(const std::vector<IT> &rows,
                                   const std::vector<IT> &cols,
                                   const std::vector<VT> &values) : rows(rows), cols(cols), values(values) {}

        std::string next_chunk(const write_options& options) {
            std::string chunk;
            chunk.reserve(options.chunk_size_values*25);

            int64_t end_i = chunk_offset + options.chunk_size_values;
            if (end_i > rows.size()) {
                end_i = rows.size();
            }
            for (int64_t i = chunk_offset; i < end_i; ++i) {
                chunk += std::to_string(rows[i] + 1);
                chunk += kSpace;
                chunk += std::to_string(cols[i] + 1);
                chunk += kSpace;
                chunk += value_to_string(values[i]);
                chunk += kNewline;
            }
            chunk_offset = end_i;

            return chunk;
        }

    protected:
        const std::vector<IT>& rows;
        const std::vector<IT>& cols;
        const std::vector<VT>& values;

        size_t chunk_offset = 0;
    };


    template <typename IT, typename VT>
    void write_matrix_market_triplet(std::ostream &os,
                                     matrix_market_header header,
                                     const std::vector<IT>& rows,
                                     const std::vector<IT>& cols,
                                     const std::vector<VT>& values,
                                     const write_options& options = {}) {
        header.nnz = values.size();

        header.object = matrix;
        header.field = get_field_type(VT());
        header.format = coordinate;

        write_header(os, header);

        auto formatter = triplet_formatter(rows, cols, values);
        write_body(os, header, formatter, options);
    }
}