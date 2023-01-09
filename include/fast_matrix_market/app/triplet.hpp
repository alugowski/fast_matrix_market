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
     * A special version of the triplet loader that does not create duplicate main diagonal entries
     * if options.generalize_symmetry is set.
     *
     * The main drawback is that parallelism is disabled.
     */
    template <typename IT, typename VT>
    void read_matrix_market_triplet_no_symmetry_dupes(
                                    std::istream &instream,
                                    matrix_market_header& header,
                                    std::vector<IT>& rows, std::vector<IT>& cols, std::vector<VT>& values,
                                    const read_options& options = {}) {
        read_header(instream, header);

        rows.resize(0);
        cols.resize(0);
        values.resize(0);

        rows.reserve(get_storage_nnz(header, options));
        cols.reserve(get_storage_nnz(header, options));
        values.reserve(get_storage_nnz(header, options));

        auto handler = triplet_appending_parse_handler(rows, cols, values);
        read_matrix_market_body(instream, header, handler, 1, options);
    }

    /**
     * Read a Matrix Market vector file into a doublet sparse vector (i.e. index, value vectors).
     *
     * Any vector-like Matrix Market file will work:
     *  - object=vector file, either dense or sparse
     *  - object=matrix file as long as nrows=1 or ncols=1
     */
    template <typename IT, typename VT>
    void read_matrix_market_doublet(std::istream &instream,
                                    matrix_market_header& header,
                                    std::vector<IT>& indices, std::vector<VT>& values,
                                    const read_options& options = {}) {
        read_header(instream, header);

        indices.resize(header.nnz);
        values.resize(get_storage_nnz(header, options));

        auto handler = doublet_parse_handler(indices.begin(), values.begin());
        read_matrix_market_body(instream, header, handler, 1, options);
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
                                           values.begin(), values.end());
        write_body(os, formatter, options);
    }
}