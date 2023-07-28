/// @file flint.h Functions for rounded floating point mathematics
///
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

/// @brief Get the max of 4 inputs
static inline double max4( double a, double b, double c, double d) {
    a = a>b?a:b;
    b = c>d?c:d;
    return a>b?a:b;
}

/// @brief Get the min of 4 inputs
static inline double min4( double a, double b, double c, double d) {
    a = a<b?a:b;
    b = c<d?c:d;
    return a<b?a:b;
}


//
// Rounded Floating Point Interval stuct
//

/// @brief Rounded floating point interval with tracked value
/// There are three tracked values, a lower and upper bound of the interval as
/// well as a tracked value that acts exactly like a 64 bit float for easy cast
/// back to float.
/// @param a the lower bound
/// @param b the upper bound
/// @param v the tracked value
typedef struct {
    double a;
    double b;
    double v;
} flint;

#define FLINT_2PI ((flint) {6.283185307179586, 6.283185307179587, 6.283185307179586})
#define FLINT_PI ((flint) {3.141592653589793, 3.1415926535897936, 3.141592653589793})
#define FLINT_PI_2 ((flint) {1.5707963267948966, 1.5707963267948968, 1.5707963267948966})
//
// Conversions
//
#define MAX_DOUBLE_INT 9.007199254740991e15
#define MIN_DOUBLE_INT -9.007199254740991e15
/// @brief Cast from a integer to a flint
/// Cast as an exact value if possible, otherwise expand the interval.
/// @param l integer
/// @return floating point interval representation of an integer
static inline flint int_to_flint(long long l) {
    double d = (double) l;
    flint f = {d, d, d};
    if (d > MAX_DOUBLE_INT || d < MIN_DOUBLE_INT) {
        f.a = nextafter(d,-INFINITY);
        f.b = nextafter(d,INFINITY);
    }
    return f;
}
/// @return Cast from a 64 bit floating point to a flint
/// Assume that the required values is not exactly represented by the float, 
/// create smallest interval surrounding the input value.
/// @param f floating point value
/// @return floating point interval representation of the float
static inline flint double_to_flint(double f) {
    return (flint) {
        nextafter(f, -INFINITY),
        nextafter(f, INFINITY),
        f
    };
}
/// @return Cast from a 32 bit floating point to a flint
/// Assume that the required values is not exactly represented by the float, 
/// create smallest interval surrounding the input value.
/// @param f floating point value
/// @return floating point interval representation of the float
static inline flint float_to_flint(float f) {
    double a = nextafterf(f, -INFINITY);
    double b = nextafterf(f, INFINITY);
    return (flint) {a, b, (double) f};
}

//
// Floating point special value queries
//
/// @return Query of the flint overlaps zero
/// @param f flint to check
/// @return true if the interval overlaps zero, false otherwise
static inline int flint_nonzero(flint f) {
    return f.a > 0.0 || f.b < 0.0;
}
/// @return Query if the flint has a IEEE-754 NaN value
/// @param f flint to check
/// @return true if any value in f is NaN, false otherwise
static inline int flint_isnan(flint f) {
    return isnan(f.a) || isnan(f.b) || isnan(f.v);
}
/// @return Query if the flint has a IEEE-754 infinite value
/// @param f flint to check
/// @return true if either interval boundary is infinite, false otherwise
static inline int flint_isinf(flint f) {
    return isinf(f.a) || isinf(f.b);
}
/// @return Query if the flint has IEEE-754 finite values
/// @param f flint to check
/// @return true if all values in interval are finite, false otherwise
static inline int flint_isfinite(flint f) {
    return isfinite(f.a) && isfinite(f.b);
}

