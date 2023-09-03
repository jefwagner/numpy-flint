/**
 * Rounded floating point interval arithmetic and simple math functions in C
 */
// Copyright (c) 2023, Jef Wagner <jefwagner@gmail.com>
//
// This file is part of numpy-flint.
//
// Numpy-flint is free software: you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// Numpy-flint is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// numpy-flint. If not, see <https://www.gnu.org/licenses/>.
//
#ifndef __FLINT_H__
#define __FLINT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>
#include <stdio.h>

// Get the max of 4 inputs
static inline double max4( double a, double b, double c, double d) {
    a = a>b?a:b;
    b = c>d?c:d;
    return a>b?a:b;
}

// Get the min of 4 inputs
static inline double min4( double a, double b, double c, double d) {
    a = a<b?a:b;
    b = c<d?c:d;
    return a<b?a:b;
}


/**
 * Rounded floating point interval with tracked value
 * 
 * There are three tracked values a lower and upper bound of the interval as well as a
 * tracked valuse that acts exactly like a 64 bit float (c double) for easy cast back
 * to a float.
 */
typedef struct {
    /**
     * The lower bound
     */
    double a;
    /**
     * The upper bound
     */
    double b;
    /**
     * The tracked value
     */
    double v;
} flint;

/**
 * .. _Constants:
 *
 * Pre-defined constants in flint types
 * ------------------------------------
 *
 * These are the real number mathicatical constants as flint types. They the lower and
 * upper boundaries are two consecutive floating point numbers that bound the actual
 * constant, and the tracked value is the boundary value that is closer to the
 * constant's value.
 *
 * .. todo::
 *
 *     Add in the other constants defined in the GNU math.h header file.
 */

/**
 * .. _flint_2pi:
 */
#define FLINT_2PI ((flint) {6.283185307179586, 6.283185307179587, 6.283185307179586})
/**
 * .. _flint_pi:
 */
#define FLINT_PI ((flint) {3.141592653589793, 3.1415926535897936, 3.141592653589793})
/**
 * .. _flint_pi_2:
 */
#define FLINT_PI_2 ((flint) {1.5707963267948966, 1.5707963267948968, 1.5707963267948966})

// 64 bit floating point values that capture the largest and smallest integers that are
// exactly reprentable.
#define MAX_DOUBLE_INT 9.007199254740991e15
#define MIN_DOUBLE_INT -9.007199254740991e15

/**
 * .. _Casting:
 * 
 * Casting to and from flints
 * --------------------------
 * 
 * The following rules are followed when casting *to* flints:
 * 
 * 1. For integers, if the integer can be exactly represented as a 64-bit float, then
 *    the lower, upper, and tracked values are all set to the tracked values
 * 2. For intergers, if they are larger than can be represented as a 64-bit float, then
 *    the tracked value is set the the closest floating point value, and the lower and
 *    upper values are raised and lowered by 1 ULP.
 * 3. For floating point values, it will be assumed that the intended value is NOT
 *    exactly represetned by the input. The tracked value shall be set to the input
 *    and the upper and lower bounds are moved outware by 1 ULP of the input type.
 * 
 * When casting *from* flints, the tracked value is returns as a 64 bit floating
 * point (c double) which can then be cast using standard C casting rules.
 */

/**
 * .. _int_to_flint:
 */
static inline flint int_to_flint(long long l) {
    double d = (double) l;
    flint f = {d, d, d};
    if (d > MAX_DOUBLE_INT || d < MIN_DOUBLE_INT) {
        f.a = nextafter(d,-INFINITY);
        f.b = nextafter(d,INFINITY);
    }
    return f;
}

/**
 * .. _float_to_flint:
 */
static inline flint float_to_flint(float f) {
    double a = nextafterf(f, -INFINITY);
    double b = nextafterf(f, INFINITY);
    return (flint) {a, b, (double) f};
}

/**
 * .. _double_to_flint:
 */
static inline flint double_to_flint(double f) {
    return (flint) {
        nextafter(f, -INFINITY),
        nextafter(f, INFINITY),
        f
    };
}

/**
 *  .. _flint_to_double:
*/
static inline double flint_to_double(flint f) {
    return f.v;
}


