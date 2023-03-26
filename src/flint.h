/// @file flint.h Functions for rounded floating point mathematics
///
// Copyright (c) 2023, Jef Wagner <jefwagner@gmail.com>
//
/// This file is part of numpy-flint.
///
/// Numpy-flint is free software: you can redistribute it and/or modify it under the
/// terms of the GNU General Public License as published by the Free Software
/// Foundation, either version 3 of the License, or (at your option) any later version.
///
/// Numpy-flint is distributed in the hope that it will be useful, but WITHOUT ANY
/// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
/// PARTICULAR PURPOSE. See the GNU General Public License for more details.
///
/// You should have received a copy of the GNU General Public License along with Foobar.
/// If not, see <https://www.gnu.org/licenses/>.
///
#ifndef __FLINT_H__
#define __FLINT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>
#include <stdio.h>

#if defined(__GNUC__)
    #if defined(__STRICT_ANSI__)
        #define NPY_INLINE __inline__
    #else
        #define NPY_INLINE inline
    #endif
#else
    #define NPY_INLINE
#endif

/// @brief Get the max of 4 inputs
#define max4(a,b,c,d)           \
({                              \
    __typeof__ (a) _a = (a);    \
    __typeof__ (b) _b = (b);    \
    __typeof__ (c) _c = (c);    \
    __typeof__ (d) _d = (d);    \
    _a = _a > _b ? _a : _b;     \
    _b = _c > _d ? _c : _d;     \
    _a > _b ? _a: _b;           \
})

/// @brief Get the min of 4 inputs
#define min4(a,b,c,d)           \
({                              \
    __typeof__ (a) _a = (a);    \
    __typeof__ (b) _b = (b);    \
    __typeof__ (c) _c = (c);    \
    __typeof__ (d) _d = (d);    \
    _a = _a < _b ? _a : _b;     \
    _b = _c < _d ? _c : _d;     \
    _a < _b ? _a: _b;           \
})


//
// Rounded Floating Point Interval stuct
//

/// @brief Rounded floating point interval with tracked value
/// @param a the lower bound
/// @param b the upper bound
/// @param v the tracked value
typedef struct {
    double a;
    double b;
    double v;
} flint;

#define FLINT_ZERO (flint) {0.0, 0.0, 0.0}
#define FLINT_ONE (flint) {1.0, 1.0, 1.0}
#define FLINT_HALF (flint) {0.5, 0.5, 0.5}
#define FLINT_TWO (flint) {2.0, 2.0, 2.0}

//
// Conversions
//
// Cast from a integer as an exact value if possible, otherwise expand the 
// interval
#define MAX_DOUBLE_INT 9.007199254740991e15
#define MIN_DOUBLE_INT -9.007199254740991e15
static NPY_INLINE flint int_to_flint(long long l) {
    double d = (double) l;
    flint f = {d, d, d};
    if (d > MAX_DOUBLE_INT || d < MIN_DOUBLE_INT) {
        f.a = nextafter(d,-INFINITY);
        f.b = nextafter(d,INFINITY);
    }
    return f;
}
// Cast from a simple floating point create smallest interval surrounding it
static NPY_INLINE flint double_to_flint(double f) {
    return (flint) {
        nextafter(f, -INFINITY),
        nextafter(f, INFINITY),
        f
    };
}
static NPY_INLINE flint float_to_flint(float f) {
    double a = nextafterf(f, -INFINITY);
    double b = nextafterf(f, INFINITY);
    return (flint) {a, b, (double) f};
}

//
// Floating point special value queries
//
// Interval must not overlap zero
static NPY_INLINE int flint_nonzero(flint f) {
    return f.a > 0.0 || f.b < 0.0;
}
// Any value is a NaN
static NPY_INLINE int flint_isnan(flint f) {
    return isnan(f.a) || isnan(f.b) || isnan(f.v);
}
// Either boundary value is infinite 
static NPY_INLINE int flint_isinf(flint f) {
    return isinf(f.a) || isinf(f.v);
}
// Both boundaries are finite
static NPY_INLINE int flint_isfinite(flint f) {
    return isfinite(f.a) && isfinite(f.b);
}

//
// Comparisons
//
// Any overlap should trigger as equal
static NPY_INLINE int flint_eq(flint f1, flint f2) {
    return 
        !flint_isnan(f1) && !flint_isnan(f2) &&
        (f1.a <= f2.b) && (f1.b >= f2.a);
}
// No overlay - all above or all below
static NPY_INLINE int flint_ne(flint f1, flint f2) {
    return
        flint_isnan(f1) || flint_isnan(f2) ||
        (f1.a > f2.b) || (f1.b < f2.a);
}
// Less than or equal allows for any amount of overlap
static NPY_INLINE int flint_le(flint f1, flint f2) {
    return
        !flint_isnan(f1) && !flint_isnan(f2) &&
        f1.a <= f2.b;
}
// Less than must not overlap at all
static NPY_INLINE int flint_lt(flint f1, flint f2) {
    return 
        !flint_isnan(f1) && !flint_isnan(f2) &&
        f1.b < f2.a;
}
// Greater than or equal allows for any amount of overlap
static NPY_INLINE int flint_ge(flint f1, flint f2) {
    return 
        !flint_isnan(f1) && !flint_isnan(f2) &&
        f1.b >= f2.a;
}
// Greater than must not overlap
static NPY_INLINE int flint_gt(flint f1, flint f2) {
    return 
        !flint_isnan(f1) && !flint_isnan(f2) &&
        f1.a > f2.b;
}

