// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#ifndef FAST_MATRIX_MARKET_H
#define FAST_MATRIX_MARKET_H

#pragma once

#include <complex>
#include <iostream>
#include <map>
#include <string>
#include <sstream>
#include <utility>

namespace fast_matrix_market {

constexpr std::string_view kSpace = " ";
constexpr std::string_view kNewline = "\n";

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
};

struct write_options {
    int64_t chunk_size_values = 2 << 12;

    /**
     * If true then a parallel implementation is used.
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
class fmm_error : std::exception {
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
class invalid_mm : fmm_error {
public:
    explicit invalid_mm(std::string msg): fmm_error(std::move(msg)) {}
    explicit invalid_mm(const std::string& msg, int64_t line_num):
        fmm_error(std::string("Line: ") + std::to_string(line_num) + ": " + msg) {}
};

/**
 * Passed in argument was not valid.
 */
class invalid_argument : fmm_error {
public:
    explicit invalid_argument(std::string msg): fmm_error(std::move(msg)) {}
};

/**
 * Matrix Market file has complex fields but the datastructure to load into cannot handle complex values.
 */
class complex_incompatible : invalid_argument {
public:
    explicit complex_incompatible(std::string msg): invalid_argument(std::move(msg)) {}
};

/**
 * Thrown when a certain part of the Matrix Market spec has not been implemented yet.
 */
class not_implemented : fmm_error {
public:
    explicit not_implemented(std::string msg): fmm_error(std::move(msg)) {}
};

}

#include "header.hpp"
#include "read_body.hpp"
#include "write_body.hpp"
#include "parse_handlers.hpp"
#include "formatters.hpp"
#include "interface_triplet.hpp"
#include "interface_array.hpp"

#endif
