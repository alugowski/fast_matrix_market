// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#pragma once

#include <charconv>
#include <complex>
#include <type_traits>

#include "fast_matrix_market.hpp"

namespace fast_matrix_market {
    /**
     * Get header field type based on the C++ type of the values to be written.
     */
    struct get_field_type {
        template <typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
        static field_type value() {
            return integer;
        }

        template <typename T, typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
        static field_type value() {
            return real;
        }

        template <typename T, typename std::enable_if<is_complex<T>::value, int>::type = 0>
        static field_type value() {
            return complex;
        }
    };

    ////////////////////////////////////////////
    // Value to String conversions
    ////////////////////////////////////////////

#ifdef TO_CHARS_INT_SUPPORTED
    /**
     * Convert integral types to string.
     * std::to_string and std::to_chars has similar performance, however std::to_string is locale dependent and
     * therefore will cause thread serialization.
     */
    template <typename T>
    std::string int_to_string(const T& value) {
        std::string ret(15, ' ');
        std::to_chars_result result = std::to_chars(ret.data(), ret.data() + ret.size(), value);
        if (result.ec == std::errc()) {
            ret.resize(result.ptr - ret.data());
            return ret;
        } else {
            return std::to_string(value);
        }
    }
#else
    /**
     * Convert integral types to string. Fallback due to locale dependence (and hence thread serialization).
     */
    template <typename T>
    std::string int_to_string(const T& value) {
        return std::to_string(value);
    }
#endif

    template <typename T>
    std::string value_to_string(const T& value) {
        return std::to_string(value);
    }

    inline std::string value_to_string(const std::complex<float>& value) {
        return value_to_string(value.real()) + " " + value_to_string(value.imag());
    }

    inline std::string value_to_string(const std::complex<double>& value) {
        return value_to_string(value.real()) + " " + value_to_string(value.imag());
    }

    inline std::string value_to_string(const std::complex<long double>& value) {
        return value_to_string(value.real()) + " " + value_to_string(value.imag());
    }


    /**
     * Write Matrix Market body.
     *
     * Chunk based so that it can be made parallel. Each chunk is written by a FORMATTER class.
     * @tparam FORMATTER implementation class that writes chunks.
     */
    template <typename FORMATTER>
    void write_body(std::ostream& os,
                    FORMATTER& formatter, const write_options& options = {}) {

        while (formatter.has_next()) {
            std::string chunk = formatter.next_chunk(options).get();

            os.write(chunk.c_str(), (std::streamsize)chunk.size());
        }
    }
}