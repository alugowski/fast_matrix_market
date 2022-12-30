// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#pragma once

#include <charconv>
#include <complex>
#include <type_traits>
#include <fast_float/fast_float.h>

#include "fast_matrix_market.hpp"

namespace fast_matrix_market {

    /**
     * A type to use for pattern matrices. Pattern Matrix Market files do not write a value column, only the
     * coordinates.
     */
    struct pattern_placeholder_type {};

    // Negation needed to support symmetry expansion. Skew-symmetric symmetry negates values.
    inline pattern_placeholder_type operator-(const pattern_placeholder_type& o) { return o; }


    /**
     * A handler wrapper for easily handling pattern matrices. This forwards a fixed value. For example, write 1.0 to
     * double matrices. Avoid using zero.
     */
    template <typename FWD_HANDLER>
    class pattern_parse_adapter {
    public:
        using coordinate_type = typename FWD_HANDLER::coordinate_type;
        using value_type = pattern_placeholder_type;

        explicit pattern_parse_adapter(FWD_HANDLER &handler, typename FWD_HANDLER::value_type fwd_value) : handler(handler), fwd_value(fwd_value) {}

        void handle(const coordinate_type row, const coordinate_type col, [[maybe_unused]] const value_type ignored) {
            handler.handle(row, col, fwd_value);
        }

    protected:
        FWD_HANDLER& handler;
        typename FWD_HANDLER::value_type fwd_value;
    };

    /**
     * A handler wrapper so that real/integer files can be read into std::complex matrices by setting all
     * imaginary parts to zero.
     */
    template <typename COMPLEX_HANDLER>
    class complex_parse_adapter {
    public:
        using coordinate_type = typename COMPLEX_HANDLER::coordinate_type;
        using complex_type = typename COMPLEX_HANDLER::value_type;
        using value_type = typename complex_type::value_type;

        explicit complex_parse_adapter(COMPLEX_HANDLER &handler) : handler(handler) {}

        void handle(const coordinate_type row, const coordinate_type col, const value_type real) {
            handler.handle(row, col, complex_type(real, 0));
        }

    protected:
        COMPLEX_HANDLER& handler;
    };

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


    ///////////////////////////////////////////////////////////////////
    // Chunks
    ///////////////////////////////////////////////////////////////////

    template <typename HANDLER>
    int64_t read_chunk_matrix_coordinate(const std::string& chunk, const matrix_market_header& header, int64_t line_num,
                                         HANDLER& handler, const read_options& options) {
        const char* pos = chunk.c_str();
        const char* end = pos + chunk.size();

        while (pos != end && pos != nullptr) {
            typename HANDLER::coordinate_type row, col;
            typename HANDLER::value_type value;

            pos = skip_spaces(pos);
            pos = read_int(pos, end, row);
            pos = skip_spaces(pos);
            pos = read_int(pos, end, col);
            pos = skip_spaces(pos);
            pos = read_value(pos, end, value);
            pos = bump_to_next_line(pos, end);

            // validate
            if (row <= 0 || row > header.nrows) {
                throw invalid_mm("Row index out of bounds", line_num);
            }
            if (col <= 0 || col > header.ncols) {
                throw invalid_mm("Column index out of bounds", line_num);
            }

            // Matrix Market is one-based
            handler.handle(row - 1, col - 1, value);

            if (header.symmetry != general && options.generalize_symmetry && col != row) {
                switch (header.symmetry) {
                    case symmetric:
                        handler.handle(col - 1, row - 1, value);
                        break;
                    case skew_symmetric:
                        handler.handle(col - 1, row - 1, -value);
                        break;
                    case hermitian:
                        handler.handle(col - 1, row - 1, complex_conjugate(value));
                        break;
                    case general:
                        break;
                }
            }

            ++line_num;
        }
        return line_num;
    }

    template <typename HANDLER>
    int64_t read_chunk_vector_coordinate(const std::string& chunk, const matrix_market_header& header, int64_t line_num,
                                         HANDLER& handler) {
        const char* pos = chunk.c_str();
        const char* end = pos + chunk.size();

        while (pos != end && pos != nullptr) {
            typename HANDLER::coordinate_type row;
            typename HANDLER::value_type value;

            pos = skip_spaces(pos);
            pos = read_int(pos, end, row);
            pos = skip_spaces(pos);
            pos = read_value(pos, end, value);
            pos = bump_to_next_line(pos, end);

            // validate
            if (row <= 0 || row > header.vector_length) {
                throw invalid_mm("Index out of bounds", line_num);
            }

            // Matrix Market is one-based
            handler.handle(row - 1, 0, value);

            ++line_num;
        }
        return line_num;
    }

    template <typename HANDLER>
    int64_t read_chunk_array(const std::string& chunk, const matrix_market_header& header, int64_t line_num,
                             HANDLER& handler,
                             typename HANDLER::coordinate_type& row,
                             typename HANDLER::coordinate_type& col) {
        const char* pos = chunk.c_str();
        const char* end = pos + chunk.size();

        while (pos != end && pos != nullptr) {
            if (col == header.ncols) {
                throw invalid_mm("Too many values in array", line_num);
            }

            typename HANDLER::value_type value;

            pos = skip_spaces(pos);
            pos = read_value(pos, end, value);
            pos = bump_to_next_line(pos, end);

            handler.handle(row, col, value);

            // Matrix Market is column-major.
            ++row;
            if (row == header.nrows) {
                ++col;
                row = 0;
            }

            ++line_num;
        }
        return line_num;
    }