/**
 * .. _special_value_query:
 *
 * Floating point special value queries
 * ------------------------------------
 *
 * There are several special values in the `IEEE-754 floating point representation
 * <https://en.wikipedia.org/wiki/IEEE_754>`_ including +/- infinity and not-a-number
 * (NaN). Since those values might be fed as inputs into various functions that have
 * been chained together, `numpy <https://numpy.org/>`_ requires four functions to check
 * for those values: ``nonzero``, ``isnan``, ``isinf``, and ``isfinite``.
 *
 * For the flint types:
 *
 * ``nonzero`` : the interval does NOT contain zero. ``isnan`` : neither boundary or
 * tracked value are a NaN. ``isinf`` : one of the two boundaries is infinite.
 * ``isfinite`` : neither boundary is infinite.
 */

/**
 * .. _flint_nonzero:
 */
static inline int flint_nonzero(flint f) {
    return f.a > 0.0 || f.b < 0.0;
}

/**
 * .. _flint_isnan:
 */
static inline int flint_isnan(flint f) {
    return isnan(f.a) || isnan(f.b) || isnan(f.v);
}

/**
 * .. _flint_isinf:
 */
static inline int flint_isinf(flint f) {
    return isinf(f.a) || isinf(f.b);
}

/**
 * .. _flint_isinfinite:
 */
static inline int flint_isfinite(flint f) {
    return isfinite(f.a) && isfinite(f.b);
}

/**
 * .. _Comparisons:
 *
 * Comparisons
 * -----------
 *
 * For comparisons we have to worry about equality as well as a sense or order (greater
 * than or less than). Since flints are intervals, with an lower and upper boundary
 * we can use standard `interval order <https://en.wikipedia.org/wiki/Interval_order>`_ 
 * where we define equality as the intervals overlapping, and less that or greater than
 * as an intervals not overlapping at all and lying completely to the left or right
 * along the number line.
 * 
 * .. note::
 * 
 *     Although the flints have a partial order, they do not have a strict total
 *     order. Specifically the equality of values is NOT transitive.
 *    
 *     .. math::
 *     
 *         a = b, \quad\text{and}\quad b = c, \quad\text{but}\quad a \ne c.
 *        
 * The following functions implement all comparisons ``==``, ``!=``, ``>``, ``>=``, 
 * ``<``, and ``<=``. The return value is 1 if it evalutes true, 0 if false.
 */


/**
 * .. _flint_eq:
 */
static inline int flint_eq(flint f1, flint f2) {
    return 
        !flint_isnan(f1) && !flint_isnan(f2) &&
        (f1.a <= f2.b) && (f1.b >= f2.a);
}

/**
 * .. _flint_ne:
 */
static inline int flint_ne(flint f1, flint f2) {
    return
        flint_isnan(f1) || flint_isnan(f2) ||
        (f1.a > f2.b) || (f1.b < f2.a);
}

/**
 * .. _flint_le:
 */
static inline int flint_le(flint f1, flint f2) {
    return
        !flint_isnan(f1) && !flint_isnan(f2) &&
        f1.a <= f2.b;
}

/**
 * .. _flint_lt:
 */
static inline int flint_lt(flint f1, flint f2) {
    return 
        !flint_isnan(f1) && !flint_isnan(f2) &&
        f1.b < f2.a;
}

/**
 * .. _flint_ge:
 */
static inline int flint_ge(flint f1, flint f2) {
    return 
        !flint_isnan(f1) && !flint_isnan(f2) &&
        f1.b >= f2.a;
}

/**
 * .. _flint_gt:
 */
static inline int flint_gt(flint f1, flint f2) {
    return 
        !flint_isnan(f1) && !flint_isnan(f2) &&
        f1.a > f2.b;
}

/**
 * .. _Arithmetic:
 *
 * Arithmetic
 * ----------
 *
 * The IEEE-754 standard for floating point operations is as follows:
 *
 * 1. Assume that all input operand are exact
 * 2. The result should be calculated exactly. If the result can not be exactly
 *    represented by a floating point value it should be rounded to a neighboring
 *    floating point value.
 * 
 * In this work, we assume that the flint contains the exact value somewhere within the 
 * interval, whose boundaries are given by floating point numbers. We can then carry
 * out exact `interval arithmetic <https://en.wikipedia.org/wiki/Interval_arithmetic>`_
 * and then round lower and upper the boundarys of the result down and up to gaurantee 
 * that the result lies somewhere in the new rounded interval.
 */

