// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#pragma once

#include <charconv>
#include <complex>
#include <iomanip>
#include <type_traits>

#ifdef FMM_USE_FAST_FLOAT
#include <fast_float/fast_float.h>
#endif

#ifdef FMM_USE_DRAGONBOX
#include <dragonbox/dragonbox_to_chars.h>
#endif

#ifdef FMM_USE_RYU
#include <ryu/ryu.h>
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
        if (pos == end) {
            return pos;
        }

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

#ifdef FMM_FROM_CHARS_INT_SUPPORTED
    /**
     * Parse integer using std::from_chars
     */
    template <typename IT>
    const char* read_int(const char* pos, const char* end, IT& out) {
        std::from_chars_result result = std::from_chars(pos, end, out);
        if (result.ec != std::errc()) {
            throw invalid_mm("Invalid integer value.");
        }
        return result.ptr;
    }
#else
    /**
     * Parse integers using C routines.
     *
     * This is a compatibility fallback.
     */
    template <typename T>
    const char* read_int(const char* pos, [[maybe_unused]] const char* end, T& out) {
        errno = 0;

        char* value_end;
        long long parsed_value = std::strtoll(pos, &value_end, 10);
        if (errno != 0 || pos == value_end) {
            throw invalid_mm("Invalid integer value.");
        }
        out = static_cast<T>(parsed_value);
        return value_end;
    }
#endif

#ifdef FMM_USE_FAST_FLOAT
    /**
     * Parse float or double using fast_float::from_chars
     */
    template <typename FT>
    const char* read_float(const char* pos, const char* end, FT& out) {
        fast_float::from_chars_result result = fast_float::from_chars(pos, end, out, fast_float::chars_format::general);
        if (result.ec != std::errc()) {
            throw invalid_mm("Invalid floating-point value.");
        }
        return result.ptr;
    }
#elif defined(FMM_FROM_CHARS_DOUBLE_SUPPORTED)
    /**
     * Parse float or double using std::from_chars
     */
    template <typename FT>
    const char* read_float(const char* pos, const char* end, FT& out) {
        std::from_chars_result result = std::from_chars(pos, end, out);
        if (result.ec != std::errc()) {
            throw invalid_mm("Invalid floating-point value.");
        }
        return result.ptr;
    }

#else
    /**
     * Parse double using strtod(). This is a compatibility fallback.
     */
    inline const char* read_float(const char* pos, [[maybe_unused]] const char* end, double& out) {
        errno = 0;

        char* value_end;
        out = std::strtod(pos, &value_end);
        if (errno != 0 || pos == value_end) {
            throw invalid_mm("Invalid floating-point value.");
        }
        return value_end;
    }

    /**
     * Parse float using strtof(). This is a compatibility fallback.
     */
    inline const char* read_float(const char* pos, [[maybe_unused]] const char* end, float& out) {
        errno = 0;

        char* value_end;
        out = std::strtof(pos, &value_end);
        if (errno != 0 || pos == value_end) {
            throw invalid_mm("Invalid floating-point value.");
        }
        return value_end;
    }

#endif

#ifdef FMM_FROM_CHARS_LONG_DOUBLE_SUPPORTED
    /**
     * Parse long double using std::from_chars
     */
    inline const char* read_float(const char* pos, const char* end, long double& out) {
        std::from_chars_result result = std::from_chars(pos, end, out);
        if (result.ec != std::errc()) {
            throw invalid_mm("Invalid floating-point value.");
        }
        return result.ptr;
    }
