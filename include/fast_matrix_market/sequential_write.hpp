// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#pragma once

#include <charconv>
#include <complex>
#include <type_traits>

#include "fast_matrix_market.hpp"

namespace fast_matrix_market {
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

    inline std::string value_to_string(const std::complex<float>& value) {
        return std::to_string(value.real()) + " " + std::to_string(value.imag());
    }

    inline std::string value_to_string(const std::complex<double>& value) {
        return std::to_string(value.real()) + " " + std::to_string(value.imag());
    }

    inline std::string value_to_string(const std::complex<long double>& value) {
        return std::to_string(value.real()) + " " + std::to_string(value.imag());
    }

    template <typename T>
    std::string value_to_string(const T& value) {
        return std::to_string(value);
    }


    template <typename FORMATTER>
    void write_body(std::ostream& os,
                    FORMATTER& formatter, const write_options& options = {}) {

        while (formatter.has_next()) {
            std::string chunk = formatter.next_chunk(options).get();

            os.write(chunk.c_str(), (std::streamsize)chunk.size());
        }
    }
}