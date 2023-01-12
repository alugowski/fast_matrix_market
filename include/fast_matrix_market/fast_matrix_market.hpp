// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#ifndef FAST_MATRIX_MARKET_H
#define FAST_MATRIX_MARKET_H

#pragma once

#include <complex>
#include <future>
#include <iostream>
#include <map>
#include <string>
#include <sstream>
#include <utility>

namespace fast_matrix_market {

#define FAST_MATRIX_MARKET_VERSION_MAJOR 1
#define FAST_MATRIX_MARKET_VERSION_MINOR 1
#define FAST_MATRIX_MARKET_VERSION_PATCH 0

    constexpr std::string_view kSpace = " ";
    constexpr std::string_view kNewline = "\n";

    enum storage_order {row_major = 1, col_major = 2};

    struct read_options {
        /**
         * Chunk size for the parsing step, in bytes.
         */
        int64_t chunk_size_bytes = 2 << 20;

        /**
         * If true then any symmetries other than general are expanded out.
         * For any symmetries other than general, only entries in the lower triangular portion need be supplied.
         * symmetric: for (row, column, value), also generate (column, row, value) except if row==column
         * skew-symmetric: for (row, column, value), also generate (column, row, -value) except if row==column
         * hermitian: for (row, column, value), also generate (column, row, complex_conjugate(value)) except if row==column
         */
        bool generalize_symmetry = true;

        /**
         * Generalize Symmetry:
         * How to handle a value on the diagonal of a symmetric coordinate matrix.
         *  - DuplicateElement: Duplicate the diagonal element
         *  - ExtraZeroElement: emit a zero along with the diagonal element. The zero will appear first.
         *
         *  The extra cannot simply be omitted because the handlers work by setting already-allocated memory. This
         *  is necessary for efficient parallelization.
         *
         *  This value is ignored if the parse handler has the kAppending flag set. In that case only a single
         *  diagonal element is emitted.
         */
        enum {ExtraZeroElement, DuplicateElement} generalize_coordinate_diagnonal_values = ExtraZeroElement;

        /**
         * Whether or not parallel implementation is allowed.
         */
        bool parallel_ok = true;

        /**
         * Number of threads to use. 0 means std::thread::hardware_concurrency().
         */
        int num_threads = 0;
    };

    struct write_options {
        int64_t chunk_size_values = 2 << 12;

        /**
         * Whether or not parallel implementation is allowed.
         */
        bool parallel_ok = true;

        /**
         * Number of threads to use. 0 means std::thread::hardware_concurrency().
         */
        int num_threads = 0;
    };

    template<class T> struct is_complex : std::false_type {};
    template<class T> struct is_complex<std::complex<T>> : std::true_type {};

    /**
     *
     */
    class fmm_error : public std::exception {
    public:
        explicit fmm_error(std::string msg): msg(std::move(msg)) {}

        [[nodiscard]] const char* what() const noexcept override {
            return msg.c_str();
        }
    protected:
        std::string msg;
    };

    /**
     * The provided stream does not represent a Matrix Market file.
     */
    class invalid_mm : public fmm_error {
    public:
        explicit invalid_mm(std::string msg): fmm_error(std::move(msg)) {}
        explicit invalid_mm(std::string msg, int64_t line_num) : fmm_error(std::move(msg)) {
            prepend_line_number(line_num);
        }

        void prepend_line_number(int64_t line_num) {
            msg = std::string("Line ") + std::to_string(line_num) + ": " + msg;
        }
    };

    /**
     * Passed in argument was not valid.
     */
    class invalid_argument : public fmm_error {
    public:
        explicit invalid_argument(std::string msg): fmm_error(std::move(msg)) {}
    };

    /**
     * Matrix Market file has complex fields but the datastructure to load into cannot handle complex values.
     */
    class complex_incompatible : public invalid_argument {
    public:
        explicit complex_incompatible(std::string msg): invalid_argument(std::move(msg)) {}
    };

    /**
     * A value type to use for pattern matrices. Pattern Matrix Market files do not write a value column, only the
     * coordinates. Setting this as the value type signals the parser to not attempt to read a column that isn't there.
     */
    struct pattern_placeholder_type {};

    /**
     * Negation of a pattern_placeholder_type needed to support symmetry generalization.
     * Skew-symmetric symmetry negates values.
     */
    inline pattern_placeholder_type operator-(const pattern_placeholder_type& o) { return o; }

    /**
     * Zero generator for generalize symmetry with ExtraZeroElement.
     */
    template <typename T>
    T get_zero() {
        return {};
    }

    /**
     * Determine if a std::future is ready to return a result, i.e. finished computing.
     * @return true if the future is ready.
     */
    template<typename R>
    bool is_ready(std::future<R> const& f)
    {
        return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }

    /**
     * @param flags flags bitwise ORed together
     * @param flag flag bit to test for
     * @return true if the flag bit is set in flags, false otherwise
     */
    inline bool test_flag(int flags, int flag) {
        return (flags & flag) == flag;
    }

    inline bool ends_with(const std::string &str, const std::string& suffix) {
        if (suffix.size() > str.size()) {
            return false;
        }
        return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
    }
}

#include "field_conv.hpp"
#include "header.hpp"
#include "parse_handlers.hpp"
#include "formatters.hpp"
#include "read_body.hpp"
#include "write_body.hpp"
#include "app/array.hpp"
#include "app/doublet.hpp"
#include "app/triplet.hpp"

#endif
