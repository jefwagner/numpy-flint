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
} PyFlintScalarObject;

// Define the flint member variables
PyMemberDef PyFlintArrType_members[] = {
    {"a", T_DOUBLE, offset(PyFlintScalarObject, obval.a), READONLY,
        "The lower bound of the floating point interval"},
    {"b", T_DOUBLE, offset(PyFlintScalarObject, obval.b), READONLY,
        "The upper bound of the floating point interval"},
    {"v", T_DOUBLE, offset(PyFlintScalarObject, obval.v), READONLY,
        "The tracked float value"},
    {NULL}
}

// Define a getter for the interval
static PyObject *
PyFlintArrType_get_interval(PyObject *self, void *closure) {
    flint *f = &(((PyFlintScalarObject *)self)->obval);
    PyObject *tuple = PyTuple_New(2);
    PyTuple_SET_ITEM(tuple, 0, PyFloat_FromDouble(f->a));
    PyTuple_SET_ITEM(tuple, 1, PyFloat_FromDouble(f->b));
    return tuple;
}

// --- Might not need this since we can just access the v member
// // Define a getter for the value
// static PyObject *
// PyFlintArrType_get_value(PyObject *self, void *closure) {
//     flint *f = &(((PyFlintScalarObject *)self)->obval);
//     PyObject *val = PyFloat_FromDouble(f->v);
//     return val;
// }

// Set the getters
PyGetSetDef PyFlintArrType_getset[] = {
    {"interval", PyFlintArrType_get_interval, NULL,
    "The full interval of the flint object as a (a,b) tuple", NULL},
    // {"value", PyFlintArrType_get_value, NULL,
    // "The internal value of the flint object as a 64 bit float", NULL},
    {NULL}
};

// Define the new python array type
PyTypeObject PyFlintArrType_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "flint.flint",
    .tp_basicsize = sizeof(PyFlintScalarObject),
    .tp_members = PyFlintArrType_members,
    .tp_getset = PyFlintArrType_getset,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
};
