/// @file numpy_flint.c Python/Numpy interface for flints
//
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
#include <stdint.h>
#include <Python.h>

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>
#include <numpy/npy_math.h>
// #include <numpy/halffloat.h>
#include <numpy/ufuncobject.h>
#include "structmember.h"

#include "flint.h"

#define NUMPY_FLINT_MODULE
#include "numpy_flint.h"


/// @brief The array of data members of the flint object
PyMemberDef pyflint_members[] = {
    {"a", T_DOUBLE, offsetof(PyFlint, obval.a), READONLY,
        "The lower bound of the floating point interval"},
    {"b", T_DOUBLE, offsetof(PyFlint, obval.b), READONLY,
        "The upper bound of the floating point interval"},
    {"v", T_DOUBLE, offsetof(PyFlint, obval.v), READONLY,
        "The tracked float value"},
    {NULL}
};

// ###############################
// ---- Convenience functions ----
// ###############################
// This section contains some convenience functions that are used by the method
// implementations below.

/// @brief A macro to check if an object is a PyFlint and grab the c struct.
/// @param f The c flint struct to copy the data into
/// @param F The python PyObject to check
/// @return If F is not a PyFlint, this forces an early return with NULL
#define PyFlint_CheckedGetFlint(f, F) \
    if (PyFlint_Check(F)) { \
        f = ((PyFlint*) F)->obval; \
    } else { \
        PyErr_SetString(PyExc_TypeError, "Input object is not PyFlint"); \
        return NULL; \
    }

/// @brief A macro that defines function of one variable that returns a bool
/// @param name the name of the function in the c and pyflint implementation
/// @return Returns the result of pure c flint_{name} function
#define UNARY_BOOL_RETURNER(name) \
static PyObject* pyflint_##name(PyObject* a) { \
    flint f = {0.0, 0.0, 0.0}; \
    PyFlint_CheckedGetFlint(f, a); \
    return PyBool_FromLong(flint_##name(f)); \
}

/// @brief A macro that defines a functions of one variable that return a flint
/// @param name the name of the function in the c and pyflint implementation
/// @return The result of hte pure c flint_{name} function
#define UNARY_FLINT_RETURNER(name) \
static PyObject* pyflint_##name(PyObject* a) { \
    flint f = {0.0, 0.0, 0.0}; \
    PyFlint_CheckedGetFlint(f, a); \
    return PyFlint_FromFlint(flint_##name(f)); \
}

/// @brief A macro that makes a unary operator a method acting on self
/// @param name The name of the function in the c and pyflint implementation
/// @return The result of the Python/C pyflint_{name} function
#define UNARY_TO_SELF_METHOD(name) \
static PyObject* pyflint_##name##_meth(PyObject* self, \
                                       PyObject* NPY_UNUSED(args)) { \
    return pyflint_##name(self); \
}

/// @brief A macro that defines functions of two variables that return a flint
/// @param name the name of the function in the c and pyflint implementation
/// @return The result of c function flint_{name} or Py_NotImplemented
#define BINARY_FLINT_RETURNER(name) \
static PyObject* pyflint_##name(PyObject* a, PyObject* b) { \
    flint fa = {0.0, 0.0, 0.0}; \
    flint fb = {0.0, 0.0, 0.0}; \
    double d = 0.0; \
    PyObject* D = {0}; \
    if (PyFlint_Check(a)) { \
        fa = ((PyFlint*)a)->obval;\
        if (PyFlint_Check(b)) {\
            fb = ((PyFlint*)b)->obval;\
            return PyFlint_FromFlint(flint_##name(fa,fb)); \
        } else { \
            D = PyNumber_Float(b); \
            if (D) { \
                d = PyFloat_AsDouble(D); \
                fb = double_to_flint(d); \
                return PyFlint_FromFlint(flint_##name(fa, fb)); \
            } \
        } \
    } else { \
        D = PyNumber_Float(a); \
        if (D) { \
            fb = ((PyFlint*)b)->obval; \
            d = PyFloat_AsDouble(D); \
            fa = double_to_flint(d); \
            return PyFlint_FromFlint(flint_##name(fa, fb)); \
        } \
    } \
    PyErr_SetString(PyExc_TypeError, \
        "Binary operations for functions with PyFlint must be with numeric type"); \
    Py_INCREF(Py_NotImplemented); \
    return Py_NotImplemented; \
}

/// @brief A macro that makes a operator or function a method acting on self
/// @param name The name of the function in the c and pyflint implementation
/// @return The result of the Python/C pyflint_{name} function
#define BINARY_TO_SELF_METHOD(name) \
static PyObject* pyflint_##name##_meth(PyObject* self, PyObject* args) { \
    Py_ssize_t size = PyTuple_Size(args); \
    PyObject* O = {0}; \
    if (size == 1) { \
        if (PyArg_ParseTuple(args, "O", &O)) { \
            return pyflint_##name(self, O); \
        } \
    } \
    PyErr_SetString(PyExc_TypeError, \
        "Binary operations for functions with PyFlint must be with numeric type"); \
    Py_INCREF(Py_NotImplemented); \
    return Py_NotImplemented; \
}

/// @brief A macro that defines an inplace operator
/// @param name the name of the operation in the c and pyflint implementation
/// @return The `a` PyFlint object c func flint_inplace_{name} acting on `obval`
#define BINARY_FLINT_INPLACE(name) \
static PyObject* pyflint_inplace_##name(PyObject* a, PyObject* b) { \
    flint* fptr = NULL; \
    flint fb = {0.0, 0.0, 0.0}; \
    double d = 0.0; \
    PyObject* D = {0}; \
    if (PyFlint_Check(a)) { \
        fptr = &(((PyFlint*) a)->obval); \
        if (PyFlint_Check(b)) { \
            fb = ((PyFlint*) b)->obval; \
            flint_inplace_##name(fptr, fb); \
            Py_INCREF(a); \
            return a; \
        } else { \
            D = PyNumber_Float(b); \
            if (D) { \
                d = PyFloat_AsDouble(D); \
                fb = double_to_flint(d); \
                flint_inplace_##name(fptr, fb); \
                Py_INCREF(a); \
                return a; \
            } \
        } \
    } \
    PyErr_SetString(PyExc_TypeError, \
        "+=,-=,*=,/= inplace operations with PyFlint must be with numeric type"); \
    Py_INCREF(Py_NotImplemented); \
    return Py_NotImplemented; \
}

/// @brief A macro that wraps a binary function into a tertiary function
/// @param name the name of the operation in the c and pyflint implementation
/// @return The result of the Python/C pyflint_{name} function
#define BINARY_TO_TERTIARY(name) \
static inline PyObject* pyflint_b2t_##name(PyObject *a, PyObject* b, \
                                               PyObject* NPY_UNUSED(c)) { \
    return pyflint_##name(a,b); \
}

/// @brief A macro that wraps an inplace binary func into a tertiary func
/// @param name the name of the operation in the c and pyflint implementation
/// @return The result of the Python/C pyflint_inplace_{name} function
#define BINARY_TO_TERTIARY_INPLACE(name) \
static inline PyObject* pyflint_b2t_inplace_##name(PyObject *a, PyObject* b, \
                                                       PyObject* NPY_UNUSED(c)) { \
    return pyflint_inplace_##name(a,b); \
}

// #####################################
// ---- Flint Method Implementation ----
// #####################################
// This section contains all of the methods include many __dunder__ methods

// --------------------------------
// ---- Object handler methods ----
// --------------------------------
/// @brief The __new__ allocating constructor
/// @param type The type of the PyObject
/// @return A new PyObject of type `type`
static PyObject* pyflint_new(PyTypeObject* type, 
                             PyObject* NPY_UNUSED(args),
                             PyObject* NPY_UNUSED(kwargs)) {
    PyFlint* self = (PyFlint*) type->tp_alloc(type, 0);
    return (PyObject*) self;
}

/// @brief The __init__ initializing constructor
/// @param self The object to be initialized
/// @param args A tuple containing 1 PyObject with either a flint, float, or int
/// @param kwargs An empty tuple
/// @return 0 on success, -1 on failure
static int pyflint_init(PyObject* self, PyObject* args, PyObject* kwargs) {
    Py_ssize_t size = PyTuple_Size(args);
    PyObject* O = {0};
    flint* fp = &(((PyFlint*) self)->obval);
    double d;
    long long n;

    if (kwargs && PyDict_Size(kwargs)) {
        PyErr_SetString(PyExc_TypeError,
                        "flint constructor doesn't take keyword arguments");
        return -1;
    }

    if (size == 1) {
        if (PyArg_ParseTuple(args, "O", &O)) {
            // One argument of an integer type (standard constructor) 
            if (PyLong_Check(O)) {
                n = PyLong_AsLongLong(O);
                *fp = int_to_flint(n);
                return 0;
            }
            // One argument of a floating type (standard constructor)
            else if (PyFloat_Check(O)) {
                d = PyFloat_AsDouble(O);
                *fp = double_to_flint(d);
                return 0;
            }
            // One argument of a PyFlint (copy constructor)
            else if (PyFlint_Check(O)) {
                *fp = ((PyFlint*) O)->obval;
                return 0;
            }
        }
    }

    PyErr_SetString(PyExc_TypeError,
                    "flint constructor one numeric argument");
    return -1;
}

