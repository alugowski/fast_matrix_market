// Copyright (C) 2022 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#pragma once

#include "../fast_matrix_market.hpp"

#if defined(__clang__)
// Disable some pedantic warnings from Eigen headers.
#pragma clang diagnostic ignored "-Wunused-but-set-variable"
#endif

#include <Eigen/Sparse>



namespace fast_matrix_market {
    /**
     * Read Matrix Market file into an Eigen matrix and a header struct.
     */
    template <typename SparseType>
    void read_matrix_market_eigen(std::istream &instream,
                                  matrix_market_header &header,
                                  SparseType& mat,
                                  const read_options& options = {},
                                  typename SparseType::Scalar default_pattern_value = 1) {

        typedef typename SparseType::Scalar Scalar;
        typedef typename SparseType::StorageIndex StorageIndex;
        typedef Eigen::Triplet<Scalar, StorageIndex> Triplet;

        read_header(instream, header);
        mat.resize(header.nrows, header.ncols);

        // read into tuples
        std::vector<Triplet> elements;
        elements.resize(get_storage_nnz(header, options));

        auto handler = tuple_parse_handler<StorageIndex, Scalar, decltype(elements.begin())>(elements.begin());
        read_matrix_market_body(instream, header, handler, default_pattern_value, options);

        // set the values into the matrix
        mat.setFromTriplets(elements.begin(), elements.end());
    }

    /**
     * Read Matrix Market file into an Eigen matrix.
     */
    template <typename SparseType>
    void read_matrix_market_eigen(std::istream &instream,
                                  SparseType& mat,
                                  const read_options& options = {},
                                  typename SparseType::Scalar default_pattern_value = 1) {
        matrix_market_header header;
        read_matrix_market_eigen<SparseType>(instream, header, mat, options, default_pattern_value);
    }

    /**
     * Read Matrix Market file into an Eigen Dense matrix.
     */
    template <typename DenseType>
    void read_matrix_market_eigen_dense(std::istream &instream,
                                        matrix_market_header& header,
                                        DenseType& mat,
                                        const read_options& options = {},
                                        typename DenseType::Scalar default_pattern_value = 1) {
        read_header(instream, header);
        mat.setZero(header.nrows, header.ncols);

        auto handler = dense_2d_call_adding_parse_handler<DenseType, typename DenseType::Index, typename DenseType::Scalar>(mat);
        read_matrix_market_body(instream, header, handler, default_pattern_value, options);
    }

    /**
     * Read Matrix Market file into an Eigen Dense matrix.
     */
    template <typename DenseType>
    void read_matrix_market_eigen_dense(std::istream &instream,
                                        DenseType& mat,
                                        const read_options& options = {},
                                        typename DenseType::Scalar default_pattern_value = 1) {
        matrix_market_header header;
        read_matrix_market_eigen_dense<DenseType>(instream, header, mat, options, default_pattern_value);
    }

    /**
    * Format Eigen Sparse Matrix.
    */
    template<typename SparseMatrixType>
    class sparse_Eigen_formatter {
    public:
        typedef typename SparseMatrixType::Index MatIndex;
        explicit sparse_Eigen_formatter(const SparseMatrixType& mat, bool pattern_only) : mat(mat),
                                                                                          pattern_only(pattern_only) {
            nnz_per_column = ((double)mat.nonZeros()) / mat.outerSize();
        }

        [[nodiscard]] bool has_next() const {
            return outer_iter < mat.outerSize();
        }

        class chunk {
        public:
            explicit chunk(const SparseMatrixType& mat, MatIndex outer_iter, MatIndex outer_end, bool pattern_only) :
            mat(mat), outer_iter(outer_iter), outer_end(outer_end), pattern_only(pattern_only) {}

            std::string operator()() {
                std::string chunk;
                chunk.reserve((outer_end - outer_iter)*250);

                // iterate over assigned columns
                for (; outer_iter != outer_end; ++outer_iter) {

                    for (typename SparseMatrixType::InnerIterator it(mat, outer_iter); it; ++it)
                    {
                        chunk += int_to_string(it.row() + 1);
                        chunk += kSpace;
                        chunk += int_to_string(it.col() + 1);
                        if (!pattern_only) {
                            chunk += kSpace;
                            chunk += value_to_string(it.value());
                        }
                        chunk += kNewline;
                    }
                }

                return chunk;
            }

            const SparseMatrixType& mat;
            MatIndex outer_iter, outer_end;
            bool pattern_only;
        };

        chunk next_chunk(const write_options& options) {
            auto num_columns = (MatIndex)(nnz_per_column * (double)options.chunk_size_values + 1);
            num_columns = std::min(num_columns, mat.outerSize() - outer_iter);

            MatIndex outer_end = outer_iter + num_columns;
            chunk c(mat, outer_iter, outer_end, pattern_only);
            outer_iter = outer_end;

            return c;
        }

    protected:
        const SparseMatrixType& mat;
        double nnz_per_column;
        MatIndex outer_iter = 0;
        bool pattern_only;
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

            std::string operator()() {
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
            auto num_columns = (MatIndex)(mat.rows() * (double)options.chunk_size_values + 1);
            num_columns = std::min(num_columns, mat.cols() - col_iter);

            MatIndex col_end = col_iter + num_columns;
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
                                   const write_options& options = {}, matrix_market_header header = {}) {
        header.nrows = mat.rows();
        header.ncols = mat.cols();
        header.nnz = mat.nonZeros();

        header.object = matrix;
        if (header.field != pattern) {
            header.field = get_field_type((const typename SparseType::Scalar*)nullptr);
        }
        header.format = coordinate;

        write_header(os, header);

        auto formatter = sparse_Eigen_formatter(mat, header.field == pattern);
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
        header.field = get_field_type((const typename DenseType::Scalar*)nullptr);
        header.format = array;

        write_header(os, header);

        auto formatter = dense_Eigen_formatter(mat);
        write_body(os, formatter, options);
    }
}