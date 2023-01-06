// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#pragma once

#include <charconv>
#include <complex>
#include <type_traits>

#include "fast_matrix_market/fast_matrix_market.hpp"

// Disable some pedantic warnings from Eigen headers.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-but-set-variable"
#include <Eigen/Sparse>
#pragma clang diagnostic pop


namespace fast_matrix_market {

//    template <typename, typename = void>
//    struct is_eigen_sparse : std::false_type {};
//
//    template <typename T>
//    struct is_eigen_sparse<T, std::void_t<decltype(&T::isCompressed)>> : std::is_same<bool, decltype(std::declval<T>().isCompressed())> {};

    /**
     * Read Matrix Market file into an Eigen matrix.
     */
    template <typename SparseType>
    void read_matrix_market_eigen(std::istream &instream,
                                  SparseType& mat,
                                  const read_options& options = {}) {

        typedef typename SparseType::Scalar Scalar;
        typedef typename SparseType::StorageIndex StorageIndex;
        typedef Eigen::Triplet<Scalar, StorageIndex> Triplet;

        matrix_market_header header;
        read_header(instream, header);
        mat.resize(header.nrows, header.ncols);
        mat.reserve(header.nnz);

        // read into tuples
        std::vector<Triplet> elements;
        elements.reserve(header.nnz);

        auto handler = tuple_parse_handler<Triplet, StorageIndex, Scalar>(elements);
        read_matrix_market_body(instream, header, handler, 1, options);

        // set the values into the matrix
        mat.setFromTriplets(elements.begin(), elements.end());
    }

    /**
     * Read Matrix Market file into an Eigen Dense matrix.
     */
    template <typename DenseType>
    void read_matrix_market_eigen_dense(std::istream &instream,
                                  DenseType& mat,
                                  const read_options& options = {}) {
        matrix_market_header header;
        read_header(instream, header);
        mat.resize(header.nrows, header.ncols);

        auto handler = setting_2d_parse_handler<DenseType, typename DenseType::Index, typename DenseType::Scalar>(mat);
        read_matrix_market_body(instream, header, handler, 1, options);
    }

    /**
    * Format Eigen Sparse Matrix.
    */
    template<typename SparseMatrixType>
    class sparse_Eigen_formatter {
    public:
        typedef typename SparseMatrixType::Index MatIndex;
        explicit sparse_Eigen_formatter(const SparseMatrixType& mat) : mat(mat) {
            nnz_per_column = ((double)mat.nonZeros()) / mat.outerSize();
        }

        [[nodiscard]] bool has_next() const {
            return outer_iter < mat.outerSize();
        }

        class chunk {
        public:
            explicit chunk(const SparseMatrixType& mat, MatIndex outer_iter, MatIndex outer_end) :
            mat(mat), outer_iter(outer_iter), outer_end(outer_end) {}

            std::string get() {
                std::string chunk;
                chunk.reserve((outer_end - outer_iter)*250);

                // iterate over assigned columns
                for (; outer_iter != outer_end; ++outer_iter) {

                    for (typename SparseMatrixType::InnerIterator it(mat, outer_iter); it; ++it)
                    {
                        chunk += int_to_string(it.row() + 1);
                        chunk += kSpace;
                        chunk += int_to_string(it.col() + 1);
                        chunk += kSpace;
                        chunk += value_to_string(it.value());
                        chunk += kNewline;
                    }
                }

                return chunk;
            }

            const SparseMatrixType& mat;
            MatIndex outer_iter, outer_end;
        };

        chunk next_chunk(const write_options& options) {
            auto num_columns = (ptrdiff_t)(nnz_per_column * (double)options.chunk_size_values + 1);

            MatIndex outer_end = std::min(outer_iter + num_columns, mat.outerSize());
            chunk c(mat, outer_iter, outer_end);
            outer_iter = outer_end;

            return c;
        }

    protected:
        const SparseMatrixType& mat;
        double nnz_per_column;
        MatIndex outer_iter = 0;
    };

    /**
     * Format Eigen Dense Matrix/Vector.
     *
     * Supports any structure that has:
     * .cols() - returns number of columns
     * .rows() - returns number of rows
     * .operator(row, col) - returns the value at (row, col)
     */
    template<typename DenseType>
    class dense_Eigen_formatter {
    public:
        typedef typename DenseType::Index MatIndex;
        explicit dense_Eigen_formatter(const DenseType& mat) : mat(mat) {}

        [[nodiscard]] bool has_next() const {
            return col_iter < mat.cols();
        }

        class chunk {
        public:
            explicit chunk(const DenseType& mat, MatIndex col_iter, MatIndex col_end) :
                    mat(mat), col_iter(col_iter), col_end(col_end) {}

            std::string get() {
                std::string chunk;
                chunk.reserve((col_end - col_iter) * mat.rows() * 15);

                // iterate over assigned columns
                for (; col_iter != col_end; ++col_iter) {

                    for (MatIndex row = 0; row < mat.rows(); ++row)
                    {
                        chunk += value_to_string(mat(row, col_iter));
                        chunk += kNewline;
                    }
                }

                return chunk;
            }

            const DenseType& mat;
            MatIndex col_iter, col_end;
        };

        chunk next_chunk(const write_options& options) {
            auto num_columns = (ptrdiff_t)(mat.rows() * (double)options.chunk_size_values + 1);

            MatIndex col_end = std::min(col_iter + num_columns, mat.cols());
            chunk c(mat, col_iter, col_end);
            col_iter = col_end;

            return c;
        }

    protected:
        const DenseType& mat;
        MatIndex col_iter = 0;
    };

    /**
     * Write an Eigen sparse matrix to MatrixMarket.
     */
    template <typename SparseType>
    void write_matrix_market_eigen(std::ostream &os,
                                   SparseType& mat,
                                   const write_options& options = {}) {
        matrix_market_header header;
        header.nrows = mat.rows();
        header.ncols = mat.cols();
        header.nnz = mat.nonZeros();

        header.object = matrix;
        header.field = get_field_type::value<typename SparseType::Scalar>();
        header.format = coordinate;

        write_header(os, header);

        auto formatter = sparse_Eigen_formatter(mat);
        write_body(os, formatter, options);
    }

    /**
     * Write an Eigen dense matrix to MatrixMarket.
     */
    template <typename DenseType>
    void write_matrix_market_eigen_dense(std::ostream &os,
                                         DenseType& mat,
                                         const write_options& options = {}) {
        matrix_market_header header;
        header.nrows = mat.rows();
        header.ncols = mat.cols();
        header.nnz = mat.rows() * mat.cols();

        header.object = matrix;
        header.field = get_field_type::value<typename DenseType::Scalar>();
        header.format = array;

        write_header(os, header);

        auto formatter = dense_Eigen_formatter(mat);
        write_body(os, formatter, options);
    }
}