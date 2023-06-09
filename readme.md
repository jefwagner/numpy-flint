# Rounded Floating Point Arithmetic

This package implements a rounded **fl**oating point **int**ervals or `flint` data-type in python and NumPy. The floating point interval type contains a pair of numbers that define the endpoint of an interval, and the exact value of a computation always lies somewhere in that interval. This type addresses one shortcoming of floating point numbers: equality comparisons. For this package, the equality operator for flints is implemented such that any overlap of the interval will be treated as equal, and should be though of as 'could be equal'.

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

Please be cautious. Working with floating point intervals is much slower than standard floating point operations, and should only be used where the added guarantee of 'could be equal' is required. Also note that the project is in an early and mostly untested state. As of right now, `numpy-flint` implements most real functions defined in the C99 `math.h` header file. but lacks some of the functions for interacting with integers such as `round`, `ceil`, `floor`, and `fmod`.

# Installation

Binary packages are avialable from [PyPI](https://pypi.org). Use `pip` to install the binary package.
```sh
> python -m pip install numpy-flint
```
Binary packages have been only been built for CPython 3.7 and later and for some operating systems and architectures including:
* macos x86
* glibc linux i686
* glibc linux x86_64
* glibc linux aarch64
* windows win32
* windows amd64

If your system is not seen here, you will need to build the package from source. Follow these steps to download download and build the project.
1. Clone the repo and switch to the desired development branch
```sh
> git clone https://github.com/jefwagner/numpy-flint.git
> cd numpy-flint
> git checkout <branch-name>
```
2. Create a python environment for building and add `numpy` and `build`. Note: This assumes you are in a bash shell using python's venv package. Replace the venv command with the appropriate conda or other environment command then activate it and install the requirements as appropriate.
```sh
> python -m venv .venv
> source .venv/bin/activate
(.venv) > pip install -r requirements.txt
```
3. Build the project. This requires a working c99 compliant c compiler. If this works (and that's a big IF - this is a new package and hasn't been tested in many architectures), then it will build a wheel file in the dist folder.
```sh
(.venv) > python -m build
```
4. Deactivate the build environment and then perform a 'local' install with pip. Note: same assumption as above, if using other environment manager replace with appropriate deactivation and wheel install commands as appropriate.
```sh
(.venv) > deactivate
> pip install .
```

# Why? What purpose do rounded floating point intervals serve?

Computers are math machines, and in some sense all they do is use simple math to manipulate numbers. At the peril of great generalization there are two ways to represent numbers with computers, integers and floating point numbers. As a developer, the first choice is to use integers if they are an option. Integers are great! Most arithmetic works barring overflow, and most algorithms using integers can be directly implemented exactly with the computer.

However some unfortunate cases we need more than just integers, and that 'more' includes both fractions AND irrational numbers. In those cases, we as developers tend to switch to floating point numbers. We tend to treat floating point numbers as the most general of number: a real number. And this works most of the time ... until it doesn't. Let's look at why they don't always work, and examine some of the odd consequences.

## The problem with floating point numbers

A floating point numbers is similar to a number written in decimal with a decimal point and a finite number of digits. It is well known some numbers have infinite decimal representations, for example the fraction 1/3 with the decimal representation 0.3333... repeating to infinity. But a floating point numpy can only includes a finite number of digits, so we have to truncate the number, and it will no longer exactly represent 1/3. Here-in lies the first issue we can have with floating point numbers:

1. Floating pointer numbers can not represent all numbers exactly.

For many very important numbers such as $\pi$ or $\sqrt{2}$ or even the humble 1/3rd do not have finite decimal representations, and so they can not be represented exactly by floating point numbers. Frequently, we can not exactly capture the input needed for some mathematical calculations. For example, since we can not exactly represent $\pi$ with floating point numbers the result of $\sin( \text{nfp}(\pi))$, where $\text{nfp}(x)$ represents the 'nearest floating point' to $x$, will be close to, but NOT equal zero.

This limited ability to represent numbers leads to another consequence of floating point numbers:

2. Exact math with floating point numbers does not always yield floating point numbers.

A very simple example is just division. The number 1 is exactly representable, as is the number 3. But the result of dividing 1 by 3, the fraction 1/3rd is NOT representable. This result can lead to some results that seem to invalidate math. A classic example with binary floating point numbers is this: 2/10 + 2/10 + 2/10 will not yield the same result as (2+2+2)/10. Go ahead and try it out. Open up python and try
```py
> (0.2 + 0.2 + 0.2) == 0.6
```
you will find that the result yields `False`. This problem is well known, and most experienced programmers have a rule to never check equality with floating point numbers. When an equality comparison is required, a 'close enough' style comparison of the type where the absolute different is required to be less than some predetermined small value epsilon. An example python implementation could be
```py
def almost_eq(a: float, b: float, eps: float = 1.0e-8):
    """Compare two floats to see if they are close to each other"""
    return abs(b-a) < eps
```
This works well enough if we know the approximate size of the numbers we expect to be working with, but will often fail if we are working with large ( > 1 billion) or small ( < 1 billionth) numbers and both of those values are WELL within the bounds of what can be represented by the typical 64 bit floating point number.

## Rounded floating point intervals

Let us introduce a new type of number that addressed the two issues above: the rounded **fl**oating point **int**erval or `flint`. To fully understand how these numbers (or really data-structures) allow us to address some of the issues of floating point numbers, lets break down the name in reverse order. First, notice the 'interval' in the name. Unlike typical numbers, which can be represented by 0-length point on a number-line, a flint will be represented by a small but finite-length interval with an upper and lower bound. The 'exact' value of any number can now be captured as long as it lies between the upper and lower bound.

A very real objection to the new number might be: it's and interval; it's not really a number! This is true, but I as long as the interval is small it CAN be treated as a number and the size of the interval can capture the uncertainty in the exact value. That brings us to the second term in the name 'floating point'. In a flint, the upper and lower bounds of the interval are 64 bit floating point numbers (a c `double` or numpy `float64`). Remember, that a floating point number is number with a decimal point and a finite number of non-zero digits. An important concept relating to those finite number of digits for floating point numbers is the 'unit in last place' or _ulp_. One _ulp_ is the distance between two consecutive floating point number with a difference of 1 in the least significant digit. Now, when we want to represent _any_ number $x$, we can turn that number into it's nearest floating point $\text{nfp}(x)$, and then define the upper and lower bounds as one _ulp_ above and below $\text{nfp}(x)$. For a 64 bit number, the unit in last place is typically 16 orders of magnitude smaller than the number itself, so this new number is still quite precise.

Now you are perhaps satisfied that the interval can represent a number, AND we can make sure that _any_ exact number can be captured in a small interval of only a few _ulp_ wide. Lets try and satisfy the last criterion: can we guarantee that we can calculate a new small interval for all math operations that is guaranteed to hold the _exact_ result from the _all_ numbers contained in the input intervals? Yes, and we do so with the first and final term in the name, by 'rounding' the interval after each math operation. For all continuous functions, an interval in the input will map to an interval in the result, with the endpoints of the interval OR the extrema of the function mapping to the endpoints of the resulting interval. The IEEE-754 standard for floating point numbers requires that the result of all math operations be within 1 _ulp_ of the exact result for exact inputs. This means if we round the lower boundary down by 1 _ulp_ and round the upper boundary up by 1 _ulp_ we can guarantee that the resulting interval will contain the exact result of _all_ possible values in the input interval. This can grow the interval as more and more operations are performed, but this can be a be thought of as capturing the growing uncertainty of the final result from using floating point numbers in the first place.

## References

The following references are use for the description of the floating point interval
* [Patrikalakis et al](https://web.mit.edu/hyperbook/Patrikalakis-Maekawa-Cho/node46.html) contains details of the mathematical implementation of the flint objects
* [Know Maximum Errors in Math Functions](https://www.gnu.org/software/libc/manual/html_node/Errors-in-Math-Functions.html) from the gnu libc manual.

## Development References

The following references are used in implementing the C extension to Python and NumPy.
* The [Extending and Embedding the Python Interpreter](https://docs.python.org/3/extending/index.html) documentation contain details and examples on how to write c code to interface with python.
* The [Python/C API Reference Manual](https://docs.python.org/3/c-api/index.html) has all the details needed for extending python with c.
* The [NumPy C API](https://numpy.org/doc/stable/reference/c-api/index.html) has all the details need for interfacing numpy using c.

In particular, the following examples were especially helpful.
* Mobiles[quaterions](https://github.com/moble/quaternion) Has been my goto source for understanding the process of extending Python and then Numpy with c.
* Martin Ling's [numpy_quaternion](https://github.com/martinling/numpy_quaternion) the previous version of the above quaternion project was also used as an example.
* Mark Wiebe's [numpy_half](https://github.com/mwiebe/numpy_half) which was the first project upon which Martin Ling's projet was based. 

### License

Copyright (c) 2023, Jef Wagner

Numpy-flint is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Numpy-flint is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Numpy-flint. If not, see <https://www.gnu.org/licenses/>.
