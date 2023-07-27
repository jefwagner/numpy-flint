#!/usr/bin/env python
# -*- coding: utf-8 -*-
## @file setup.py Python/Numpy interface for flints
"""\
This package creates a rounded floating point interval (flint) type in python
and extends the type to numpy allowing for the creation and manipulation of
arrays of flints. All the arithmetic operations as well as a few elementary
mathematical functions are enabled for them. The core code is written in c...
because 'why not?'
"""
# Copyright (c) 2023, Jef Wagner <jefwagner@gmail.com>
# This file is part of numpy-flint.
#
# Numpy-flint is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# Numpy-flint is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# numpy-flint. If not, see <https://www.gnu.org/licenses/>.

from setuptools import setup, Extension
import numpy as np

setup_args = dict(
    ext_modules = [
        Extension(
            name='flint.numpy_flint',
            sources=['src/flint/numpy_flint.c'],
            depends=[
                'src/flint/flint.h',
                'src/flint/numpy_flint.h',
                'src/flint/numpy_flint.c',
            ],
            include_dirs=[np.get_include()],
        )
    ]
)

setup(**setup_args)
