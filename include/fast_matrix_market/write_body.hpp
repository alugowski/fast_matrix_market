// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#pragma once

#include <charconv>
#include <complex>
#include <type_traits>

#ifdef DRAGONBOX_AVAILABLE
#include <dragonbox/dragonbox_to_chars.h>
#endif

#include "fast_matrix_market.hpp"

#include "write_body_threads.hpp"

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
     * Convert integral types to string. This is the fallback due to locale dependence (and hence thread serialization).
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

    inline std::string value_to_string(const int32_t & value) {
        return int_to_string(value);
    }

    inline std::string value_to_string(const int64_t & value) {
        return int_to_string(value);
    }

#ifdef TO_CHARS_DOUBLE_SUPPORTED
    inline std::string value_to_string(const float& value) {
        std::string ret(15, ' ');
        std::to_chars_result result = std::to_chars(ret.data(), ret.data() + ret.size(), value);
        if (result.ec == std::errc()) {
            ret.resize(result.ptr - ret.data());
            return ret;
        } else {
            return std::to_string(value);
        }
    }

    inline std::string value_to_string(const double& value) {
        std::string ret(25, ' ');
        std::to_chars_result result = std::to_chars(ret.data(), ret.data() + ret.size(), value);
        if (result.ec == std::errc()) {
            ret.resize(result.ptr - ret.data());
            return ret;
        } else {
            return std::to_string(value);
        }
    }
#else
#ifdef DRAGONBOX_AVAILABLE

#ifndef DRAGONBOX_DROP_E0
#define DRAGONBOX_DROP_E0 true
#endif

    inline bool ends_with(const std::string &str, const std::string& suffix) {
        if (suffix.size() > str.size()) {
            return false;
        }
        return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
    }

    inline std::string value_to_string(const float& value) {
        std::string buffer(jkj::dragonbox::max_output_string_length<jkj::dragonbox::ieee754_binary32> + 1, ' ');

        char *end_ptr = jkj::dragonbox::to_chars(value, buffer.data());
        buffer.resize(end_ptr - buffer.data());

        if (DRAGONBOX_DROP_E0 && ends_with(buffer, "E0")) {
            buffer.resize(buffer.size() - 2);
        }
        return buffer;
    }

    inline std::string value_to_string(const double& value) {
        std::string buffer(jkj::dragonbox::max_output_string_length<jkj::dragonbox::ieee754_binary64> + 1, ' ');

        char *end_ptr = jkj::dragonbox::to_chars(value, buffer.data());
        buffer.resize(end_ptr - buffer.data());

        if (DRAGONBOX_DROP_E0 && ends_with(buffer, "E0")) {
            buffer.resize(buffer.size() - 2);
        }
        return buffer;
    }
#endif
#endif

#ifdef TO_CHARS_LONG_DOUBLE_SUPPORTED
    inline std::string value_to_string(const long double& value) {
        std::string ret(35, ' ');
        std::to_chars_result result = std::to_chars(ret.data(), ret.data() + ret.size(), value);
        if (result.ec == std::errc()) {
            ret.resize(result.ptr - ret.data());
            return ret;
        } else {
            return std::to_string(value);
        }
    }
#endif

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
     * Write Matrix Market body sequentially.
     *
     * Chunks are computed and written sequentially.
     */
    template <typename FORMATTER>
    void write_body_sequential(std::ostream& os,
                               FORMATTER& formatter, const write_options& options = {}) {

        while (formatter.has_next()) {
            std::string chunk = formatter.next_chunk(options)();

            os.write(chunk.c_str(), (std::streamsize)chunk.size());
        }
    }

    /**
     * Write Matrix Market body.
     *
     * @tparam FORMATTER implementation class that writes chunks.
     */
    template <typename FORMATTER>
    void write_body(std::ostream& os,
                    FORMATTER& formatter, const write_options& options = {}) {
        if (options.parallel_ok && options.num_threads != 1) {
            write_body_threads(os, formatter, options);
            return;
        }
        write_body_sequential(os, formatter, options);
    }
}