/**
 * Unary operations
 * ^^^^^^^^^^^^^^^^
 * 
 * There are two unary operators defined:
 * 1. plus ``+``
 * 2. negation ``-``
 */

/**
 * .. _flint_positive:
 */
static inline flint flint_positive(flint f) {
    return f;
}

/**
 * .. _flint_negative:
 */
static inline flint flint_negative(flint f) {
    flint _f = {-f.b, -f.a, -f.v};
    return _f;
}

/**
 * Binary operations
 * ^^^^^^^^^^^^^^^^^
 * 
 * There are 4 binary operations defined and their in-place versions:
 */

/**
 * Addition ``+`` and ``+=``
 * """""""""""""""""""""""""
 */

/**
 * .. _flint_add:
 */
static inline flint flint_add(flint f1, flint f2) {
    flint _f = {
        nextafter(f1.a+f2.a, -INFINITY),
        nextafter(f1.b+f2.b, INFINITY),
        f1.v+f2.v
    };
    return _f;
}

/**
 * .. _flint_inplace_add:
 */
static inline void flint_inplace_add(flint* f1, flint f2) {
    f1->a = nextafter(f1->a + f2.a, -INFINITY);
    f1->b = nextafter(f1->b + f2.b, INFINITY);
    f1->v += f2.v;
    return;
}

/**
 * .. _flint_scalar_add:
 */
static inline flint flint_scalar_add(double s, flint f) {
    return flint_add(f, double_to_flint(s));
}

/**
 * .. _flint_add_scalar:
 */
static inline flint flint_add_scalar(flint f, double s) {
    return flint_add(f, double_to_flint(s));    
}

/**
 * .. _flint_inplace_add_scalar:
 */
static inline void flint_inplace_add_scalar(flint* f, double s) {
    flint_inplace_add(f, double_to_flint(s));
    return;
}

/**
 * Subtraction ``-`` and ``-=``
 * """"""""""""""""""""""""""""
 */

/**
 * .. _flint_subtract:
 */
static inline flint flint_subtract(flint f1, flint f2) {
    flint _f = {
        nextafter(f1.a-f2.b, -INFINITY),
        nextafter(f1.b-f2.a, INFINITY),
        f1.v-f2.v
    };
    return _f;
}

/**
 * .. _flint_inplace_subtract:
 */
static inline void flint_inplace_subtract(flint* f1, flint f2) {
    f1->a = nextafter(f1->a - f2.b, -INFINITY);
    f1->b = nextafter(f1->b - f2.a, INFINITY);
    f1->v -= f2.v;
    return;
}

/**
 * .. _flint_scalar_subtract:
 */
static inline flint flint_scalar_subtract(double s, flint f) {
    return flint_subtract(double_to_flint(s), f);
}

/**
 * .. _flint_subtract_scalar:
 */
static inline flint flint_subtract_scalar(flint f, double s) {
    return flint_subtract(f, double_to_flint(s));
}

/**
 * .. _flint_inplace_subtract_scalar:
 */
static inline void flint_inplace_subtract_scalar(flint* f, double s) {
    flint_inplace_subtract(f, double_to_flint(s));
}

/**
 * Multiplication ``-`` and ``-=``
 * """""""""""""""""""""""""""""""
 */

/**
 * .. _flint_mulitply:
 */
static inline flint flint_multiply(flint f1, flint f2) {
    double a = min4(f1.a*f2.a, f1.a*f2.b, f1.b*f2.a, f1.b*f2.b);
    double b = max4(f1.a*f2.a, f1.a*f2.b, f1.b*f2.a, f1.b*f2.b);
    flint _f = {
        nextafter(a, -INFINITY),
        nextafter(b, INFINITY),
        f1.v*f2.v
    };
    return _f;
}

/**
 * .. _flint_inplace_mulitply:
 */
static inline void flint_inplace_multiply(flint* f1, flint f2) {
    double _a = min4(f1->a*f2.a, f1->a*f2.b, f1->b*f2.a, f1->b*f2.b);
    f1->b = max4(f1->a*f2.a, f1->a*f2.b, f1->b*f2.a, f1->b*f2.b);
    f1->a = _a;
    f1->v *= f2.v;
    return;
}

