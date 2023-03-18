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

#include <Python.h>
#include <numpy/arrayobject.h>
#include <numpy/npy_math.h>
#include <numpy/ufuncobject.h>
#include "structmember.h"

#include "flint.h"

/// @brief A python flint object
/// @param obval The internal c representation of the flint object
typedef struct {
    PyObject_HEAD
    flint obval;
} PyFlint;

/// @brief The flint PyTypeObject
static PyTypeObject PyFlint_Type;

/// @brief The array of member of the flint object
PyMemberDef PyFlint_members[] = {
    {"a", T_DOUBLE, offset(PyFlint, obval.a), READONLY,
        "The lower bound of the floating point interval"},
    {"b", T_DOUBLE, offset(PyFlint, obval.b), READONLY,
        "The upper bound of the floating point interval"},
    {"v", T_DOUBLE, offset(PyFlint, obval.v), READONLY,
        "The tracked float value"},
    {NULL}
};

// ###############################
// ---- Convenience functions ----
// ###############################
// This section contains some convenience functions that are used by the method
// implementations below.

/// @brief Check if an object is a flint
/// @param ob The PyObject to check
/// @return 1 if the object is a flint, 0 otherwise
static NPY_INLINE int PyFlint_Check(PyObject* ob) {
    return PyObject_IsInstance(ob, (PyObject*) &PyFlint_Type);
}

/// @brief Create a PyFlint object from a c flint struct.
/// @param f The c flint struct
/// @return A new PyFlint object that contains a copy of f
static NPY_INLINE PyObject* PyFlint_FromFlint(flint f) {
    PyFlint* p = (PyFlint*) PyFlint_Type.tp_alloc(&PyFlint_Type, 0);
    if (p) {
        p->obval = f;
    }
    return (PyObject*) p;
}

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
    } \

/// @brief A macro that defines function of one variable that returns a bool
/// @param name the name of the function in the c and pyflint implementation
#define UNARY_BOOL_RETURNER(name) \
static PyObject* pyflint_##name(PyObject* a) { \
    flint f = {0.0, 0.0, 0.0}; \
    PyFlint_CheckedGetFlint(f, a); \
    return PyBool_FromLong(flint_##name(f)); \
}

/// @brief A macro that defines a functions of one variable that return a flint
/// @param name the name of the function in the c and pyflint implementation
#define UNARY_FLINT_RETURNER(name) \
static PyObject* pyflint_##name(PyObject* a) { \
    flint f = {0.0, 0.0, 0.0}; \
    PyFlint_CheckedGetFlint(f, a); \
    return PyFlint_FromFlint(flint_##name(f)); \
}

/// @brief A macro that defines functions of two variables that return a flint
/// @param name the name of the function in the c and pyflint implementation
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
            return PyFlint_FromFlint(flint_##name(f, b->obval)); \
        } \
    } \
    PyErr_SetString(PyExc_TypeError, "+,-,*,/,** operations with PyFlint must be with numeric type"); \
    return NULL; \
}

/// @brief A macro that defines an inplace operator
/// @param name the name of the operation in the c and pyflint implementation
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
            a = PyFlint_FromFlint(flint_##name(f, b->obval)); \
            return a; \
        } \
    } \
    PyErr_SetString(PyExc_TypeError, "+=,-=,*=,/= inplace operations with PyFlint must be with numeric type"); \
    return NULL; \
}


// #####################################################
// ---- Special Python Object Method Implementation ----
// #####################################################
// This section contains the 'special' dunder python implementation for
// for the flint object

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

/// @brief The __reduce__ method reproduces the internal structure of the flint 
///        struct as object as PyObjects
/// @return a Tuple with Type and a Tuple of the object members as PyObjects
static PyObject* pyflint_reduce(PyFlint* self) {
    return Py_BuildValue("O(OOO)", Py_TYPE(self),
                         PyFloat_FromDouble(self->obval.a),
                         PyFloat_FromDouble(self->obval.b),
                         PyFloat_FromDouble(self->obval.v));
}

/// @brief The __getstate__ method builds the data member as PyObjects
/// @param args A getstate flag
/// @return A Tuble of the object members as PyObjects
static PyObject* pyflint_getstate(PyFlint* self, PyObject* args) {
    if (!PyArg_ParseTuple(args, ":getstate")) {
        return NULL;
    }
    return PyBuildValue("OOO"
                         PyFloat_FromDouble(self->obval.a),
                         PyFloat_FromDouble(self->obval.b),
                         PyFloat_FromDouble(self->obval.v));
} 

