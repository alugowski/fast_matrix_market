// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#pragma once

#include <map>
#include <string>

namespace fast_matrix_market {

    enum object_type {matrix, vector};
    const std::map<object_type, const std::string> object_map = {
            {matrix, "matrix"},
            {vector, "vector"},
    };

    enum format_type {array, coordinate};
    const std::map<format_type, const std::string> format_map = {
            {array, "array"},
            {coordinate, "coordinate"},
    };

    enum field_type {real, double_, complex, integer, pattern};
    const std::map<field_type, const std::string> field_map = {
            {real, "real"},
            {double_, "double"},
            {complex, "complex"},
            {integer, "integer"},
            {pattern, "pattern"},
    };

    enum symmetry_type {general, symmetric, skew_symmetric, hermitian};
    const std::map<symmetry_type, const std::string> symmetry_map = {
            {general, "general"},
            {symmetric, "symmetric"},
            {skew_symmetric, "skew-symmetric"},
            {hermitian, "hermitian"},
    };

    /**
     * Matrix Market header
     */
    struct matrix_market_header {
        matrix_market_header() = default;
        explicit matrix_market_header(int64_t vector_length) : object(vector), vector_length(vector_length) {}
        matrix_market_header(int64_t nrows, int64_t ncols) : nrows(nrows), ncols(ncols) {}

        object_type object = matrix;
        format_type format = coordinate;
        field_type field = real;
        symmetry_type symmetry = general;

        // Matrix dimensions
        int64_t nrows = 0;
        int64_t ncols = 0;

        // Vector dimensions
        int64_t vector_length = 0;

        // Number of non-zeros for sparse objects
        int64_t nnz = 0;

        // Comment written in the file header
        std::string comment;

        // Number of lines the header takes up. This is populated by read_header().
        int64_t header_line_count = 1;
    };

}
