/// @file numpy_flint.c Python/Numpy interface for flints
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

#include <Python.h>
#include <numpy/arrayobject.h>
#include <numpy/npy_math.h>
#include <numpy/ufuncobject.h>
#include "structmember.h"

#include "flint.h"

// Define a new python object that contains a flint object as a member
typedef struct {
    PyObject_HEAD
    flint obval;
} PyFlint;

// Define a type for the flints
static PyTypeObject PyFlint_Type;

// Type check for flints
state NPY_INLINE int PyFlint_Check(PyObject* ob) {
    return PyObject_IsInstance(ob, (PyObject*) &PyFlint_Type);
}
// From flint -> PyFlint
static NPY_INLINE PyObject* PyFlint_FromFlint(flint f) {
    PyFlint* p = (PyFlint*) PyFlint_Type.tp_alloc(&PyFlint_Type, 0);
    if (p) {
        p->obval = f;
    }
    return (PyObject*) p;
}

// PyFlint object allocating constructor (the __new__ method in python)
static PyObject* pyflint_new(PyTypeObject* type, 
                             PyObject* NPY_UNUSED(args),
                             PyObject* NPY_UNUSED(kwargs)) {
    PyFlint* self = (PyFlint*) type->tp_alloc(type, 0);
    return (PyObject*) self;
}

// PyFlint object initiallizing constructor (the __init__ method in python)
static int pyflint_init(PyObject* self, PyObject* args, PyObject* kwargs) {
    Py_ssize_t size = PyTuple_Size(args);
    PyObject* F = {0};
    flint* fp = &(((PyFlint*) self)->obval);
    double d;
    long long n;

    if (kwargs && PyDictSize(kwargs)) {
        PyErr_SetString(PyExc_TypeError,
                        "flint constructor doesn't take keyword arguments");
        return -1;
    }

    if (size == 1) {
        // One argument of a PyFlint (copy constructor)
        if (PyArg_ParseTuple(args, "O", &F), PyFlint_Check(F)) {
            *fp = ((PyFlint*) F)->obval;
            return 0;
        }
        // One argument of a floating type (standard constructor)
        else if (PyArg_ParseTuple(args, "d", &d)) {
            *fp = double_to_flint(d);
            return 0;
        }
        // One argument of an integer type (standard constructor) 
        else if (PyArg_ParseTuple(args, "L", &n)) {
            d = (double) n;
            *fp = double_to_float(d);
            return 0;
        }
    }

    PyErr_SetString(PyExc_TypeError,
                    "flint constructor one numeric argument");
    return -1;
}

// Multiline macro for a checking if object is PyFlint, 
// then grabbing the flint from inside
#define PyFlint_CheckedGetFlint(f, F) \
    if (PyFlint_Check(F)) { \
        f = ((PyFlint*) F)->obval; \
    } else { \
        PyErr_SetString(PyExc_TypeError, "Input object is not PyFlint"); \
        return NULL; \
    } \

// Multiline macro for all unary function that return a bool
#define UNARY_BOOL_RETURNER(name) \
static PyObject* pyflint_##name(PyObject* a, PyObject* NPY_UNUSED(b)) { \
    flint f = {0.0, 0.0, 0.0}; \
    PyFlint_CheckedGetFlint(f, a); \
    return PyBool_FromLong(flint_##name(f)); \
}
// all the floating point special value queries
UNARY_BOOL_RETURNER(isnonzero)
UNARY_BOOL_RETURNER(isnan)
UNARY_BOOL_RETURNER(isinf)
UNARY_BOOL_RETURNER(isfinite)
// We're done with the macro now
#undef UNARY_BOOL_RETURNER