/// @brief The __setstate__ reads in the data as pickled by __getstate__
/// @param args A Tuple of object members a PyObjects and a setstate flag
/// @return NULL on failure or None on success, The value of the `self` is 
///         set from the values read in from the args tuple 
state PyObject* pyflint_setstate(PyFlint* self, PyObject* args) {
    flint *f;
    f = &(self->obval);
    if (PyArg_ParseTuple(args, "ddd:setstate", &f->a, &f->b, &f->v)) {
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

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
    } else if { // otherwise try to cast into a float then a flint
        D = PyNumber_Float(b);
        if (!D) {
            PyErr_SetString(PyExc_TypeError, 
                "Comparison with PyFlint must be with numeric type");
            return NULL;
        }
        d = PyFloat_AsDouble(D);
        fo = double_to_flint(d);
    }
    switch (op) {
        case PY_EQ : {
            return PyBool_FromLong(pyflint_eq(f, fo));
        }
        case PY_NE : {
            return PyBool_FromLong(pyflint_ne(f, fo));
        }
        case PY_LT : {
            return PyBool_FromLong(pyflint_lt(f, fo));
        }
        case PY_LE : {
            return PyBool_FromLong(pyflint_le(f, fo));
        }
        case PY_GT : {
            return PyBool_FromLong(pyflint_gt(f, fo));
        }
        case PY_GE : {
            return PyBool_FromLong(pyflint_ge(f, fo));
        }
        default:
            PyErr_SetString(PyExc_TypeError, 
                "Supported comparison operators are ==, !=, <, <=, >, >=");
            return Py_NotImplemented;
    }
}

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
/// @brief The _pow_ or _rpow_ operator, evaluate a general power exponential
/// @param a The base
/// @param b The exponent
/// @return The a**b
static PyObject* pyflint_power(PyObject* a, PyObject* b, 
                               PyObject* NPY_UNUSED(c)) {
    flint f = {0.0, 0.0, 0.0};
    double d = 0.0;
    PyObject* D = {0};
    if (PyFlint_Check(a)) {
        if (PyFlint_Check(b)) {\ 
            return PyFlint_FromFlint(flint_power(a->obval,b->obval));
        } else {
            D = PyNumber_Float(b);
            if (D) {
                d = PyFloat_AsDouble(D);
                f = double_to_flint(d);
                return PyFlint_FromFlint(flint_power(a->obval, f));
            }
        }
    } else {
        D = PyNumber_Float(a);
        if (D) {
            d = PyFloat_AsDouble(D);
            f = double_to_flint(d);
            return PyFlint_FromFlint(flint_power(f, b->obval));
        }
    }
    PyErr_SetString(PyExc_TypeError, "The ** operations with a PyFlint must be with numeric type");
    return NULL;
}

/// @brief The array of math dunder methods for the pyflint objects
/// This array contains pointers to the arithmetic operators
static PyNumberMethods pyflint_as_number = {
    .nb_add = pyflint_add, // binaryfunc nb_add;
    .nb_subtract = pyflint_subtract, // binaryfunc nb_subtract;
    .nb_multiply = pyflint_multiply, // binaryfunc nb_multiply;
    .nb_power = pyflint_power, // ternaryfunc nb_power;
    .nb_negative = pyflint_negative, // unaryfunc nb_negative;
    .nb_positive = pyflint_positive, // unaryfunc nb_positive;
    .nb_absolute = pyflint_absolute, // unaryfunc nb_absolute;
    .nb_inplace_add = pyflint_inplace_add, // binaryfunc nb_inplace_add;
    .nb_inplace_subtract = pyflint_inplace_subtract, // binaryfunc nb_inplace_subtract;
    .nb_inplace_multiply = pyflint_inplace_multiply, // binaryfunc nb_inplace_multiply;
    .np_true_divide = pyflint_divide, // binaryfunc nb_true_divide;
    .nb_inplace_true_divide = pyflint_inplace_divide, // binaryfunc nb_inplace_true_divide;
};

/// @brief Query if a PyFlint interval contains zero
/// @param a The PyFlint object
/// @return Py_True if a != 0 otherwise Py_False
UNARY_BOOL_RETURNER(isnonzero)
/// @brief Query if the PyFlint is NaN
/// @param a The PyFlint object
/// @return Py_True if a is NaN otherwise Py_False
UNARY_BOOL_RETURNER(isnan)
/// @brief Query if the PyFlint interval stretches to infinity
/// @param a The PyFlint object
/// @return Py_True if a is +/- infinity otherwise Py_False
UNARY_BOOL_RETURNER(isinf)
/// @brief Query if the PyFlint interval is finite
/// @param a The PyFlint object
/// @return Py_False if a is NaN or +/- infinity otherwise Py_True
UNARY_BOOL_RETURNER(isfinite)
/// @brief Evaluate the square root of the interval
/// @param a The PyFlint object
/// @return The square root of the interval if a >= 0 else NaN
UNARY_FLINT_RETURNER(sqrt)
/// @brief Evaluate the natural log of the interval
/// @param a The PyFlint object
/// @return The log of the interval if a >= 0 else NaN
UNARY_FLINT_RETURNER(log)
/// @brief Evaluate the exponential of the interval
/// @param a The PyFlint object
/// @return The exponential of the interval
UNARY_FLINT_RETURNER(exp)

/// @brief A PyMethodDef 
/// Defines the flint methods accessible in python the structure is
/// (name, function, ARGTYPE_MACRO, description)
PyMethodDef pyflint_methods[] = {
    // methods for querying special float values
    {"isnonzero", pyflint_isnonzero, METH_NOARGS,
    "True if the interval does not inersect zero"},
    {"isnan", pyflint_isnan, METH_NOARGS,
    "True if the flint contains NaN components"},
    {"isinf", pyflint_isinf, METH_NOARGS,
    "True if the interval extends to +/-infinity"},
    {"isfinite", pyflint_isfinite, METH_NOARGS,
    "True if the interval has covers a finite range"},
    {"sqrt", pyflint_sqrt, METH_NOARGS,
    "Evaluate the square root of the interval"},
    {"log", pyflint_log, METH_NOARGS,
    "Evaluate the natural log of the interval"},
    {"exp", pyflint_exp, METH_NOARGS,
    "Evaluate the exponential func of an interval"},
    // sentinel
    {NULL, NULL, 0, NULL}
};

// Define a getter for the interval
static PyObject *
PyFlint_get_interval(PyObject *self, void *closure) {
    flint *f = &(((PyFlint*) self)->obval);
    PyObject *tuple = PyTuple_New(2);
    PyTuple_SET_ITEM(tuple, 0, PyFloat_FromDouble(f->a));
    PyTuple_SET_ITEM(tuple, 1, PyFloat_FromDouble(f->b));
    return tuple;
}


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
