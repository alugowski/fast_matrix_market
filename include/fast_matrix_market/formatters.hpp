// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#pragma once

#include <algorithm>
#include <charconv>
#include <complex>
#include <type_traits>

#include "fast_matrix_market.hpp"

namespace fast_matrix_market {
    /**
     * Format row, column, value vectors.
     */
    template<typename A_ITER, typename B_ITER, typename C_ITER>
    class triplet_formatter {
    public:
        explicit triplet_formatter(const A_ITER row_begin, const A_ITER row_end,
                                   const B_ITER col_begin, const B_ITER col_end,
                                   const C_ITER val_begin, const C_ITER val_end) :
                                   row_iter(row_begin), row_end(row_end),
                                   col_iter(col_begin), col_end(col_end),
                                   val_iter(val_begin), val_end(val_end) {
        }

        [[nodiscard]] bool has_next() const {
            return row_iter != row_end;
        }

        class chunk {
        public:
            explicit chunk(const A_ITER row_begin, const A_ITER row_end,
                           const B_ITER col_begin, const B_ITER col_end,
                           const C_ITER val_begin, const C_ITER val_end) :
                    row_iter(row_begin), row_end(row_end),
                    col_iter(col_begin), col_end(col_end),
                    val_iter(val_begin), val_end(val_end) {}

            std::string get() {
                std::string chunk;
                chunk.reserve((row_end - row_iter)*25);

                for (; row_iter != row_end; ++row_iter, ++col_iter) {
                    chunk += std::to_string(*row_iter + 1);
                    chunk += kSpace;
                    chunk += std::to_string(*col_iter + 1);
                    if (val_iter != val_end) {
                        chunk += kSpace;
                        chunk += value_to_string(*val_iter);
                        ++val_iter;
                    }
                    chunk += kNewline;
                }

                return chunk;
            }

            A_ITER row_iter, row_end;
            B_ITER col_iter, col_end;
            C_ITER val_iter, val_end;
        };

        chunk next_chunk(const write_options& options) {
            A_ITER row_chunk_end = std::min(row_iter + options.chunk_size_values, row_end);
            B_ITER col_chunk_end = std::min(col_iter + options.chunk_size_values, col_end);
            C_ITER val_chunk_end = std::min(val_iter + options.chunk_size_values, val_end);

            chunk c(row_iter, row_chunk_end,
                    col_iter, col_chunk_end,
                    val_iter, val_chunk_end);

            row_iter = row_chunk_end;
            col_iter = col_chunk_end;
            val_iter = val_chunk_end;

            return c;
        }

    protected:
        A_ITER row_iter, row_end;
        B_ITER col_iter, col_end;
        C_ITER val_iter, val_end;
    };

    /**
     * Format dense row-major arrays.
     */
    template<typename VT>
    class row_major_array_formatter {
    public:
        explicit row_major_array_formatter(const std::vector<VT> &values, int64_t ncols) : values(values), ncols(ncols), nrows(values.size() / ncols) {}

        [[nodiscard]] bool has_next() const {
            return cur_column != ncols;
        }

        class chunk {
        public:
            explicit chunk(const std::vector<VT> &values, int64_t nrows, int64_t ncols, int64_t cur_column) :
            values(values), nrows(nrows), ncols(ncols), cur_column(cur_column) {}

            std::string get() {
                std::string c;
                c.reserve(ncols * 15);

                for (int64_t i = 0; i < nrows; ++i) {
                    c += value_to_string(values[(i * ncols) + cur_column]);
                    c += kNewline;
                }

                return c;
            }

            const std::vector<VT>& values;
            int64_t nrows, ncols;
            int64_t cur_column = 0;
        };

        chunk next_chunk(const write_options& options) {
            return chunk(values, nrows, ncols, cur_column++);
        }

    protected:
        const std::vector<VT>& values;
        int64_t nrows, ncols;
        int64_t cur_column = 0;
    };
}