// Multiline macro for all binary function that return a bool
#define BINARY_BOOL_RETURNER(name) \
static PyObject* pyflint_##name(PyObject* a, PyObject* b) { \
    flint f = {0.0, 0.0, 0.0}; \
    double d = 0.0; \
    PyObject* D = {0}; \
    if (PyFlint_Check(a)) { \
        if (PyFlint_Check(b)) {\ 
            return PyBool_FromLong(flint_##name(a->obval,b->obval)); \
        } else { \
            D = PyNumber_Float(b); \
            if (D) { \
                d = PyFloat_AsDouble(D); \
                f = double_to_flint(d); \
                return PyBool_FromLong(flint_##name(a->obval, f)); \
            } \
        } \
    } else { \
        D = PyNumber_Float(a); \
        if (D) { \
            d = PyFloat_AsDouble(D); \
            f = double_to_flint(d); \
            return PyBool_FromLong(flint_##(f, b->obval)); \
        } \
    } \
    PyErr_SetString(PyExc_TypeError, "Comparison with PyFlint must be with numeric type"); \
    return NULL; \
}
// All the comparison operators
BINARY_BOOL_RETURNER(equal)
BINARY_BOOL_RETURNER(not_equal)
BINARY_BOOL_RETURNER(less_equal)
BINARY_BOOL_RETURNER(less)
BINARY_BOOL_RETURNER(greater_equal)
BINARY_BOOL_RETURNER(greater)
// We're done with the macro now
#undef BINARY_BOOL_RETURNER

// Multiline macro for all unary functions that return a flint
#define UNARY_FLINT_RETURNER(name) \
static PyObject* pyflint_##name(PyObject* a, PyObject* NPY_UNUSED(b)) {
    flint f = {0.0, 0.0, 0.0}; \
    PyFlint_CheckedGetFlint(f, a); \
    return PyFlint_FromFlint(flint_##name(f)); \
}
// Negation for arithmetic and some elementary math functions
UNARY_FLINT_RETURNER(negative)
UNARY_FLINT_RETURNER(absolute)
UNARY_FLINT_RETURNER(sqrt)
UNARY_FLINT_RETURNER(log)
UNARY_FLINT_RETURNER(exp)
//
#undef UNARY_FLINT_RETURNER

// Multiline macro for all binary function that return a flint
#define BINARY_FLINT_RETURNER(name) \
static PyObject* pyflint_##name(PyObject* a, PyObject* b) { \
    flint f = {0.0, 0.0, 0.0}; \
    double d = 0.0; \
    PyObject* D = {0}; \
    if (PyFlint_Check(a)) { \
        if (PyFlint_Check(b)) {\ 
            return PyFlint_FromFlint(flint_##name(a->obval,b->obval)); \
        } else { \
            D = PyNumber_Float(b); \
            if (D) { \
                d = PyFloat_AsDouble(D); \
                f = double_to_flint(d); \
                return PyFlint_FromFlint(flint_##name(a->obval, f)); \
            } \
        } \
    } else { \
        D = PyNumber_Float(a); \
        if (D) { \
            d = PyFloat_AsDouble(D); \
            f = double_to_flint(d); \
            return PyFlint_FromFlint(flint_##(f, b->obval)); \
        } \
    } \
    PyErr_SetString(PyExc_TypeError, "+,-,*,/,** operations with PyFlint must be with numeric type"); \
    return NULL; \
}
// All binary arithmetic and power function
BINARY_FLINT_RETURNER(add)
BINARY_FLINT_RETURNER(subtract)
BINARY_FLINT_RETURNER(multiply)
BINARY_FLINT_RETURNER(divide)
BINARY_FLINT_RETURNER(power)

// Multiline macro for all binary function that return a flint
#define BINARY_FLINT_INPLACE(name) \
static PyObject* pyflint_inplace_##name(PyObject* a, PyObject* b) { \
    flint f = {0.0, 0.0, 0.0}; \
    double d = 0.0; \
    PyObject* D = {0}; \
    if (PyFlint_Check(a)) { \
        if (PyFlint_Check(b)) {\ 
            flint_inplace_##name(&(a->obval),b->obval); \
            Py_INCREF(a); \
            return a; \
        } else { \
            D = PyNumber_Float(b); \
            if (D) { \
                d = PyFloat_AsDouble(D); \
                f = double_to_flint(d); \
                flint_inplace_##name(&(a->obval), f); \
                Py_INCREF(a); \
                return a; \
            } \
        } \
    } else { \
        D = PyNumber_Float(a); \
        if (D) { \            
            d = PyFloat_AsDouble(D); \
            f = double_to_flint(d); \
            a = PyFlint_FromFlint(flint_##(f, b->obval)); \
            return a; \
        } \
    } \
    PyErr_SetString(PyExc_TypeError, "+=,-=,*=,/= inplace operations with PyFlint must be with numeric type"); \
    return NULL; \
}
// Arithmetic operators
BINARY_FLINT_INPLACE(add)
BINARY_FLINT_INPLACE(subtract)
BINARY_FLINT_INPLACE(multiply)
BINARY_FLINT_INPLACE(divide)
// we're done with that macro
#undef BINARY_FLINT_INPLACE

// // Define the flint member variables
// PyMemberDef PyFlintArrType_members[] = {
//     {"a", T_DOUBLE, offset(PyFlintScalarObject, obval.a), READONLY,
//         "The lower bound of the floating point interval"},
//     {"b", T_DOUBLE, offset(PyFlintScalarObject, obval.b), READONLY,
//         "The upper bound of the floating point interval"},
//     {"v", T_DOUBLE, offset(PyFlintScalarObject, obval.v), READONLY,
//         "The tracked float value"},
//     {NULL}
// }

// // Define a getter for the interval
// static PyObject *
// PyFlintArrType_get_interval(PyObject *self, void *closure) {
//     flint *f = &(((PyFlintScalarObject *)self)->obval);
//     PyObject *tuple = PyTuple_New(2);
//     PyTuple_SET_ITEM(tuple, 0, PyFloat_FromDouble(f->a));
//     PyTuple_SET_ITEM(tuple, 1, PyFloat_FromDouble(f->b));
//     return tuple;
// }

// // --- Might not need this since we can just access the v member
// // // Define a getter for the value
// // static PyObject *
// // PyFlintArrType_get_value(PyObject *self, void *closure) {
// //     flint *f = &(((PyFlintScalarObject *)self)->obval);
// //     PyObject *val = PyFloat_FromDouble(f->v);
// //     return val;
// // }

// // Set the getters
// PyGetSetDef PyFlintArrType_getset[] = {
//     {"interval", PyFlintArrType_get_interval, NULL,
//     "The full interval of the flint object as a (a,b) tuple", NULL},
//     // {"value", PyFlintArrType_get_value, NULL,
//     // "The internal value of the flint object as a 64 bit float", NULL},
//     {NULL}
// };

// // Define the new python array type
// PyTypeObject PyFlintArrType_Type = {
//     PyVarObject_HEAD_INIT(NULL, 0)
//     .tp_name = "flint.flint",
//     .tp_basicsize = sizeof(PyFlintScalarObject),
//     .tp_members = PyFlintArrType_members,
//     .tp_getset = PyFlintArrType_getset,
//     .tp_flags = Py_TPFLAGS_DEFAULT,
//     .tp_new = PyType_GenericNew,
// };
