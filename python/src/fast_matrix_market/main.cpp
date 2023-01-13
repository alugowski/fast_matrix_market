// Copyright (C) 2022-2023 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

#include <fstream>
#include <fast_matrix_market/fast_matrix_market.hpp>

namespace py = pybind11;
using namespace pybind11::literals;
namespace fmm = fast_matrix_market;

fmm::matrix_market_header read_header_file(const std::string& filename) {
    std::ifstream f(filename);
    fmm::matrix_market_header header;
    fast_matrix_market::read_header(f, header);
    return header;
}

fmm::matrix_market_header read_header_string(const std::string& str) {
    std::istringstream f(str);
    fmm::matrix_market_header header;
    fast_matrix_market::read_header(f, header);
    return header;
}

void write_header_file(const fmm::matrix_market_header& header, const std::string& filename) {
    std::ofstream f(filename);
    fast_matrix_market::write_header(f, header);
}

std::string write_header_string(const fmm::matrix_market_header& header) {
    std::ostringstream f;
    fast_matrix_market::write_header(f, header);
    return f.str();
}

std::tuple<int64_t, int64_t> get_header_shape(const fmm::matrix_market_header& header) {
    return std::make_tuple(header.nrows, header.ncols);
}

void set_header_shape(fmm::matrix_market_header& header, const std::tuple<int64_t, int64_t>& shape) {
    header.nrows = std::get<0>(shape);
    header.ncols = std::get<1>(shape);
}

std::string get_header_object(const fmm::matrix_market_header& header) {
    return fmm::object_map.at(header.object);
}
std::string get_header_format(const fmm::matrix_market_header& header) {
    return fmm::format_map.at(header.format);
}
std::string get_header_field(const fmm::matrix_market_header& header) {
    return fmm::field_map.at(header.field);
}
std::string get_header_symmetry(const fmm::matrix_market_header& header) {
    return fmm::symmetry_map.at(header.symmetry);
}

void set_header_object(fmm::matrix_market_header& header, const std::string& value) {
    try {
        header.object = fmm::parse_enum<fmm::object_type>(value, fmm::object_map);
    } catch (fmm::invalid_argument e) { throw std::invalid_argument(e.what()); }
}
void set_header_format(fmm::matrix_market_header& header, const std::string& value) {
    try {
        header.format = fmm::parse_enum<fmm::format_type>(value, fmm::format_map);
    } catch (fmm::invalid_argument e) { throw std::invalid_argument(e.what()); }
}
void set_header_field(fmm::matrix_market_header& header, const std::string& value) {
    try {
        header.field = fmm::parse_enum<fmm::field_type>(value, fmm::field_map);
    } catch (fmm::invalid_argument e) { throw std::invalid_argument(e.what()); }
}
void set_header_symmetry(fmm::matrix_market_header& header, const std::string& value) {
    try {
        header.symmetry = fmm::parse_enum<fmm::symmetry_type>(value, fmm::symmetry_map);
    } catch (fmm::invalid_argument e) { throw std::invalid_argument(e.what()); }
}

fmm::matrix_market_header create_header(const std::tuple<int64_t, int64_t>& shape, int64_t nnz,
                                        const std::string& comment,
                                        const std::string& object, const std::string& format,
                                        const std::string& field, const std::string& symmetry) {
    fmm::matrix_market_header header{};
    set_header_shape(header, shape);
    header.nnz = nnz;
    header.comment = comment;
    set_header_object(header, object);
    set_header_format(header, format);
    set_header_field(header, field);
    set_header_symmetry(header, symmetry);
    return header;
}

py::dict header_to_dict(fmm::matrix_market_header& header) {
    py::dict dict;
    dict["shape"] = py::make_tuple(header.nrows, header.ncols);
    dict["nnz"] = header.nnz;
    dict["comment"] = header.comment;
    dict["object"] = get_header_object(header);
    dict["format"] = get_header_format(header);
    dict["field"] = get_header_field(header);
    dict["symmetry"] = get_header_symmetry(header);

    return dict;
}

std::string header_repr(const fmm::matrix_market_header& header) {
    std::ostringstream oss;
    oss << "header(";
    oss << "shape=(" << header.nrows << ", " << header.ncols << "), ";
    oss << "nnz=" << header.nnz << ", ";
    oss << "comment=\"" << header.comment << "\", ";
    oss << "object=\"" << get_header_object(header) << "\", ";
    oss << "format=\"" << get_header_format(header) << "\", ";
    oss << "field=\"" << get_header_field(header) << "\", ";
    oss << "symmetry=\"" << get_header_symmetry(header) << "\"";
    oss << ")";
    return oss.str();
}


struct read_cursor {
    read_cursor(const std::string& filename): stream(std::make_unique<std::ifstream>(filename)) {}
    read_cursor(const std::string& str, bool string_source): stream(std::make_unique<std::istringstream>(str)) {}

    std::unique_ptr<std::istream> stream;

    fmm::matrix_market_header header{};
    fmm::read_options options{};
};

void open_read_rest(read_cursor& cursor) {
    // This is done later in Python to match SciPy behavior
    cursor.options.generalize_symmetry = false;

    // read header
    fmm::read_header(*cursor.stream, cursor.header);
}

read_cursor open_read_file(const std::string& filename, int num_threads) {
    read_cursor cursor(filename);
    // Set options
    cursor.options.num_threads = num_threads;

    open_read_rest(cursor);
    return cursor;
}

read_cursor open_read_string(const std::string& str, int num_threads) {
    read_cursor cursor(str, true);
    // Set options
    cursor.options.num_threads = num_threads;

    open_read_rest(cursor);
    return cursor;
}

