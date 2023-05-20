// Copyright (C) 2022-2023 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
// SPDX-License-Identifier: BSD-2-Clause

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

////////////
///  Test reading integers
////////////

template <typename T>
class ReadInt : public testing::Test {
    T ignored = 0;
};
using ReadIntTypes = ::testing::Types<int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t>;
TYPED_TEST_SUITE(ReadInt, ReadIntTypes);

TYPED_TEST(ReadInt, Integer) {
    TypeParam i;

    std::string eight("8");
    std::string invalid("asdf");

#ifdef FMM_FROM_CHARS_INT_SUPPORTED
    EXPECT_THROW(fmm::read_int_from_chars(invalid.c_str(), invalid.c_str() + invalid.size(), i), fmm::invalid_mm);
    fmm::read_int_from_chars(eight.c_str(), eight.c_str() + eight.size(), i);
    EXPECT_EQ(i, 8);
#endif

    EXPECT_THROW(fmm::read_int_fallback(invalid.c_str(), invalid.c_str() + invalid.size(), i), fmm::invalid_mm);
    fmm::read_int_fallback(eight.c_str(), eight.c_str() + eight.size(), i);
    EXPECT_EQ(i, 8);

    EXPECT_THROW(fmm::read_int(invalid.c_str(), invalid.c_str() + invalid.size(), i), fmm::invalid_mm);
    fmm::read_int(eight.c_str(), eight.c_str() + eight.size(), i);
    EXPECT_EQ(i, 8);
}

TEST(ReadOverflow, Integer) {
    int8_t i8;
    int32_t i32;
    int64_t i64;

    std::string over_8("257");
    std::string over_64("19223372036854775808");

#ifdef FMM_FROM_CHARS_INT_SUPPORTED
    EXPECT_THROW(fmm::read_int_from_chars(over_8.c_str(), over_8.c_str() + over_8.size(), i8), fmm::out_of_range);
    EXPECT_THROW(fmm::read_int_from_chars(over_64.c_str(), over_64.c_str() + over_64.size(), i32), fmm::out_of_range);
    EXPECT_THROW(fmm::read_int_from_chars(over_64.c_str(), over_64.c_str() + over_64.size(), i64), fmm::out_of_range);
#endif

    EXPECT_THROW(fmm::read_int_fallback(over_8.c_str(), over_8.c_str() + over_8.size(), i8), fmm::out_of_range);
    EXPECT_THROW(fmm::read_int_fallback(over_64.c_str(), over_64.c_str() + over_64.size(), i32), fmm::out_of_range);
    EXPECT_THROW(fmm::read_int_fallback(over_64.c_str(), over_64.c_str() + over_64.size(), i64), fmm::out_of_range);

    EXPECT_THROW(fmm::read_int(over_8.c_str(), over_8.c_str() + over_8.size(), i8), fmm::out_of_range);
    EXPECT_THROW(fmm::read_int(over_64.c_str(), over_64.c_str() + over_64.size(), i32), fmm::out_of_range);
    EXPECT_THROW(fmm::read_int(over_64.c_str(), over_64.c_str() + over_64.size(), i64), fmm::out_of_range);
}


////////////
///  Test reading floating-point
////////////

template <typename T>
class ReadFloat : public testing::Test {
    T ignored = 0;
};
using ReadFloatTypes = ::testing::Types<float, double>;
TYPED_TEST_SUITE(ReadFloat, ReadFloatTypes);

TYPED_TEST(ReadFloat, Float) {
    TypeParam f;

    std::string eight("8");
    std::string invalid("asdf");

#ifdef FMM_USE_FAST_FLOAT
    EXPECT_THROW(fmm::read_float_fast_float(invalid.c_str(), invalid.c_str() + invalid.size(), f, fast_matrix_market::ThrowOutOfRange), fmm::invalid_mm);
    fmm::read_float_fast_float(eight.c_str(), eight.c_str() + eight.size(), f, fast_matrix_market::ThrowOutOfRange);
    EXPECT_EQ(f, 8);
#endif

#ifdef FMM_FROM_CHARS_DOUBLE_SUPPORTED
    EXPECT_THROW(fmm::read_float_from_chars(invalid.c_str(), invalid.c_str() + invalid.size(), f, fast_matrix_market::ThrowOutOfRange), fmm::invalid_mm);
    fmm::read_float_from_chars(eight.c_str(), eight.c_str() + eight.size(), f, fast_matrix_market::ThrowOutOfRange);
    EXPECT_EQ(f, 8);
#endif

    EXPECT_THROW(fmm::read_float_fallback(invalid.c_str(), invalid.c_str() + invalid.size(), f, fast_matrix_market::ThrowOutOfRange), fmm::invalid_mm);
    fmm::read_float_fallback(eight.c_str(), eight.c_str() + eight.size(), f, fast_matrix_market::ThrowOutOfRange);
    EXPECT_EQ(f, 8);

    EXPECT_THROW(fmm::read_float(invalid.c_str(), invalid.c_str() + invalid.size(), f, fast_matrix_market::ThrowOutOfRange), fmm::invalid_mm);
    fmm::read_float(eight.c_str(), eight.c_str() + eight.size(), f, fast_matrix_market::ThrowOutOfRange);
    EXPECT_EQ(f, 8);
}

