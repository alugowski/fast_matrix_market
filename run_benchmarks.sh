#!/bin/bash

# Build
cmake -S . -B cmake-build-release/ -D CMAKE_BUILD_TYPE=Release -D FAST_MATRIX_MARKET_TEST=OFF -D FAST_MATRIX_MARKET_BENCH=ON
cmake --build cmake-build-release/ --target benchmark/fmm_bench

./cmake-build-release/benchmark/fmm_bench --benchmark_out_format=json --benchmark_out=benchmark_plots/benchmark_output/output.json