#else
    /**
     * Parse `long double` using std::strtold().
     *
     * fast_float does not support long double.
     */
    inline const char* read_float(const char* pos, [[maybe_unused]] const char* end, long double& out) {
        errno = 0;

        char* value_end;
        out = std::strtold(pos, &value_end);
        if (errno != 0 || pos == value_end) {
            throw invalid_mm("Invalid floating-point value.");
        }
        return value_end;
    }
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

    inline const char* read_value(const char* pos, const char* end, bool& out) {
        double parsed;
        auto ret = read_float(pos, end, parsed);
        out = (parsed != 0);
        return ret;
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

#ifdef FMM_TO_CHARS_INT_SUPPORTED
    /**
     * Convert integral types to string.
     * std::to_string and std::to_chars has similar performance, however std::to_string is locale dependent and
     * therefore will cause thread serialization.
     */
    template <typename T>
    std::string int_to_string(const T& value) {
        std::string ret(20, ' ');
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

    inline std::string value_to_string([[maybe_unused]] const pattern_placeholder_type& value, [[maybe_unused]] int precision) {
        return {};
    }

    inline std::string value_to_string(const bool & value, [[maybe_unused]] int precision) {
        return value ? "1" : "0";
    }

    inline std::string value_to_string(const int32_t & value, [[maybe_unused]] int precision) {
        return int_to_string(value);
    }

    inline std::string value_to_string(const int64_t & value, [[maybe_unused]] int precision) {
        return int_to_string(value);
    }

    /**
     * stdlib fallback
     */
    template <typename T>
    std::string value_to_string_fallback(const T& value, int precision) {
        if (precision < 0) {
            // shortest representation
            return std::to_string(value);
        } else {
            std::ostringstream oss;
            oss << std::setprecision(precision) << value;
            return oss.str();
        }
    }

#ifdef FMM_USE_DRAGONBOX

    #ifndef DRAGONBOX_DROP_E0
    #define DRAGONBOX_DROP_E0 true
    #endif

    inline std::string value_to_string_dragonbox(const float& value) {
        std::string buffer(jkj::dragonbox::max_output_string_length<jkj::dragonbox::ieee754_binary32> + 1, ' ');

        char *end_ptr = jkj::dragonbox::to_chars(value, buffer.data());
        buffer.resize(end_ptr - buffer.data());

        if (DRAGONBOX_DROP_E0 && ends_with(buffer, "E0")) {
            buffer.resize(buffer.size() - 2);
        }
        return buffer;
    }

    inline std::string value_to_string_dragonbox(const double& value) {
        std::string buffer(jkj::dragonbox::max_output_string_length<jkj::dragonbox::ieee754_binary64> + 1, ' ');

        char *end_ptr = jkj::dragonbox::to_chars(value, buffer.data());
        buffer.resize(end_ptr - buffer.data());

        if (DRAGONBOX_DROP_E0 && ends_with(buffer, "E0")) {
            buffer.resize(buffer.size() - 2);
        }
        return buffer;
    }
#endif

#ifdef FMM_USE_RYU
    inline std::string value_to_string_ryu(const float& value, int precision) {
        std::string ret(16, ' ');

        if (precision < 0) {
            // shortest representation
            auto len = f2s_buffered_n(value, ret.data());
            ret.resize(len);
        } else {
            // explicit precision
            if (precision > 0) {
                // d2exp_buffered_n's precision means number of places after the decimal point, but
                // we expect it to mean number of sigfigs.
                --precision;
            }
            auto len = d2exp_buffered_n(static_cast<double>(value), precision, ret.data());
            ret.resize(len);
        }

        return ret;
    }

    inline std::string value_to_string_ryu(const double& value, int precision) {
        std::string ret(26, ' ');

        if (precision < 0) {
            // shortest representation
            auto len = d2s_buffered_n(value, ret.data());
            ret.resize(len);
        } else {
            // explicit precision
            if (precision > 0) {
                // d2exp_buffered_n's precision means number of places after the decimal point, but
                // we expect it to mean number of sigfigs.
                --precision;
            }
            auto len = d2exp_buffered_n(value, precision, ret.data());
            ret.resize(len);
        }

        return ret;
    }
#endif

#ifdef FMM_TO_CHARS_DOUBLE_SUPPORTED
    inline std::string value_to_string_to_chars(const float& value, int precision) {
        std::string ret(16, ' ');
        std::to_chars_result result;
        if (precision < 0) {
            // shortest representation
            result = std::to_chars(ret.data(), ret.data() + ret.size(), value);
        } else {
            // explicit precision
            result = std::to_chars(ret.data(), ret.data() + ret.size(), value, std::chars_format::general, precision);
        }
        if (result.ec == std::errc()) {
            ret.resize(result.ptr - ret.data());
            return ret;
        } else {
            return value_to_string_fallback(value, precistion);
        }
    }

    inline std::string value_to_string_to_chars(const double& value, int precision) {
        std::string ret(25, ' ');
        std::to_chars_result result;
        if (precision < 0) {
            // shortest representation
            result = std::to_chars(ret.data(), ret.data() + ret.size(), value);
        } else {
            // explicit precision
            result = std::to_chars(ret.data(), ret.data() + ret.size(), value, std::chars_format::general, precision);
        }
        if (result.ec == std::errc()) {
            ret.resize(result.ptr - ret.data());
            return ret;
        } else {
            return value_to_string_fallback(value, precistion);
        }
    }
#endif

#ifdef FMM_TO_CHARS_LONG_DOUBLE_SUPPORTED
    inline std::string value_to_string_to_chars(const long double& value, int precision) {
        std::string ret(50, ' ');
        std::to_chars_result result;
        if (precision < 0) {
            // shortest representation
            result = std::to_chars(ret.data(), ret.data() + ret.size(), value);
        } else {
            // explicit precision
            result = std::to_chars(ret.data(), ret.data() + ret.size(), value, std::chars_format::general, precision);
        }
        if (result.ec == std::errc()) {
            ret.resize(result.ptr - ret.data());
            return ret;
        } else {
            return value_to_string_fallback(value, precistion);
        }
    }
#endif

    /**
     * float to string.
     *
     * Preference order: Dragonbox (no precision support), to_chars, Ryu, fallback.
     */
    inline std::string value_to_string(const float& value, int precision) {
#ifdef FMM_USE_DRAGONBOX
        if (precision < 0) {
            // Shortest representation. Dragonbox is fastest.
            return value_to_string_dragonbox(value);
        }
#endif

#if defined(FMM_TO_CHARS_DOUBLE_SUPPORTED)
        return value_to_string_to_chars(value, precision);
#elif defined(FMM_USE_RYU)
        return value_to_string_ryu(value, precision);
#else
        return value_to_string_fallback(value, precision);
#endif
    }

    /**
     * double to string.
     *
     * Preference order: Dragonbox (no precision support), to_chars, Ryu, fallback.
     */
    inline std::string value_to_string(const double& value, int precision) {
#ifdef FMM_USE_DRAGONBOX
        if (precision < 0) {
            // Shortest representation. Dragonbox is fastest.
            return value_to_string_dragonbox(value);
        }
#endif

#if defined(FMM_TO_CHARS_DOUBLE_SUPPORTED)
        return value_to_string_to_chars(value, precision);
#elif defined(FMM_USE_RYU)
        return value_to_string_ryu(value, precision);
#else
        return value_to_string_fallback(value, precision);
#endif
    }

    /**
     * long double to string.
     *
     * Preference order: to_chars, fallback.
     * Note: Ryu's generic_128 can do this on some platforms, but it is not reliable.
     * see https://github.com/ulfjack/ryu/issues/215
     */
    inline std::string value_to_string(const long double& value, int precision) {
#if defined(FMM_TO_CHARS_LONG_DOUBLE_SUPPORTED)
        return value_to_string_to_chars(value, precision);
#else
        return value_to_string_fallback(value, precision);
#endif
    }

    inline std::string value_to_string(const std::complex<float>& value, int precision) {
        return value_to_string(value.real(), precision) + " " + value_to_string(value.imag(), precision);
    }

    inline std::string value_to_string(const std::complex<double>& value, int precision) {
        return value_to_string(value.real(), precision) + " " + value_to_string(value.imag(), precision);
    }

    inline std::string value_to_string(const std::complex<long double>& value, int precision) {
        return value_to_string(value.real(), precision) + " " + value_to_string(value.imag(), precision);
    }

    /**
     * Catchall
     */
    template <typename T>
    std::string value_to_string(const T& value, int precision) {
        return value_to_string_fallback(value, precision);
    }
}