//
// Arithmatic
//
// --Identity--
static NPY_INLINE flint flint_positive(flint f) {
    return f;
}
// --Negation--
// swap upper and lower interval boundaries
static NPY_INLINE flint flint_negative(flint f) {
    flint _f = {-f.b, -f.a, -f.v};
    return _f;
}
// --Addition--
// add the boundaries
static NPY_INLINE flint flint_add(flint f1, flint f2) {
    flint _f = {
        nextafter(f1.a+f2.a, -INFINITY),
        nextafter(f1.b+f2.b, INFINITY),
        f1.v+f2.v
    };
    return _f;
}
static NPY_INLINE void flint_inplace_add(flint* f1, flint f2) {
    f1->a = nextafter(f1->a + f2.a, -INFINITY);
    f1->b = nextafter(f1->b + f2.b, INFINITY);
    f1->v += f2.v;
    return;
}
static NPY_INLINE flint flint_scalar_add(double s, flint f) {
    return flint_add(f, double_to_flint(s));
}
static NPY_INLINE flint flint_add_scalar(flint f, double s) {
    return flint_add(f, double_to_flint(s));    
}
static NPY_INLINE void flint_inplace_add_scalar(flint* f, double s) {
    flint_inplace_add(f, double_to_flint(s));
    return;
}
// --Subtraction--
// swap second interval then subtract
static NPY_INLINE flint flint_subtract(flint f1, flint f2) {
    flint _f = {
        nextafter(f1.a-f2.b, -INFINITY),
        nextafter(f1.b-f2.a, INFINITY),
        f1.v-f2.v
    };
    return _f;
}
static NPY_INLINE void flint_inplace_subtract(flint* f1, flint f2) {
    f1->a = nextafter(f1->a - f2.b, -INFINITY);
    f1->b = nextafter(f1->b - f2.a, INFINITY);
    f1->v -= f2.v;
    return;
}
static NPY_INLINE flint flint_scalar_subtract(double s, flint f) {
    return flint_subtract(double_to_flint(s), f);
}
static NPY_INLINE flint flint_subtract_scalar(flint f, double s) {
    return flint_subtract(f, double_to_flint(s));
}
static NPY_INLINE void flint_inplace_subtract_scalar(flint* f, double s) {
    flint_inplace_subtract(f, double_to_flint(s));
}
// --Multiplication--
// try all products of boundaries, choose min and max
static NPY_INLINE flint flint_multiply(flint f1, flint f2) {
    double a = min4(f1.a*f2.a, f1.a*f2.b, f1.b*f2.a, f1.b*f2.b);
    double b = max4(f1.a*f2.a, f1.a*f2.b, f1.b*f2.a, f1.b*f2.b);
    flint _f = {
        nextafter(a, -INFINITY),
        nextafter(b, INFINITY),
        f1.v*f2.v
    };
    return _f;
}
static NPY_INLINE void flint_inplace_multiply(flint* f1, flint f2) {
    double _a = min4(f1->a*f2.a, f1->a*f2.b, f1->b*f2.a, f1->b*f2.b);
    f1->b = max4(f1->a*f2.a, f1->a*f2.b, f1->b*f2.a, f1->b*f2.b);
    f1->a = _a;
    f1->v *= f2.v;
    return;
};
static NPY_INLINE flint flint_scalar_multiply(double s, flint f) {
    return flint_multiply(double_to_flint(s), f);
}
static NPY_INLINE flint flint_multiply_scalar(flint f, double s) {
    return flint_multiply(f, double_to_flint(s));
}
static NPY_INLINE void flint_inplace_multiply_scalar(flint* f, double s) {
    flint_inplace_multiply(f, double_to_flint(s));
}
// --Division--
// try all quotients of boundaries, choose min and max
static NPY_INLINE flint flint_divide(flint f1, flint f2) {
    double a = min4(f1.a/f2.a, f1.a/f2.b, f1.b/f2.a, f1.b/f2.b);
    double b = max4(f1.a/f2.a, f1.a/f2.b, f1.b/f2.a, f1.b/f2.b);
    flint _f = {
        nextafter(a, -INFINITY),
        nextafter(b, INFINITY),
        f1.v/f2.v
    };
    return _f;
}
static NPY_INLINE void flint_inplace_divide(flint* f1, flint f2) {
    double _a = min4(f1->a/f2.a, f1->a/f2.b, f1->b/f2.a, f1->b/f2.b);
    f1->b = max4(f1->a/f2.a, f1->a/f2.b, f1->b/f2.a, f1->b/f2.b);
    f1->a = _a;
    f1->v /= f2.v;
    return;
};
static NPY_INLINE flint flint_scalar_divide(double s, flint f) {
    return flint_divide(double_to_flint(s), f);
}
static NPY_INLINE flint flint_divide_scalar(flint f, double s) {
    return flint_divide(f, double_to_flint(s));
}
static NPY_INLINE void flint_inplace_divide_scalar(flint* f, double s) {
    flint_inplace_divide(f, double_to_flint(s));
}

