# Copyright (C) 2022-2023 Adam Lugowski. All rights reserved.
# Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.
from io import BytesIO, StringIO
import unittest
from pathlib import Path

import fast_matrix_market as fmm

matrices = Path("matrices")


class TestHeader(unittest.TestCase):
    def test_object(self):
        for value in ["matrix", "vector", "Matrix", "Vector"]:
            h = fmm.header(object=value)
            self.assertEqual(h.object, value.lower(), value)
            h.object = value
            self.assertEqual(h.object, value.lower(), value)

        with self.assertRaises(ValueError):
            fmm.header(object="foo")

    def test_format(self):
        for value in ["coordinate", "array", "Coordinate"]:
            h = fmm.header(format=value)
            self.assertEqual(h.format, value.lower(), value)
            h.format = value
            self.assertEqual(h.format, value.lower(), value)

        with self.assertRaises(ValueError):
            fmm.header(format="foo")

    def test_field(self):
        for value in ["real", "double", "integer", "pattern", "complex"]:
            h = fmm.header(field=value)
            self.assertEqual(h.field, value.lower(), value)
            h.field = value
            self.assertEqual(h.field, value.lower(), value)

        with self.assertRaises(ValueError):
            fmm.header(field="foo")

    def test_symmetry(self):
        for value in ["general", "symmetric", "skew-symmetric", "hermitian"]:
            h = fmm.header(symmetry=value)
            self.assertEqual(h.symmetry, value.lower(), value)
            h.symmetry = value
            self.assertEqual(h.symmetry, value.lower(), value)

        with self.assertRaises(ValueError):
            fmm.header(symmetry="foo")

    def test_read_file(self):
        path = matrices / "eye3.mtx"
        if not path.exists():
            self.skipTest("eye3.mtx is missing. Only happens when testing with cibuildwheel for some reason.")

        h = fmm.read_header(path)
        expected = fmm.header(shape=(3, 3), nnz=3, comment="3-by-3 identity matrix",
                              object="matrix", format="coordinate", field="real", symmetry="general")
        self.assertEqual(h.to_dict(), expected.to_dict())

    def test_read_write(self):
        h = fmm.header(shape=(3, 3), nnz=3, comment="3-by-3 identity matrix",
                       object="matrix", format="coordinate", field="real", symmetry="general")

        # Write to a buffer
        bio = BytesIO()
        fmm.write_header(bio, h)
        s = bio.getvalue().decode()

        h2 = fmm.read_header(StringIO(s))
        self.assertEqual(h.to_dict(), h2.to_dict())


if __name__ == '__main__':
    unittest.main()
