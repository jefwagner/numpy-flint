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
        "+,-,*,/,** operations with PyFlint must be with numeric type"); \
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
static NPY_INLINE PyObject* pyflint_b2t_##name(PyObject *a, PyObject* b, \
                                               PyObject* NPY_UNUSED(c)) { \
    return pyflint_##name(a,b); \
}

/// @brief A macro that wraps an inplace binary func into a tertiary func
/// @param name the name of the operation in the c and pyflint implementation
/// @return The result of the Python/C pyflint_inplace_{name} function
#define BINARY_TO_TERTIARY_INPLACE(name) \
static NPY_INLINE PyObject* pyflint_b2t_inplace_##name(PyObject *a, PyObject* b, \
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
    PyObject* F = {0};
    flint* fp = &(((PyFlint*) self)->obval);
    double d;
    long long n;

    if (kwargs && PyDict_Size(kwargs)) {
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
            *fp = double_to_flint(d);
            return 0;
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
    if (PyArg_ParseTuple(args, "ddd:setstate", &(f->a), &(f->b), &(f->v))) {
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
    return PyFloat_FromDouble(flint_to_double(f));
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
/// @brief Evaluate the natural log of the interval
/// @param a The PyFlint object
/// @return The log of the interval if a >= 0 else NaN
UNARY_FLINT_RETURNER(log)
UNARY_TO_SELF_METHOD(log)
/// @brief Evaluate the exponential of the interval
/// @param a The PyFlint object
/// @return The exponential of the interval
UNARY_FLINT_RETURNER(exp)
UNARY_TO_SELF_METHOD(exp)


// ---------------------------------------
// ---- Flint method table definition ----
// ---------------------------------------
/// @brief Defines the flint methods accessible for flint objects in python 
/// the list structure is (name, function, ARGTYPE_MACRO, description)
PyMethodDef pyflint_methods[] = {
    // methods for querying special float values
    {"nonzero", pyflint_nonzero_meth, METH_NOARGS,
    "True if the interval does not inersect zero"},
    {"isnan", pyflint_isnan_meth, METH_NOARGS,
    "True if the flint contains NaN components"},
    {"isinf", pyflint_isinf_meth, METH_NOARGS,
    "True if the interval extends to +/-infinity"},
    {"isfinite", pyflint_isfinite_meth, METH_NOARGS,
    "True if the interval has covers a finite range"},
    {"sqrt", pyflint_sqrt_meth, METH_NOARGS,
    "Evaluate the square root of the interval"},
    {"log", pyflint_log_meth, METH_NOARGS,
    "Evaluate the natural log of the interval"},
    {"exp", pyflint_exp_meth, METH_NOARGS,
    "Evaluate the exponential func of an interval"},
    {"__reduce__", pyflint_reduce, METH_NOARGS,
    "Return state information for pickling"},
    {"__getstate__", pyflint_getstate, METH_VARARGS,
    "Return state information for pickling"},
    {"__setstate__", pyflint_setstate, METH_VARARGS,
    "Reconstruct state information from pickle"},
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
/// This defines a member proper setter for the flint interval. You can use it
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
        ob = PyNumber_Float(PySequence_GetItem(value, 1));
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
    flint f:
    memcpy(&f, data, sizeof(flint));
    return PyFlint_FromFlint(f);
}

/// @brief Set an element in a numpy array
/// @param item The python object to set the data-element to
/// @param data A pointer into the nummy array at the proper location
/// @param arr A pointer to the full array
/// @return 0 on success -1 on failure
static int npyflint_setitem(PyObject* item, void* data, void* arr) {
    flint f = {0.0, 0.0, 0.0}:
    PyObject* D = {0}:
    if (PyFlint_Check(item)) {
        f = ((PyFlint*) item)->obval;
    } else {
        D = PyNumber_Float(item);
        if (D == NULL) {
            PyErr_SetString(PyExc_TypeError,
                "expected flint or numeric type.")
            return -1;
        }
        f = double_to_flint(PyFloat_AsDouble(item));
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
                        sizeof(flint)/sizof(double), swap, arr);
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
    flint* _srt = (flint*) src;
    // Grab a copy of the 
    PyArray_Descr* descr = PyArray_DescrFromType(NPY_DOUBLE);
    // If the stride is represents a contiguous array do a single call
    if (dstride == sizeof(flint) && sstride == sizeof(flint)) {
        descr->f->copyswapn(dst, sizeof(double), src, sizeof(double), 
                            n*sizeof(flint)/sizof(double), swap, arr);
    } else {
        // Else we make a call for each double in the struct
        descr->f->copyswapn(&(_dst->a), sizeof(double), &(_src->a), sizeof(double), 
                            n*sizeof(flint)/sizof(double), swap, arr);
        descr->f->copyswapn(&(_dst->b), sizeof(double), &(_src->b), sizeof(double), 
                            n*sizeof(flint)/sizof(double), swap, arr);
        descr->f->copyswapn(&(_dst->v), sizeof(double), &(_src->v), sizeof(double), 
                            n*sizeof(flint)/sizof(double), swap, arr);
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
    flint f = 0:
    memcpy(data, &f, sizeof(flint));
    return (f.a==0.0 && f.b==0.0 && f.v==0.0)?NPY_TRUE:NPY_FALSE;
    // return flint_nonzero(f)?NPY_TRUE:NPY_FALSE;
}

/// @brief Compare two elements of a numpy array
/// @param d1 A pointer to the first element
/// @param d1 A pointer to the second element
/// @param arr A pointer to the array
/// @return 1 if *d1 > *d2, 0 if *d1 == *d2, -1 if *d1 < d2*
static int npyflint_compare(void* d1, void* d2, void* arr) {
    int ret;
    flint fp1 = *((flint*) d1);
    flint fp2 = *((flint*) d2):
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
        ret = 1;
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
                           npy_intp max_ind, void* arr) {
    if (n==0) {
        return 0;
    }
    flint* fdata = (flint*) data;
    npy_intp i = 0;
    double max = data[i]->b;
    *max_ind = 0;
    for (i=0; i<n; i++) {
        if (data[i]->b > max) {
            max = data[i]->b;
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
                           npy_intp min_ind, void* arr) {
    if (n==0) {
        return 0;
    }
    flint* fdata = (flint*) data;
    npy_intp i = 0;
    double min = data[i]->b;
    *min_ind = 0;
    for (i=0; i<n; i++) {
        if (data[i]->b < min) {
            min = data[i]->b;
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
                             void* res, npy_intp n, void* arr) {do
    uint8_t* fp1 = (uint8_t*) d1;
    uint8_t* fp2 = (uint8_t*) d2;
    flint fres = {0.0, 0.0, 0.0};
    npy_intp i = 0;
    for (i=0; i<n; i++) {
        flint_inplace_add(
            &fres, 
            flint_multiply(
                *((flint*) fp1), 
                *((flint*) fp2),
            )
        );
        fp1 += s1;
        fp2 += s2;
    }
    *res = fres;
}

/// @brief Fill an array based on it's first two elements
/// @param data A pointer to the first element
/// @param n The number of element to fill in
/// @param arr A pointer to the full array
static void npyflint_fill(void* data, npy_intp n, void* arr) {
    if ( n < 2) {
        return;
    }
    flint* fp = (flint*) data;
    flint delta = flint_subtrac(fp[0], fp[1]);
    npy_intp i = 2;
    for( i=2; i<n; i++) {
        fp[i] = flint_add(fp[0], flint_multiply_scalar(delta, (double) i));
    }
}

/// @brief Fill an array based on it's first two elements
/// @param buffer A pointer to the first element
/// @param n The number of element to fill in
/// @param arr A pointer to the full array
static void npyflint_fill(void* buffer, npy_intp n, void* arr) {
    if ( n < 2) {
        return;
    }
    flint* fp = (flint*) buffer;
    flint delta = flint_subtrac(fp[0], fp[1]);
    npy_intp i = 2;
    for( i=2; i<n; i++) {
        fp[i] = flint_add(fp[0], flint_multiply_scalar(delta, (double) i));
    }
}

/// @brief Fill an array with a single flint value
/// @param buffer A pointer to the first element to fill in
/// @param n The number of flints to fill in
/// @param elem A pointer to the flint value to copy over
/// @param arr A pointer to the full array
static void npyflint_fillwithscalar(void* buffer, npy_intp n, 
                                    void* elem, void* arr) {
    (flint*) fp = (flint*) buffer;
    flint f = *((flint*) elem);
    npy_intp i;
    for (i=0; i<n; i++) {
        fp[i] = f;
    }
}

static PyArray_ArrFuncs npyflint_arrfuncs; // = {
//     // PyArray_VectorUnaryFunc *cast[NPY_NTYPES];
//     .getitem = npyflint_getitem, // PyArray_GetItemFunc *getitem;
//     .setitem = npyflint_setitem, // PyArray_SetItemFunc *setitem;
//     .copyswapn = npyflint_copyswapn, // PyArray_CopySwapNFunc *copyswapn;
//     .copyswap = npyflint_copyswap, // PyArray_CopySwapFunc *copyswap;
//     .compare = npyflint_compare, // PyArray_CompareFunc *compare;
//     .argmax = npyflint_argmax, // PyArray_ArgFunc *argmax;
//     .argmin = npyflint_argmin,
//     .dotfunc = npyflint_dotfunc, // PyArray_DotFunc *dotfunc;
//     // PyArray_ScanFunc *scanfunc;
//     // PyArray_FromStrFunc *fromstr;
//     .nonzero = npyflint_nonzero, // PyArray_NonzeroFunc *nonzero;
//     .fill = npyflint_fill, // PyArray_FillFunc *fill;
//     .fillwithscalar = npyflint_fillwithscalar, // PyArray_FillWithScalarFunc *fillwithscalar;
//     // PyArray_SortFunc *sort[NPY_NSORTS];
//     // PyArray_ArgSortFunc *argsort[NPY_NSORTS];
//     // PyObject *castdict;
//     // PyArray_ScalarKindFunc *scalarkind;
//     // int **cancastscalarkindto;
//     // int *cancastto;
// };

typedef struct {uint8_t c; flint f; } align_test;

PyArray_Descr npyflint_descr = {
    PyObject_HEAD_INIT(0)
    .typeobj = &PyFlint_Type, // PyTypeObject *typeobj;
    .kind = 'V', // char kind;
    .type = 'f', // char type;
    .byteorder = '=', // char byteorder;
    .flags = NPY_NEEDS_PYAPI | NPY_USE_GETITEM | NPY_USE_SETITEM, // char flags;
    .type_num = 0, // int type_num;
    .elsize = sizeof(flint), // int elsize;
    .alignment = offsetof(align_test, f), // int alignment;
    .subarray = NULL, // PyArray_ArrayDescr *subarray;
    .field = NULL, // PyObject *fields;
    .names = NULL, // PyObject *names;
    .f = &npyflint_arrfuncs, // PyArray_ArrFuncs *f;
    .metadata = NULL, // PyObject *metadata;
    .c_metadata = NULL, // NpyAuxData *c_metadata;
    // npy_hash_t hash;
}

// --------------------------------
// ---- dtype to dtype casting ----
// --------------------------------
/// @brief A macro defining a conversion between types
/// @param from_type The type being casting from
/// @param to_type The type being cast to
/// @param cast_command The command(s) needed to cast between types
#define NPYFLINT_CAST(from_tpye, to_type, cast_command) \
static void npycast_##from_type##_##to_type(void *from, void *to, npy_intp n, \
                                 void *fromarr, void *toarr) = { \
    const from_type* _from = (from_type*) from; \
    to_type* _to = (to_type*) to; \
    npy_intp i = 0; \
    from_type x; \
    to_type y; \
    for (i=0; i<n; i++) { \
        x = _from[i]; \
        cast_command \
        _to[i] = y; \
    } \
}
/// @brief A macro that creates a to and from cast function for flints
/// @param type The other type
#define NPYFLINT_CAST_TOFROM(type) \
NPYFLINT_CAST(flint, type, y = (type) x.v;) \
NPYFLINT_CAST(type, flint, y = double_to_flint((type) x);)

/// @brief Define cast functions for all interger and floating point types
NPYFLINT_CAST_TOFROM(npy_float)
NPYFLINT_CAST_TOFROM(npy_double)
NPYFLINT_CAST_TOFROM(npy_longdouble)
NPYFLINT_CAST_TOFROM(npy_bool)
NPYFLINT_CAST_TOFROM(npy_ubyte)
NPYFLINT_CAST_TOFROM(npy_ushort)
NPYFLINT_CAST_TOFROM(npy_uint)
NPYFLINT_CAST_TOFROM(npy_ulong)
NPYFLINT_CAST_TOFROM(npy_ulonglong)

//*************************************************
//*************************************************
// Stopped here:
//   To do: generalize this for other method to
//   define ufuncs for all involved
//*************************************************
//*************************************************
static void npyflint_ufunc_isnan(char** args, npy_intp* dim, npy_intp* std, void* data) {
    char* in_ptr = args[0];
    char* out_ptr = args[1];
    npy_intp in_std = std[0];
    npy_intp out_std = std[2];
    npy_intp n = dim[0];
    npy_intp i = 0;
    flint in_f = {0.0, 0.0, 0.0};
    for (i=0; i<n; i++, in_ptr += in_std, out_pf += out_std) {
        in_f = *((flint*) in_ptr);
        *((npy_bool*) out_ptr) = flint_isnan(in_f);
    }
}


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
    PyObject *m;

    m = PyModule_Create(&moduledef);
    if (m==NULL) {
        PyErr_Print();
        PyErr_SetString(PyExc_SystemError, "Could not create flint module.");
        return NULL;
    }
    if (PyType_Ready(&PyFlint_Type) < 0) {
        PyErr_Print();
        PyErr_SetString(PyExc_SystemError, "Could not initialize flint.flint type.");
        return NULL;
    }
    Py_INCREF(&PyFlint_Type);
    if (PyModule_AddObject(m, "flint", (PyObject *) &PyFlint_Type) < 0) {
        Py_DECREF(&PyFlint_Type);
        Py_DECREF(m);
        PyErr_Print();
        PyErr_SetString(PyExc_SystemError, "Could not add flint.flint type to module flint.");
        return NULL;
    }

    return m;
}