/**
 * .. _flint_scalar_mulitply:
 */
static inline flint flint_scalar_multiply(double s, flint f) {
    return flint_multiply(double_to_flint(s), f);
}

/**
 * .. _flint_mulitply_scalar:
 */
static inline flint flint_multiply_scalar(flint f, double s) {
    return flint_multiply(f, double_to_flint(s));
}

/**
 * .. _flint_inplace_mulitply_scalar:
 */
static inline void flint_inplace_multiply_scalar(flint* f, double s) {
    flint_inplace_multiply(f, double_to_flint(s));
}

/**
 * Division ``/`` and ``/=``
 * """""""""""""""""""""""""
 */

/**
 * .. _flint_divide:
 */
static inline flint flint_divide(flint f1, flint f2) {
    double a = min4(f1.a/f2.a, f1.a/f2.b, f1.b/f2.a, f1.b/f2.b);
    double b = max4(f1.a/f2.a, f1.a/f2.b, f1.b/f2.a, f1.b/f2.b);
    flint _f = {
        nextafter(a, -INFINITY),
        nextafter(b, INFINITY),
        f1.v/f2.v
    };
    return _f;
}

/**
 * .. _flint_inplace_divide:
 */
static inline void flint_inplace_divide(flint* f1, flint f2) {
    double _a = min4(f1->a/f2.a, f1->a/f2.b, f1->b/f2.a, f1->b/f2.b);
    f1->b = max4(f1->a/f2.a, f1->a/f2.b, f1->b/f2.a, f1->b/f2.b);
    f1->a = _a;
    f1->v /= f2.v;
    return;
}

/**
 * .. _flint_scalar_divide:
 */
static inline flint flint_scalar_divide(double s, flint f) {
    return flint_divide(double_to_flint(s), f);
}

/**
 * .. _flint_divide_scalar:
 */
static inline flint flint_divide_scalar(flint f, double s) {
    return flint_divide(f, double_to_flint(s));
}

/**
 * .. _flint_inplace_divide_scalar:
 */
static inline void flint_inplace_divide_scalar(flint* f, double s) {
    flint_inplace_divide(f, double_to_flint(s));
}