    inline std::string get_next_chunk(std::istream& instream, const read_options& options) {
        constexpr size_t chunk_extra = 4096; // extra chunk bytes to leave room for rest of line

        // allocate chunk
        std::string chunk(options.chunk_size_bytes, ' ');
        size_t chunk_length = 0;

        // read chunk from the stream
        auto bytes_to_read = chunk.size() > chunk_extra ? (std::streamsize)(chunk.size() - chunk_extra) : 0;
        if (bytes_to_read > 0) {
            instream.read(chunk.data(), bytes_to_read);
            auto num_read = instream.gcount();
            chunk_length = num_read;

            // test for EOF
            if (num_read == 0 || instream.eof() || chunk[chunk_length-1] == '\n') {
                chunk.resize(chunk_length);
                return chunk;
            }
        }

        // Read rest of line and append to the chunk.
        std::string suffix;
        std::getline(instream, suffix);
        if (instream.good()) {
            suffix += "\n";
        }

        if (chunk_length + suffix.size() > chunk.size()) {
            // rest of line didn't fit in the extra space, must copy
            chunk.resize(chunk_length);
            chunk += suffix;
        } else {
            // the suffix fits in the chunk.
            std::copy(suffix.begin(), suffix.end(), chunk.begin() + (ptrdiff_t)chunk_length);
            chunk_length += suffix.size();
            chunk.resize(chunk_length);
        }
        return chunk;
    }

    ////////////////////////////////////////////////
    // Read Matrix Market body
    // Get chunks from file, read chunks
    ///////////////////////////////////////////////

    template <typename HANDLER>
    void read_coordinate_body(std::istream& instream, const matrix_market_header& header,
                              HANDLER& handler, const read_options& options = {}) {
        auto line_num = header.header_line_count;
        auto expected_line_count = line_num + header.nnz;

        // Read the file in chunks
        while (instream.good()) {
            std::string chunk = get_next_chunk(instream, options);

            // parse the chunk
            if (header.object == matrix) {
                line_num = read_chunk_matrix_coordinate(chunk, header, line_num, handler, options);
            } else {
                if (header.symmetry != general && options.generalize_symmetry) {
                    throw not_implemented("Non-general symmetry for vectors not implemented.");
                }
                line_num = read_chunk_vector_coordinate(chunk, header, line_num, handler);
            }
        }

        if (line_num < expected_line_count) {
            throw invalid_mm(std::string("Truncated file. Expected another ") + std::to_string(expected_line_count - line_num) + " lines.");
        }
    }

    template <typename HANDLER>
    void read_array_body(std::istream& instream, const matrix_market_header& header,
                         HANDLER& handler,
                         const read_options& options = {}) {
        if (header.field == pattern) {
            throw not_implemented("Pattern arrays not implemented.");
        }

        auto line_num = header.header_line_count;
        auto expected_line_count = line_num + header.nnz;

        typename HANDLER::coordinate_type row = 0;
        typename HANDLER::coordinate_type col = 0;

        // Read the file in chunks
        while (instream.good()) {
            std::string chunk = get_next_chunk(instream, options);

            // parse the chunk
            line_num = read_chunk_array(chunk, header, line_num, handler, row, col);
        }

        if (line_num < expected_line_count) {
            throw invalid_mm(std::string("Truncated file. Expected another ") + std::to_string(expected_line_count - line_num) + " lines.");
        }
    }

    template <typename HANDLER>
    void read_matrix_market_body_no_complex(std::istream& instream, matrix_market_header& header,
                                            HANDLER& handler, const read_options& options = {}) {
        if (header.format == coordinate) {
            read_coordinate_body(instream, header, handler, options);
        } else {
            if (header.symmetry != general && options.generalize_symmetry) {
                throw not_implemented("Non-general symmetry for array matrices not implemented.");
            }
            read_array_body(instream, header, handler, options);
        }

        if (header.symmetry != general && options.generalize_symmetry) {
            // The symmetry has been generalized, so mark this in the parsed header.
            // This is because to the calling code this might as well be a 'general' file.
            header.symmetry = general;
        }
    }

    template <typename HANDLER, typename std::enable_if<is_complex<typename HANDLER::value_type>::value, int>::type = 0>
    void read_matrix_market_body(std::istream& instream, matrix_market_header& header,
                                 HANDLER& handler, const read_options& options = {}) {
        if (header.field == complex) {
            read_matrix_market_body_no_complex(instream, header, handler, options);
        } else {
            // the handler is expecting std::complex values, but the file is only integer/real
            // provide adapter
            auto fwd_handler = complex_parse_adapter<HANDLER>(handler);
            read_matrix_market_body_no_complex(instream, header, fwd_handler, options);
        }
    }

    template <typename HANDLER, typename std::enable_if<!is_complex<typename HANDLER::value_type>::value, int>::type = 0>
    void read_matrix_market_body(std::istream& instream, matrix_market_header& header,
                                 HANDLER& handler, const read_options& options = {}) {
        if (header.field != complex) {
            read_matrix_market_body_no_complex(instream, header, handler, options);
        } else {
            // the file is complex but the values are not
            throw complex_incompatible("Matrix Market file has complex fields but passed data structure cannot handle complex values.");
        }
    }

    template <typename HANDLER>
    void read_matrix_market_body_with_pattern(std::istream& instream, matrix_market_header& header,
                                              HANDLER& handler,
                                              typename HANDLER::value_type pattern_value,
                                              const read_options& options = {}) {
        if (header.field == pattern) {
            auto fwd_handler = pattern_parse_adapter<HANDLER>(handler, pattern_value);
            read_matrix_market_body(instream, header, fwd_handler, options);
        } else {
            read_matrix_market_body(instream, header, handler, options);
        }
    }
}