Using flints in python
======================

Installation
------------

Binary packages are avialable from `PyPI <https://pypi.org/project/numpy-flint/>`_. Use
`pip` to install the binary package.

.. prompt :: bash $>

    pip install numpy-flint

Binary packages have been only been built for CPython 3.7 and later and for some operating systems and architectures including:
    * macos x86
    * glibc linux i686
    * glibc linux x86_64
    * glibc linux aarch64
    * windows win32
    * windows amd64


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

To use with NumPy, import NumPy as well (first?). Mark the array's dtype as `flint`.

.. code-block :: python

    import numpy as np
    from flint import flint

    a = np.fill((3,), 0.2, dtype=flint)
    b = flint(0.6)
    # This evaluates to True
    print( np.sum(a) == b )


.. caution::

    Working with floating point intervals is much slower than standard floating point
    operations, and should only be used where the added guarantee of 'could be equal' is
    required. Also note that the project is in an early and mostly untested state. As of
    right now, ``numpy-flint`` implements most real functions defined in the C99
    ``math.h`` header file, but lacks some of the functions for interacting with
    integers such as ``round``, ``ceil``, ``floor``, and ``fmod``.