/// @brief The __repr__ printing method
/// @return A python string representation of the tracked value
static PyObject* pyflint_repr(PyObject* self) {
    double v = ((PyFlint*) self)->obval.v;
    PyObject* V = PyFloat_FromDouble(v);
    return PyObject_Repr(V);
}

/// @brief The __str__ printing method
/// @return A python string representation of the tracked value
static PyObject* pyflint_str(PyObject* self) {
    double v = ((PyFlint*) self)->obval.v;
    PyObject* V = PyFloat_FromDouble(v);
    return PyObject_Str(V);
}

/// @brief The __hash__ function create an unique-ish integer from the flint.
///        Implements Bob Jenkin's one-at-a-time hash.
/// @return An integer to be used in a hash-table
static Py_hash_t pyflint_hash(PyObject *self) {
    flint* f = &(((PyFlint*)self)->obval);
    uint8_t* flint_as_data = (uint8_t*) &f;
    size_t i = 0;
    Py_hash_t h = 0;
    for (i=0; i<sizeof(flint); ++i) {
        h += flint_as_data[i];
        h += h << 10;
        h ^= h >> 6;
    }
    h += h << 3;
    h ^= h >> 11;
    h += h << 15;
    return (h==-1)?2:h;
}

/// @brief The __reduce__ method reproduces the internal structure of the flint 
///        struct as object as PyObjects
/// @return a Tuple with Type and a Tuple of the object members as PyObjects
static PyObject* pyflint_reduce(PyObject* self, PyObject* NPY_UNUSED(args)) {
    return Py_BuildValue("O(OOO)", Py_TYPE(self),
                         PyFloat_FromDouble(((PyFlint*) self)->obval.a),
                         PyFloat_FromDouble(((PyFlint*) self)->obval.b),
                         PyFloat_FromDouble(((PyFlint*) self)->obval.v));
}

/// @brief The __getstate__ method builds the data member as PyObjects
/// @param args A getstate flag
/// @return A Tuble of the object members as PyObjects
static PyObject* pyflint_getstate(PyObject* self, PyObject* args) {
    if (!PyArg_ParseTuple(args, ":getstate")) {
        return NULL;
    }
    return Py_BuildValue("OOO",
                         PyFloat_FromDouble(((PyFlint*) self)->obval.a),
                         PyFloat_FromDouble(((PyFlint*) self)->obval.b),
                         PyFloat_FromDouble(((PyFlint*) self)->obval.v));
} 