/**
 * .. _MathFunctions:
 *
 * Math functions
 * --------------
 *
 * In addition to arithmetic, we need to make sure that we can apply most of the
 * `elementary` functions to the flints and preserve our guarantee that the result
 * interval included the exact value of the function applied to any value in the input
 * interval. There are few parts to consider for this: how accurate is the result for
 * the elementary function on floating point values? And how do we convert a function
 * for a floating point number into a function for a flint object?
 *
 * To address the first issue: how accurate are the elementary function on floating
 * point numbers, we have to turn to the documentation for the math libraries. The only
 * one I was able to find was the `Errors in Math Functions
 * <https://www.gnu.org/software/libc/manual/html_node/Errors-in-Math-Functions.html>`_
 * section of the gnu libc manual. The answer turned ot to be more complicated that I
 * anticipated, depending on the hardware architecture as well as the individual
 * functions. But for a quick look through seemed to indicate that for most functions
 * for the arm 64bit and x86 32 and 64 bit, the double precision versions of most math
 * functions had either 1 or 2 ULP of accuracy.
 *
 * Now lets discuss a naive implementation of the math functions for a flint object. If
 * the function is monotonic,
 *
 * .. math::
 *
 *     a > b \to f(a) > f(b),
 *
 * then the endpoints of the boundaries of the input interval will be become the
 * boundaries of the output interval. So for a function `double func(double x)` we can
 * write the flint version as
 *
 * .. code:: c
 *
 *     flint flint_func(flint x) {
 *         flint res;
 *         res.a = nextafter(nextafter(func(x.a), -INFINITY), -INFINITY);
 *         res.b = nextafter(nextafter(func(x.b), INFINITY) INFINITY);
 *         res.v = func(x.v);
 *         return res;
 *     }
 *
 * This works for all monotonically increasing functions whose domain is the full real
 * number line, and will work with trivial changes for monotonically decreasing
 * functions. However, what happens when the input interval contains an extremum of the
 * function? What happens if the domain is NOT the full number line. We will need some
 * special consideration for each case.
 *
 * The first case is what happens when the function is not monotoic. In most cases,
 * there will be a single extremum, such as the minium in the `hypot` function as either
 * value goes through zero. If the interval contains that extremum, then the upper or
 * lower boundary of the output flint need to be reset to use the extremum as the
 * boundary AND that particular boundary value does not need to be expanded. Finally
 * some care need to be taken in the case of sine and cosine, that oscillat with
 * multiple extrememum.
 *
 * The second case is what happens when the function's domain does not cover the entire
 * number line. The standard response is for a C function to return NaN for an input
 * ouside of it's accepted domain. However for flints, we can be a little more robust.
 * If the interval spans the edge of the domain, then the function would return NaN for
 * one boundary value and a non NaN number for the other. In that case we will assume
 * that the value's we're interested in are only the 'real' (non NaN) values and replace
 * the NaN's with the corresponding values at the edge of the domain. To give an
 * explicity example, consider the C ``sqrt`` function. lets say we have a flint whose
 * lower value is slightly below zero (1.5e-16) and upper value is above zero (1.0e-8). 
 * The endpoints would evaluate to NaN and 1.0e-4, but we can replace the lower endpoint
 * with 0.0 instead of NaN. The tracked value is either kept as the direct result or
 * replaced with the value at the edge of the domain if it would be NaN.
 * 
 * .. note::
 * 
 *     It is because of the special behavior depending on the interval spaning extremums
 *     or domain boundaries that make the evaluation of these functions for flints
 *     significantly slower compared to using floating point value directly
 * 
 * The following functions defined in the c99 ``math.h`` header file have flint
 * implementation with the following signature:
 * 
 * .. c:function:: static inline flint flint_FUNCNAME(flint fa, ...)
 * 
 * Functions
 * ^^^^^^^^^
 * 
 * ``power``
 * ``absolute``
 * ``sqrt``
 * ``cbrt``
 * ``hypot``
 * ``exp``
 * ``exp2``
 * ``expm1``
 * ``log``
 * ``log10``
 * ``log2``
 * ``log1p``
 * ``erf``
 * ``erfc``
 * ``sin``
 * ``cos``
 * ``tan``
 * ``asin``
 * ``acos``
 * ``atan``
 * ``atan2``
 * ``sinh``
 * ``cosh``
 * ``tanh``
 * ``asinh``
 * ``acosh``
 * ``atanh``
 */


/**
 * .. _foo:
 */
#define FLINT_MONOTONIC(fname) \
static inline flint flint_##fname(flint f) { \
    flint _f = { \
        nextafter(nextafter(fname(f.a), -INFINITY), -INFINITY), \
        nextafter(nextafter(fname(f.b), INFINITY), INFINITY), \
        fname(f.v) \
    }; \
    return _f; \
}

static inline flint flint_power(flint f1, flint f2) {
    double aa = pow(f1.a, f2.a);
    double ab = pow(f1.a, f2.b);
    double ba = pow(f1.b, f2.a);
    double bb = pow(f1.b, f2.b);
    double v = pow(f1.v, f2.v);
    flint ret = {0.0, 0.0, 0.0};
    if (isnan(aa) || isnan(ab) || isnan(ba) || isnan(bb) || isnan(v)) {
        v = NAN;
        ret.a = v; ret.b = v; ret.v = v;
    } else {
        ret.a = nextafter(nextafter(min4(aa,ab,ba,bb),-INFINITY),-INFINITY);
        ret.b = nextafter(nextafter(max4(aa,ab,ba,bb),INFINITY),INFINITY);
        ret.v = v;
    }
    return ret;
}

static inline void flint_inplace_power(flint* f1, flint f2) {
    double aa = pow(f1->a, f2.a);
    double ab = pow(f1->a, f2.b);
    double ba = pow(f1->b, f2.a);
    double bb = pow(f1->b, f2.b);
    double v = pow(f1->v, f2.v);
    if (isnan(aa) || isnan(ab) || isnan(ba) || isnan(bb) || isnan(v)) {
        v = NAN;
        f1->a = v; f1->b = v; f1->v = v;
    } else {
        f1->a = nextafter(nextafter(min4(aa,ab,ba,bb),-INFINITY),-INFINITY);
        f1->b = nextafter(nextafter(max4(aa,ab,ba,bb),INFINITY),INFINITY);
        f1->v = v;
    }
}

