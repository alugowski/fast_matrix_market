// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
// SPDX-License-Identifier: BSD-2-Clause

#include <sstream>

#include "fmm_tests.hpp"

std::string empty;

std::string newline_only = "\n";

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
    EXPECT_EQ(empty, chunk_and_recombine(empty, GetParam()));
    EXPECT_EQ(newline_only, chunk_and_recombine(newline_only, GetParam()));
    EXPECT_EQ(one_line_no_newline, chunk_and_recombine(one_line_no_newline, GetParam()));
    EXPECT_EQ(one_line_newline, chunk_and_recombine(one_line_newline, GetParam()));
    EXPECT_EQ(short_s, chunk_and_recombine(short_s, GetParam()));
}

INSTANTIATE_TEST_SUITE_P(Chunking, ChunkingSuite, testing::Range(0, 10),
                         ChunkingSuite::PrintToStringParamName());


std::pair<int64_t, int64_t> make_i64_pair(int64_t lhs, int64_t rhs) {
    return std::make_pair(lhs, rhs);
}

TEST(LineCount, LineCount) {
    EXPECT_EQ(fast_matrix_market::count_lines(""), make_i64_pair(1, 1));
    EXPECT_EQ(fast_matrix_market::count_lines(" "), make_i64_pair(1, 1));
    EXPECT_EQ(fast_matrix_market::count_lines("asdf"), make_i64_pair(1, 0));
    EXPECT_EQ(fast_matrix_market::count_lines("\n"), make_i64_pair(1, 1));
    EXPECT_EQ(fast_matrix_market::count_lines(" \n"), make_i64_pair(1, 1));
    EXPECT_EQ(fast_matrix_market::count_lines("\n "), make_i64_pair(2, 2));
    EXPECT_EQ(fast_matrix_market::count_lines(" \n "), make_i64_pair(2, 2));
    EXPECT_EQ(fast_matrix_market::count_lines("  \t \n  "), make_i64_pair(2, 2));
    EXPECT_EQ(fast_matrix_market::count_lines("aa\n"), make_i64_pair(1, 0));
    EXPECT_EQ(fast_matrix_market::count_lines("aa\nbb"), make_i64_pair(2, 0));
    EXPECT_EQ(fast_matrix_market::count_lines("aa\nbb\n"), make_i64_pair(2, 0));
    EXPECT_EQ(fast_matrix_market::count_lines("aa\n "), make_i64_pair(2, 1));
    EXPECT_EQ(fast_matrix_market::count_lines(" \nbb"), make_i64_pair(2, 1));
    EXPECT_EQ(fast_matrix_market::count_lines("aa\n\n"), make_i64_pair(2, 1));
    EXPECT_EQ(fast_matrix_market::count_lines("aa\n\n\n"), make_i64_pair(3, 2));
}
