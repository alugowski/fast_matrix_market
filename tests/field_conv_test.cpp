// Copyright (C) 2022-2023 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#include "fmm_tests.hpp"

#if defined(__clang__)
// for TYPED_TEST_SUITE
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

namespace fmm = fast_matrix_market;


template <typename T>
bool almost_equal(T lhs, T rhs, double diff = 1E-6) {
    return std::abs(lhs - rhs) < diff;
}

template <typename T>
void parse(const std::string& s, T& val) {
    fmm::read_value(s.c_str(), s.c_str() + s.size(), val);
}


template <typename VT>
class FloatDoubleSuite : public testing::Test {
    VT ignored{};
};

using FloatDoubleTypes = ::testing::Types<float, double>;
TYPED_TEST_SUITE(FloatDoubleSuite, FloatDoubleTypes);

TYPED_TEST(FloatDoubleSuite, Basic) {
    TypeParam val = 1.23456789;
    TypeParam val2;

    // Test the main handler
    parse(fmm::value_to_string(val, -1), val2);
    EXPECT_TRUE(almost_equal(val, val2));

    // try precision
    parse(fmm::value_to_string(val, 8), val2);
    EXPECT_TRUE(almost_equal(val, val2, 1E-6));

    parse(fmm::value_to_string(val, 4), val2);
    EXPECT_FALSE(almost_equal(val, val2, 1E-6));

    // Test Dragonbox
#ifdef FMM_USE_DRAGONBOX
    parse(fmm::value_to_string_dragonbox(val), val2);
    EXPECT_TRUE(almost_equal(val, val2));
#endif

    // Test Ryu
#ifdef FMM_USE_RYU
    parse(fmm::value_to_string_ryu(val, -1), val2);
    EXPECT_TRUE(almost_equal(val, val2));

    // try precision
    parse(fmm::value_to_string_ryu(val, 8), val2);
    EXPECT_TRUE(almost_equal(val, val2, 1E-6));

    parse(fmm::value_to_string_ryu(val, 4), val2);
    EXPECT_FALSE(almost_equal(val, val2, 1E-6));

    EXPECT_EQ(fmm::value_to_string_ryu(val, 4).size() + 1, fmm::value_to_string_ryu(val, 5).size());

    // ensure the precision is the same as for the fallback
    {
        TypeParam ryu_val, fallback_val;
        parse(fmm::value_to_string_ryu(val, 4), ryu_val);
        parse(fmm::value_to_string_fallback(val, 4), fallback_val);
        EXPECT_TRUE(almost_equal(ryu_val, fallback_val));
    }
#endif

    // Test std::to_chars
#ifdef FMM_TO_CHARS_DOUBLE_SUPPORTED
    parse(fmm::value_to_string_to_chars(val, -1), val2);
    EXPECT_TRUE(almost_equal(val, val2));

    // try precision
    parse(fmm::value_to_string_to_chars(val, 8), val2);
    EXPECT_TRUE(almost_equal(val, val2, 1E-6));

    parse(fmm::value_to_string_to_chars(val, 4), val2);
    EXPECT_FALSE(almost_equal(val, val2, 1E-6));

    // ensure the precision is the same as for the fallback
    {
        TypeParam to_chars_val, fallback_val;
        parse(fmm::value_to_string_to_chars(val, 4), to_chars_val);
        parse(fmm::value_to_string_fallback(val, 4), fallback_val);
        EXPECT_TRUE(almost_equal(to_chars_val, fallback_val));
    }
#endif

    // Test fallback
    parse(fmm::value_to_string_fallback(val, -1), val2);
    EXPECT_TRUE(almost_equal(val, val2));

    // try precision
    parse(fmm::value_to_string_fallback(val, 8), val2);
    EXPECT_TRUE(almost_equal(val, val2, 1E-6));

    parse(fmm::value_to_string_fallback(val, 4), val2);
    EXPECT_FALSE(almost_equal(val, val2, 1E-6));
}

// Test long double
TEST(LongDoubleSuite, Basic) {
    long double val = 1.23456789;
    long double val2;

    // Test the main handler
    parse(fmm::value_to_string(val, -1), val2);
    EXPECT_TRUE(almost_equal(val, val2));

    // try precision
    parse(fmm::value_to_string(val, 8), val2);
    EXPECT_TRUE(almost_equal(val, val2, 1E-6));

    parse(fmm::value_to_string(val, 4), val2);
    EXPECT_FALSE(almost_equal(val, val2, 1E-6));

    // Test std::to_chars
#ifdef FMM_TO_CHARS_LONG_DOUBLE_SUPPORTED
    parse(fmm::value_to_string_to_chars(val, -1), val2);
    EXPECT_TRUE(almost_equal(val, val2));

    // try precision
    parse(fmm::value_to_string_to_chars(val, 8), val2);
    EXPECT_TRUE(almost_equal(val, val2, 1E-6));

    parse(fmm::value_to_string_to_chars(val, 4), val2);
    EXPECT_FALSE(almost_equal(val, val2, 1E-6));
#endif

    // Test fallback
    parse(fmm::value_to_string_fallback(val, -1), val2);
    EXPECT_TRUE(almost_equal(val, val2));

    // try precision
    parse(fmm::value_to_string_fallback(val, 8), val2);
    EXPECT_TRUE(almost_equal(val, val2, 1E-6));

    parse(fmm::value_to_string_fallback(val, 4), val2);
    EXPECT_FALSE(almost_equal(val, val2, 1E-6));
}