## @file flint/__init__.py 
"""
A python/NumPy interface for rounded FLoating point INTervals or flints.
"""
# Copyright (c) 2023, Jef Wagner <jefwagner@gmail.com>
#
# This file is part of numpy-flint.
#
# numpy-flint is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# numpy-flint is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# numpy-flint. If not, see <https://www.gnu.org/licenses/>.

from .numpy_flint import flint

__version__ = "0.3.0"

def get_include() -> str:
    """Return the directory with the 'flint.h' header file"""
    import os
    return os.path.dirname(__file__)
