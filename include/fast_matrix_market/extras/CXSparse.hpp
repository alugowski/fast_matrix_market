// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#pragma once

#include <charconv>
#include <complex>
#include <type_traits>

#include "fast_matrix_market/fast_matrix_market.hpp"

namespace fast_matrix_market {
    template<typename IT, typename VT, typename CS, typename ENTRY, typename FREE>
    class cxsparse_parse_handler {
    public:
        using coordinate_type = IT;
        using value_type = VT;

        explicit cxsparse_parse_handler(CS* cs, ENTRY entry, FREE spfree) : cs(cs), entry(entry), spfree(spfree) {}

        void handle(const coordinate_type row, const coordinate_type col, const value_type value) {
            if (!entry(cs, row, col, value)) {
                spfree(cs);
                throw fmm_error("Error setting entry at position");
            }
        }

    protected:
        CS* cs;
        ENTRY entry;
        FREE spfree;
    };

    /**
     * Read Matrix Market file into a CXSparse cs_* triplet structure.
     */
    template <typename CS, typename ALLOC, typename ENTRY, typename FREE>
    void read_matrix_market_cxsparse(std::istream &instream,
                                    CS **cs,
                                    ALLOC spalloc, ENTRY entry, FREE spfree,
                                    const read_options& options = {}) {

        matrix_market_header header;
        read_header(instream, header);

        // allocate
        *cs = spalloc(header.nrows, header.ncols, header.nnz,
                      header.field == pattern ? 0 : 1, // pattern field means do not allocate values
                      1);
        if (*cs == nullptr) {
            return;
        }

        auto handler = cxsparse_parse_handler<
                decltype((*cs)->m),                                     // CS struct's index type
                typename std::remove_pointer<decltype((*cs)->x)>::type, // CS struct's value type
                CS, ENTRY, FREE> (*cs, entry, spfree);

        read_matrix_market_body(instream, header, handler, 1, options);
    }

    /**
     * Write a CXSparse cs_* structure to MatrixMarket.
     */
    template <typename CS>
    void write_matrix_market_cxsparse(std::ostream &os,
                                     CS* cs,
                                     const write_options& options = {}) {
        matrix_market_header header;
        header.nrows = cs->m;
        header.ncols = cs->n;
        header.nnz = cs->nz == -1 ? cs->nzmax : cs->nz;

        header.object = matrix;
        if (cs->x == nullptr) {
            header.field = pattern;
        } else {
            header.field = get_field_type(*(cs->x));
        }
        header.format = coordinate;

        write_header(os, header);

        if (cs->nz == -1) {
            // compressed
            throw not_implemented("Need CSC formatter");
        } else {
            // triplet
            auto formatter = triplet_formatter(cs->i, cs->i + cs->nzmax,
                                               cs->p, cs->p + cs->nzmax,
                                               cs->x, header.field == pattern ? nullptr : cs->x + cs->nzmax);
            write_body(os, header, formatter, options);
        }
    }
}