// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#include <sstream>

#include <gtest/gtest.h>

#include <fast_matrix_market/fast_matrix_market.hpp>

std::string one_line_no_newline = "1 2 3";

std::string one_line_newline = "1 2 3\n";

std::string short_s = "123456 234567 333.323\n"
                "1 234567 333.323\n"
                "1 2 3";

std::string chunk_and_recombine(const std::string& s, int chunk_size) {
    std::string recombined;

    std::istringstream iss(s);
    fast_matrix_market::read_options options;
    options.chunk_size_bytes = chunk_size;

    while (iss.good()) {
        std::string chunk = fast_matrix_market::get_next_chunk(iss, options);
        recombined += chunk;
    }

    return recombined;
}

class ChunkingSuite : public testing::TestWithParam<int> {
public:
    struct PrintToStringParamName
    {
        template <class ParamType>
        std::string operator()( const testing::TestParamInfo<ParamType>& info ) const
        {
            return std::string("chunksize_") + std::to_string(info.param) + "";
        }
    };
};

TEST_P(ChunkingSuite, Small) {
    EXPECT_EQ(one_line_no_newline, chunk_and_recombine(one_line_no_newline, GetParam()));
    EXPECT_EQ(one_line_newline, chunk_and_recombine(one_line_newline, GetParam()));
    EXPECT_EQ(short_s, chunk_and_recombine(short_s, GetParam()));
}

INSTANTIATE_TEST_SUITE_P(Chunking, ChunkingSuite, testing::Range(0, 10),
                         ChunkingSuite::PrintToStringParamName());