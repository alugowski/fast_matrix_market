// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#pragma once

#include "../fast_matrix_market.hpp"

namespace fast_matrix_market {

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
     * Convenience method that omits the header requirement if the user only cares about the dimensions.
     */
    template <typename IT, typename VT, typename DIM>
    void read_matrix_market_doublet(std::istream &instream,
                                    DIM& length,
                                    std::vector<IT>& indices, std::vector<VT>& values,
                                    const read_options& options = {}) {
        matrix_market_header header;
        read_matrix_market_doublet(instream, header, indices, values, options);
        length = header.vector_length;
    }

    /**
     * Write doublets to a Matrix Market file.
     */
    template <typename IT, typename VT>
    void write_matrix_market_doublet(std::ostream &os,
                                     matrix_market_header header,
                                     const std::vector<IT>& indices,
                                     const std::vector<VT>& values,
                                     const write_options& options = {}) {
        header.nnz = values.size();

        header.object = vector;
        header.field = get_field_type::value<VT>();
        header.format = coordinate;

        write_header(os, header);

        auto formatter = triplet_formatter<decltype(indices.begin()),
                                           decltype(values.begin()),
                                           decltype(values.begin()), true>(
                                                   indices.begin(), indices.end(),
                                                   values.begin(), values.end(),
                                                   values.end(), values.end());
        write_body(os, formatter, options);
    }
}