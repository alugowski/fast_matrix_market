// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#pragma once

#include <charconv>
#include <complex>
#include <type_traits>

#ifdef FROM_CHARS_DOUBLE_NOT_SUPPORTED
#include <fast_float/fast_float.h>
#endif

#ifdef DRAGONBOX_AVAILABLE
#include <dragonbox/dragonbox_to_chars.h>
#endif

#include "fast_matrix_market.hpp"

namespace fast_matrix_market {
    ///////////////////////////////////////////
    // Whitespace management
    ///////////////////////////////////////////

    inline const char* skip_spaces(const char* pos) {
        return pos + std::strspn(pos, " ");
    }

    inline const char* bump_to_next_line(const char* pos, const char* end) {
        // find the newline
        pos = std::strchr(pos, '\n');

        // bump to start of next line
        if (pos != end) {
            ++pos;
        }
        return pos;
    }

    ///////////////////////////////////////////
    // Integer / Floating Point Field parsers
    ///////////////////////////////////////////

    /**
     * Parse integer using std::from_chars
     */
    template <typename IT>
    const char* read_int(const char* pos, const char* end, IT& out) {
        std::from_chars_result result = std::from_chars(pos, end, out);
        if (result.ec != std::errc()) {
            throw invalid_mm("Error reading integer value.");
        }
        return result.ptr;
    }

#ifdef FROM_CHARS_DOUBLE_NOT_SUPPORTED
    /**
     * Parse float or double using fast_float::from_chars
     */
    template <typename FT>
    const char* read_float(const char* pos, const char* end, FT& out) {
        fast_float::from_chars_result result = fast_float::from_chars(pos, end, out, fast_float::chars_format::general);
        if (result.ec != std::errc()) {
            throw invalid_mm("Error reading floating-point value.");
        }
        return result.ptr;
    }
#else
    /**
     * Parse float or double using std::from_chars
     */
    template <typename FT>
    const char* read_float(const char* pos, const char* end, FT& out) {
        std::from_chars_result result = std::from_chars(pos, end, out);
        if (result.ec != std::errc()) {
            throw invalid_mm("Error reading floating-point value.");
        }
        return result.ptr;
    }
#endif

#ifdef FROM_CHARS_LONG_DOUBLE_NOT_SUPPORTED
    /**
     * Parse `long double` using std::strtold().
     *
     * fast_float does not support long double.
     */
    inline const char* read_float(const char* pos, [[maybe_unused]] const char* end, long double& out) {
        errno = 0;

        char* value_end;
        out = std::strtold(pos, &value_end);
        if (errno != 0) {
            throw invalid_mm("Error reading floating-point value.");
        }
        return value_end;
    }
#else
    // Float block above handles long doubles too.
#endif
    //////////////////////////////////////
    // Read value. These evaluate to the field parsers above, depending on requested type
    //////////////////////////////////////

    /**
     * Pattern values are no-ops.
     */
    inline const char* read_value(const char* pos, [[maybe_unused]] const char* end, [[maybe_unused]] pattern_placeholder_type& out) {
        return pos;
    }

    template <typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
    const char* read_value(const char* pos, const char* end, T& out) {
        return read_int(pos, end, out);
    }

    inline const char* read_value(const char* pos, const char* end, float& out) {
        return read_float(pos, end, out);
    }

    inline const char* read_value(const char* pos, const char* end, double& out) {
        return read_float(pos, end, out);
    }

    inline const char* read_value(const char* pos, const char* end, long double& out) {
        return read_float(pos, end, out);
    }

    template <typename COMPLEX, typename std::enable_if<is_complex<COMPLEX>::value, int>::type = 0>
    const char* read_value(const char* pos, const char* end, COMPLEX& out) {
        typename COMPLEX::value_type real, imaginary;
        pos = read_float(pos, end, real);
        pos = skip_spaces(pos);
        pos = read_float(pos, end, imaginary);

        out.real(real);
        out.imag(imaginary);

        return pos;
    }

    template <typename T, typename std::enable_if<is_complex<T>::value, int>::type = 0>
    T complex_conjugate(const T& value) {
        return T(value.real(), -value.imag());
    }

    template <typename T, typename std::enable_if<!is_complex<T>::value, int>::type = 0>
    T complex_conjugate(const T& value) {
        return value;
    }

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
}