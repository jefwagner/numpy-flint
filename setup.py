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

from setuptools import Extension, setup
import numpy as np

# 0.0.X -> Not all functionality has been implemented
# 0.X.Y -> 'Most' functionality has been implemented and is ready for testing
# X.Y.Z -> A stable and tested version is ready for release
version = '0.0.1'

extra_compiler_args = ['-fmax_errors=5']

extensions = [
    Extension(
        name='flint.numpy_flint',
        sources=['numpy_flint.c'],
        depends=[
            'flint.h',
            'numpy_flint.c',
        ],
        include_dirs=[np.get_include()],
    )
]

setup_metadata = dict(
    name='numpy-flint', # Name of the package on pypi
    packages=['flint'], # Actual package name as used in python
    # package_dir = {'':'.'},
    url='https://github.com/jefwagner/numpy-flint',
    author="Jef Wagner",
    author_email="jefwagner@gmail.com",
    description="Add a rounded floating point interval (flint) dtype to NumPy",
    long_description=__doc__,
    ext_modules=extensions,
    install_requires=[],
    extras_require={},
    version=version,
)

def build(setup_kwargs):
    setup_kwargs.update({"ext_modules": extensions})

if __name__=='__main__':
    setup(**setup_metadata)
