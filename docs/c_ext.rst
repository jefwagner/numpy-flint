C Extensions with flints
========================

Numpy-flint is can be used as either a python module or a library for another C
extension. This page gives the necessary details by giving the structure you would need
to build another C extension that uses a custom Numpy Universal Function that works with
flints.

When setting up the project, you will need to add the ``numpy-flint`` as a build
requirement to your ``pyproject.toml`` file.

.. code-block:: toml

    [build-system]
    requires = ["setuptools>=61","oldest-supported-numpy","numpy-flint>=0.3.3"]
    build-backend = "setuptools.build_meta"

Second, you will need to add the ``flint`` include directory to your extension settings
in the ``setup.py`` file.

.. code-block:: python

    from setuptools import setup, Extension
    import numpy as np
    import flint

    setup_args = dict(
        ext_modules = [
            Extension(
                name='new_project.compiled_module_name',
                sources=['src/new_project/c_source_file_name.c'],
                depends=[],
                include_dirs=[
                    np.get_include(),
                    flint.get_include()
                ],
            )
        ]
    )

    setup(**setup_args)

Now you can use the ``numpy-flint`` in your C project itself. The ``flint.h`` header has all
the pure C flint functions defined in it, and the ``numpy_flint.h`` header has the
functions used to interface flints with python. At the top of your C source file you
will want to add both header files,

.. code-block:: c

    #include <flint.h>
    #include <numpy_flint.h>

and inside the ``PyMODINIT_FUNC`` module initialization function you will want to import the flint C-API
with the ``import_flint()`` function.

.. code-block:: C

    PyMODINIT_FUNC PyInit_compiled_module_name(void) {
        /* stuff */
        if (import_flint() < 0) {
            PyErr_Print();
            PyErr_SetString(PyExc_SystemError, "Count not load flint c API");
            return NULL;
        }
        /* stuff */
    }


In the C source file you can then create the new numpy `ufunc
<https://numpy.org/doc/stable/reference/c-api/ufunc.html#c.PyUFuncGenericFunction>`_. 

.. code-block:: c

    static void new_ufunc(char** args, 
                          npy_intp const* dims,
                          npy_intp const* strides,
                          void* data) {
        /* do the magic with flints here! */
    }

It is well beyond the scope of this documentation to cover how to write a ufunc, and
there is already a good `tutorial
<https://numpy.org/devdocs/user/c-info.ufunc-tutorial.html>`_ in the Numpy docs.
However, it does bare mentioning that you can not fully follow that example when
implementing a universal function for a custom data-type. The differences are:

1. In the tutorial, the data-types (and many other variables) are declared static
   outside of the module initialization function. Because the NPY_FLINT index is not
   known at compile time, trying to do this will cause the compiler to throw an error.
   Instead you must declare the data-types inside the ``PyMODINIT_FUNC`` module
   initialization function.
2. In the example, you only need to make a call to ``PyUFunc_FromFuncAndData``. However
   when implementing custom data types you can only add functionality to an existing
   ufunc. If there is not already an existing universal function, you have to declare
   and empty ufunc, then add the functionality for the new data type.

.. code-block:: c

    PyMODINIT_FUNC PyInit_compiled_module_name(void) {
        /* stuff */
        uf = PyUFunc_FromFuncAndData(
            NULL, NULL, NULL, 0, 1, 1, PyUFunc_None,
            "new_ufunc_name", "docstring", 0);
        int new_ufunc_types[] = {NPY_FLINT, NPY_FLINT};
        PyUFunc_RegisterLoopForType(
            (PyUFuncObject*) uf, NPY_FLINT,
            &new_ufunc, new_ufunc_types, NULL);
        d = PyModule_GetDict(m);
        PyDict_SetItemString(d, "new_ufunc_name", uf);
        Py_DECREF(uf);
        /* stuff */
    }

All together, making a new universal function that works with the ``flint`` dtype would
look something like the sketch below.

.. code-block:: C

    /* stuff */

    #include <Python.h>

    #define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
    #include <numpy/arrayobject.h>
    #include <numpy/ufuncobject.h>

    #include <flint.h>
    #include <numpy_flint.h>

    /* stuff */

    static void new_ufunc(char** args, 
                          npy_intp const* dims,
                          npy_intp const* strides,
                          void* data) {
        /* stuff */
    }

    /* stuff */

    PyMODINIT_FUNC PyInit_compiled_module_name(void) {
        PyObject* m;
        PyObject* d;
        PyObject* uf;
        /* stuff */
        // Import numpys array api
        import_array();
        if (PyErr_Occurred()) {
            PyErr_Print();
            PyErr_SetString(PyExc_SystemError, "Could not initialize NumPy.");
            return NULL;
        }
        // Import numpys ufunc api
        import_ufunc();
        if (PyErr_Occurred()) {
            PyErr_Print();
            PyErr_SetString(PyExc_SystemError, "Could not load NumPy ufunc c API.");
            return NULL;
        }
        // Import flint c API
        if (import_flint() < 0) {
            PyErr_Print();
            PyErr_SetString(PyExc_SystemError, "Count not load flint c API");
            return NULL;
        }
        /* stuff */

        uf = PyUFunc_FromFuncAndData(
            NULL, NULL, NULL, 0, 1, 1, PyUFunc_None,
            "new_ufunc_name", "docstring", 0);
        int new_ufunc_types[] = {NPY_FLINT, NPY_FLINT};
        PyUFunc_RegisterLoopForType(
            (PyUFuncObject*) uf, NPY_FLINT,
            &new_ufunc, new_ufunc_types, NULL);
        d = PyModule_GetDict(m);
        PyDict_SetItemString(d, "new_ufunc_name", uf);
        Py_DECREF(uf);

        /* stuff */

        return m;
    }

References
----------

The following references are used in implementing the C extension to Python and NumPy.

* The `Extending and Embedding the Python Interpreter
  <https://docs.python.org/3/extending/index.html>`_ documentation contain details and
  examples on how to write c code to interface with python.
* The `Python/C API Reference Manual <https://docs.python.org/3/c-api/index.html>`_ has
  all the details needed for extending python with c. the details need for interfacing
  numpy using c.
* The `NumPy C API <https://numpy.org/doc/stable/reference/c-api/index.html>`_ has all
* The `NumPy UFUNC Tutorial
  <https://numpy.org/devdocs/user/c-info.ufunc-tutorial.html>`_ gives concrete example
  of writing new universal functions.

In particular, the following examples were especially helpful.

* Mobiles `quaterions <https://github.com/moble/quaternion>`_ Has been my goto source
  for understanding the process of extending Python and then Numpy with c.
* Martin Ling's `numpy_quaternion <https://github.com/martinling/numpy_quaternion>`_ the
  previous version of the above quaternion project was also used as an example.
* Mark Wiebe's `numpy_half <https://github.com/mwiebe/numpy_half>`_ which was the first
  project upon which Martin Ling's projet was based. 