/// @brief The __setstate__ reads in the data as pickled by __getstate__
/// @param args A Tuple of object members a PyObjects and a setstate flag
/// @return NULL on failure or None on success, The value of the `self` is 
///         set from the values read in from the args tuple 
static PyObject* pyflint_setstate(PyObject* self, PyObject* args) {
    flint *f;
    f = &(((PyFlint*) self)->obval);
    if (!PyArg_ParseTuple(args, "(ddd):setstate", &(f->a), &(f->b), &(f->v))) {
        PyErr_SetString(PyExc_ValueError, "Could not unpack state tuple");
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

// ------------------------------------
// ---- Flint comparison operators ----
// ------------------------------------
/// @brief A rich comparison operator that implements __eq__, __ne__, __lt__, 
///        __le__, __gt__, and __ge__.
/// @param a The first object to compare - should always be a PyFlint
/// @param b The second object to compare
/// @param op An enum as an op-code for ==, !=, <, <=, >, >=
/// @return A PyBool of Py_True if `a op b`, otherwise Py_False
static PyObject* pyflint_richcomp(PyObject* a, PyObject* b, int op) {
    // First argument _should_ be guaranteed to be a flint
    flint f = {0.0, 0.0, 0.0};
    PyFlint_CheckedGetFlint(f, a);
    // Comparisons can happen for all other numerical values
    flint fo = {0.0, 0.0, 0.0};
    double d = 0.0;
    PyObject* D = {0};
    if (PyFlint_Check(b)) { // check if its a flint already
        fo = ((PyFlint*) b)->obval;
    } else { // otherwise try to cast into a float then a flint
        D = PyNumber_Float(b);
        if (!D) {
            PyErr_SetString(PyExc_TypeError, 
                "Comparison with PyFlint must be with numeric type");
            Py_INCREF(Py_NotImplemented);
            return Py_NotImplemented;
        }
        d = PyFloat_AsDouble(D);
        fo = double_to_flint(d);
    }
    switch (op) {
        case Py_EQ : {
            return PyBool_FromLong(flint_eq(f, fo));
        }
        case Py_NE : {
            return PyBool_FromLong(flint_ne(f, fo));
        }
        case Py_LT : {
            return PyBool_FromLong(flint_lt(f, fo));
        }
        case Py_LE : {
            return PyBool_FromLong(flint_le(f, fo));
        }
        case Py_GT : {
            return PyBool_FromLong(flint_gt(f, fo));
        }
        case Py_GE : {
            return PyBool_FromLong(flint_ge(f, fo));
        }
        default:
            PyErr_SetString(PyExc_TypeError, 
                "Supported comparison operators are ==, !=, <, <=, >, >=");
            Py_INCREF(Py_NotImplemented);
            return Py_NotImplemented;
    }
}

// ---------------------------
// ---- Numeric operators ----
// ---------------------------
/// @brief The _pos_ method, acts as the identity
/// @param a The PyFlint interval value
/// @return The reflected interval
UNARY_FLINT_RETURNER(positive)
/// @brief The _neg_ method, reflects the interval around 0
/// @param a The PyFlint interval value
/// @return The reflected interval
UNARY_FLINT_RETURNER(negative)
/// @brief The _abs_ method, evaluates the absolute value of the interval
/// @param a The PyFlint interval value
/// @return The absolute value of the interval
UNARY_FLINT_RETURNER(absolute)
UNARY_TO_SELF_METHOD(absolute)
/// @brief The _add_ and _radd_ addition method for intervals
/// @param a The first number/flint
/// @param b The second number/flint
/// @return a+b
BINARY_FLINT_RETURNER(add)
/// @brief The _sub_ and _rsub_ subtraction method for intervals
/// @param a The first number/flint
/// @param b The second number/flint
/// @return a-b
BINARY_FLINT_RETURNER(subtract)
/// @brief The _mul_ and _rmul_ multiplication method for intervals
/// @param a The first number/flint
/// @param b The second number/flint
/// @return a*b
BINARY_FLINT_RETURNER(multiply)
/// @brief The _truediv_ and _rtruediv_ division method for intervals
/// @param a The first number/flint
/// @param b The second number/flint
/// @return a/b
BINARY_FLINT_RETURNER(divide)
/// @brief The _pow_ or _rpow_ operator, evaluate a general power exponential
/// @param a The base
/// @param b The exponent
/// @return The a**b
BINARY_FLINT_RETURNER(power)
BINARY_TO_TERTIARY(power)
/// @brief The _iadd_ addition operator for intervals
/// @param a The first operand, value replaced with a+b
/// @param b The second operand
BINARY_FLINT_INPLACE(add)
/// @brief The _isub_ addition operator for intervals
/// @param a The first operand, value replaced with a+b
/// @param b The second operand
BINARY_FLINT_INPLACE(subtract)
/// @brief The _imul_ addition operator for intervals
/// @param a The first operand, value replaced with a+b
/// @param b The second operand
BINARY_FLINT_INPLACE(multiply)
/// @brief The _itruediv_ addition operator for intervals
/// @param a The first operand, value replaced with a+b
/// @param b The second operand
BINARY_FLINT_INPLACE(divide)
/// @brief The _ipow_ operator, evaluate a general power exponential
/// @param a The base
/// @param b The exponent
/// @return The a**b
BINARY_FLINT_INPLACE(power)
BINARY_TO_TERTIARY_INPLACE(power)
/// @brief The _float_ function to return a single float from the interval
/// @param a The flint value
/// @return The float value
static PyObject* pyflint_float(PyObject* a) {
    flint f = {0.0, 0.0, 0.0};
    PyFlint_CheckedGetFlint(f, a);
    return PyFloat_FromDouble(f.v);
}

// -----------------------------------------
// ---- Flint numeric struct definition ----
// -----------------------------------------
/// @brief The array of math dunder methods for the pyflint objects
/// This array contains pointers to the arithmetic operators
static PyNumberMethods pyflint_as_number = {
    .nb_add = pyflint_add, // binaryfunc nb_add;
    .nb_subtract = pyflint_subtract, // binaryfunc nb_subtract;
    .nb_multiply = pyflint_multiply, // binaryfunc nb_multiply;
    .nb_power = pyflint_b2t_power, // ternaryfunc nb_power;
    .nb_negative = pyflint_negative, // unaryfunc nb_negative;
    .nb_positive = pyflint_positive, // unaryfunc nb_positive;
    .nb_absolute = pyflint_absolute, // unaryfunc nb_absolute;
    .nb_inplace_add = pyflint_inplace_add, // binaryfunc nb_inplace_add;
    .nb_inplace_subtract = pyflint_inplace_subtract, // binaryfunc nb_inplace_subtract;
    .nb_inplace_multiply = pyflint_inplace_multiply, // binaryfunc nb_inplace_multiply;
    .nb_true_divide = pyflint_divide, // binaryfunc nb_true_divide;
    .nb_inplace_true_divide = pyflint_inplace_divide, // binaryfunc nb_inplace_true_divide;
    .nb_inplace_power = pyflint_b2t_inplace_power, // ternaryfunc np_inplace_power;
    .nb_float = pyflint_float, // unaryfunc np_float;
};

// ----------------------------------------------
// ---- Floating point special value queries ----
// ----------------------------------------------
/// @brief Query if a PyFlint interval contains zero
/// @param a The PyFlint object
/// @return Py_True if a != 0 otherwise Py_False
UNARY_BOOL_RETURNER(nonzero)
UNARY_TO_SELF_METHOD(nonzero)
/// @brief Query if the PyFlint is NaN
/// @param a The PyFlint object
/// @return Py_True if a is NaN otherwise Py_False
UNARY_BOOL_RETURNER(isnan)
UNARY_TO_SELF_METHOD(isnan)
/// @brief Query if the PyFlint interval stretches to infinity
/// @param a The PyFlint object
/// @return Py_True if a is +/- infinity otherwise Py_False
UNARY_BOOL_RETURNER(isinf)
UNARY_TO_SELF_METHOD(isinf)
/// @brief Query if the PyFlint interval is finite
/// @param a The PyFlint object
/// @return Py_False if a is NaN or +/- infinity otherwise Py_True
UNARY_BOOL_RETURNER(isfinite)
UNARY_TO_SELF_METHOD(isfinite)

// -----------------------------------
// ---- Elementary math functions ----
// -----------------------------------
/// @brief Evaluate the square root of the interval
/// @param a The PyFlint object
/// @return The square root of the interval if a >= 0 else NaN
UNARY_FLINT_RETURNER(sqrt)
UNARY_TO_SELF_METHOD(sqrt)
/// @brief Evaluate the cube root of the interval
/// @param a The PyFlint object
/// @return The cube root of the interval
UNARY_FLINT_RETURNER(cbrt)
UNARY_TO_SELF_METHOD(cbrt)
/// @brief Evaluate the hypotenuse distance from two intervals
/// @param a The first PyFlint object
/// @param b The second PyFlint object
/// @return The hypotenuse distance of the two intervals sqrt(a^2+b^2)
BINARY_FLINT_RETURNER(hypot)
BINARY_TO_SELF_METHOD(hypot)
/// @brief Evaluate the exponential of the interval
/// @param a The PyFlint object
/// @return The exponential of the interval
UNARY_FLINT_RETURNER(exp)
UNARY_TO_SELF_METHOD(exp)
/// @brief Evaluate the exponential base 2 (2^a) of the interval
/// @param a The PyFlint object
/// @return The exponential base 2 of the interval
UNARY_FLINT_RETURNER(exp2)
UNARY_TO_SELF_METHOD(exp2)
/// @brief Evaluate the exponential function minus 1 (e^a -1) of the interval
/// @param a The PyFlint object
/// @return The exponential minus 1 of the interval
UNARY_FLINT_RETURNER(expm1)
UNARY_TO_SELF_METHOD(expm1)
/// @brief Evaluate the natural log of the interval
/// @param a The PyFlint object
/// @return The log of the interval if a >= 0 else NaN
UNARY_FLINT_RETURNER(log)
UNARY_TO_SELF_METHOD(log)
/// @brief Evaluate the log base 10 of the interval
/// @param a The PyFlint object
/// @return The log base 10 of the interval if a >= 0 else NaN
UNARY_FLINT_RETURNER(log10)
UNARY_TO_SELF_METHOD(log10)
/// @brief Evaluate the log base 2 of the interval
/// @param a The PyFlint object
/// @return The log base 2 of the interval if a >= 0 else NaN
UNARY_FLINT_RETURNER(log2)
UNARY_TO_SELF_METHOD(log2)
/// @brief Evaluate the natural log of 1 plus the argument ln(1+a) of the interval
/// @param a The PyFlint object
/// @return The natural log of 1+a of the interval if a >= -1 else NaN
UNARY_FLINT_RETURNER(log1p)
UNARY_TO_SELF_METHOD(log1p)
/// @brief Evaluate the sine of the interval
/// @param a The PyFlint object
/// @return The sine of the interval
UNARY_FLINT_RETURNER(sin)
UNARY_TO_SELF_METHOD(sin)
/// @brief Evaluate the cosine of the interval
/// @param a The PyFlint object
/// @return The cosine of the interval
UNARY_FLINT_RETURNER(cos)
UNARY_TO_SELF_METHOD(cos)
/// @brief Evaluate the tangent of the interval
/// @param a The PyFlint object
/// @return The tangent of the interval
UNARY_FLINT_RETURNER(tan)
UNARY_TO_SELF_METHOD(tan)
/// @brief Evaluate the inverse sine of the interval
/// @param a The PyFlint object
/// @return The inverse sine of the interval
UNARY_FLINT_RETURNER(asin)
UNARY_TO_SELF_METHOD(asin)
/// @brief Evaluate the inverse cosine of the interval
/// @param a The PyFlint object
/// @return The inverse cosine of the interval
UNARY_FLINT_RETURNER(acos)
UNARY_TO_SELF_METHOD(acos)
/// @brief Evaluate the inverse tangent of the interval
/// @param a The PyFlint object
/// @return The inverse tangent of the interval
UNARY_FLINT_RETURNER(atan)
UNARY_TO_SELF_METHOD(atan)
/// @brief Evaluate the 2-input inverse tangent of the interval
/// @param a The y coordinate PyFlint object
/// @param b The x coordinate PyFlint object
/// @return The inverse tangent of the two intervals
BINARY_FLINT_RETURNER(atan2)
BINARY_TO_SELF_METHOD(atan2)
/// @brief Evaluate the hyperbolic sine of the interval
/// @param a The PyFlint object
/// @return The hyperbolic sine of the interval
UNARY_FLINT_RETURNER(sinh)
UNARY_TO_SELF_METHOD(sinh)
/// @brief Evaluate the hyperbolic cosine of the interval
/// @param a The PyFlint object
/// @return The hyperbolic cosine of the interval
UNARY_FLINT_RETURNER(cosh)
UNARY_TO_SELF_METHOD(cosh)
/// @brief Evaluate the hyperbolic tangent of the interval
/// @param a The PyFlint object
/// @return The hyperbolic tangent of the interval
UNARY_FLINT_RETURNER(tanh)
UNARY_TO_SELF_METHOD(tanh)
/// @brief Evaluate the inverse hyperbolic sine of the interval
/// @param a The PyFlint object
/// @return The inverse hyperbolic sine of the interval
UNARY_FLINT_RETURNER(asinh)
UNARY_TO_SELF_METHOD(asinh)
/// @brief Evaluate the inverse hyperbolic cosine of the interval
/// @param a The PyFlint object
/// @return The inverse hyperbolic cosine of the interval
UNARY_FLINT_RETURNER(acosh)
UNARY_TO_SELF_METHOD(acosh)
/// @brief Evaluate the inverse hyperbolic tangent of the interval
/// @param a The PyFlint object
/// @return The inverse hyperbolic tangent of the interval
UNARY_FLINT_RETURNER(atanh)
UNARY_TO_SELF_METHOD(atanh)


// ---------------------------------------
// ---- Flint method table definition ----
// ---------------------------------------
/// @brief Defines the flint methods accessible for flint objects in python 
/// the list structure is (name, function, ARGTYPE_MACRO, description)
PyMethodDef pyflint_methods[] = {
    // Pickle support functions
    {"__reduce__", pyflint_reduce, METH_NOARGS,
    "Return state information for pickling"},
    {"__getstate__", pyflint_getstate, METH_VARARGS,
    "Return state information for pickling"},
    {"__setstate__", pyflint_setstate, METH_VARARGS,
    "Reconstruct state information from pickle"},
    // methods for querying special float values
    {"nonzero", pyflint_nonzero_meth, METH_NOARGS,
    "True if the interval does not inersect zero"},
    {"isnan", pyflint_isnan_meth, METH_NOARGS,
    "True if the flint contains NaN components"},
    {"isinf", pyflint_isinf_meth, METH_NOARGS,
    "True if the interval extends to +/-infinity"},
    {"isfinite", pyflint_isfinite_meth, METH_NOARGS,
    "True if the interval has covers a finite range"},
    // Math functions
    {"abs", pyflint_absolute_meth, METH_NOARGS,
    "Evaluate the absolute value of the interval"},
    {"sqrt", pyflint_sqrt_meth, METH_NOARGS,
    "Evaluate the square root of the interval"},
    {"cbrt", pyflint_cbrt_meth, METH_NOARGS,
    "Evaluate the cube root of the interval"},
    {"hypot", pyflint_hypot_meth, METH_VARARGS,
    "Evaluate the hypotenuse distance with the two intervals"},
    {"exp", pyflint_exp_meth, METH_NOARGS,
    "Evaluate the exponential func of an interval"},
    {"exp2", pyflint_exp2_meth, METH_NOARGS,
    "Evaluate the exponential base 2 of an interval"},
    {"expm1", pyflint_expm1_meth, METH_NOARGS,
    "Evaluate the exponential minus 1 of an interval"},
    {"log", pyflint_log_meth, METH_NOARGS,
    "Evaluate the natural log of the interval"},
    {"log10", pyflint_log10_meth, METH_NOARGS,
    "Evaluate the log base 10 of the interval"},
    {"log2", pyflint_log2_meth, METH_NOARGS,
    "Evaluate the log base 2 of the interval"},
    {"log1p", pyflint_log1p_meth, METH_NOARGS,
    "Evaluate the natural log of one plus the interval"},
    {"sin", pyflint_sin_meth, METH_NOARGS,
    "Evaluate the sine of the interval"},
    {"cos", pyflint_cos_meth, METH_NOARGS,
    "Evaluate the cosine of the interval"},
    {"tan", pyflint_tan_meth, METH_NOARGS,
    "Evaluate the tangent of the interval"},
    {"arcsin", pyflint_asin_meth, METH_NOARGS,
    "Evaluate the inverse sine of the interval"},
    {"arccos", pyflint_acos_meth, METH_NOARGS,
    "Evaluate the inverse cosine of the interval"},
    {"arctan", pyflint_atan_meth, METH_NOARGS,
    "Evaluate the inverse tangent of the interval"},
    {"arctan2", pyflint_atan2_meth, METH_VARARGS,
    "Evalute the two-input inverse tangent of the intervals"},
    {"sinh", pyflint_sinh_meth, METH_NOARGS,
    "Evaluate the hyperbolic sine of the interval"},
    {"cosh", pyflint_cosh_meth, METH_NOARGS,
    "Evaluate the hyperbolic cosine of the interval"},
    {"tanh", pyflint_tanh_meth, METH_NOARGS,
    "Evaluate the hyperbolic tangent of the interval"},
    {"arcsinh", pyflint_asinh_meth, METH_NOARGS,
    "Evaluate the inverse hyperbolic sine of the interval"},
    {"arccosh", pyflint_acosh_meth, METH_NOARGS,
    "Evaluate the inverse hyperbolic cosine of the interval"},
    {"arctanh", pyflint_atanh_meth, METH_NOARGS,
    "Evaluate the inverse hyperbolic tangent of the interval"},
    // sentinel
    {NULL, NULL, 0, NULL}
};

// --------------------------------------
// ---- Property setters and getters ----
// --------------------------------------
/// @brief Get the size of the interval of flint object
/// This defines a member property getter for the size of the interval
/// so you can get the endpoints of hte interval with `eps = f.eps`
static PyObject* pyflint_get_eps(PyObject *self, void *NPY_UNUSED(closure)) {
    flint *f = &(((PyFlint*) self)->obval);
    PyObject *eps = PyFloat_FromDouble((f->b)-(f->a));
    return eps;
}

/// @brief Get the interval from a flint object
/// This defines a member property getter for the interval. It returns a tuple
/// with the (lower, upper) endpoints of the interval. use it to get the the
/// interval with `a,b = f.interval`
static PyObject* pyflint_get_interval(PyObject* self, 
                                      void* NPY_UNUSED(closure)) {
    flint *f = &(((PyFlint*) self)->obval);
    PyObject *tuple = PyTuple_New(2);
    PyTuple_SET_ITEM(tuple, 0, PyFloat_FromDouble(f->a));
    PyTuple_SET_ITEM(tuple, 1, PyFloat_FromDouble(f->b));
    return tuple;
}

/// @brief Set the flint from an interval
/// This defines a member property setter for the flint interval. You can use it
/// to either set the endpoints `f.interval = (a,b)`, in which case the
/// tracked value will be the midpoint `v =0.5* (a+b)'. You can also set the
/// interval AND tracked value `f.interval = (a,b,v)`
static int pyflint_set_interval(PyObject* self, PyObject* value, 
                                void* NPY_UNUSED(closure)) {
    flint* f = &(((PyFlint*) self)->obval);
    PyObject *ob;
    // Confirm it's not empty
    if (value == NULL) {
        PyErr_SetString(PyExc_ValueError, "Cannot set interval from empty value");
        return -1;
    }
    // Confirm its a sequence of length 2 or 3
    if (!PySequence_Check(value) && 
        !(PySequence_Size(value) == 2) && 
        !(PySequence_Size(value) == 3)) {
        PyErr_SetString(PyExc_ValueError, "The interval must be a sequence of length 2 or 3");
        return -1;
    }
    // Get the first element - that's our a value
    ob = PyNumber_Float(PySequence_GetItem(value, 0));
    if (ob == NULL) {
        PyErr_SetString(PyExc_ValueError, "Values must be numeric types");
    }
    f->a = PyFloat_AsDouble(ob);
    Py_DECREF(ob);
    // Get the second element - that's are b value
    ob = PyNumber_Float(PySequence_GetItem(value, 1));
    if (ob == NULL) {
        PyErr_SetString(PyExc_ValueError, "Values must be numeric types");
    }
    f->b = PyFloat_AsDouble(ob);
    Py_DECREF(ob);
    // Calculate or get the v value
    if (PySequence_Size(value) == 2) {
        f->v = 0.5*(f->a+f->b);
    } else {
        ob = PyNumber_Float(PySequence_GetItem(value, 2));
        if (ob == NULL) {
            PyErr_SetString(PyExc_ValueError, "Values must be numeric types");
        }
        f->v = PyFloat_AsDouble(ob);
        Py_DECREF(ob);
    }
    return 0;
}

// -----------------------------------------
// ---- Flint property table definition ----
// -----------------------------------------
/// @brief Defines the properties with getters or setters for flints
/// The structure is {"name", getter, setter, "description", NULL}
PyGetSetDef pyflint_getset[] = {
    {"eps", pyflint_get_eps, NULL,
    "The size of the interval (b-a)", NULL},
    {"interval", pyflint_get_interval, pyflint_set_interval,
    "The interval as a tuple (a,b) or (a,b,v)"},
    //sentinal
    {NULL, NULL, NULL, NULL, NULL}
};


// ------------------------------------------
// ---- Flint custom type implementation ----
// ------------------------------------------
/// @brief The Custom type structure for the new Flint object
static PyTypeObject PyFlint_Type = {
    PyVarObject_HEAD_INIT(NULL, 0) // PyObject_VAR_HEAD
    .tp_name = "flint", // const char *tp_name; /* For printing, in format "<module>.<name>" */
    .tp_basicsize = sizeof(PyFlint), //Py_ssize_t tp_basicsize, tp_itemsize; /* For allocation */
    .tp_repr = pyflint_repr, // reprfunc tp_repr;
    .tp_as_number = &pyflint_as_number, // PyNumberMethods *tp_as_number;
    .tp_hash = pyflint_hash, // hashfunc tp_hash;
    .tp_str = pyflint_str, // reprfunc tp_str;
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // unsigned long tp_flags; /* Flags to define presence of optional/expanded features */
    // const char *tp_doc; /* Documentation string */
    .tp_richcompare = pyflint_richcomp, // richcmpfunc tp_richcompare;
    /* Attribute descriptor and subclassing stuff */
    .tp_methods = pyflint_methods, // struct PyMethodDef *tp_methods;
    .tp_members = pyflint_members, // struct PyMemberDef *tp_members;
    .tp_getset = pyflint_getset, // struct PyGetSetDef *tp_getset;
    // struct _typeobject *tp_base;
    .tp_init = pyflint_init, // initproc tp_init;
    .tp_new = pyflint_new, //newfunc tp_new;
    // unsigned int tp_version_tag;
};


// ##########################################
// ---- End of standard Python Extension ----
// ##########################################

// #######################
// ---- NumPy support ----
// #######################

// -------------------------------------
// ---- NumPy NewType Array Methods ----
// -------------------------------------
/// @brief Get an flint element from a numpy array
/// @param data A pointer into the numpy array at the proper location
/// @param arr A pointer to the full array
/// @return A python object representing the data element from the numpy array
static PyObject* npyflint_getitem(void* data, void* arr) {
    flint f;
    memcpy(&f, data, sizeof(flint));
    return PyFlint_FromFlint(f);
}

/// @brief Set an element in a numpy array
/// @param item The python object to set the data-element to
/// @param data A pointer into the nummy array at the proper location
/// @param arr A pointer to the full array
/// @return 0 on success -1 on failure
static int npyflint_setitem(PyObject* item, void* data, void* arr) {
    flint f = {0.0, 0.0, 0.0};
    PyObject* D = {0};
    if (PyFlint_Check(item)) {
        f = ((PyFlint*) item)->obval;
    } else {
        D = PyNumber_Float(item);
        if (D == NULL) {
            PyErr_SetString(PyExc_TypeError,
                "expected flint or numeric type.");
            return -1;
        }
        f = double_to_flint(PyFloat_AsDouble(D));
        Py_DECREF(D);
    }
    memcpy(data, &f, sizeof(flint));
    return 0;
}

/// @brief Copy an element of an ndarray from src to dst, possibly swapping
///        Utilizes the existing copyswap function for doubles
/// @param dst A pointer to the destination
/// @param src A pointer to the source
/// @param swap A flag to swap data, or simply copy
/// @param arr A pointer to the full array
static void npyflint_copyswap(void* dst, void* src, int swap, void* arr) {
    // Get a pointer to the array description for doubles
    PyArray_Descr* descr = PyArray_DescrFromType(NPY_DOUBLE);
    // Call the double copyswap rountine for an flint sized array (3) 
    descr->f->copyswapn(dst, sizeof(double), src, sizeof(double), 
                        sizeof(flint)/sizeof(double), swap, arr);
    Py_DECREF(descr);
}

/// @brief Copy a section of an ndarray from src to dst, possibly swapping
///        Utilizes the existing copyswap function for doubles
/// @param dst A pointer to the destination
/// @param dstride The number of bytes between entries in the destination array
/// @param src A pointer to the source
/// @param sstride The number of bytes between entries in the source array
/// @param n The number of elements to copy
/// @param swap A flag to swap data, or simply copy
/// @param arr A pointer to the full array
static void npyflint_copyswapn(void* dst, npy_intp dstride,
                               void* src, npy_intp sstride,
                               npy_intp n, int swap, void* arr) {
    // Cast the destination and source points into flint type
    flint* _dst = (flint*) dst;
    flint* _src = (flint*) src;
    // Grab a copy of the 
    PyArray_Descr* descr = PyArray_DescrFromType(NPY_DOUBLE);
    // If the stride is represents a contiguous array do a single call
    if (dstride == sizeof(flint) && sstride == sizeof(flint)) {
        descr->f->copyswapn(dst, sizeof(double), src, sizeof(double), 
                            n*sizeof(flint)/sizeof(double), swap, arr);
    } else {
        // Else we make a call for each double in the struct
        descr->f->copyswapn(&(_dst->a), dstride, &(_src->a), sstride, 
                            n, swap, arr);
        descr->f->copyswapn(&(_dst->b), dstride, &(_src->b), sstride, 
                            n, swap, arr);
        descr->f->copyswapn(&(_dst->v), dstride, &(_src->v), sstride, 
                            n, swap, arr);
    }
    Py_DECREF(descr);
}

/// @brief Check if an element of a numpy array is zero
/// @param data a pointer to the element in a numpy array
/// @param arr a pointer to the full array
/// @return NPY_TRUE if zero, NPY_FALSE otherwise
///
/// Note: Because the we've defined overlap as equal, we can think of two
/// different forms of equal zero - one: all zero bits in the flint object, two:
/// the flint has non-zero elements but still overlaps with zero. In this case
/// I've decided to use the first definition.
static npy_bool npyflint_nonzero(void* data, void* arr) {
    flint f = {0.0, 0.0, 0.0};
    memcpy(data, &f, sizeof(flint));
    return (f.a==0.0 && f.b==0.0 && f.v==0.0)?NPY_FALSE:NPY_TRUE;
    // return flint_nonzero(f)?NPY_TRUE:NPY_FALSE;
}

/// @brief Compare two elements of a numpy array
/// @param d1 A pointer to the first element
/// @param d1 A pointer to the second element
/// @param arr A pointer to the array
/// @return 1 if *d1 > *d2, 0 if *d1 == *d2, -1 if *d1 < d2*
static int npyflint_compare(const void* d1, const void* d2, void* arr) {
    int ret;
    flint fp1 = *((flint*) d1);
    flint fp2 = *((flint*) d2);
    npy_bool dnan1 = flint_isnan(fp1);
    npy_bool dnan2 = flint_isnan(fp2);
    if (dnan1) {
        ret = dnan2 ? 0 : -1;
    } else if (dnan2) {
        ret = 1;
    } else if (fp1.b < fp2.a) {
        ret = -1;
    } else if (fp1.a > fp2.b) {
        ret = 1;
    } else {
        ret = 0;
    }
    return ret;
}

/// @brief Find the index of the max element of the array
/// @param data A pointer to the first elemetn in the array to check
/// @param n The number of elements to check
/// @param max_ind A pointer to an int, the max will be written here
/// @return Always returns 0;
///
/// Note: Since the comparisons with flints is inexact, I've chose to use this
/// to find the index of the flint with the largest upper limit.
static int npyflint_argmax(void* data, npy_intp n, 
                           npy_intp* max_ind, void* arr) {
    if (n==0) {
        return 0;
    }
    flint* fdata = (flint*) data;
    npy_intp i = 0;
    double max = fdata[i].b;
    *max_ind = 0;
    for (i=0; i<n; i++) {
        if (fdata[i].b > max) {
            max = fdata[i].b;
            *max_ind = i;
        }
    }
    return 0;
}

/// @brief Find the index of the min element of the array
/// @param data A pointer to the first elemetn in the array to check
/// @param n The number of elements to check
/// @param min_ind A pointer to an int, the min will be written here
/// @return Always returns 0;
///
/// Note: Since the comparisons with flints is inexact, I've chose to use this
/// to find the index of the flint with the smallest lower limit.
static int npyflint_argmin(void* data, npy_intp n, 
                           npy_intp* min_ind, void* arr) {
    if (n==0) {
        return 0;
    }
    flint* fdata = (flint*) data;
    npy_intp i = 0;
    double min = fdata[i].b;
    *min_ind = 0;
    for (i=0; i<n; i++) {
        if (fdata[i].b < min) {
            min = fdata[i].b;
            *min_ind = i;
        }
    }
    return 0;
}

/// @brief Compute the dot product between two arrays of flint
/// @param d1 A pointer to the first element of the first array
/// @param s1 A distance between data element of the first array in bytes
/// @param d2 A pointer to the first element of the second array
/// @param s1 A distance between data element of the second array inbytes
/// @param res A pointer to a flint, will hold the result
/// @param n The number of elements to use in calcuating the dot product
/// @param arr A pointer to the full array? (even for two arrays?)
static void npyflint_dotfunc(void* d1, npy_intp s1,
                             void* d2, npy_intp s2, 
                             void* res, npy_intp n, void* arr) {
    uint8_t* fp1 = (uint8_t*) d1;
    uint8_t* fp2 = (uint8_t*) d2;
    flint _fres = {0.0, 0.0, 0.0};
    flint* fres = (flint*) res;
    npy_intp i = 0;
    for (i=0; i<n; i++) {
        flint_inplace_add(
            &_fres, 
            flint_multiply(
                *((flint*) fp1), 
                *((flint*) fp2)
            )
        );
        fp1 += s1;
        fp2 += s2;
    }
    *fres = _fres;
}

/// @brief Fill an array based on it's first two elements
/// @param data A pointer to the first element
/// @param n The number of element to fill in
/// @param arr A pointer to the full array
/// @return ??
static int npyflint_fill(void* data, npy_intp n, void* arr) {
    if ( n < 2) {
        return 0;
    }
    flint* fp = (flint*) data;
    flint delta = flint_subtract(fp[1], fp[0]);
    npy_intp i = 2;
    for( i=2; i<n; i++) {
        fp[i] = flint_add(fp[0], flint_multiply_scalar(delta, (double) i));
    }
    return 0;
}

/// @brief Fill an array with a single flint value
/// @param buffer A pointer to the first element to fill in
/// @param n The number of flints to fill in
/// @param elem A pointer to the flint value to copy over
/// @param arr A pointer to the full array
static int npyflint_fillwithscalar(void* buffer, npy_intp n, 
                                    void* elem, void* arr) {
    flint* fp = (flint*) buffer;
    flint f = *((flint*) elem);
    npy_intp i;
    for (i=0; i<n; i++) {
        fp[i] = f;
    }
    return 0;
}

// --------------------------------
// ---- dtype to dtype casting ----
// --------------------------------
// The section contains all the casting functions between the new type and 18
// existing numpy types. They all have the same signature (enforced by numpy)
// and naming convention (chosen by me) `npycase_sourcetype_desttype`
// ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
// ---- dtype to flint casting ----
// ````````````````````````````````
// For the most part this section uses the existing c casting from numeric types
// to double.
/// @brief A macro to conversions from real scalar type to flint
/// @param npy_dtype_num The NumPy dtype number
#define SCALAR_TO_FLINT(npy_dtype_num, type) \
static void npycast_##type##_flint(void* from, void* to, npy_intp n, \
                                   void* fromarr, void* toarr) { \
    type* _from = (type*) from; \
    flint* _to = (flint*) to; \
    npy_intp i = 0; \
    for (i=0; i<n; i++) { \
        _to[i] = double_to_flint(((double) _from[i])); \
    } \
}
/// @brief A macro to conversions from complex type to flint
/// @param npy_dtype_num The NumPy dtype number
#define COMPLEX_TO_FLINT(npy_ctype_num, ctype) \
static void npycast_##ctype##_flint(void* from, void* to, npy_intp n, \
                                    void* fromarr, void* toarr) { \
    ctype* _from = (ctype*) from; \
    flint* _to = (flint*) to; \
    npy_intp i = 0; \
    for (i=0; i<n; i++) { \
        _to[i] = double_to_flint(((double) _from[i].real)); \
    } \
}
// All integers
SCALAR_TO_FLINT(NPY_BOOL, npy_bool)
SCALAR_TO_FLINT(NPY_BYTE, npy_byte)
SCALAR_TO_FLINT(NPY_SHORT, npy_short)
SCALAR_TO_FLINT(NPY_INT, npy_int)
SCALAR_TO_FLINT(NPY_LONG, npy_long)
SCALAR_TO_FLINT(NPY_LONGLONG, npy_longlong)
SCALAR_TO_FLINT(NPY_UBYTE, npy_ubyte)
SCALAR_TO_FLINT(NPY_USHORT, npy_ushort)
SCALAR_TO_FLINT(NPY_UINT, npy_uint)
SCALAR_TO_FLINT(NPY_ULONG, npy_ulong)
SCALAR_TO_FLINT(NPY_ULONGLONG, npy_ulonglong)
// real floating points
// static void npycast_npy_half_flint(void* from, void* to, npy_intp n,
//                                    void* fromarr, void* toarr) {
//     npy_half* _from = (npy_half*) from;
//     flint* _to = (flint*) to;
//     npy_half h = 0.0;
//     flint f = {0.0, 0.0, 0.0};
//     npy_intp i = 0;
//     for (i=0; i<n; i++) {
//         h = _from[i];
//         f.a = npy_half_to_double(npy_half_nextafter(h, NPY_HALF_NINF));
//         f.b = npy_half_to_double(npy_half_nextafter(h, NPY_HALF_PINF));
//         f.v = npy_half_to_double(h);
//         _to[i] = f;
//     }
// }
static void npycast_npy_float_flint(void* from, void* to, npy_intp n,
                                    void* fromarr, void* toarr) {
    npy_float* _from = (npy_float*) from;
    flint* _to = (flint*) to;
    npy_intp i = 0;
    for (i=0; i<n; i++) {
        _to[i] = float_to_flint(_from[i]);
    }
}
SCALAR_TO_FLINT(NPY_DOUBLE, npy_double)
SCALAR_TO_FLINT(NPY_LONGDOUBLE, npy_longdouble)
// complex floatin points
static void npycast_npy_cfloat_flint(void* from, void* to, npy_intp n,
                                     void* fromarr, void* toarr) {
    npy_cfloat* _from = (npy_cfloat*) from;
    flint* _to = (flint*) to;
    npy_intp i = 0;
    for (i=0; i<n; i++) {
        _to[i] = float_to_flint(_from[i].real);
    }
}
COMPLEX_TO_FLINT(NPY_CDOUBLE, npy_cdouble)
COMPLEX_TO_FLINT(NPY_CLONGDOUBLE, npy_clongdouble)
// ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
// ---- dtype to flint casting ----
// ````````````````````````````````
// This section use's the v value and then the pre-defined numpy rules to for
// doubles
#define FLINT_TO_TYPE(npy_type_num, type) \
static void npycast_flint_##type(void* src, void* dst, npy_intp n, \
                                 void* srcarr, void* dstarr) { \
    PyArray_Descr* descr = PyArray_DescrFromType(NPY_DOUBLE); \
    flint* _src = (flint*) src; \
    type* _dst = (type*) dst; \
    npy_intp i = 0; \
    for (i=0; i<n; i++) { \
        descr->f->cast[npy_type_num](&(_src[i].v), &(_dst[i]), n, NULL, NULL); \
    } \
    Py_DECREF(descr); \
}
FLINT_TO_TYPE(NPY_BOOL, npy_bool)
FLINT_TO_TYPE(NPY_BYTE, npy_byte)
FLINT_TO_TYPE(NPY_SHORT, npy_short)
FLINT_TO_TYPE(NPY_INT, npy_int)
FLINT_TO_TYPE(NPY_LONG, npy_long)
FLINT_TO_TYPE(NPY_LONGLONG, npy_longlong)
FLINT_TO_TYPE(NPY_UBYTE, npy_ubyte)
FLINT_TO_TYPE(NPY_USHORT, npy_ushort)
FLINT_TO_TYPE(NPY_UINT, npy_uint)
FLINT_TO_TYPE(NPY_ULONG, npy_ulong)
FLINT_TO_TYPE(NPY_ULONGLONG, npy_ulonglong)
//FLINT_TO_TYPE(NPY_HALF, npy_half)
FLINT_TO_TYPE(NPY_FLOAT, npy_float)
FLINT_TO_TYPE(NPY_DOUBLE, npy_double)
FLINT_TO_TYPE(NPY_LONGDOUBLE, npy_longdouble)
FLINT_TO_TYPE(NPY_CFLOAT, npy_cfloat)
FLINT_TO_TYPE(NPY_CDOUBLE, npy_cdouble)
FLINT_TO_TYPE(NPY_CLONGDOUBLE, npy_clongdouble)