static inline flint flint_absolute(flint f) {
    flint _f = f;
    if (f.b < 0.0) { // interval is all negative - so invert
        _f.a = -f.b;
        _f.b = -f.a;
        _f.v = -f.v;
    } else if (f.a < 0) { // interval spans 0
        _f.a = 0.0; // 0 is the new lower bound
        _f.b = ((-f.a > f.b)? -f.a : f.b); // upper bound is the greater
        _f.v = ((f.v > 0.0)? f.v : -f.v); // value is absolute valued
    }
    return _f;
}

static inline flint flint_sqrt(flint f) {
    flint _f;
    if (f.b < 0.0) {
        double nan = NAN;
        _f.a = nan; _f.b = nan; _f.v = nan;
    } else if (f.a < 0) {
        _f.a = 0.0;
        _f.b = nextafter(sqrt(f.b), INFINITY);
        _f.v = (f.v > 0.0) ? sqrt(f.v) : 0.0;
    } else {
        _f.a = nextafter(sqrt(f.a), -INFINITY);
        _f.b = nextafter(sqrt(f.b), INFINITY);
        _f.v = sqrt(f.v);
    }
    return _f;
}

FLINT_MONOTONIC(cbrt)

static inline flint flint_hypot(flint f1, flint f2) {
    double f1a, f1b, f2a, f2b;
    double a, b, v;
    // Set f1a and f1b to arguments that give min and max outputs wrt f1
    if (f1.a<0) {
        if (f1.b<0) {
            f1a = f1.b;
            f1b = f1.a;
        } else {
            f1a = 0;
            f1b = (-f1.a>f1.b)?(-f1.a):f1.b;
        }
    } else {
        f1a = f1.a;
        f1b = f1.b;
    }
    // Set f2a and f2b to arguments that give min and max outputs wrt f2
    if (f2.a<0) {
        if (f2.b<0) {
            f2a = f2.b;
            f2b = f2.a;
        } else {
            f2a = 0;
            f2b = -f2.a>f2.b?-f2.a:f2.b;
        }
    } else {
        f2a = f2.a;
        f2b = f2.b;
    }
    a = hypot(f1a, f2a);
    // don't shift down if it's already zero
    a = (a==0)?0:nextafter(nextafter(a,-INFINITY),-INFINITY);
    b = nextafter(nextafter(hypot(f1b, f2b), INFINITY), INFINITY);
    v = hypot(f1.v, f2.v);
    flint _f = {a, b, v};
    return _f;
}

FLINT_MONOTONIC(exp)

FLINT_MONOTONIC(exp2)

FLINT_MONOTONIC(expm1)

#define FLINT_LOGFUNC(log, min) \
static inline flint flint_##log(flint f) { \
    flint _f; \
    if (f.b < min) { \
        double nan = NAN; \
        _f.a = nan; _f.b = nan; _f.v = nan; \
    } else if (f.a < min) { \
        _f.a = -INFINITY; \
        _f.b = nextafter(log(f.b), INFINITY); \
        _f.v = (f.v > min) ? log(f.v) : -INFINITY; \
    } else { \
        _f.a = nextafter(log(f.a), -INFINITY); \
        _f.b = nextafter(log(f.b), INFINITY); \
        _f.v = log(f.v); \
    } \
    return _f; \
}

FLINT_LOGFUNC(log, 0.0)

FLINT_LOGFUNC(log10, 0.0)

FLINT_LOGFUNC(log2, 0.0)

FLINT_LOGFUNC(log1p, -1.0)

FLINT_MONOTONIC(erf)

static inline flint flint_erfc(flint f) {
    flint _f = {
        nextafter(nextafter(erfc(f.b), -INFINITY), -INFINITY),
        nextafter(nextafter(erfc(f.a), INFINITY), INFINITY),
        erfc(f.v)
    };
    return _f;
}