/**
 * Read Matrix Market body into a numpy array.
 *
 * @param cursor Opened by open_read().
 * @param array NumPy array. Assumed to be the correct size and zeroed out.
 */
template <typename T>
void read_body_array(read_cursor& cursor, py::array_t<T>& array) {
    auto unchecked = array.mutable_unchecked();
    auto handler = fmm::dense_2d_call_adding_parse_handler<decltype(unchecked), int64_t, T>(unchecked);
    fmm::read_matrix_market_body(*cursor.stream, cursor.header, handler, 1, cursor.options);
}


/**
 * Triplet handler. Separate row, column, value iterators.
 */
template<typename IT, typename VT, typename IT_ARR, typename VT_ARR>
class triplet_numpy_parse_handler {
public:
    using coordinate_type = IT;
    using value_type = VT;
    static constexpr int flags = fmm::kParallelOk;

    explicit triplet_numpy_parse_handler(IT_ARR& rows,
                                         IT_ARR& cols,
                                         VT_ARR& values,
                                         int64_t offset = 0) : rows(rows), cols(cols), values(values), offset(offset) {}

    void handle(const coordinate_type row, const coordinate_type col, const value_type value) {
        rows(offset) = row;
        cols(offset) = col;
        values(offset) = value;

        ++offset;
    }

    triplet_numpy_parse_handler<IT, VT, IT_ARR, VT_ARR> get_chunk_handler(int64_t offset_from_begin) {
        return triplet_numpy_parse_handler(rows, cols, values, offset_from_begin);
    }

protected:
    IT_ARR& rows;
    IT_ARR& cols;
    VT_ARR& values;

    int64_t offset;
};


template <typename IT, typename VT>
void read_body_triplet(read_cursor& cursor, py::array_t<IT>& row, py::array_t<IT>& col, py::array_t<VT>& data) {
    if (row.size() != cursor.header.nnz || col.size() != cursor.header.nnz || data.size() != cursor.header.nnz) {
        throw std::invalid_argument("NumPy Array sizes need to equal matrix nnz");
    }
    auto row_unchecked = row.mutable_unchecked();
    auto col_unchecked = col.mutable_unchecked();
    auto data_unchecked = data.mutable_unchecked();
    auto handler = triplet_numpy_parse_handler<IT, VT, decltype(row_unchecked), decltype(data_unchecked)>(row_unchecked, col_unchecked, data_unchecked);
    fmm::read_matrix_market_body(*cursor.stream, cursor.header, handler, 1, cursor.options);
}



PYBIND11_MODULE(_core, m) {
    m.doc() = R"pbdoc(
        fast_matrix_market
        -----------------------
    )pbdoc";

    py::register_exception<fmm::invalid_mm>(m, "InvalidMatrixMarket");

    py::class_<fmm::matrix_market_header>(m, "header")
    .def(py::init<>())
    .def(py::init<int64_t, int64_t>())
    .def(py::init([](std::tuple<int64_t, int64_t> shape) { return fmm::matrix_market_header{std::get<0>(shape), std::get<1>(shape)}; }))
    .def(py::init(&create_header), py::arg("shape")=std::make_tuple(0, 0), "nnz"_a=0, "comment"_a=std::string(), "object"_a="matrix", "format"_a="coordinate", "field"_a="real", "symmetry"_a="general")
    .def_readwrite("nrows", &fmm::matrix_market_header::nrows)
    .def_readwrite("ncols", &fmm::matrix_market_header::ncols)
    .def_property("shape", &get_header_shape, &set_header_shape)
    .def_readwrite("nnz", &fmm::matrix_market_header::nnz)
    .def_readwrite("comment", &fmm::matrix_market_header::comment)
    .def_property("object", &get_header_object, &set_header_object)
    .def_property("format", &get_header_format, &set_header_format)
    .def_property("field", &get_header_field, &set_header_field)
    .def_property("symmetry", &get_header_symmetry, &set_header_symmetry)
    .def("to_dict", &header_to_dict, R"pbdoc(
        Return the values in the header as a dict.
    )pbdoc")
    .def("__repr__", [](const fmm::matrix_market_header& header) { return header_repr(header); });

    m.def("read_header_file", &read_header_file, R"pbdoc(
        Read Matrix Market header from a file.
    )pbdoc");
    m.def("read_header_string", &read_header_string, R"pbdoc(
        Read Matrix Market header from a string.
    )pbdoc");
    m.def("write_header_file", &write_header_file, R"pbdoc(
        Write Matrix Market header to a file.
    )pbdoc");
    m.def("write_header_string", &write_header_string, R"pbdoc(
        Write Matrix Market header to a string.
    )pbdoc");

    py::class_<read_cursor>(m, "_read_cursor")
    .def_readonly("header", &read_cursor::header);

    m.def("open_read_file", &open_read_file, py::arg("path"), py::arg("num_threads")=0);
    m.def("open_read_string", &open_read_string, py::arg("str"), py::arg("num_threads")=0);

    m.def("read_body_array", &read_body_array<int64_t>);
    m.def("read_body_array", &read_body_array<double>);
    m.def("read_body_array", &read_body_array<std::complex<double>>);

    m.def("read_body_triplet", &read_body_triplet<int32_t, double>);
    m.def("read_body_triplet", &read_body_triplet<int32_t, int64_t>);
    m.def("read_body_triplet", &read_body_triplet<int32_t, std::complex<double>>);

    m.def("read_body_triplet", &read_body_triplet<int64_t, double>);
    m.def("read_body_triplet", &read_body_triplet<int64_t, int64_t>);
    m.def("read_body_triplet", &read_body_triplet<int64_t, std::complex<double>>);

#ifdef VERSION_INFO
#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif
}
