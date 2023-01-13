// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#pragma once

#include <fast_matrix_market/fast_matrix_market.hpp>

namespace fast_matrix_market {

    /**
     * Matrix Market header starts with this string.
     */
    const std::string kMatrixMarketBanner = "%%MatrixMarket";

    /**
     * Invalid banner, but some packages emit this instead of the double %% version.
     */
    const std::string kMatrixMarketBanner2 = "%MatrixMarket";

    template <typename ENUM>
    ENUM parse_enum(const std::string& s, std::map<ENUM, const std::string> mp) {
        // Make s lowercase for a case-insensitive match
        std::string lower(s);
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char c){ return std::tolower(c); });

        for (const auto& [key, value] : mp) {
            if (value == lower) {
                return key;
            }
        }
        throw invalid_argument(std::string("Invalid value: ") + s);
    }

    /**
     * Calculate how many nonzero elements will need to be stored.
     * For general matrices this will be the same as header.nnz, but if the MatrixMarket file has symmetry and
     * generalize symmetry is selected then this function will calculate the total needed.
     */
    inline int64_t get_storage_nnz(const matrix_market_header& header, const read_options options) {
        if (header.symmetry != general && options.generalize_symmetry) {
            return 2 * header.nnz;
        } else {
            return header.nnz;
        }
    }

    /**
     * Parse a Matrix Market header comment line.
     * @param header
     * @param line
     * @return
     */
    inline bool read_comment(matrix_market_header& header, const std::string& line) {
        if (line.empty() || line[0] != '%') {
            return false;
        }

        // Line is a comment. Save it to the header.
        if (!header.comment.empty()) {
            header.comment += "\n";
        }
        header.comment += line.substr(1);
        return true;
    }

    /**
     * Parse an enum, but with error message fitting parsing of header.
     */
    template <typename ENUM>
    ENUM parse_header_enum(const std::string& s, std::map<ENUM, const std::string> mp, int64_t line_num) {
        // Make s lowercase for a case-insensitive match
        std::string lower(s);
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char c){ return std::tolower(c); });

        for (const auto& [key, value] : mp) {
            if (value == lower) {
                return key;
            }
        }
        throw invalid_mm(std::string("Invalid MatrixMarket header element: ") + s, line_num);
    }


    /**
     * Reads
     * @param instream stream to read from
     * @param header structure that will be filled with read header
     * @return number of lines read
     */
    inline int64_t read_header(std::istream& instream, matrix_market_header& header) {
        int64_t lines_read = 0;
        std::string line;

        // read banner
        std::getline(instream, line);
        lines_read++;

        if (line.rfind(kMatrixMarketBanner, 0) != 0
            && line.rfind(kMatrixMarketBanner2, 0) != 0) {
            // not a matrix market file because the banner is missing
            throw invalid_mm("Not a Matrix Market file. Missing banner.", lines_read);
        }

        // parse banner
        {
            std::istringstream iss(line);
            std::string banner, f_object, f_format, f_field, f_symmetry;
            iss >> banner >> f_object >> f_format >> f_field >> f_symmetry;

            header.object = parse_header_enum(f_object, object_map, lines_read);
            header.format = parse_header_enum(f_format, format_map, lines_read);
            header.field = parse_header_enum(f_field, field_map, lines_read);
            header.symmetry = parse_header_enum(f_symmetry, symmetry_map, lines_read);
        }

        // Read any comments
        do {
            std::getline(instream, line);
            lines_read++;

            if (!instream) {
                throw invalid_mm("Invalid MatrixMarket header: Premature EOF", lines_read);
            }
        } while (read_comment(header, line));

        // parse the dimension line
        {
            std::istringstream iss(line);

            if (header.object == vector) {
                iss >> header.vector_length;
                if (header.vector_length < 0) {
                    throw invalid_mm("Vector length can't be negative.", lines_read);
                }

                if (header.format == coordinate) {
                    iss >> header.nnz;
                } else {
                    header.nnz = header.vector_length;
                }

                header.nrows = header.vector_length;
                header.ncols = 1;
            } else {
                iss >> header.nrows >> header.ncols;
                if (header.nrows < 0 || header.ncols < 0) {
                    throw invalid_mm("Matrix dimensions can't be negative.", lines_read);
                }

                if (header.format == coordinate) {
                    iss >> header.nnz;
                    if (header.nnz < 0) {
                        throw invalid_mm("Matrix NNZ can't be negative.", lines_read);
                    }
                } else {
                    header.nnz = header.nrows * header.ncols;
                }
                header.vector_length = -1;
            }
        }

        header.header_line_count = lines_read;

        return lines_read;
    }

    inline bool write_header(std::ostream& os, const matrix_market_header& header) {
        // Write the banner
        os << kMatrixMarketBanner << kSpace;
        os << object_map.at(header.object) << kSpace;
        os << format_map.at(header.format) << kSpace;
        os << field_map.at(header.field) << kSpace;
        os << symmetry_map.at(header.symmetry) << kNewline;

        // Write the comment
        {
            std::istringstream iss(header.comment);
            std::string line;
            while (std::getline(iss, line)) {
                os << "%" << line << kNewline;
            }
        }

        // Write dimension line
        if (header.object == vector) {
            os << header.vector_length;
            if (header.format == coordinate) {
                os << kSpace << header.nnz;
            }
        } else {
            os << header.nrows << kSpace << header.ncols;
            if (header.format == coordinate) {
                os << kSpace << header.nnz;
            }
        }
        os << kNewline;

        return true;
    }

}