//
// Math functions
//
// Don't righty know how copysign should work ...
// static NPY_INLINE flint flint_copysign(flint f1, flint f2) {
// }
// Power uses the pow function, it's like mul except with a NaN check
static NPY_INLINE flint flint_power(flint f1, flint f2) {
    double aa = pow(f1.a, f2.a);
    double ab = pow(f1.a, f2.b);
    double ba = pow(f1.b, f2.a);
    double bb = pow(f1.b, f2.b);
    double v = pow(f1.v, f2.v);
    flint ret = {0.0, 0.0, 0.0};
    if (isnan(aa) || isnan(ab) || isnan(ba) || isnan(bb) || isnan(v)) {
        v = sqrt(-1.0);
        ret.a = v; ret.b = v; ret.v = v;
    } else {
        ret.a = nextafter(min4(aa,ab,ba,bb),-INFINITY);
        ret.b = nextafter(max4(aa,ab,ba,bb),INFINITY);
        ret.v = v;
    }
    return ret;
}
static NPY_INLINE void flint_inplace_power(flint* f1, flint f2) {
    double aa = pow(f1->a, f2.a);
    double ab = pow(f1->a, f2.b);
    double ba = pow(f1->b, f2.a);
    double bb = pow(f1->b, f2.b);
    double v = pow(f1->v, f2.v);
    if (isnan(aa) || isnan(ab) || isnan(ba) || isnan(bb) || isnan(v)) {
        v = sqrt(-1.0);
        f1->a = v; f1->b = v; f1->v = v;
    } else {
        f1->a = nextafter(min4(aa,ab,ba,bb),-INFINITY);
        f1->b = nextafter(max4(aa,ab,ba,bb),INFINITY);
        f1->v = v;
    }
}
// Absolute value 'folds' the interval if it spans zero
static NPY_INLINE flint flint_absolute(flint f) {
    flint _f = f;
    if (f.b < 0.0) { // interval is all negative - so invert
        _f.a = -f.b;
        _f.b = -f.a;
        _f.v = -f.v;
    } else if (f.a < 0) { // interval spans 0
        _f.a = 0.0; // 0 is the new lower bound
        _f.b = ((-f.a > f.b)? -f.a : f.b); // upper bound is the greater
        _f.v = abs(f.v); // value is absolute valued
    }
    return _f;
}
// Square root, only gives NaN if whole interval is less than zero
static NPY_INLINE flint flint_sqrt(flint f) {
    flint _f;
    if (f.b < 0.0) {
        double nan = sqrt(-1.0);
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
// Log only gives NAN if whole interval is less than zero
static NPY_INLINE flint flint_log(flint f) {
    flint _f;
    if (f.b < 0.0) {
        double nan = sqrt(-1.0);
        _f.a = nan; _f.b = nan; _f.v = nan;
    } else if (f.a < 0.0) {
        _f.a = -INFINITY;
        _f.b = nextafter(log(f.b), INFINITY);
        _f.v = (f.v > 0.0) ? log(f.v) : -INFINITY;
    } else {
        _f.a = nextafter(log(f.a), -INFINITY);
        _f.b = nextafter(log(f.b), INFINITY);
        _f.v = log(f.v);
    }
    return _f;
}
// Exponential function is a monotonic function with full range
static NPY_INLINE flint flint_exp(flint f){
    flint _f = {
        nextafter(exp(f.a), -INFINITY),
        nextafter(exp(f.b), INFINITY),
        exp(f.v)
    };
    return _f;
}
// // Power function uses log, so has the same limit
// static NPY_INLINE flint flint_power(flint f, flint p) {
//     return flint_exp(flint_multiply(p, flint_log(f)));
// }
// static NPY_INLINE void flint_inplace_power(flint* f, flint p) {
//     flint _f = *f;
//     *f = flint_exp(flint_multiply(p, flint_log(_f)));
//     return;
// }
// static NPY_INLINE flint flint_power_scalar(flint f, double p) {
//     return flint_exp(flint_multiply(double_to_flint(p), flint_log(f)));
// }

#ifdef __cplusplus
}
#endif

#endif // __FLINT_H__