//
// Comparisons
//
/// @brief Compare two intervals for equality
/// @param f1 first flint
/// @param f2 second flint
/// @return true if the two intervals overlap, false otherwise
static inline int flint_eq(flint f1, flint f2) {
    return 
        !flint_isnan(f1) && !flint_isnan(f2) &&
        (f1.a <= f2.b) && (f1.b >= f2.a);
}
/// @brief Compare two intervals for non-equality
/// @param f1 first flint
/// @param f2 second flint
/// @return true if the two intervals do not overlap, false otherwise
static inline int flint_ne(flint f1, flint f2) {
    return
        flint_isnan(f1) || flint_isnan(f2) ||
        (f1.a > f2.b) || (f1.b < f2.a);
}
/// @brief Compare if first interval is less than or equal to second
/// @param f1 first flint
/// @param f2 second flint
/// @return true if any values in first interval are less or equal to any value in the second
static inline int flint_le(flint f1, flint f2) {
    return
        !flint_isnan(f1) && !flint_isnan(f2) &&
        f1.a <= f2.b;
}
/// @brief Compare if first interval is less than second
/// @param f1 first flint
/// @param f2 second flint
/// @return true if all values in first interval are less than all in the second
static inline int flint_lt(flint f1, flint f2) {
    return 
        !flint_isnan(f1) && !flint_isnan(f2) &&
        f1.b < f2.a;
}
/// @brief Compare if first interval is greater than or equal to second
/// @param f1 first flint
/// @param f2 second flint
/// @return true if any values in first interval are greater or equal to any value in the second
static inline int flint_ge(flint f1, flint f2) {
    return 
        !flint_isnan(f1) && !flint_isnan(f2) &&
        f1.b >= f2.a;
}
/// @brief Compare if first interval is greater than second
/// @param f1 first flint
/// @param f2 second flint
/// @return true if all values in first interval are greater than all in the second
static inline int flint_gt(flint f1, flint f2) {
    return 
        !flint_isnan(f1) && !flint_isnan(f2) &&
        f1.a > f2.b;
}

