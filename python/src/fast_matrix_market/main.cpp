// Copyright (C) 2022-2023 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#include <pybind11/pybind11.h>

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
    dict["nrows"] = header.nrows;
    dict["ncols"] = header.ncols;

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


int add(int i, int j) {
    return i + j;
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

    m.def("_read_header_file", &read_header_file, R"pbdoc(
        Read Matrix Market header from a file.
    )pbdoc");
    m.def("_read_header_string", &read_header_string, R"pbdoc(
        Read Matrix Market header from a string.
    )pbdoc");
    m.def("_write_header_file", &write_header_file, R"pbdoc(
        Write Matrix Market header to a file.
    )pbdoc");
    m.def("_write_header_string", &write_header_string, R"pbdoc(
        Write Matrix Market header to a string.
    )pbdoc");


#ifdef VERSION_INFO
#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif
}
