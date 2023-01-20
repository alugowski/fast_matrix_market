# Copyright (C) 2022-2023 Adam Lugowski. All rights reserved.
# Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
import math
from io import BytesIO
from pathlib import Path
import tempfile

import numpy as np
import scipy.io
from scipy.sparse import coo_matrix

import google_benchmark as benchmark

import fast_matrix_market as fmm


def generate_large_coo_matrix(nnz):
    i = np.arange(nnz, dtype="int32")  # scipy uses intc indices (intc is usually int32)
    j = i
    vals = np.arange(nnz, dtype="float64")
    vals /= 100
    return coo_matrix((vals, (i, j)), shape=(nnz, nnz))


def generate_large_array_matrix(nnz):
    n = int(math.sqrt(nnz))
    m = int(nnz / n)
    mat = np.arange(m*n, dtype="float64")
    mat /= 100
    mat = np.reshape(mat, (m, n))
    return mat


def matrix_to_bytes(mat):
    bio = BytesIO()
    fmm.write_scipy(bio, a=mat)
    return bio.getvalue()


num_elements = 10_000_000

tempdir = tempfile.TemporaryDirectory()
write_path = Path(tempdir.name) / "writefile.mtx"

# Generate coordinate matrix
large_coo_matrix = generate_large_coo_matrix(num_elements)
large_coo_bytes = matrix_to_bytes(large_coo_matrix)
large_coo_length = len(large_coo_bytes)   # About 265 MB for nnz=10M

coo_read_path = Path(tempdir.name) / "readfile_coo.mtx"
coo_read_path.write_bytes(large_coo_bytes)

# Generate array matrix
large_array_matrix = generate_large_array_matrix(num_elements)

large_array_bytes = matrix_to_bytes(large_array_matrix)
large_array_length = len(large_array_bytes)  # About 50 MB for nnz=10M

array_read_path = Path(tempdir.name) / "readfile_array.mtx"
array_read_path.write_bytes(large_array_bytes)

print(f"Array matrix has {large_array_matrix.size:,} elements and {large_array_length:,} bytes in MatrixMarket format.")
print(f"Triplet matrix has {large_coo_matrix.size:,} elements and {large_coo_length:,} bytes in MatrixMarket format.")


##############################
# Arrays

@benchmark.register(name="op:read/matrix:Array/impl:FMM(Python)/lang:Python")
@benchmark.option.use_real_time()
@benchmark.option.iterations(3)
@benchmark.option.arg_name("p")
@benchmark.option.arg(1)
@benchmark.option.arg_name("p")
@benchmark.option.arg(2)
@benchmark.option.arg_name("p")
@benchmark.option.arg(3)
@benchmark.option.arg_name("p")
@benchmark.option.arg(4)
@benchmark.option.arg_name("p")
@benchmark.option.arg(6)
@benchmark.option.arg_name("p")
@benchmark.option.arg(8)
def triplet_fmm(state):
    while state:
        parallelism = state.range(0)
        _ = fmm.read_scipy(array_read_path, parallelism=parallelism)
    state.bytes_processed = state.iterations * array_read_path.stat().st_size


@benchmark.register(name="op:write/matrix:Array/impl:FMM(Python)/lang:Python")
@benchmark.option.use_real_time()
@benchmark.option.iterations(3)
@benchmark.option.arg_name("p")
@benchmark.option.arg(1)
@benchmark.option.arg_name("p")
@benchmark.option.arg(2)
@benchmark.option.arg_name("p")
@benchmark.option.arg(3)
@benchmark.option.arg_name("p")
@benchmark.option.arg(4)
@benchmark.option.arg_name("p")
@benchmark.option.arg(6)
@benchmark.option.arg_name("p")
@benchmark.option.arg(8)
def triplet_fmm(state):
    while state:
        parallelism = state.range(0)
        fmm.write_scipy(write_path, large_array_matrix, parallelism=parallelism)
    state.bytes_processed = state.iterations * write_path.stat().st_size


@benchmark.register(name="op:read/matrix:Array/impl:SciPy/lang:Python")
@benchmark.option.use_real_time()
def triplet_scipy(state):
    while state:
        _ = scipy.io.mmread(array_read_path)
    state.bytes_processed = state.iterations * array_read_path.stat().st_size


@benchmark.register(name="op:write/matrix:Array/impl:SciPy/lang:Python")
@benchmark.option.use_real_time()
def triplet_scipy(state):
    while state:
        # Specifying a symmetry prevents scipy from searching for a symmetry.
        # This can greatly speed up mmwrite()
        scipy.io.mmwrite(write_path, large_array_matrix, symmetry="general")
    state.bytes_processed = state.iterations * write_path.stat().st_size


##############################
# Triplets

@benchmark.register(name="op:read/matrix:Coordinate/impl:FMM(Python)/lang:Python")
@benchmark.option.use_real_time()
@benchmark.option.iterations(3)
@benchmark.option.arg_name("p")
@benchmark.option.arg(1)
@benchmark.option.arg_name("p")
@benchmark.option.arg(2)
@benchmark.option.arg_name("p")
@benchmark.option.arg(3)
@benchmark.option.arg_name("p")
@benchmark.option.arg(4)
@benchmark.option.arg_name("p")
@benchmark.option.arg(6)
@benchmark.option.arg_name("p")
@benchmark.option.arg(8)
def triplet_fmm(state):
    while state:
        parallelism = state.range(0)
        _ = fmm.read_scipy(coo_read_path, parallelism=parallelism)
    state.bytes_processed = state.iterations * coo_read_path.stat().st_size


@benchmark.register(name="op:write/matrix:Coordinate/impl:FMM(Python)/lang:Python")
@benchmark.option.use_real_time()
@benchmark.option.iterations(3)
@benchmark.option.arg_name("p")
@benchmark.option.arg(1)
@benchmark.option.arg_name("p")
@benchmark.option.arg(2)
@benchmark.option.arg_name("p")
@benchmark.option.arg(3)
@benchmark.option.arg_name("p")
@benchmark.option.arg(4)
@benchmark.option.arg_name("p")
@benchmark.option.arg(6)
@benchmark.option.arg_name("p")
@benchmark.option.arg(8)
def triplet_fmm(state):
    while state:
        parallelism = state.range(0)
        fmm.write_scipy(write_path, large_coo_matrix, parallelism=parallelism)
    state.bytes_processed = state.iterations * write_path.stat().st_size


@benchmark.register(name="op:read/matrix:Coordinate/impl:SciPy/lang:Python")
@benchmark.option.use_real_time()
def triplet_scipy(state):
    while state:
        _ = scipy.io.mmread(coo_read_path)
    state.bytes_processed = state.iterations * coo_read_path.stat().st_size


@benchmark.register(name="op:write/matrix:Coordinate/impl:SciPy/lang:Python")
@benchmark.option.use_real_time()
def triplet_scipy(state):
    while state:
        # Specifying a symmetry prevents scipy from searching for a symmetry.
        # This can greatly speed up mmwrite()
        scipy.io.mmwrite(write_path, large_coo_matrix, symmetry="general")
    state.bytes_processed = state.iterations * write_path.stat().st_size


if __name__ == "__main__":
    benchmark.main()
