# Rounded Floating Point Arithmetic

**Project still under active development, not all math functions implemented**

This package implements a rounded **fl**oating point **int**ervals or `flint` type in python and NumPy. The floating point interval type contains a pair of numbers that define the endpoint of an interval, and the exact value of a computation always lies somewhere in that interval. This type addresses one shortcoming of floating point numbers, namely that of equality comparisons. For this package, the equality operator for flints is implemented such that any overlap of the interval will be treated as equals, and should be though of as 'could be equal'.

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

To use with NumPy, import NumPy as well (first?). Mark the array's dtype as `flint`.
```py
import numpy as np
from flint import flint

a = np.fill((3,), 0.2, dtype=flint)
b = flint(0.6)
# This evaluates to True
print( np.sum(a) == b )
```

As of right now, the package is still in early development and there are several limitations. It is much slower than standard floating point operations, and should only be used where the added guarantee of 'could be equal' is required. In addition, few math function beyond arithmetic are currently supported, namely the exponential function and the natural log function.

# The problem with computers and numbers

Computers are math machines, and in some sense all they do is use simple math to manipulate numbers (we will ignore strings - they greatly complicate things). And At the peril of great generalization there are two ways to represent numbers with computers, integers and floating point numbers. As a developer, the first choice is to use integers if they are an option. However in many cases we need more than just integers, whether is fractions when we need numbers between integers, even (gasp) irrational numbers. In those cases, we switch to floating point numbers. We tend to treat floating point numbers as the most general of number, a real number. And this works most of the time ... until it doesn't. Let's look at why they don't always work, and examine some of the odd consequences.

Floating point numbers are great:
+ allow us to represent a very large range values with great precision
+ special hardware the allows for simple arithmetic quickly

Floating point numbers cannot represent all real numbers. A floating point numbers is similar to a number written in decimal with a decimal point and a finite number of digits, and importantly they have the same limitations. As is well known, all irrational numbers have infinite non-repeating decimal representations, so clearly floating point numbers with their finite representations can irrational numbers. In fact, since most fractions have infinite repeating representations, floating point numbers cannot even represent most fractions exactly. And this inability to represent most number exactly has some interesting consequences.

Floating point numbers are not so great:
- Simple math will often break down (a != b, but a-b == 0)
- Comparisons, such as equals are particularly problematic (0.2 + 0.2 + 0.2 != 0.6)

New type of number: Rounded Floating Point Interval
- based on floating point numbers, so we can still use hardware
- guarantee that comparisons will work (for some definition of work)

## Definition

There is a standard for floating point numbers that guarantees a certain precision of all arithmetic and certain defined math operations. The guarantee is as follows: The operation is completed as if all inputs were exact. The result will often fall between two possible floating point values, so the result should be rounded to one of the two neighboring values[^1]. There are two sources of inaccuracy with this:
1. The operands might not have been able to exactly represent the wanted input value,
2. The output result reported is rounded.

Rounded floating point intervals, which I'm calling `flint`s, are able to address both of these shortcomings. A `flint` object carries with it with an upper and lower bound, and the exact value lies somewhere in that interval. For each arithmetic and supported math operation the inteval will be caculated using standard interval arithmetic, but then  will grow by the minimum needed to guarantee that the result is within the new bound even after rounding. By using `flints` throughout a calculation, you can identify if numbers will be 'possibly equal' by checking if the intervals overlap. In some cases, such as testing for possible intersections/collisions in a CAD program they are a good choice.

[^1] There are four approved methods of rounding, but it doesn't matter which one is chosen, the rounded interval arithmetic will alway be make the same guarantee.


## To Do

- [ ] Write documentation
- [ ] Write test suite
- [ ] Extend to trig functions
- [ ] Publish

## References

* [Patrikalakis et al](https://web.mit.edu/hyperbook/Patrikalakis-Maekawa-Cho/node46.html) contains details of the mathematical implementation of the flint objects
* [Know Maximum Errors in Math Functions](https://www.gnu.org/software/libc/manual/html_node/Errors-in-Math-Functions.html) from the gnu libc manual.

## Development References
* The [Extending and Embedding the Python Interpreter](https://docs.python.org/3/extending/index.html) documentation contain details and examples on how to write c code to interface with python.
* The [Python/C API Reference Manual](https://docs.python.org/3/c-api/index.html) has all the details needed for extending python with c.
* The [NumPy C API](https://numpy.org/doc/stable/reference/c-api/index.html) has all the details need for interfacing numpy using c.
* Mobiles[quaterions](https://github.com/moble/quaternion) Has been my goto source for understanding the process of extending Python and then Numpy with c.
* Martin Ling's [numpy_quaternion](https://github.com/martinling/numpy_quaternion) the previous version of the above quaternion project was also used as an example.
* Mark Wiebe's [numpy_half](https://github.com/mwiebe/numpy_half) which was the first project upon which Martin Ling's projet was based. 

### License

Copyright (c) 2023, Jef Wagner

Numpy-flint is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Numpy-flint is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Numpy-flint. If not, see <https://www.gnu.org/licenses/>.
