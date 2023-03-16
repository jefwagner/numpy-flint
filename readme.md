# WARNING
**Project still under active development - does not work**

# Rounded Floating Point Math

Floating point numbers are great:
+ allow us to represent a very large range values with great precision
+ special hardware the allows for simple arithmetic quickly

Floating point numbers are not so great:
- Simple math will often break down (a != b, but a-b == 0)
- Comparisons, such as equals are particularly problematic (0.2 + 0.2 + 0.2 != 0.6)

New type of number: Rounded Floating Point Interval
- based on floating point numbers, so we can still use hardware
- guarantee that comparisons will work (for some definition of work)

## Definition

There is a standard for floating point numbers that guarantees a certain precision of all arithmetic and some math operations. The guarantee is as follows: The operation is completed as if all inputs were exact. The result will often fall between two possible floating point values, so the result should be rounded to one of the two neighboring values[^1]. There are two sources of inaccuracy with this:
1. The operands might not have been able to exactly represent the wanted value,
2. The result reported is rounded.

Rounded floating point intervals, which I'm calling `flint`s, are able to address both of these shortcomings. A `flint` object carries with it with an upper and lower bound, and the exact value lies somewhere in that interval. For each arithmetic and supported math operation the inteval will be caculated using standard interval arithmetic, but then  will grow by the minimum needed to guarantee that the result is within the new bound even after rounding. By using `flints` throughout a calculation, you can identify if numbers will be 'possibly equal' by checking if the intervals overlap. In some cases, such as testing for possible intersections/collisions in a CAD program they are a good choice.

[^1] There are four approved methods of rounding, but it doesn't matter which one is chosen, the rounded interval arithmetic will alway be make the same guarantee.

## References

* [Patrikalakis et al](https://web.mit.edu/hyperbook/Patrikalakis-Maekawa-Cho/node46.html) contains details of the mathematical implementation of the flint objecs
* Chapters [2]() and [3]() of the _Extending and Embedding the Python Interpreter_ documentation contain details and examples on how to write c code to interface with python.
* Section [14.5] of the _Python Extension Patterns_ contains some details on how to interface with numpy.
* Martin Ling's [numpy_quaternion](https://github.com/martinling/numpy_quaternion) project was used as an example
* Mark Wiebe's [numpy_half](https://github.com/mwiebe/numpy_half) project was used as an example

### License

Numpy-flint is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation, either
version 3 of the License, or (at your option) any later version.

Numpy-flint is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Numpy-flint. If not, see <https://www.gnu.org/licenses/>.