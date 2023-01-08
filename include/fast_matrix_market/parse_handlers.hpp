// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#pragma once

#include <algorithm>
#include <charconv>
#include <complex>
#include <iterator>
#include <type_traits>

#include "fast_matrix_market.hpp"

namespace fast_matrix_market {
    /**
     * Tuple handler. A single vector of (row, column, value) tuples.
     */
    template<typename IT, typename VT, typename ITER>
    class tuple_parse_handler {
    public:
        using coordinate_type = IT;
        using value_type = VT;

        using TUPLE = typename std::iterator_traits<ITER>::value_type;

        explicit tuple_parse_handler(const ITER& iter) : iter(iter) {}

        void handle(const coordinate_type row, const coordinate_type col, const value_type value) {
            *iter = TUPLE(row, col, value);
            ++iter;
        }

    protected:
        ITER iter;
    };

    /**
     * Triplet handler. Separate row, column, value vectors.
     */
    template<typename IT_ITER, typename VT_ITER>
    class triplet_parse_handler {
    public:
        using coordinate_type = typename std::iterator_traits<IT_ITER>::value_type;
        using value_type = typename std::iterator_traits<VT_ITER>::value_type;

        explicit triplet_parse_handler(const IT_ITER& rows,
                                       const IT_ITER& cols,
                                       const VT_ITER& values) : rows(rows), cols(cols), values(values) {}

        void handle(const coordinate_type row, const coordinate_type col, const value_type value) {
            *rows = row;
            *cols = col;
            *values = value;

            ++rows;
            ++cols;
            ++values;
        }

    protected:
        IT_ITER rows;
        IT_ITER cols;
        VT_ITER values;
    };

    /**
     * Triplet handler for pattern matrices. Separate row, column vectors.
     */
    template<typename IT_ITER>
    class triplet_pattern_parse_handler {
    public:
        using coordinate_type = typename std::iterator_traits<IT_ITER>::value_type;
        using value_type = pattern_placeholder_type;

        explicit triplet_pattern_parse_handler(const IT_ITER& rows,
                                               const IT_ITER& cols) : rows(rows), cols(cols) {}

        void handle(const coordinate_type row, const coordinate_type col, [[maybe_unused]] const value_type ignored) {
            *rows = row;
            *cols = col;

            ++rows;
            ++cols;
        }

    protected:
        IT_ITER rows;
        IT_ITER cols;
    };

    /**
     * Doublet handler, for a (index, value) sparse vector.
     */
    template<typename IT_ITER, typename VT_ITER>
    class doublet_parse_handler {
    public:
        using coordinate_type = typename std::iterator_traits<IT_ITER>::value_type;
        using value_type = typename std::iterator_traits<VT_ITER>::value_type;

        explicit doublet_parse_handler(const IT_ITER& index,
                                       const VT_ITER& values) : index(index), values(values) {}

        void handle(const coordinate_type row, const coordinate_type col, const value_type value) {
            *index = std::max(row, col);
            *values = value;

            ++index;
            ++values;
        }

    protected:
        IT_ITER index;
        VT_ITER values;
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
     * Dense array handler (row-major).
     */
    template <typename VT_ITER>
    class row_major_parse_handler {
    public:
        using coordinate_type = int64_t;
        using value_type = typename std::iterator_traits<VT_ITER>::value_type;

        explicit row_major_parse_handler(const VT_ITER& values, int64_t ncols) : values(values), ncols(ncols) {}

        void handle(const coordinate_type row, const coordinate_type col, const value_type value) {
            values[col * ncols + row] = value;
        }

    protected:
        VT_ITER values;
        int64_t ncols;
    };
}