static inline flint flint_sin(flint f) {
    int n = (int) floor(f.a/FLINT_2PI.a);
    double da = f.a-n*FLINT_2PI.a;
    double db = f.b-n*FLINT_2PI.a;
    double sa = sin(f.a);
    double sb = sin(f.b);
    flint _f;
    _f.a = nextafter(nextafter((sa<sb?sa:sb), -INFINITY), -INFINITY);
    _f.b = nextafter(nextafter((sa>sb?sa:sb), INFINITY), INFINITY);
    if (da <= FLINT_PI_2.a && db > FLINT_PI_2.a) {
        _f.b = 1.0;
    } else if (da <= 3*FLINT_PI_2.a) {
        if (db > 3*FLINT_PI_2.a) {
            _f.a = -1.0;
        }
        if (db > 5*FLINT_PI_2.a) {
            _f.b = 1.0;
        }
    } else {
        if (db > 5*FLINT_PI_2.a) {
            _f.b = 1.0;
        }
        if (db > 7*FLINT_PI_2.a) {
            _f.a = -1.0;
        }
    }
    _f.v = sin(f.v);
    return _f;
}

static inline flint flint_cos(flint f) {
    int n = (int) floor(f.a/FLINT_2PI.a);
    double da = f.a-n*FLINT_2PI.a;
    double db = f.b-n*FLINT_2PI.a;
    double ca = cos(f.a);
    double cb = cos(f.b);
    flint _f;
    _f.a = nextafter(nextafter((ca<cb?ca:cb), -INFINITY), -INFINITY);
    _f.b = nextafter(nextafter((ca>cb?ca:cb), INFINITY), INFINITY);
    if (da <= FLINT_PI.a && db > FLINT_PI.a) {
        _f.a = -1.0;
        if (db > FLINT_2PI.a) {
            _f.b = 1.0;
        }
    } else {
        if (db > FLINT_2PI.a) {
            _f.b = 1.0;
        }
        if (db > 3*FLINT_PI.a) {
            _f.a = -1.0;
        }
    }
    _f.v = cos(f.v);
    return _f;
}

static inline flint flint_tan(flint f) {
    double ta = tan(f.a);
    double tb = tan(f.b);
    flint _f;
    if (ta > tb || (f.b-f.a) > FLINT_PI.a) {
        _f.a = -INFINITY;
        _f.b = INFINITY;
    } else {
        _f.a = nextafter(nextafter(ta, -INFINITY), -INFINITY);
        _f.b = nextafter(nextafter(tb, INFINITY), INFINITY);
    }
    _f.v = tan(f.v);
    return _f;
}

static inline flint flint_asin(flint f) {
    flint _f;
    if (f.b < -1.0 || f.a > 1.0) {
        double nan = NAN;
        _f.a = nan; _f.b = nan; _f.v = nan;
    } else {
        if (f.a < -1.0) {
            _f.a = -FLINT_PI_2.b;
        } else {
            _f.a = nextafter(nextafter(asin(f.a), -INFINITY), -INFINITY);
        }
        if (f.b > 1.0) {
            _f.b = FLINT_PI_2.b;
        } else {
            _f.b = nextafter(nextafter(asin(f.b), INFINITY), INFINITY);
        }
        if (f.v < -1.0) {
            _f.v = -FLINT_PI_2.v;
        } else if (f.v > 1.0) {
            _f.v = FLINT_PI_2.v;
        } else {
            _f.v = asin(f.v);
        }
    }
    return _f;
}

static inline flint flint_acos(flint f) {
    flint _f;
    if (f.b < -1.0 || f.a > 1.0) {
        double nan = NAN;
        _f.a = nan; _f.b = nan; _f.v = nan;
    } else {
        if (f.a < -1.0) {
            _f.b = FLINT_PI.b;
        } else {
            _f.b = nextafter(nextafter(acos(f.a), INFINITY), INFINITY);
        }
        if (f.b > 1.0) {
            _f.a = 0.0;
        } else {
            _f.a = nextafter(nextafter(acos(f.b), -INFINITY), -INFINITY);
        }
        if (f.v < -1.0) {
            _f.v = FLINT_PI.v;
        } else if (f.v > 1.0) {
            _f.v = 0;
        } else {
            _f.v = acos(f.v);
        }
    }
    return _f;
}

FLINT_MONOTONIC(atan)

