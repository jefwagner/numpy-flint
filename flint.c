/// @file flint.c Functions for rounded floating point mathematics
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

#include "flint.h"
#include "math.h"

// Conversions
float flint_to_float(flint f) {
    return (float) f.v;
}
double flint_to_double(flint f) {
    return f.v;
}
flint float_to_flint(float f) {
    double d = (double) f;
    return (flint) {
        nextafter(d, -INFINITY),
        nextafter(d, ININITY),
        d
    };
}
flint double_to_flint(double f) {
    return (flint) {
        nextafter(d, -INFINITY),
        nextafter(d, ININITY),
        d
    };
}
// Comparisons
int flint_eq(flint fa, flint fb) {
    
}
int flint_ne(flint fa, flint fb) {

}
int flint_le(flint fa, flint fb) {

}
int flint_lt(flint fa, flint fb) {

}
int flint_ge(flint fa, flint fb) {

}
int flint_gt(flint fa, flint fb) {

}
// Floating point status functions
int flint_isnonzero(flint f) {

}
int flint_isnan(flint f) {

}
int flint_isinf(flint f) {

}
int flint_isfinite(flint f) {

}
// Arithmatic
flint flint_negative(flint f) {

}
flint flint_add(flint f1, flint f2) {

}
flint flint_subtract(flint f1, flint f2) {

}
flint flint_multiply(flint f1, flint f2) {

}
flint flint_divide(flint f1, flint f2) {

}
flint flint_multiply_scalar(flint f, double s) {

}
flint flint_divide_scalar(flint f, double s) {

}
// Whatever these things are
flint flint_absolute(flint f) {

}
flint flint_copysign(flint f1, flint f2) {

}
// Math functions
flint flint_sqrt(flint f) {

}
flint flint_log(flint f) {

}
flint flint_exp(flint f) {

}
flint flint_power(flint f, flint p) {

}
flint flint_power_scalar(flint f, double p) {

}