/// @brief Macro to define the internal loop for a universal function
/// @param name The name of the function in c, Python and now NumPy
/// @param out_type the data type returned by the c function
#define NPYFLINT_UNARY_UFUNC(name, out_type) \
static void npyflint_ufunc_##name(char** args, const npy_intp* dim, \
                                  const npy_intp* std, void* data) { \
    char* in_ptr = args[0]; \
    char* out_ptr = args[1]; \
    npy_intp in_std = std[0]; \
    npy_intp out_std = std[1]; \
    npy_intp n = dim[0]; \
    npy_intp i = 0; \
    flint in_f = {0.0, 0.0, 0.0}; \
    for (i=0; i<n; i++) { \
        in_f = *((flint*) in_ptr); \
        *((out_type*) out_ptr) = flint_##name(in_f); \
        in_ptr += in_std; \
        out_ptr += out_std; \
    } \
}

/// @brief Macro to define the internal loop for a universal function
/// @param name The name of the function in c, Python and now NumPy
/// @param in0_type The data type for the first argumetn of the c function
/// @param in1_type The data type for the second argument of the c function
/// @param out_type the data type returned by the c function
#define NPYFLINT_BINARY_UFUNC(name, in0_type, in1_type, out_type) \
static void npyflint_ufunc_##name(char** args, const npy_intp* dim, \
                                  const npy_intp* std, void* data) { \
    char* in0_ptr = args[0]; \
    char* in1_ptr = args[1]; \
    char* out_ptr = args[2]; \
    npy_intp in0_std = std[0]; \
    npy_intp in1_std = std[1]; \
    npy_intp out_std = std[2]; \
    npy_intp n = dim[0]; \
    npy_intp i = 0; \
    in0_type in0_f = {0.0, 0.0, 0.0}; \
    in1_type in1_f = {0.0, 0.0, 0.0}; \
    for (i=0; i<n; i++) { \
        in0_f = *((in0_type*) in0_ptr); \
        in1_f = *((in1_type*) in1_ptr); \
        *((out_type*) out_ptr) = flint_##name(in0_f, in1_f); \
        in0_ptr += in0_std; \
        in1_ptr += in1_std; \
        out_ptr += out_std; \
    } \
}
// Arithmetic
NPYFLINT_UNARY_UFUNC(negative, flint)
NPYFLINT_UNARY_UFUNC(positive, flint)
NPYFLINT_BINARY_UFUNC(add, flint, flint, flint)
NPYFLINT_BINARY_UFUNC(subtract, flint, flint, flint)
NPYFLINT_BINARY_UFUNC(multiply, flint, flint, flint)
NPYFLINT_BINARY_UFUNC(divide, flint, flint, flint)
NPYFLINT_BINARY_UFUNC(power, flint, flint, flint)
// Comparisons
NPYFLINT_BINARY_UFUNC(eq, flint, flint, npy_bool)
NPYFLINT_BINARY_UFUNC(ne, flint, flint, npy_bool)
NPYFLINT_BINARY_UFUNC(lt, flint, flint, npy_bool)
NPYFLINT_BINARY_UFUNC(le, flint, flint, npy_bool)
NPYFLINT_BINARY_UFUNC(gt, flint, flint, npy_bool)
NPYFLINT_BINARY_UFUNC(ge, flint, flint, npy_bool)
// elementary functions
NPYFLINT_UNARY_UFUNC(isnan, npy_bool)
NPYFLINT_UNARY_UFUNC(isinf, npy_bool)
NPYFLINT_UNARY_UFUNC(isfinite, npy_bool)
NPYFLINT_UNARY_UFUNC(absolute, flint)
NPYFLINT_UNARY_UFUNC(sqrt, flint)
NPYFLINT_UNARY_UFUNC(cbrt, flint)
NPYFLINT_BINARY_UFUNC(hypot, flint, flint, flint)
NPYFLINT_UNARY_UFUNC(exp, flint)
NPYFLINT_UNARY_UFUNC(exp2, flint)
NPYFLINT_UNARY_UFUNC(expm1, flint)
NPYFLINT_UNARY_UFUNC(log, flint)
NPYFLINT_UNARY_UFUNC(log10, flint)
NPYFLINT_UNARY_UFUNC(log2, flint)
NPYFLINT_UNARY_UFUNC(log1p, flint)
NPYFLINT_UNARY_UFUNC(sin, flint)
NPYFLINT_UNARY_UFUNC(cos, flint)
NPYFLINT_UNARY_UFUNC(tan, flint)
NPYFLINT_UNARY_UFUNC(asin, flint)
NPYFLINT_UNARY_UFUNC(acos, flint)
NPYFLINT_UNARY_UFUNC(atan, flint)
NPYFLINT_BINARY_UFUNC(atan2, flint, flint, flint)
NPYFLINT_UNARY_UFUNC(sinh, flint)
NPYFLINT_UNARY_UFUNC(cosh, flint)
NPYFLINT_UNARY_UFUNC(tanh, flint)
NPYFLINT_UNARY_UFUNC(asinh, flint)
NPYFLINT_UNARY_UFUNC(acosh, flint)
NPYFLINT_UNARY_UFUNC(atanh, flint)

