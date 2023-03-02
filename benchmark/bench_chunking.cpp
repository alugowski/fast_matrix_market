// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#include <sstream>

#include "fmm_bench.hpp"

/**
 * Benchmark chunking.
 *
 * This needs to be fast because it is a sequential operation and can thus be a bottleneck.
 */
static void bench_chunking(benchmark::State& state) {
    std::string large = construct_large_coord_string(kCoordTargetBytes);
    // read options
    fast_matrix_market::read_options options{};

    std::size_t num_bytes = 0;

    for ([[maybe_unused]] auto _ : state) {
        std::istringstream iss(large);

        while (iss.good()) {
            std::string chunk = fast_matrix_market::get_next_chunk(iss, options);
            benchmark::DoNotOptimize(chunk);
        }

        num_bytes += large.size();
    }

    state.SetBytesProcessed((int64_t)num_bytes);
}

BENCHMARK(bench_chunking)->Name("op:chunking/impl:FMM/lang:C++")->UseRealTime();

/**
 * Benchmark counting newlines using std::count
 */
static void bench_count_lines_stdcount(benchmark::State& state) {
    const std::string large = construct_large_coord_string(kCoordTargetBytes);

    std::size_t num_bytes = 0;

    for ([[maybe_unused]] auto _ : state) {
        auto num_newlines = std::count(std::begin(large), std::end(large), '\n');
        benchmark::DoNotOptimize(num_newlines);
        num_bytes += large.size();
    }

    state.SetBytesProcessed((int64_t)num_bytes);
}

BENCHMARK(bench_count_lines_stdcount)->Name("op:count_lines/impl:stdcount/lang:C++")->UseRealTime();

/**
 * Benchmark counting newlines with iterators.
 */
static void bench_count_lines_iter(benchmark::State& state) {
    const std::string large = construct_large_coord_string(kCoordTargetBytes);

    std::size_t num_bytes = 0;

    for ([[maybe_unused]] auto _ : state) {
        long num_newlines = 0;

        auto first = std::cbegin(large);
        auto end = std::cend(large);
        for (; first != end; ++first) {
            if (*first == '\n') {
                ++num_newlines;
            }
        }

        benchmark::DoNotOptimize(num_newlines);
        num_bytes += large.size();
    }

    state.SetBytesProcessed((int64_t)num_bytes);
}

BENCHMARK(bench_count_lines_iter)->Name("op:count_lines/impl:iter/lang:C++")->UseRealTime();

/**
 * Benchmark counting newlines with a simple foreach loop.
 */
static void bench_count_lines_foreach(benchmark::State& state) {
    const std::string large = construct_large_coord_string(kCoordTargetBytes);

    std::size_t num_bytes = 0;

    for ([[maybe_unused]] auto _ : state) {
        long num_newlines = 0;

        for (const auto c : large) {
            if (c == '\n') {
                ++num_newlines;
            }
        }
        benchmark::DoNotOptimize(num_newlines);
        num_bytes += large.size();
    }

    state.SetBytesProcessed((int64_t)num_bytes);
}

BENCHMARK(bench_count_lines_foreach)->Name("op:count_lines/impl:foreach/lang:C++")->UseRealTime();

/**
 * Benchmark counting newlines with string::find().
 */
static void bench_count_lines_find(benchmark::State& state) {
    const std::string large = construct_large_coord_string(kCoordTargetBytes);

    std::size_t num_bytes = 0;

    for ([[maybe_unused]] auto _ : state) {
        long num_newlines = 0;

        size_t pos = 0;
        while (true) {
            pos = large.find('\n', pos);
            if (pos == std::string::npos) {
                break;
            } else {
                ++num_newlines;
                ++pos;
            }
        }

        benchmark::DoNotOptimize(num_newlines);
        num_bytes += large.size();
    }

    state.SetBytesProcessed((int64_t)num_bytes);
}

BENCHMARK(bench_count_lines_find)->Name("op:count_lines/impl:find/lang:C++")->UseRealTime();

/**
 * Benchmark counting lines using fmm::count_lines.
 */
static void bench_count_lines(benchmark::State& state) {
    const std::string large = construct_large_coord_string(kCoordTargetBytes);

    std::size_t num_bytes = 0;

    for ([[maybe_unused]] auto _ : state) {
        auto [lines, empties] = fast_matrix_market::count_lines(large);
        benchmark::DoNotOptimize(lines);
        benchmark::DoNotOptimize(empties);
        num_bytes += large.size();
    }

    state.SetBytesProcessed((int64_t)num_bytes);
}

BENCHMARK(bench_count_lines)->Name("op:count_lines/impl:count_lines/lang:C++")->UseRealTime();

/**
 * Benchmark counting empty lines using std::count
 */
static void bench_count_lines_empties(benchmark::State& state) {
    const std::string large = construct_large_coord_string(kCoordTargetBytes);

    std::size_t num_bytes = 0;

    for ([[maybe_unused]] auto _ : state) {
        long num_empty_lines = 0;

        auto pos = std::cbegin(large);
        auto end = std::cend(large);
        auto line_start = pos;
        for (; pos != end; ++pos) {
            if (*pos == '\n') {
                if (std::all_of(line_start, pos, [](char c) { return c == ' ' || c == '\t'; })) {
                    ++num_empty_lines;
                }
                line_start = pos + 1;
            }
        }

        benchmark::DoNotOptimize(num_empty_lines);
        num_bytes += large.size();
    }

    state.SetBytesProcessed((int64_t)num_bytes);
}

BENCHMARK(bench_count_lines_empties)->Name("op:count_lines_empties/impl:stdcount/lang:C++")->UseRealTime();

/**
 * Benchmark counting newlines using std::adjacent_find to find only completely empty lines (a line consisting of whitespace is not considered empty)
 */
static void bench_count_lines_empties_adjacent(benchmark::State& state) {
    const std::string large = construct_large_coord_string(kCoordTargetBytes);

    std::size_t num_bytes = 0;

    for ([[maybe_unused]] auto _ : state) {
        long num_empty_lines = 0;

        auto pos = std::cbegin(large);
        auto end = std::cend(large);
        while (pos != end) {
            auto empty_start = std::adjacent_find(pos, end, [](auto lhs, auto rhs) { return lhs == rhs && lhs == '\n'; });

            if (empty_start != end) {
                ++num_empty_lines;
                pos = empty_start + 1;
            } else {
                break;
            }
        }

        benchmark::DoNotOptimize(num_empty_lines);
        num_bytes += large.size();
    }

    state.SetBytesProcessed((int64_t)num_bytes);
}

BENCHMARK(bench_count_lines_empties_adjacent)->Name("op:count_lines_empties/impl:adjacent_find/lang:C++")->UseRealTime();
