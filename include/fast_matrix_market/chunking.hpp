// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#pragma once

#include <istream>
#include <string>

namespace fast_matrix_market {
    inline std::string get_next_chunk(std::istream &instream, const read_options &options) {
        constexpr size_t chunk_extra = 4096; // extra chunk bytes to leave room for rest of line

        // allocate chunk
        std::string chunk(options.chunk_size_bytes, ' ');
        size_t chunk_length = 0;

        // read chunk from the stream
        auto bytes_to_read = chunk.size() > chunk_extra ? (std::streamsize) (chunk.size() - chunk_extra) : 0;
        if (bytes_to_read > 0) {
            instream.read(chunk.data(), bytes_to_read);
            auto num_read = instream.gcount();
            chunk_length = num_read;

            // test for EOF
            if (num_read == 0 || instream.eof() || chunk[chunk_length - 1] == '\n') {
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
            std::copy(suffix.begin(), suffix.end(), chunk.begin() + (ptrdiff_t) chunk_length);
            chunk_length += suffix.size();
            chunk.resize(chunk_length);
        }
        return chunk;
    }

    /**
     * Find the number of lines in a multiline string.
     */
    inline int64_t count_lines(std::string chunk) {
        auto num_newlines = std::count(std::begin(chunk), std::end(chunk), '\n');

        if (num_newlines == 0) {
            // single line is still a line
            return 1;
        }

        if (chunk[chunk.size()-1] == '\n') {
            return num_newlines;
        } else {
            return num_newlines + 1;
        }
    }
}
