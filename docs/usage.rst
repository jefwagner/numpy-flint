Installation and Basic Usage
============================

Installation
------------

Binary packages are avialable from `PyPI <https://pypi.org/project/numpy-flint/>`_. Use
``pip`` to install the binary package.

.. prompt :: bash $>

    pip install numpy-flint

Numpy-flint is a c-extension python module, and must be built for each python version,
each operating system, and each hardware architechure. Binary packages have been built
for most recent python versions and common architechures. If you find that your
particular system is not supported you can try and `build the project form source
<devel.html>`_

Usage
-----

To use in python, import the flint package and declare the number as a flint type.

.. code-block :: python

    from flint import flint

    # Floating point numbers sometimes don't make sense
    a = 0.2
    b = 0.6
    # This evaluate to False
    print( (a+a+a) == b )

    # Rounded floating point intervals will fix these issues
    x = flint(0.2)
    y = flint(0.6)
    # This evalautes to True
    print( (x+x+x) == y )

The flint type renders as a single number: the tracked value. To see the full interval
you need to use the ``interval`` property which returns a tuple with the lower bound,
upper bound. If all you need is the size of the interval, you can use the ``eps``
property.

.. code-block :: python

    f = flint(1.5)
    print(f) # shows 1.5
    print(f.interval) # shows (1.4999999999999998, 1.5000000000000002)
    print(f.eps) # Shows 4.440892098500626e-16

In addition to arithmetic, many math functions have been implemented as methods on the
flint object. To see a full list of supported math functions look at the `python api
<python_api.html>`_.

.. code-block :: python

    f = flint(2)
    print(f.sqrt()) # 1.4142135623730951

In addition to a new python class, ``flint``s have been added as a new custom dtype. To
use with NumPy, import NumPy as well. Then you can create your array as usual, simply
mark the array's dtype as ``flint``.

.. code-block :: python

    import numpy as np
    from flint import flint

    a = np.fill((3,), 0.2, dtype=flint)
    b = flint(0.6)
    # This evaluates to True
    print( np.sum(a) == b ) # True

Finally all math functions have been added as recognized data-types to NumPy's math
functions, so you can write:

.. code-block :: python

    a = np.arange(5, dtype=flint)
    print(np.sqrt(a)) # np.array([0.0, 1.0,  1.4142135623730951, 1.7320508075688772, 2.0], dtype=flint)


.. caution::

    Working with floating point intervals is much slower than standard floating point
    operations, and should only be used where the added guarantee of 'could be equal' is
    required. Also note that the project is in an early and mostly untested state. As of
    right now, ``numpy-flint`` implements most real functions defined in the C99
    ``math.h`` header file, but lacks some of the functions for interacting with
    integers such as ``round``, ``ceil``, ``floor``, and ``fmod``.
