/**
 * Python C-API for flints
 */
//
// Copyright (c) 2023, Jef Wagner <jefwagner@gmail.com>
//
// This file is part of numpy-flint.
//
// numpy-flint is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option) any
// later version.
//
// numpy-flint is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// numpy-flint. If not, see <https://www.gnu.org/licenses/>.
//
#ifndef PY_NUMPY_FLINT_H
#define PY_NUMPY_FLINT_H
#ifdef __cplusplus
extern "C" {
#endif

#include <Python.h>
#include "flint.h"

/**
 * The python flint object
 */
typedef struct {
    PyObject_HEAD
    /**
     * The flint c struct
     */
    flint obval;
} PyFlint;

static PyTypeObject PyFlint_Type;
/**
 * A pointer to the PyTypeObject for the PyFlints
 * 
 * Because of the programming standard or declaring everything static outside of
 * enclosing functions, it is neccessary to use this pointer when you need to access
 * the PyTypeObject* for the PyFlint instead of the direct address &PyFlint_Type.
 */
static PyTypeObject* PyFlint_Type_Ptr;

/**
 * The integer enumeration for the custom 'flint' type in numpy.
 */
static int NPY_FLINT;

#ifdef NUMPY_FLINT_MODULE // If header file accessed as part of numpy-flint project

// Get the flint PyTypeObject
static PyTypeObject* get_pyflint_type_ptr() {
    return &PyFlint_Type;
}

// Get the NPY_FLINT type identifier
static int get_npy_flint() {
    return NPY_FLINT;
}

#else // If header file is used outside of the numpy-flint project

#define get_pyflint_type_ptr (*(PyTypeObject* (*)()) PyFlint_API[0])
#define get_npy_flint (*(int (*)()) PyFlint_API[1])

static void** PyFlint_API;

/**
 * Import the c api for numpy-flint python module
 * 
 * :return: 0 on success -1 on failure
 */
static int import_flint(void) {
    PyFlint_API = (void**) PyCapsule_Import("flint.numpy_flint.c_api", 0);
    if (PyFlint_API == NULL) {
        return -1;
    }
    PyFlint_Type_Ptr = get_pyflint_type_ptr();
    NPY_FLINT = get_npy_flint();
    return 0;
}

#endif // NUMPY_FLINT_MODULE

/**
 * Check if an object is a flint
 * 
 * :param ob: The PyObject to check
 * :return: 1 if the object is a flint, 0 otherwise
 */
static inline int PyFlint_Check(PyObject* ob) {
    return PyObject_IsInstance(ob, (PyObject*) PyFlint_Type_Ptr);
}

/**
 * Create a PyFlint object from a c flint struct.
 * 
 * :param f: The c flint struct
 * :return: A new PyFlint object that contains a copy of f
 */
static inline PyObject* PyFlint_FromFlint(flint f) {
    PyFlint* p = (PyFlint*) PyFlint_Type.tp_alloc(PyFlint_Type_Ptr, 0);
    if (p) {
        p->obval = f;
    }
    return (PyObject*) p;
}

/**
 * Get a C flint from a PyFlint
 * 
 * :param ob: The PyFlint object
 * :return: The coresponding flint value
 */
static inline flint PyFlint_AsFlint(PyObject* ob) {
    return  ((PyFlint*) ob)->obval;
}

/**
 * Get a C double from a PyFlint
 * 
 * :param ob: The PyFlint object
 * :return: The coresponding flint center value
 */
static inline double PyFlint_AsDouble(PyObject* ob) {
    return (((PyFlint*) ob)->obval).v;
}

#ifdef __cplusplus
}
#endif
#endif // PY_NUMPY_FLINT_H