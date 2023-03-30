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

#define FLINT_2PI ((flint) {6.283185307179586, 6.283185307179587, 6.283185307179586})
#define FLINT_PI ((flint) {3.141592653589793, 3.1415926535897936, 3.141592653589793})
#define FLINT_PI_2 ((flint) {1.5707963267948966, 1.5707963267948968, 1.5707963267948966})
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
#define FLINT_MONOTONIC(fname) \
static NPY_INLINE flint flint_##fname(flint f) { \
    flint _f = { \
        nextafter(nextafter(fname(f.a), -INFINITY), -INFINITY), \
        nextafter(nextafter(fname(f.b), INFINITY), INFINITY), \
        fname(f.v) \
    }; \
    return _f; \
}
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
        ret.a = nextafter(nextafter(min4(aa,ab,ba,bb),-INFINITY),-INFINITY);
        ret.b = nextafter(nextafter(max4(aa,ab,ba,bb),INFINITY),INFINITY);
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
        f1->a = nextafter(nextafter(min4(aa,ab,ba,bb),-INFINITY),-INFINITY);
        f1->b = nextafter(nextafter(max4(aa,ab,ba,bb),INFINITY),INFINITY);
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
// Cube root is a monotonic function with full range
FLINT_MONOTONIC(cbrt)
// Hypoteneus has a single minima in both f1 and f2
static NPY_INLINE flint flint_hypot(flint f1, flint f2) {
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
// Exponential function is a monotonic function with full range
FLINT_MONOTONIC(exp)
FLINT_MONOTONIC(exp2)
FLINT_MONOTONIC(expm1)
#define FLINT_LOGFUNC(log, min) \
static NPY_INLINE flint flint_##log(flint f) { \
    flint _f; \
    if (f.b < min) { \
        double nan = sqrt(-1.0); \
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
// Error fuction is monotonic with full range
FLINT_MONOTONIC(erf)
static NPY_INLINE flint flint_erfc(flint f) {
    flint _f = {
        nextafter(nextafter(erfc(f.b), -INFINITY), -INFINITY),
        nextafter(nextafter(erfc(f.a), INFINITY), INFINITY),
        erfc(f.v)
    };
    return _f;
}
// FLINT_MONOTONIC(erfc) -> decresing, need some swapy-swap
// Trig Functions
static NPY_INLINE flint flint_sin(flint f) {
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
static NPY_INLINE flint flint_cos(flint f) {
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
static NPY_INLINE flint flint_tan(flint f) {
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
// Inverse trig functions
static NPY_INLINE flint flint_asin(flint f) {
    flint _f;
    if (f.b < -1.0 || f.a > 1.0) {
        double nan = sqrt(-1.0);
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
static NPY_INLINE flint flint_acos(flint f) {
    flint _f;
    if (f.b < -1.0 || f.a > 1.0) {
        double nan = sqrt(-1.0);
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
// atan2
static NPY_INLINE flint flint_atan2(flint fy, flint fx) {
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
// Hyperbolic trig functions
FLINT_MONOTONIC(sinh)
static NPY_INLINE flint flint_cosh(flint f) {
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
// Inverse hyperbolic trig
FLINT_MONOTONIC(asinh)
static NPY_INLINE flint flint_acosh(flint f) {
    flint _f;
    if (f.b < 1.0) {
        double nan = sqrt(-1.0);
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
// atanh
static NPY_INLINE flint flint_atanh(flint f) {
    flint _f;
    if (f.b < -1.0 || f.a > 1.0) {
        double nan = sqrt(-1.0);
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