static inline flint flint_atan2(flint fy, flint fx) {
    flint _f;
    if (fy.a > 0) {
        // monotonic dec in fx
        if (fx.a > 0 ) {
            // monotonic inc in fy
            _f.a = atan2(fy.a, fx.b);
            _f.b = atan2(fy.b, fx.a);
        } else if (fx.b > 0) {
            // along positive y axis
            _f.a = atan2(fy.a, fx.b);
            _f.b = atan2(fy.a, fx.a);
        } else {
            // monotonic dec in fy
            _f.a = atan2(fy.b, fx.b);
            _f.b = atan2(fy.a, fx.a);
        }
    } else if (fy.b > 0) {
        // along x axis
        if (fx.a > 0 ) {
            // along positive x axis
            _f.a = atan2(fy.a, fx.a);
            _f.b = atan2(fy.b, fx.a);
        } else if (fx.b > 0) {
            // has the branch point
            _f.a = -FLINT_PI.a;
            _f.b = FLINT_PI.a;
        } else {
            // has the branch line
            _f.a = atan2(fy.b, fx.b); // always between pi/2 and pi
            _f.b = atan2(fy.a, fx.b); // always between -pi and -pi/2
            if (fy.v > 0) {
                // on positive branch
                _f.b += FLINT_2PI.a; // move to positive branch
            } else {
                // on negative branch
                _f.a -= FLINT_2PI.a; // move to negative branch
            }
        }
    } else {
        // monotonic inc in fx
        if (fx.a > 0) {
            // monotonic inc in fy
            _f.a = atan2(fy.a, fx.a);
            _f.b = atan2(fy.b, fx.b);
        } else if (fx.b > 0) {
            // along negative y axis
            _f.a = atan2(fy.b, fx.a);
            _f.b = atan2(fy.b, fx.b);
        } else {
            // monotonic dec in fy
            _f.a = atan2(fy.b, fx.a);
            _f.b = atan2(fy.a, fx.b);
        }
    }
    _f.a = nextafter(nextafter(_f.a, -INFINITY), -INFINITY);
    _f.b = nextafter(nextafter(_f.b, INFINITY), INFINITY);
    _f.v = atan2(fy.v, fx.v);
    return _f;
}

FLINT_MONOTONIC(sinh)

static inline flint flint_cosh(flint f) {
    double a = cosh(f.a);
    double b = cosh(f.b);
    flint _f;
    if (f.a > 0.0 || f.b < 0) {
        _f.a = nextafter(nextafter(a<b?a:b, -INFINITY), -INFINITY);
    } else { // interval spans 0
        _f.a = 1.0; // 1 is the new lower bound
    }
    _f.b = nextafter(nextafter(a>b?a:b, INFINITY), INFINITY);
    _f.v = cosh(f.v);
    return _f;
}

FLINT_MONOTONIC(tanh)

FLINT_MONOTONIC(asinh)

static inline flint flint_acosh(flint f) {
    flint _f;
    if (f.b < 1.0) {
        double nan = NAN;
        _f.a = nan; _f.b = nan; _f.v = nan;
    } else if (f.a < 1.0) {
        _f.a = 0.0;
        _f.b = nextafter(nextafter(acosh(f.b), INFINITY), INFINITY);
        _f.v = (f.v > 1.0) ? acosh(f.v) : 0.0;
    } else {
        _f.a = nextafter(nextafter(acosh(f.a), -INFINITY), -INFINITY);
        _f.b = nextafter(nextafter(acosh(f.b), INFINITY), INFINITY);
        _f.v = acosh(f.v);
    }
    return _f;
}

static inline flint flint_atanh(flint f) {
    flint _f;
    if (f.b < -1.0 || f.a > 1.0) {
        double nan = NAN;
        _f.a = nan; _f.b = nan; _f.v = nan;
    } else {
        if (f.a < -1.0) {
            _f.a = -INFINITY;
        } else {
            _f.a = nextafter(nextafter(atanh(f.a), -INFINITY), -INFINITY);
        }
        if (f.b > 1.0) {
            _f.b = INFINITY;
        } else {
            _f.b = nextafter(nextafter(atanh(f.b), INFINITY), INFINITY);
        }
        if (f.v < -1.0) {
            _f.v = -INFINITY;
        } else if (f.v > 1.0) {
            _f.v = INFINITY;
        } else {
            _f.v = atanh(f.v);
        }
    }
    return _f;
} 


#ifdef __cplusplus
}
#endif

#endif // __FLINT_H__
