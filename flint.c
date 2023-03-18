// /// @file flint.c Functions for rounded floating point mathematics
// ///
// /// This file is part of numpy-flint.
// ///
// /// Numpy-flint is free software: you can redistribute it and/or modify it under the
// /// terms of the GNU General Public License as published by the Free Software
// /// Foundation, either version 3 of the License, or (at your option) any later version.
// ///
// /// Numpy-flint is distributed in the hope that it will be useful, but WITHOUT ANY
// /// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
// /// PARTICULAR PURPOSE. See the GNU General Public License for more details.
// ///
// /// You should have received a copy of the GNU General Public License along with Foobar.
// /// If not, see <https://www.gnu.org/licenses/>.
// ///

// #include "flint.h"
// #include "math.h"

// #define max4(a,b,c,d)           \
// ({                              \
//     __typeof__ (a) _a = (a);    \
//     __typeof__ (b) _b = (b);    \
//     __typeof__ (c) _c = (c);    \
//     __typeof__ (d) _d = (d);    \
//     _a = _a > _b ? _a : _b;     \
//     _b = _c > _d ? _c : _d;     \
//     _a > _b ? _a: _b;           \
// })

// #define min4(a,b,c,d)           \
// ({                              \
//     __typeof__ (a) _a = (a);    \
//     __typeof__ (b) _b = (b);    \
//     __typeof__ (c) _c = (c);    \
//     __typeof__ (d) _d = (d);    \
//     _a = _a < _b ? _a : _b;     \
//     _b = _c < _d ? _c : _d;     \
//     _a < _b ? _a: _b;           \
// })

// //
// // Conversions
// //
// float flint_to_float(flint f) {
//     return (float) f.v;
// }

// double flint_to_double(flint f) {
//     return f.v;
// }

// flint float_to_flint(float f) {
//     double d = (double) f;
//     return (flint) {
//         nextafter(d, -INFINITY),
//         nextafter(d, INFINITY),
//         d
//     };
// }

// flint double_to_flint(double f) {
//     return (flint) {
//         nextafter(d, -INFINITY),
//         nextafter(d, ININITY),
//         d
//     };
// }

// //
// // Comparisons
// //
// // Any overlap should trigger as equal
// int flint_eq(flint f1, flint f2) {
//     return (f1.a <= f2.b) && (f1.b >= f2.a);
// }
// // No overlay - all above or all below
// int flint_ne(flint f1, flint f2) {
//     return (f1.a > f2.b) || (f1.b < f2.a);
// }
// // Less than or equal allows for any amount of overlap
// int flint_le(flint f1, flint f2) {
//     return f1.a <= f2.b;
// }
// // Less than must not overlap at all
// int flint_lt(flint f1, flint f2) {
//     return f1.b < f2.b;
// }
// // Greater than or equal allows for any amount of overlap
// int flint_ge(flint f1, flint f2) {
//     return f1.b >= f2.a;
// }
// // Greater than must not overlap
// int flint_gt(flint f1, flint f2) {
//     return f1.a > f2.b;
// }

// // Floating point status functions
// int flint_isnonzero(flint f) {
//     return f.a > 0.0 || f.b < 0.0;
// }

// int flint_isnan(flint f) {
//     return isnan(f.a) || isnan(f.b) || isnan(f.v);
// }

// int flint_isinf(flint f) {
//     return isinf(f.a) || isinf(f.b) || isnan(f.v);
// }

// int flint_isfinite(flint f) {
//     return isfinite(f.a) && isfinite(f.b);
// }

// //
// // Arithmatic
// //
// flint flint_negative(flint f) {
//     return (flint) {-f.b, -f.a, -f.v};
// }

// flint flint_add(flint f1, flint f2) {
//     return (flint) {
//         nextafter(f1.a + f2.a, -INFINITY),
//         nextafter(f1.b + f2.b, INFINITY),
//         f1.v + f2.v
//     };
// }

// flint flint_subtract(flint f1, flint f2) {
//     return (flint) {
//         nextafter(f1.a - f2.b, -INFINITY),
//         nextafter(f1.b - f2.a, INFINITY),
//         f1.v - f2.v
//     };
// }

// flint flint_multiply(flint f1, flint f2) {
//     double a = min4(f1.a*f2.a, f1.a*f2.b, f1.b*f2.a, f1.b*f2.b);
//     double b = max4(f1.a*f2.a, f1.a*f2.b, f1.b*f2.a, f1.b*f2.b);
//     return (flint) {
//         nextafter(a, -INFINITY),
//         nextafter(b, INFINITY),
//         f1.v*f2.v
//     }
// }

// flint flint_divide(flint f1, flint f2) {
//     double a = min4(f1.a/f2.a, f1.a/f2.b, f1.b/f2.a, f1.b/f2.b);
//     double b = max4(f1.a/f2.a, f1.a/f2.b, f1.b/f2.a, f1.b/f2.b);
//     return (flint) {
//         nextafter(a, -INFINITY),
//         nextafter(b, INFINITY),
//         f1.v/f2.v
//     }
// }

