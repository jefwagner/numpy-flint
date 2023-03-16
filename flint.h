/// @file flint.h Functions for rounded floating point mathematics
///
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

typedef struct {
    double a;
    double b;
    double v;
} flint;

// Conversions
float flint_to_float(flint f);
double flint_to_double(flint f);
flint float_to_flint(float f);
flint double_to_flint(double f);
// Comparisons
int flint_eq(flint fa, flint fb);
int flint_ne(flint fa, flint fb);
int flint_le(flint fa, flint fb);
int flint_lt(flint fa, flint fb);
int flint_ge(flint fa, flint fb);
int flint_gt(flint fa, flint fb);
// Floating point status functions
int flint_isnonzero(flint f);
int flint_isnan(flint f);
int flint_isinf(flint f);
int flint_isfinite(flint f);
// Arithmatic
flint flint_negative(flint f);
flint flint_add(flint f1, flint f2);
flint flint_subtract(flint f1, flint f2);
flint flint_multiply(flint f1, flint f2);
flint flint_divide(flint f1, flint f2);
flint flint_add_scalar(flint f, double s);
flint flint_subtract_scalar(flint f, double s);
flint scalar_subtract_flint(double s, flint f);
flint flint_multiply_scalar(flint f, double s);
flint flint_divide_scalar(flint f, double s);
flint scalar_divide_flint(double s, flint f);
// Whatever these things are
flint flint_absolute(flint f);
flint flint_copysign(flint f1, flint f2);
// Math functions
flint flint_sqrt(flint f);
flint flint_log(flint f);
flint flint_exp(flint f);
flint flint_power(flint f, flint p);
flint flint_power_scalar(flint f, double p);

#ifdef __cplusplus
}
#endif

#endif // __FLINT_H__