//
// Arithmatic
//
/// @brief Postive: The unary `+` operator for flint
/// This is just the identity operator
/// @param f flint
/// @return The input flint returned unchanged
static inline flint flint_positive(flint f) {
    return f;
}
/// @brief Negation: The unary `-` operator
/// swap upper and lower interval boundaries but don't grow the interval
/// @param f flint
/// @return The interval reflected around the origin
static inline flint flint_negative(flint f) {
    flint _f = {-f.b, -f.a, -f.v};
    return _f;
}
/// @brief Addition: The binary '+' operator
/// add the boundaries and grow the interval by one ulp
/// @param f1 first flint
/// @param f2 second flint
/// @return The sum of the two intervals
static inline flint flint_add(flint f1, flint f2) {
    flint _f = {
        nextafter(f1.a+f2.a, -INFINITY),
        nextafter(f1.b+f2.b, INFINITY),
        f1.v+f2.v
    };
    return _f;
}
/// @brief Inplace Addition: The '+=' operator
/// add the boundaries and grow the interval by one ulp
/// @param f1 pointer to first flint
/// @param f2 second flint
static inline void flint_inplace_add(flint* f1, flint f2) {
    f1->a = nextafter(f1->a + f2.a, -INFINITY);
    f1->b = nextafter(f1->b + f2.b, INFINITY);
    f1->v += f2.v;
    return;
}
/// @brief Addition: The binary '+' operator
/// Turn the scalar into a flint, then add the flints. Uses the standard
/// coercion rules to promote smaller types into doubles.
/// @param s scalar
/// @param f flint
/// @return The sum of scalar as flint and flint
static inline flint flint_scalar_add(double s, flint f) {
    return flint_add(f, double_to_flint(s));
}
/// @brief Addition: The binary '+' operator
/// Turn the scalar into a flint, then add the flints. Uses the standard
/// coercion rules to promote smaller types into doubles.
/// @param f flint
/// @param s scalar
/// @return The sum of scalar as flint and flint
static inline flint flint_add_scalar(flint f, double s) {
    return flint_add(f, double_to_flint(s));    
}
/// @brief Inplace Addition: The '+=' operator
/// Turn the scalar into a flint, then add to the existing flint. Uses the 
/// standard coercion rules to promote smaller types into doubles.
/// @param f pointer to the flint
/// @param s scalar
/// @return The sum of scalar as flint and flint
static inline void flint_inplace_add_scalar(flint* f, double s) {
    flint_inplace_add(f, double_to_flint(s));
    return;
}
/// @brief Subtraction: The binary '-' operator
/// swap second flints upper and lower boundaries, then subtract and grow the 
/// interval by one ulp
/// @param f1 first flint
/// @param f2 second flint
/// @return The difference of the two intervals
static inline flint flint_subtract(flint f1, flint f2) {
    flint _f = {
        nextafter(f1.a-f2.b, -INFINITY),
        nextafter(f1.b-f2.a, INFINITY),
        f1.v-f2.v
    };
    return _f;
}
/// @brief Inplace Subtraction: The '-=' operator
/// swap second flints upper and lower boundaries, then subtract and grow the 
/// interval by one ulp
/// @param f1 pointer to first flint
/// @param f2 second flint
static inline void flint_inplace_subtract(flint* f1, flint f2) {
    f1->a = nextafter(f1->a - f2.b, -INFINITY);
    f1->b = nextafter(f1->b - f2.a, INFINITY);
    f1->v -= f2.v;
    return;
}
/// @brief Subtraction: The binary '-' operator
/// Turn the scalar into a flint, then subtract the flints. Uses the standard
/// coercion rules to promote smaller types into doubles.
/// @param s scalar
/// @param f flint
/// @return The difference of scalar as flint and flint
static inline flint flint_scalar_subtract(double s, flint f) {
    return flint_subtract(double_to_flint(s), f);
}
/// @brief Subtraction: The binary '-' operator
/// Turn the scalar into a flint, then subtract the flints. Uses the standard
/// coercion rules to promote smaller types into doubles.
/// @param f flint
/// @param s scalar
/// @return The difference of flint and scalar as flint
static inline flint flint_subtract_scalar(flint f, double s) {
    return flint_subtract(f, double_to_flint(s));
}
/// @brief Inplace Subtraction: The '-=' operator
/// Turn the scalar into a flint, then subtract from the existing flint. Uses 
/// the standard coercion rules to promote smaller types into doubles.
/// @param f pointer to the flint
/// @param s scalar
/// @return The difference of flint and scalar as flint
static inline void flint_inplace_subtract_scalar(flint* f, double s) {
    flint_inplace_subtract(f, double_to_flint(s));
}
/// @brief Multiplication: The binary '*' operator
/// Try product of all boundaries, take min and max, then grow by one ulp
/// @param f1 first flint
/// @param f2 second flint
/// @return The product of the two intervals
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
/// @brief Inplace Multiplication: The binary '*=' operator
/// Try product of all boundaries, take min and max, then grow by one ulp
/// @param f1 pointer to the first flint
/// @param f2 second flint
static inline void flint_inplace_multiply(flint* f1, flint f2) {
    double _a = min4(f1->a*f2.a, f1->a*f2.b, f1->b*f2.a, f1->b*f2.b);
    f1->b = max4(f1->a*f2.a, f1->a*f2.b, f1->b*f2.a, f1->b*f2.b);
    f1->a = _a;
    f1->v *= f2.v;
    return;
};
/// @brief Multiplication: The binary '*' operator
/// Turn the scalar into a flint, then multiply the flints. Uses the standard
/// coercion rules to promote smaller types into doubles.
/// @param s scalar
/// @param f flint
/// @return The product of flint and scalar as flint
static inline flint flint_scalar_multiply(double s, flint f) {
    return flint_multiply(double_to_flint(s), f);
}
/// @brief Multiplication: The binary '*' operator
/// Turn the scalar into a flint, then multiply the flints. Uses the standard
/// coercion rules to promote smaller types into doubles.
/// @param f flint
/// @param s scalar
/// @return The product of flint and scalar as flint
static inline flint flint_multiply_scalar(flint f, double s) {
    return flint_multiply(f, double_to_flint(s));
}
/// @brief Inplace Multiplication: The binary '*=' operator
/// Turn the scalar into a flint, then multiply the flints. Uses the standard
/// coercion rules to promote smaller types into doubles.
/// @param f1 pointer to the first flint
/// @param f2 second flint
static inline void flint_inplace_multiply_scalar(flint* f, double s) {
    flint_inplace_multiply(f, double_to_flint(s));
}
/// @brief Division: The binary '/' operator
/// Try quotient of all boundaries, take min and max, then grow by one ulp
/// @param f1 first flint
/// @param f2 second flint
/// @return The quotient of the two intervals
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
/// @brief Inplace Division: The binary '/=' operator
/// Try quotient of all boundaries, take min and max, then grow by one ulp
/// @param f1 pointer to first flint
/// @param f2 second flint
static inline void flint_inplace_divide(flint* f1, flint f2) {
    double _a = min4(f1->a/f2.a, f1->a/f2.b, f1->b/f2.a, f1->b/f2.b);
    f1->b = max4(f1->a/f2.a, f1->a/f2.b, f1->b/f2.a, f1->b/f2.b);
    f1->a = _a;
    f1->v /= f2.v;
    return;
};
/// @brief Division: The binary '/' operator
/// Turn the scalar into a flint, then divide the flints. Uses the standard
/// coercion rules to promote smaller types into doubles.
/// @param s scalar
/// @param f flint
/// @return The quotient of scalar as flint and flint
static inline flint flint_scalar_divide(double s, flint f) {
    return flint_divide(double_to_flint(s), f);
}
/// @brief Division: The binary '/' operator
/// Turn the scalar into a flint, then divide the flints. Uses the standard
/// coercion rules to promote smaller types into doubles.
/// @param f flint
/// @param s scalar
/// @return The quotient of flint and scalar as flint
static inline flint flint_divide_scalar(flint f, double s) {
    return flint_divide(f, double_to_flint(s));
}
/// @brief Inplace Division: The binary '/=' operator
/// Turn the scalar into a flint, then divide the flints. Uses the standard
/// coercion rules to promote smaller types into doubles.
/// @param f1 pointer to the first flint
/// @param f2 second flint
static inline void flint_inplace_divide_scalar(flint* f, double s) {
    flint_inplace_divide(f, double_to_flint(s));
}

