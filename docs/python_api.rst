Flint Python API
================

.. py:class:: flint

    A rounded floating point numeric data type. This is a numeric data type and supports
    all the standard arithmetic operations (``+``, ``-``, ``*``, ``/``, ``**``) and
    thier respective inplace operators (``+=``, ``-=``, ``*=``, ``/=``, ``**=``) between
    other flint types as well as all non-complex numeric types. In addition the flints
    can be cast back a standard floating point type with the built-in python `float`
    function.

    Members
    """""""

    .. py:data:: a

        The lower bound of the interval

    .. py:data:: b

        The upper bound of the interval

    .. py:data:: v

        The tracked value

    Properties
    """"""""""

    .. py:data:: eps

        The width of the interval (b-a)

    .. py:data:: interval

        :getter: The interval data as a tuple (a, b, v)

        :setter: Set the interval data with a tuple

            If you set only boundaries (a,b) the tracked value is set to the average (b-a)/2

    Methods
    """""""

    .. automethod:: flint.flint.abs

    .. automethod:: flint.flint.sqrt
    
    .. automethod:: flint.flint.cbrt

    .. automethod:: flint.flint.hypot

    .. automethod:: flint.flint.exp

    .. automethod:: flint.flint.exp2

    .. automethod:: flint.flint.expm1

    .. automethod:: flint.flint.log

    .. automethod:: flint.flint.log10

    .. automethod:: flint.flint.log2

    .. automethod:: flint.flint.log1p

    .. automethod:: flint.flint.sin

    .. automethod:: flint.flint.cos

    .. automethod:: flint.flint.tan

    .. automethod:: flint.flint.arcsin

    .. automethod:: flint.flint.arccos

    .. automethod:: flint.flint.arctan

    .. automethod:: flint.flint.arctan2

    .. automethod:: flint.flint.sinh

    .. automethod:: flint.flint.cosh

    .. automethod:: flint.flint.tanh

    .. automethod:: flint.flint.arcsinh

    .. automethod:: flint.flint.arccosh

    .. automethod:: flint.flint.arctanh