TEST(ReadFloat, LongDouble) {
    long double f;

    std::string eight("8");
    std::string invalid("asdf");

#ifdef FMM_FROM_CHARS_LONG_DOUBLE_SUPPORTED
    EXPECT_THROW(fmm::read_float_from_chars(invalid.c_str(), invalid.c_str() + invalid.size(), f, fast_matrix_market::ThrowOutOfRange), fmm::invalid_mm);
    fmm::read_float_from_chars(eight.c_str(), eight.c_str() + eight.size(), f, fast_matrix_market::ThrowOutOfRange);
    EXPECT_EQ(f, 8);
#endif

    EXPECT_THROW(fmm::read_float_fallback(invalid.c_str(), invalid.c_str() + invalid.size(), f, fast_matrix_market::ThrowOutOfRange), fmm::invalid_mm);
    fmm::read_float_fallback(eight.c_str(), eight.c_str() + eight.size(), f, fast_matrix_market::ThrowOutOfRange);
    EXPECT_EQ(f, 8);

    EXPECT_THROW(fmm::read_float(invalid.c_str(), invalid.c_str() + invalid.size(), f, fast_matrix_market::ThrowOutOfRange), fmm::invalid_mm);
    fmm::read_float(eight.c_str(), eight.c_str() + eight.size(), f, fast_matrix_market::ThrowOutOfRange);
    EXPECT_EQ(f, 8);
}

TEST(ReadOverflow, Float) {
    float f = -1;
    double d = -1;
    long double ld = -1;

    std::string over_ld("1e99999");

#ifdef FMM_USE_FAST_FLOAT
    EXPECT_THROW(fmm::read_float_fast_float(over_ld.c_str(), over_ld.c_str() + over_ld.size(), f, fast_matrix_market::ThrowOutOfRange), fmm::out_of_range);
    EXPECT_THROW(fmm::read_float_fast_float(over_ld.c_str(), over_ld.c_str() + over_ld.size(), d, fast_matrix_market::ThrowOutOfRange), fmm::out_of_range);

    fmm::read_float_fast_float(over_ld.c_str(), over_ld.c_str() + over_ld.size(), f, fast_matrix_market::BestMatch);
    EXPECT_EQ(std::numeric_limits<float>::infinity(), f);

    fmm::read_float_fast_float(over_ld.c_str(), over_ld.c_str() + over_ld.size(), d, fast_matrix_market::BestMatch);
    EXPECT_EQ(std::numeric_limits<double>::infinity(), d);

#endif

#ifdef FMM_FROM_CHARS_DOUBLE_SUPPORTED
    EXPECT_THROW(fmm::read_float_from_chars(over_ld.c_str(), over_ld.c_str() + over_ld.size(), f, fast_matrix_market::ThrowOutOfRange), fmm::out_of_range);
    EXPECT_THROW(fmm::read_float_from_chars(over_ld.c_str(), over_ld.c_str() + over_ld.size(), d, fast_matrix_market::ThrowOutOfRange), fmm::out_of_range);

    fmm::read_float_from_chars(over_ld.c_str(), over_ld.c_str() + over_ld.size(), f, fast_matrix_market::BestMatch);
    EXPECT_EQ(std::numeric_limits<float>::infinity(), f);

    fmm::read_float_from_chars(over_ld.c_str(), over_ld.c_str() + over_ld.size(), d, fast_matrix_market::BestMatch);
    EXPECT_EQ(std::numeric_limits<double>::infinity(), d);
#endif
#ifdef FMM_FROM_CHARS_LONG_DOUBLE_SUPPORTED
    EXPECT_THROW(fmm::read_float_from_chars(over_ld.c_str(), over_ld.c_str() + over_ld.size(), ld, fast_matrix_market::ThrowOutOfRange), fmm::out_of_range);

    fmm::read_float_from_chars(over_ld.c_str(), over_ld.c_str() + over_ld.size(), ld, fast_matrix_market::BestMatch);
    EXPECT_EQ(std::numeric_limits<long double>::infinity(), ld);
#endif

    EXPECT_THROW(fmm::read_float_fallback(over_ld.c_str(), over_ld.c_str() + over_ld.size(), f, fast_matrix_market::ThrowOutOfRange), fmm::out_of_range);
    EXPECT_THROW(fmm::read_float_fallback(over_ld.c_str(), over_ld.c_str() + over_ld.size(), d, fast_matrix_market::ThrowOutOfRange), fmm::out_of_range);
    EXPECT_THROW(fmm::read_float_fallback(over_ld.c_str(), over_ld.c_str() + over_ld.size(), ld, fast_matrix_market::ThrowOutOfRange), fmm::out_of_range);
}