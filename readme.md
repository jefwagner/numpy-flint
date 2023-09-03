# Rounded Floating Point Arithmetic

This package implements a rounded **fl**oating point **int**ervals or `flint` data-type
in python and NumPy. The floating point interval type contains a pair of numbers that
define the endpoint of an interval, and the exact value of a computation always lies
somewhere in that interval. This type addresses one shortcoming of floating point
numbers: equality comparisons. For this package, the equality operator for flints is
implemented such that any overlap of the interval will be treated as equal, and should
be though of as 'could be equal'.

To use in python, import the flint package and declare the number as a flint type.
```py
from flint import flint

# Floating point numbers sometimes don't make sense
a = 0.2
b = 0.6
# This evaluate to False
print( (a+a+a) == b )

# Rounded floating point intervals will fix these issues
x = flint(0.2)
y = flint(0.6)
# This evalautes to True
print( (x+x+x) == y )
```

To use with NumPy, import NumPy as well and mark the array's dtype as `flint`.
```py
import numpy as np
from flint import flint

a = np.fill((3,), 0.2, dtype=flint)
b = flint(0.6)
# This evaluates to True
print( np.sum(a) == b )
```

Further useage examples as well as a full API can be found on the [project homepage](https://jefwagner.github.io/flint/)

# Installation

Binary packages are avialable from [PyPI](https://pypi.org). Use `pip` to install the
binary package.
```sh
> python -m pip install numpy-flint
```

If there not a version for your system, you can visit the [project homepage](https://jefwagner.github.io/flint/) 
which has instructions on building from source or contributing to the project.

### License

Copyright (c) 2023, Jef Wagner

Numpy-flint is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation, either
version 3 of the License, or (at your option) any later version.

Numpy-flint is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
Numpy-flint. If not, see <https://www.gnu.org/licenses/>.
