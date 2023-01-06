// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
#pragma once

// Disable pedantic GTest warnings
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"
#include <gtest/gtest.h>
#pragma clang diagnostic pop

#include <fast_matrix_market/fast_matrix_market.hpp>

static const std::string kTestMatrixDir = TEST_MATRIX_DIR;  // configured in CMake