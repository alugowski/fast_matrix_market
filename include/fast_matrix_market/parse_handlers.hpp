// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#pragma once

#include <charconv>
#include <complex>
#include <type_traits>

#include "fast_matrix_market.hpp"

namespace fast_matrix_market {
    /**
     * Tuple handler. A single vector of (row, column, value) tuples.
     */
    template<typename TUPLE, typename IT, typename VT>
    class tuple_parse_handler {
    public:
        using coordinate_type = IT;
        using value_type = VT;

        explicit tuple_parse_handler(std::vector<TUPLE> &tuples) : tuples(tuples) {}

        void handle(const coordinate_type row, const coordinate_type col, const value_type value) {
            tuples.emplace_back(row, col, value);
        }

    protected:
        std::vector<TUPLE> &tuples;
    };

    /**
     * Works with any type where `mat(row, column) = value` works.
     */
    template<typename MAT, typename IT, typename VT>
    class setting_2d_parse_handler {
    public:
        using coordinate_type = IT;
        using value_type = VT;

        explicit setting_2d_parse_handler(MAT &mat) : mat(mat) {}

        void handle(const coordinate_type row, const coordinate_type col, const value_type value) {
            mat(row, col) = value;
        }

    protected:
        MAT &mat;
    };

    /**
     * Triplet handler. Separate row, column, value vectors.
     */
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
        std::vector<coordinate_type> &rows;
        std::vector<coordinate_type> &cols;
        std::vector<value_type> &values;
    };

    /**
     * Triplet handler for pattern matrices. Separate row, column vectors.
     */
    template<typename IT>
    class triplet_pattern_parse_handler {
    public:
        using coordinate_type = IT;
        using value_type = pattern_placeholder_type;

        explicit triplet_pattern_parse_handler(std::vector<coordinate_type> &rows,
                                               std::vector<coordinate_type> &cols) : rows(rows), cols(cols) {}

        void handle(const coordinate_type row, const coordinate_type col, const value_type ignored) {
            rows.emplace_back(row);
            cols.emplace_back(col);
        }

    protected:
        std::vector<coordinate_type> &rows;
        std::vector<coordinate_type> &cols;
    };

    /**
     * Doublet handler, for a (index, value) sparse vector.
     */
    template<typename IT, typename VT>
    class doublet_parse_handler {
    public:
        using coordinate_type = IT;
        using value_type = VT;

        explicit doublet_parse_handler(std::vector<coordinate_type> &index,
                                       std::vector<value_type> &values) : index(index), values(values) {}

        void handle(const coordinate_type row, const coordinate_type col, const value_type value) {
            index.emplace_back(row);
            values.emplace_back(value);
        }

    protected:
        std::vector<coordinate_type> &index;
        std::vector<value_type> &values;
    };

    /**
     * Dense array handler (row-major).
     */
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
}