//
// Math functions
//
// For math functions on floating point intervals, we would like to guarantee
// that the resulting interval contains the exact values for all values in the
// input interval(s). The GNU C compiler guarantees that the result of all of
// the math functions defined in the 'math.h' header file will give results
// that are accurate to within a few units-of-last-place or ulp's of the exact
// results. In order to keep our guarantee, we will grow the mapped interval
// by two ulp's before returning the value.
//
/// @brief Power: The binary pow function for flints / The power function
//applies the pow function to all the boundaries and takes / the min and max as
//the boundaries of the results. In addition, it checks / if any NaN's result
//from the operations, and if so returns NaN. / @param f1 the first flint /
//@param f2 the second flint / @return The results of f1 raised to the f2 power
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
/// @brief Inplace Power: The binary pow function for flints
/// The power function applies the pow function to all the boundaries and takes
/// the min and max as the boundaries of the results. In addition, it checks
/// if any NaN's result from the operations, and if so returns NaN.
/// @param f1 a pointer to the first flint
/// @param f2 the second flint
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
/// @brief Absolute Value: the absolute value function for flints
/// The absolute value acts like either identity or negation, so does not grow
/// the interval. The special case is when the interval spans the origin. The
/// absolute value will then return 0 as the lower boundary, and the max of
/// the absolute values of the upper and lower boundaries.
/// @param f flint
/// @return The absolute value of the interval
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
// Square root, only gives NaN if whole interval is less than zero
/// @brief Square Root: the square root function for flints
/// The square root function is a monotonically increasing function, but is only 
/// defined for non-negative values. If the interval overlaps zero, with some
/// negative values, we will assume that only the non-negative values were 
/// correct and will return zero as the lower bound.
/// @param f flint
/// @return the square root of the interval
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
/// @brief Macro to define a monotonic increasing function on a flint
/// For a monotonic increasing function, the lower and upper boundaries of the
/// input will map directly to the upper and lower boundaries of the output.
#define FLINT_MONOTONIC(fname) \
static inline flint flint_##fname(flint f) { \
    flint _f = { \
        nextafter(nextafter(fname(f.a), -INFINITY), -INFINITY), \
        nextafter(nextafter(fname(f.b), INFINITY), INFINITY), \
        fname(f.v) \
    }; \
    return _f; \
}
/// @brief Cube Root: The cube root function for flints
/// The cube root function is a monotonic function with full range
/// @param f
/// @return The cube root of the interval
FLINT_MONOTONIC(cbrt)
// Hypoteneus has a single minima in both f1 and f2
/// @brief Hypotenuse: The binary hypotenuse function
/// The hypotenuse function is increasing for both positive and negative values
/// of both inputs with minimums as zero. If the interval overlaps zero, the 
/// lower bound it taken as the other input and the upper bound is the max of
/// the output of the other input. If both intervals overlap zero, the interval
/// is not expanded down by two ULP.
/// @param f1 the first flint
/// @param f2 the second flint
/// @return the hypotenuse, or sqrt(f1*f1 + f2*f2), for both intervals
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
/// @brief Exponential Function: the exponential function
/// The exponential function is a monotonic increasing function
/// @param f the flint
/// @return The exponential or e^(f) for the interval
FLINT_MONOTONIC(exp)
/// @brief Exponential Base Two: the exponential function base two
/// The exponential function is a monotonic increasing function
/// @param f the flint
/// @return The exponential base 2 or 2^(f) for the interval
FLINT_MONOTONIC(exp2)
/// @brief Exponential minus one: the exponential function minus one
/// The exponential function is a monotonic increasing function
/// @param f the flint
/// @return The exponential minus one or e^(f)-1 for the interval
FLINT_MONOTONIC(expm1)
/// @brief A macro used for all logarithmic style functions on flints All
/// logarithmic functions are monotonic increasing, but have a lower limit in
/// the domain. At the lower limit the funciton goes towards negative infinity,
/// and below the limit the function return NaN. If the interval spans the lower
/// limit then it is assume that only the values that have real (not NaN) values
/// are correct and a lower limit of negative infinity will be returned.
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
/// @brief Natural Log: the natural log function
/// @param f flint
/// @return The natural log of the interval
FLINT_LOGFUNC(log, 0.0)
/// @brief Log base ten: the log function base ten
/// @param f flint
/// @return The log base ten of the interval
FLINT_LOGFUNC(log10, 0.0)
/// @brief Log base two: the log function base two
/// @param f flint
/// @return The log base two of the interval
FLINT_LOGFUNC(log2, 0.0)
/// @brief Natural Log of f minus one: ln(f-1)
/// @param f flint
/// @return ln(f-1) for the interval f
FLINT_LOGFUNC(log1p, -1.0)
/// @brief Error Function: integral from -infinity to f of a gaussian
/// The error function a monotonic increasing function
/// @param f flint
/// @return The error function evaluated on the interval f
FLINT_MONOTONIC(erf)
/// @brief Complementary Error Function: integral from f to inf of a gaussian
/// The complementary error function is a monotonic decreasing function
/// @param f flint
/// @return The complementary error function evaluated on the interval f
static inline flint flint_erfc(flint f) {
    flint _f = {
        nextafter(nextafter(erfc(f.b), -INFINITY), -INFINITY),
        nextafter(nextafter(erfc(f.a), INFINITY), INFINITY),
        erfc(f.v)
    };
    return _f;
}
/// @brief Sine: The sine function of an interval
/// The sine is periodic with many minima and maxima that make applying the
/// result to intervals difficult. In this case a `da` and `db` are calculated
/// that give the difference between the values and greatest multiple of two pi
/// less than the lower bound. Using these values, it is exhaustivly checked if
/// the interval is spans any of the minima or maxima and replaces the upper or
/// lower bounds appropriately.
/// @param f flint
/// @return The sin of the interval f
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
/// @brief Cosine: The cosine function of an interval
/// The cosine is periodic with many minima and maxima that make applying the
/// result to intervals difficult. In this case a `da` and `db` are calculated
/// that give the difference between the values and greatest multiple of two pi
/// less than the lower bound. Using these values, it is exhaustivly checked if
/// the interval is spans any of the minima or maxima and replaces the upper or
/// lower bounds appropriately.
/// @param f flint
/// @return The cos of the interval f
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
/// @brief Tangent: The tangent function
/// The tangent function includes many monotonic increasing sections that go to
/// negative infinity on the lower boundary and positive infinity on the upper
/// boundary. If the interval spans the difference between two boundaries we 
/// replace the upper and lower boundaries with +/- infinity respectively
/// @param f flint
/// @return The tan(f) for the interval f
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
/// @brief Inverse Sine: the inverse sine or arcsine function
/// The inverse sine is only defined for input values between -1 and 1. If the
/// the interval spans either of these values then the upper or lower limit is
/// replace with the appropriate arcsin(+/-1).
/// @param f
/// @return the asin(f) for the interval f
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
/// @brief Inverse Cosine: the inverse cosine or arccosine function
/// The inverse cosine is only defined for input values between -1 and 1. If the
/// the interval spans either of these values then the upper or lower limit is
/// replace with the appropriate arccos(+/-1).
/// @param f
/// @return the asin(f) for the interval f
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
/// @brief Inverse Tangent: the inverse tangent or arctan function
/// The inverse tangent is a monotonically increasing function.
/// @param f
/// @return the atan(f) for the interval f
FLINT_MONOTONIC(atan)
/// @brief Two Input Inverse Tangent: the two input arctan function
/// The two input inverse tangent gives the angle between the positive x axis
/// and the line from the origin through the point (fx, fy). This function is
/// many valued, and has principle values between negative pi and pi. When the
/// intervals for both fx and fy span 0, then the result contains all values
/// otherwise the interval will only ever be given on a single branch, and the
/// upper or lower boundary of the interval can fall outside of the range from
/// negative pi to pi. 
/// @param fy The y coordinate interval
/// @param fx The x coordinate interval
/// @return The arctan2(fy, fx) for the intervals fx and fy
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
/// @brief Hyperbolic Sine: the hyperbolic sine functions
/// The hyperbolic sine function is a monotonic increasing function
/// @param f flint
/// @return The hyperbolic sin of the interval f
FLINT_MONOTONIC(sinh)
/// @brief Hyperbolic Cosine: the hyperbolic cosine functions
/// The hyperbolic cos function has minima at zero, and is increasing for both
/// positive and negative values. When the interval spans zero, the lower bound
/// is set to exactly 1, otherwise the proper boundary of the interval is used.
/// @param f flint
/// @return The hyperbolic cos of the interval f
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
/// @brief Hyperbolic Tangent: the hyperbolic tangent functions
/// The hyperbolic tan function is a monotonic increasing function
/// @param f flint
/// @return The hyperbolic tan of the interval f
FLINT_MONOTONIC(tanh)
/// @brief Inverse Hyperbolic Sine: the inverse hyperbolic sine functions
/// The inverse hyperbolic sine function is a monotonic increasing function
/// @param f flint
/// @return The hyperbolic sin of the interval f
FLINT_MONOTONIC(asinh)
/// @brief Inverse Hyperbolic Cosine: the inverse hyperbolic cosine functions
/// The inverse hyperbolic cosine function is a monotonic increasing function,
/// but is only defined for values greater than one. If the interval overlaps
/// one, with some values less than, we will assume that only the values greater
/// than or equal to one were correct and will return zero as the lower bound.
/// If the entire interval lies less than one, then NaN is returned.
/// @param f flint 
/// @return The inverse hyperbolic cos of the interval f
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
/// @brief Inverse Hyperbolic Tangent: the inverse hyperbolic tangent function
/// The inverse hyperbolic tangent is only defined for input values between -1
/// and 1. If the the interval spans either of these values then the upper or
/// lower limit is replace with the appropriate arctan(+/-1). If the entire
/// interval lies less than negative one or greater than positive one, then NaN
/// is returned.
/// @param f flint 
/// @return The inverse hyperbolic cos of the interval f
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