/// @brief utility type to find alignment for the flint object
typedef struct {uint8_t c; flint f; } align_test;
/// @brief A NumPy object the holds pointers to required array methods
/// This gets fill in in the module initialization function below
static PyArray_ArrFuncs npyflint_arrfuncs; 
/// @brief A pointer to the Numpy array flint description type
/// This gets fill in in the module initialization function below
PyArray_Descr* npyflint_descr;

// ###########################
// ---- Module definition ----
// ###########################
/// @brief Struct with minimum needed components for the module definition
static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    .m_name = "numpy_flint",
    .m_doc = "Rounded floating point intervals (flints)",
    .m_size = -1
};

/// @brief The module initialization function
PyMODINIT_FUNC PyInit_numpy_flint(void) {
    PyObject* m;
    PyObject* numpy;
    PyObject* numpy_dict;
    PyArray_Descr* npyflint_descr;
    PyArray_Descr* from_descr;
    int arg_types[3];
    static void* PyFlint_API[2];
    PyObject* c_api_object;
    // Create the new module
    m = PyModule_Create(&moduledef);
    if (m==NULL) {
        PyErr_Print();
        PyErr_SetString(PyExc_SystemError, "Could not create flint module.");
        return NULL;
    }
    // Import and initialize numpy
    import_array();
    if (PyErr_Occurred()) {
        PyErr_Print();
        PyErr_SetString(PyExc_SystemError, "Could not initialize NumPy.");
        return NULL;
    }
    import_umath();
    if (PyErr_Occurred()) {
        PyErr_Print();
        PyErr_SetString(PyExc_SystemError, "Could not initialize NumPy/umath.");
    }
    numpy = PyImport_ImportModule("numpy");
    if (!numpy) {
        PyErr_Print();
        PyErr_SetString(PyExc_SystemError, "Could not import NumPy.");
        return NULL;
    }
    numpy_dict = PyModule_GetDict(numpy);
    if (!numpy_dict) {
        PyErr_Print();
        PyErr_SetString(PyExc_SystemError, "Could not access NumPy module __dict__.");
        return NULL;
    }

    // Finalize the PyFlint type by having it inherit from numpy arraytype
    PyFlint_Type.tp_base = &PyGenericArrType_Type;
    // Initialize flint type
    if (PyType_Ready(&PyFlint_Type) < 0) {
        PyErr_Print();
        PyErr_SetString(PyExc_SystemError, "Could not initialize flint type.");
        return NULL;
    }
    Py_INCREF(&PyFlint_Type);

    // Initialize the numpy data-type extension of the python type
    // Register standard arrayfuncs for numpy-flint
    PyArray_InitArrFuncs(&npyflint_arrfuncs);
    npyflint_arrfuncs.getitem = (PyArray_GetItemFunc*) npyflint_getitem; // PyArray_GetItemFunc *getitem;
    npyflint_arrfuncs.setitem = (PyArray_SetItemFunc*) npyflint_setitem; // PyArray_SetItemFunc *setitem;
    npyflint_arrfuncs.copyswapn = (PyArray_CopySwapNFunc*) npyflint_copyswapn; // PyArray_CopySwapNFunc *copyswapn;
    npyflint_arrfuncs.copyswap = (PyArray_CopySwapFunc*) npyflint_copyswap; // PyArray_CopySwapFunc *copyswap;
    npyflint_arrfuncs.compare = (PyArray_CompareFunc*) npyflint_compare; // PyArray_CompareFunc *compare;
    npyflint_arrfuncs.argmax = (PyArray_ArgFunc*) npyflint_argmax; // PyArray_ArgFunc *argmax;
    npyflint_arrfuncs.argmin = (PyArray_ArgFunc*) npyflint_argmin;
    npyflint_arrfuncs.dotfunc = (PyArray_DotFunc*) npyflint_dotfunc; // PyArray_DotFunc *dotfunc;
    npyflint_arrfuncs.nonzero = (PyArray_NonzeroFunc*) npyflint_nonzero; // PyArray_NonzeroFunc *nonzero;
    npyflint_arrfuncs.fill = (PyArray_FillFunc*) npyflint_fill; // PyArray_FillFunc *fill;
    npyflint_arrfuncs.fillwithscalar = (PyArray_FillWithScalarFunc*) npyflint_fillwithscalar; // PyArray_FillWithScalarFunc *fillwithscalar;
    // Register the componenet of the descr object for the flint
    npyflint_descr = PyObject_New(PyArray_Descr, &PyArrayDescr_Type);
    npyflint_descr->typeobj = &PyFlint_Type; // PyTypeObject *typeobj;
    npyflint_descr->kind = 'V'; // char kind;
    npyflint_descr->type = 'r'; // char type;
    npyflint_descr->byteorder = '='; // char byteorder;
    npyflint_descr->flags = NPY_NEEDS_PYAPI | NPY_USE_GETITEM | NPY_USE_SETITEM; // char flags;
    npyflint_descr->type_num = 0; // int type_num;
    npyflint_descr->elsize = sizeof(flint); // int elsize;
    npyflint_descr->alignment = offsetof(align_test, f); // int alignment;
    npyflint_descr->subarray = NULL; // PyArray_ArrayDescr *subarray;
    npyflint_descr->fields = NULL; // PyObject *fields;
    npyflint_descr->names = NULL; // PyObject *names;
    npyflint_descr->f = &npyflint_arrfuncs; // PyArray_ArrFuncs *f;
    npyflint_descr->metadata = NULL; // PyObject *metadata;
    npyflint_descr->c_metadata = NULL; // NpyAuxData *c_metadata;

    NPY_FLINT = PyArray_RegisterDataType(npyflint_descr);
    if (NPY_FLINT < 0) {
        PyErr_Print();
        PyErr_SetString(PyExc_SystemError, "Could not register flint type with numpy.");
        return NULL;
    }

    // Register all casting from flint to all
    #define REGISTER_CAST_FROM_FLINT(typenum, type) \
    PyArray_RegisterCastFunc(npyflint_descr, typenum, npycast_flint_##type);
    REGISTER_CAST_FROM_FLINT(NPY_BOOL, npy_bool)
    REGISTER_CAST_FROM_FLINT(NPY_BYTE, npy_byte)
    REGISTER_CAST_FROM_FLINT(NPY_SHORT, npy_short)
    REGISTER_CAST_FROM_FLINT(NPY_INT, npy_int)
    REGISTER_CAST_FROM_FLINT(NPY_LONG, npy_long)
    REGISTER_CAST_FROM_FLINT(NPY_LONGLONG, npy_longlong)
    REGISTER_CAST_FROM_FLINT(NPY_UBYTE, npy_ubyte)
    REGISTER_CAST_FROM_FLINT(NPY_USHORT, npy_ushort)
    REGISTER_CAST_FROM_FLINT(NPY_UINT, npy_uint)
    REGISTER_CAST_FROM_FLINT(NPY_ULONG, npy_ulong)
    REGISTER_CAST_FROM_FLINT(NPY_ULONGLONG, npy_ulonglong)
    // REGISTER_CAST_FROM_FLINT(NPY_HALF, npy_half)
    REGISTER_CAST_FROM_FLINT(NPY_FLOAT, npy_float)
    REGISTER_CAST_FROM_FLINT(NPY_DOUBLE, npy_double)
    REGISTER_CAST_FROM_FLINT(NPY_LONGDOUBLE, npy_longdouble)
    REGISTER_CAST_FROM_FLINT(NPY_CFLOAT, npy_cfloat)
    REGISTER_CAST_FROM_FLINT(NPY_CDOUBLE, npy_cdouble)
    REGISTER_CAST_FROM_FLINT(NPY_CLONGDOUBLE, npy_clongdouble)

    // Register casting from all to flint
    #define REGISTER_CAST_TO_FLINT(typenum, type) \
    from_descr = PyArray_DescrFromType(typenum); \
    PyArray_RegisterCastFunc(from_descr, NPY_FLINT, npycast_##type##_flint); \
    Py_DECREF(from_descr);
    REGISTER_CAST_TO_FLINT(NPY_BOOL, npy_bool)
    REGISTER_CAST_TO_FLINT(NPY_BYTE, npy_byte)
    REGISTER_CAST_TO_FLINT(NPY_SHORT, npy_short)
    REGISTER_CAST_TO_FLINT(NPY_INT, npy_int)
    REGISTER_CAST_TO_FLINT(NPY_LONG, npy_long)
    REGISTER_CAST_TO_FLINT(NPY_LONGLONG, npy_longlong)
    REGISTER_CAST_TO_FLINT(NPY_UBYTE, npy_ubyte)
    REGISTER_CAST_TO_FLINT(NPY_USHORT, npy_ushort)
    REGISTER_CAST_TO_FLINT(NPY_UINT, npy_uint)
    REGISTER_CAST_TO_FLINT(NPY_ULONG, npy_ulong)
    REGISTER_CAST_TO_FLINT(NPY_ULONGLONG, npy_ulonglong)
    // REGISTER_CAST_TO_FLINT(NPY_HALF, npy_half)
    REGISTER_CAST_TO_FLINT(NPY_FLOAT, npy_float)
    REGISTER_CAST_TO_FLINT(NPY_DOUBLE, npy_double)
    REGISTER_CAST_TO_FLINT(NPY_LONGDOUBLE, npy_longdouble)
    REGISTER_CAST_TO_FLINT(NPY_CFLOAT, npy_cfloat)
    REGISTER_CAST_TO_FLINT(NPY_CDOUBLE, npy_cdouble)
    REGISTER_CAST_TO_FLINT(NPY_CLONGDOUBLE, npy_clongdouble)
    // Registering with coersion rules
    // Anything with less precision can be coerced into a flint
    #define REGISTER_COERSION_FROM(typenum, type) \
    from_descr = PyArray_DescrFromType(typenum); \
    PyArray_RegisterCanCast(from_descr, NPY_FLINT, NPY_NOSCALAR); \
    Py_DECREF(from_descr);
    REGISTER_COERSION_FROM(NPY_BOOL, npy_bool)
    REGISTER_COERSION_FROM(NPY_BYTE, npy_byte)
    REGISTER_COERSION_FROM(NPY_SHORT, npy_short)
    REGISTER_COERSION_FROM(NPY_INT, npy_int)
    REGISTER_COERSION_FROM(NPY_LONG, npy_long)
    REGISTER_COERSION_FROM(NPY_LONGLONG, npy_longlong)
    REGISTER_COERSION_FROM(NPY_UBYTE, npy_ubyte)
    REGISTER_COERSION_FROM(NPY_USHORT, npy_ushort)
    REGISTER_COERSION_FROM(NPY_UINT, npy_uint)
    REGISTER_COERSION_FROM(NPY_ULONG, npy_ulong)
    REGISTER_COERSION_FROM(NPY_ULONGLONG, npy_ulonglong)
    // REGISTER_COERSION_FROM(NPY_HALF, npy_half)
    REGISTER_COERSION_FROM(NPY_FLOAT, npy_float)
    REGISTER_COERSION_FROM(NPY_DOUBLE, npy_double)
    // Small macro for registering methods that match name with existing numpy
    // functions
    #define REGISTER_UFUNC(npname, flname) \
    PyUFunc_RegisterLoopForType((PyUFuncObject*) PyDict_GetItemString(numpy_dict, #npname), \
                                NPY_FLINT, npyflint_ufunc_##flname, arg_types, NULL);
    // These are sorted by number and types of arguments and return value
    // flint -> bool
    arg_types[0] = NPY_FLINT;
    arg_types[1] = NPY_BOOL;
    REGISTER_UFUNC(isnan, isnan)
    REGISTER_UFUNC(isinf, isinf)
    REGISTER_UFUNC(isfinite, isfinite)
    // flint -> flint
    arg_types[0] = NPY_FLINT;
    arg_types[1] = NPY_FLINT;
    REGISTER_UFUNC(absolute, absolute)
    REGISTER_UFUNC(negative, negative)
    REGISTER_UFUNC(positive, positive)
    REGISTER_UFUNC(sqrt, sqrt)
    REGISTER_UFUNC(cbrt, cbrt)
    REGISTER_UFUNC(exp, exp)
    REGISTER_UFUNC(exp2, exp2)
    REGISTER_UFUNC(expm1, expm1)
    REGISTER_UFUNC(log, log)
    REGISTER_UFUNC(log10, log10)
    REGISTER_UFUNC(log2, log2)
    REGISTER_UFUNC(log1p, log1p)
    REGISTER_UFUNC(sin, sin)
    REGISTER_UFUNC(cos, cos)
    REGISTER_UFUNC(tan, tan)
    REGISTER_UFUNC(arcsin, asin)
    REGISTER_UFUNC(arccos, acos)
    REGISTER_UFUNC(arctan, atan)
    REGISTER_UFUNC(sinh, sinh)
    REGISTER_UFUNC(cosh, cosh)
    REGISTER_UFUNC(tanh, tanh)
    REGISTER_UFUNC(arcsinh, asinh)
    REGISTER_UFUNC(arccosh, acosh)
    REGISTER_UFUNC(arctanh, atanh)
    // flint, flint -> bool
    arg_types[0] = NPY_FLINT;
    arg_types[1] = NPY_FLINT;
    arg_types[2] = NPY_BOOL;
    REGISTER_UFUNC(equal, eq)
    REGISTER_UFUNC(not_equal, ne)
    REGISTER_UFUNC(less, lt)
    REGISTER_UFUNC(less_equal, le)
    REGISTER_UFUNC(greater, gt)
    REGISTER_UFUNC(greater_equal, ge)
    // flint, flint -> flint
    arg_types[0] = NPY_FLINT;
    arg_types[1] = NPY_FLINT;
    arg_types[2] = NPY_FLINT;
    REGISTER_UFUNC(add, add)
    REGISTER_UFUNC(subtract, subtract)
    REGISTER_UFUNC(multiply, multiply)
    REGISTER_UFUNC(true_divide, divide)
    REGISTER_UFUNC(power, power)
    REGISTER_UFUNC(hypot, hypot)
    REGISTER_UFUNC(arctan2, atan2)
    // Finally register the new type with the module
    if (PyModule_AddObject(m, "flint", (PyObject *) &PyFlint_Type) < 0) {
        Py_DECREF(&PyFlint_Type);
        Py_DECREF(m);
        PyErr_Print();
        PyErr_SetString(PyExc_SystemError, "Could not add numpy_flint.flint type to module flint.");
        return NULL;
    }
    // Register PyFlint_Type and NPY_FLINT with the c api
    PyFlint_API[0] = (void*) get_pyflint_type;
    PyFlint_API[1] = (void*) get_npy_flint;
    c_api_object = PyCapsule_New((void*) PyFlint_API, "flint.numpy_flint.c_api", NULL);
    if (c_api_object == NULL) {
        Py_XDECREF(c_api_object);
        Py_DECREF(m);
        PyErr_Print();
        PyErr_SetString(PyExc_SystemError, "Could create the c_api_object.");
        return NULL;
    }
    if (PyModule_AddObject(m, "c_api", c_api_object) < 0) {
        Py_XDECREF(c_api_object);
        Py_DECREF(m);
        PyErr_Print();
        PyErr_SetString(PyExc_SystemError, "Could not add numpy_flint.c_api to flint module.");
        return NULL;
    }

    return m;
}

