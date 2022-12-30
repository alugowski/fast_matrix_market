// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include <fast_matrix_market/fast_matrix_market.hpp>

using std::filesystem::directory_iterator;

static const std::string kTestMatrixDir = TEST_MATRIX_DIR;  // configured in CMake
static const std::string kInvalidMatrixDir = kTestMatrixDir + "invalid/";

std::vector<std::string> get_invalid_matrix_files() {
    std::vector<std::string> ret;
    for (const auto & mtx : directory_iterator(kInvalidMatrixDir)) {
        ret.emplace_back(mtx.path().filename());
    }

    std::sort(ret.begin(), ret.end());
    return ret;
}

void read_mtx(const std::string& invalid) {
    std::vector<int64_t> rows;
    std::vector<int64_t> cols;
    std::vector<double> vals;

    std::ifstream f(invalid);

    fast_matrix_market::matrix_market_header header;
    fast_matrix_market::read_matrix_market_triplet(f, header, rows, cols, vals);
}

class InvalidSuite : public testing::TestWithParam<std::string> {
public:
    struct PrintToStringParamName
    {
        template <class ParamType>
        std::string operator()( const testing::TestParamInfo<ParamType>& info ) const
        {
            std::string ret(info.param);
            std::replace( ret.begin(), ret.end(), '.', '_');
            return ret;
        }
    };
};

TEST_P(InvalidSuite, Small) {
    EXPECT_THROW(read_mtx(GetParam()), fast_matrix_market::invalid_mm);
}

INSTANTIATE_TEST_SUITE_P(Invalid, InvalidSuite, testing::ValuesIn(get_invalid_matrix_files()),
                         InvalidSuite::PrintToStringParamName());