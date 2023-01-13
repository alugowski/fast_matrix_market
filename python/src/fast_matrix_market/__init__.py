# Copyright (C) 2022-2023 Adam Lugowski. All rights reserved.
# Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

from ._core import __doc__, __version__, header, _read_header_file, _read_header_string, _write_header_file, _write_header_string

__all__ = ["__doc__", "__version__", "header"]


def read_header(fname=None, s=None) -> header:
    """
    Read a Matrix Market header from a file or from a string.

    :param fname: if not None, read from this file
    :param s: parse from a string
    :return: parsed header object
    """
    if fname:
        return _read_header_file(str(fname))
    else:
        return _read_header_string(s)


def write_header(h: header, fname=None):
    """
    Write a Matrix Market header to a file or a string.

    :param h: header to write
    :param fname: if not None, write to this file. If None then return a string.
    :return: if fname is None then a string containing h
    """
    if fname:
        _write_header_file(h, str(fname))
    else:
        return _write_header_string(h)