// flint flint_add_scalar(flint f, double s) {
//     flint fs = double_to_flint(s);
//     return flint_add(f, fs);
// }

// flint flint_subtract_scalar(flint f, double s) {
//     flint fs = double_to_flint(s);
//     return flint_subtract(f, fs);
// }

// flint scalar_subtract_flint(double s, flint f) {
//     flint fs = double_to_flint(s);
//     return flint_subtract(fs, f);
// }

// flint flint_multiply_scalar(flint f, double s) {
//     flint fs = double_to_flint(s);
//     return flint_multiply(f, fs);
// }

// flint flint_divide_scalar(flint f, double s) {
//     flint fs = double_to_flint(s);
//     return flint_divide(f, fs);
// }

// flint scalar_divide_flint(double s, flint f) {
//     flint fs = double_to_flint(s);
//     return flint_divide(fs, f);
// }

// //
// // Whatever these things are
// //
// flint flint_absolute(flint f) {
//     if (f.b < 0.0) {
//         return (flint) {-f.b, -f.a, -f.v};
//     } else if (f.a < 0.0) {
//         return (flint) {0.0, max(-f.a, f.b), abs(f.v)};
//     } else {
//         return f;
//     }
// }

// flint flint_copysign(flint f1, flint f2) {
//     if (flint.b < 0.0) {
//         return flint_negative(f1);
//     } else {
//         return f1;
//     }
// }

// //
// // Math functions
// //
// flint flint_sqrt(flint f) {
//     if (f.b < 0.0) {
//         double nan = sqrt(-1.0);
//         return (flint) {nan, nan, nan};
//     } else if (f.a < 0.0) {
//         if (f.v < 0) {
//             return (flint) {0.0, nextafter(sqrt(f.b), INFINITY), 0.0};
//         } else {
//             return (flint) {0.0, nextafter(sqrt(f.b), INFINITY), sqrt(f.v)};
//         }
//     } else {
//         return (flint) {
//             nextafter(sqrt(f.a), -INFINITY), 
//             nextafter(sqrt(f.b), INFINITY), 
//             sqrt(f.v)
//         };
//     }
// }

// flint flint_log(flint f) {
//     if (f.b < 0.0) {
//         double nan = log(-1.0);
//         return (flint) {nan, nan, nan};
//     } else if (f.a < 0.0) {
//         if (f.v < 0) {
//             return {-INFINITY, nextafter(log(f.b), INFINITY) -INFINITY};
//         } else {
//             return {-INFINITY, nextafter(log(f.b), INFINITY), log(f.v)};
//         }
//     } else {
//         return (flint) {
//             nextafter(log(f.a), -INFINITY), 
//             nextafter(log(f.b), -INFINITY), 
//             log(f.v)
//         };
//     }
// }

// flint flint_exp(flint f) {
//     return (flint) {
//         nextafter(exp(f.a), -INFINITY), 
//         nextafter(exp(f.b), INFINITY), 
//         exp(f.v)};
// }

// flint flint_power(flint f, flint p) {
//     return flint_exp(p*flint_log(f));
// }

// flint flint_power_scalar(flint f, double p) {
//     fp = double_to_flint(p);
//     return flint_power(f, fp);
// }


// // // Multiline macro for all binary function that return a bool
// // #define BINARY_BOOL_RETURNER(name) \
// // static PyObject* pyflint_##name(PyObject* a, PyObject* b) { \
// //     flint f = {0.0, 0.0, 0.0}; \
// //     double d = 0.0; \
// //     PyObject* D = {0}; \
// //     if (PyFlint_Check(a)) { \
// //         if (PyFlint_Check(b)) {\ 
// //             return PyBool_FromLong(flint_##name(a->obval,b->obval)); \
// //         } else { \
// //             D = PyNumber_Float(b); \
// //             if (D) { \
// //                 d = PyFloat_AsDouble(D); \
// //                 f = double_to_flint(d); \
// //                 return PyBool_FromLong(flint_##name(a->obval, f)); \
// //             } \
// //         } \
// //     } else { \
// //         D = PyNumber_Float(a); \
// //         if (D) { \
// //             d = PyFloat_AsDouble(D); \
// //             f = double_to_flint(d); \
// //             return PyBool_FromLong(flint_##(f, b->obval)); \
// //         } \
// //     } \
// //     PyErr_SetString(PyExc_TypeError, "Comparison with PyFlint must be with numeric type"); \
// //     return NULL; \
// // }
// // // All the comparison operators
// // BINARY_BOOL_RETURNER(equal)
// // BINARY_BOOL_RETURNER(not_equal)
// // BINARY_BOOL_RETURNER(less_equal)
// // BINARY_BOOL_RETURNER(less)
// // BINARY_BOOL_RETURNER(greater_equal)
// // BINARY_BOOL_RETURNER(greater)
// // // We're done with the macro now
// // #undef BINARY_BOOL_RETURNER
