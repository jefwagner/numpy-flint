# Copyright (c) 2023, Jef Wagner <jefwagner@gmail.com>
# This file is part of numpy-flint.
#
# Numpy-flint is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# Numpy-flint is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# numpy-flint. If not, see <https://www.gnu.org/licenses/>.
import unittest

import numpy as np
from flint import flint

class TestInit(unittest.TestCase):
    """Test for the initialization and internal structure of the flint objects"""

    def test_init_int(self):
        """Validate initialization from integers"""
        x = flint(3)
        self.assertIsInstance(x, flint)
        self.assertEqual(x.a, 3)
        self.assertEqual(x.b, 3)
        self.assertEqual(x.v, 3)
        big_int = 10000000000000000
        y = flint(big_int)
        d = np.float64(big_int)
        self.assertEqual(y.a, np.nextafter(d,-np.inf))
        self.assertEqual(y.b, np.nextafter(d,np.inf))
        self.assertEqual(y.v, d)        
        big_int = -10000000000000000
        z = flint(big_int)
        d = np.float64(big_int)
        self.assertEqual(z.a, np.nextafter(d,-np.inf))
        self.assertEqual(z.b, np.nextafter(d,np.inf))
        self.assertEqual(z.v, d)        

    def test_init_float(self):
        """Validate initialization from floats"""
        x = flint(1.5)
        d = np.float64(1.5)
        self.assertEqual(x.a, np.nextafter(d,-np.inf))
        self.assertEqual(x.b, np.nextafter(d,np.inf))
        self.assertEqual(x.v, d)

    def test_init_flint(self):
        """Validate initialization from flints"""
        x = flint(1.5)
        y = flint(x)
        self.assertIsNot(x, y)
        self.assertEqual(x.a, y.a)
        self.assertEqual(x.b, y.b)
        self.assertEqual(x.v, y.v)


class TestProperties(unittest.TestCase):
    """Test for the properties of the flint objects"""

    def test_eps(self):
        """Validate the width of the interval"""
        x = flint(1)
        self.assertEqual(x.eps, 0)
        x = flint(1.5)
        e = np.nextafter(1.5,np.inf) - np.nextafter(1.5,-np.inf)
        self.assertEqual(x.eps, e)

    def test_get_interval(self):
        """Validate getting the interval endpoints"""
        x = flint(1.5)
        a,b = x.interval
        self.assertEqual(a, x.a)
        self.assertEqual(b, x.b)

    def test_get_interval(self):
        """Validate setting the interval"""
        x = flint(1.5)
        x.interval = 1, 2
        self.assertEqual(1, x.a)
        self.assertEqual(2, x.b)
        self.assertEqual(1.5, x.v)
        x.interval = 1, 2, 1.75
        self.assertEqual(1, x.a)
        self.assertEqual(2, x.b)
        self.assertEqual(1.75, x.v)

class TestState:

    def test_getstate(self):
        x = flint(1.5)
        x.interval = 1,2
        state = x.__getstate__()
        assert isinstance(state, tuple)
        assert len(state) == 3
        assert state[0] == x.a
        assert state[1] == x.b
        assert state[2] == x.v

    def test_setstate(self):
        x = flint(0)
        x.__setstate__((1.0, 2.0, 1.5))
        assert x.a == 1.0
        assert x.b == 2.0
        assert x.v == 1.5

    def test_reduce(self):
        x = flint(1.5)
        x.interval = 1,2
        r = x.__reduce__()
        assert isinstance(r, tuple)
        assert len(r) == 2
        assert isinstance(r[0], type)
        assert r[0] == type(x)
        assert isinstance(r[1], tuple)
        assert len(r[1]) == 3
        assert r[1][0] == x.a
        assert r[1][1] == x.b
        assert r[1][2] == x.v


class TestFloatSpecialValues(unittest.TestCase):
    """Test the query functions for the float special value checks"""

    def test_nonzero(self):
        """Validate 0"""
        x = flint(0)
        self.assertFalse(np.nonzero(x))
        x = flint(1)
        self.assertTrue(np.nonzero(x))

    def test_isnan(self):
        """Validate NaN"""
        x = flint(0)
        self.assertFalse(np.isnan(x))
        x.interval = np.nan,np.nan,np.nan
        self.assertTrue(np.isnan(x))

    def test_isfinite(self):
        """Validate finite vs infinite"""
        x = flint(0)
        self.assertTrue(np.isfinite(x))
        x.interval = 0, np.inf
        self.assertFalse(np.isfinite(x))
        x.interval = -np.inf, 0
        self.assertFalse(np.isfinite(x))

    def test_isinf(self):
        """Validate finite vs infinite"""
        x = flint(0)
        self.assertFalse(np.isinf(x))
        x.interval = 0, np.inf
        self.assertTrue(np.isinf(x))
        x.interval = -np.inf, 0
        self.assertTrue(np.isinf(x))


class TestComparisons(unittest.TestCase):
    """Test for comparisons"""

    def validateEqual(self, lhs, rhs):
        """Helper function for all comparison operators for equal values"""
        self.assertFalse(lhs < rhs)
        self.assertTrue(lhs <= rhs)
        self.assertTrue(lhs == rhs)
        self.assertTrue(lhs >= rhs)
        self.assertFalse(lhs > rhs)
        self.assertFalse(lhs != rhs)

    def validateLess(self, lhs, rhs):
        """Helper function for all comparison operators for lesser values"""
        self.assertTrue(lhs < rhs)
        self.assertTrue(lhs <= rhs)
        self.assertFalse(lhs == rhs)
        self.assertFalse(lhs >= rhs)
        self.assertFalse(lhs > rhs)
        self.assertTrue(lhs != rhs)

    def validateGreater(self, lhs, rhs):
        """Helper function for all comparison operators for greater values"""
        self.assertFalse(lhs < rhs)
        self.assertFalse(lhs <= rhs)
        self.assertFalse(lhs == rhs)
        self.assertTrue(lhs >= rhs)
        self.assertTrue(lhs > rhs)
        self.assertTrue(lhs != rhs)

    def test_zerowidth(self):
        """Validate comparisons with a zero-width flint with number"""
        x = flint(2)
        self.validateEqual(x, 2)
        self.validateEqual(2, x)
        self.validateEqual(x, 2.0)
        self.validateEqual(2.0, x)
        self.validateLess(x, 3)
        self.validateGreater(3, x)
        self.validateGreater(x, 1)
        self.validateLess(1, x)

    def test_with_number(self):
        """Validate comparisons of a flint with a number"""
        x = flint(2)
        x.interval = 1, 2
        self.validateLess(x, 3)
        self.validateGreater(3, x)
        self.validateEqual(x, 2)
        self.validateEqual(2, x)
        self.validateEqual(x, 1.5)
        self.validateEqual(1.5, x)
        self.validateEqual(x, 1)
        self.validateEqual(1, x)
        self.validateGreater(x, 0)
        self.validateLess(0, x)

    def test_with_flint(self):
        """Validate comparisons between flints"""
        x = flint(2)
        x.interval = 1, 2
        y = flint(0)
        y.interval = 0, 0.5
        self.validateLess(y, x)
        self.validateGreater(x, y)
        y.interval = 0.5, 1
        self.validateEqual(y, x)
        self.validateEqual(x, y)
        y.interval = 0.75, 1.25
        self.validateEqual(y, x)
        self.validateEqual(x, y)
        y.interval = 1.25, 1.75
        self.validateEqual(y, x)
        self.validateEqual(x, y)
        

class TestArithmetic(unittest.TestCase):
    """Test arithmetic operators for flint objects"""

    def test_pos(self):
        """Test an explicit positive sign"""
        x = flint(1)
        y = +x
        self.assertIsInstance(y, flint)
        self.assertEqual(y.eps, x.eps)
        self.assertEqual(y, 1)
        x = flint(1.5)
        y = +x
        self.assertIsInstance(y, flint)
        self.assertEqual(y.eps, x.eps)
        self.assertEqual(y, 1.5)

    def test_neg(self):
        """Test negation"""
        x = flint(1)
        y = -x
        self.assertIsInstance(y, flint)
        self.assertEqual(y.eps, x.eps)
        self.assertEqual(y, -1)
        x = flint(1.5)
        y = -x
        self.assertIsInstance(y, flint)
        self.assertEqual(y.eps, x.eps)
        self.assertEqual(y, -1.5)

    def test_add(self):
        """Validate addition"""
        x = flint(1)
        y = x + 1
        self.assertIsInstance(y, flint)
        self.assertTrue(y.eps > 0)
        self.assertEqual(y, 2)

    def test_iadd(self):
        """Validate inplace addition"""
        x = flint(1)
        x += 1
        self.assertIsInstance(x, flint)
        self.assertTrue(x.eps > 0)
        self.assertEqual(x, 2)

    def test_sub(self):
        """Validate subtraction"""
        x = flint(2)
        y = x - 1
        self.assertIsInstance(y, flint)
        self.assertTrue(y.eps > 0)
        self.assertEqual(y, 1)
        x = flint(1)
        y = 2 - x
        self.assertIsInstance(y, flint)
        self.assertTrue(y.eps > 0)
        self.assertEqual(y, 1)

    def test_isub(self):
        """Validate subtraction"""
        x = flint(2)
        x -= 1
        self.assertIsInstance(x, flint)
        self.assertTrue(x.eps > 0)
        self.assertEqual(x, 1)

    def test_mul(self):
        """Validate multiplication"""
        x = flint(2)
        y = 3*x
        self.assertIsInstance(y, flint)
        self.assertTrue(y.eps > 0)
        self.assertEqual(y, 6)

    def test_imul(self):
        """Validate multiplication"""
        x = flint(2)
        x *= 3
        self.assertIsInstance(x, flint)
        self.assertTrue(x.eps > 0)
        self.assertEqual(x, 6)

    def test_div(self):
        """Validate division"""
        x = flint(6)
        y = x/3
        self.assertIsInstance(y, flint)
        self.assertTrue(y.eps > 0)
        self.assertEqual(y, 2)
        x = flint(2)
        y = 6/x
        self.assertIsInstance(y, flint)
        self.assertTrue(y.eps > 0)
        self.assertEqual(y, 3)

    def test_idiv(self):
        """Validate division"""
        x = flint(6)
        x /= 3
        self.assertIsInstance(x, flint)
        self.assertTrue(x.eps > 0)
        self.assertEqual(x, 2)

    def test_pow(self):
        """Validate exponentiation"""
        x = flint(4)
        y = x**0.5
        self.assertIsInstance(y, flint)
        self.assertTrue(y.eps > 0)
        self.assertEqual(y, 2)

    def test_ipow(self):
        """Validate exponentiation"""
        x = flint(4)
        x **= 0.5
        self.assertIsInstance(x, flint)
        self.assertTrue(x.eps > 0)
        self.assertEqual(x, 2)


class TestGeneralMath(unittest.TestCase):
    """Test the general math functions"""

    def test_abs(self):
        """Validate absolute value"""
        x = flint(-2)
        y = np.abs(x)
        self.assertIsInstance(y, flint)
        self.assertEqual(y.eps, x.eps)
        self.assertEqual(y, 2)
        x = flint(-1.5)
        y = np.abs(x)
        self.assertIsInstance(y, flint)
        self.assertEqual(y.eps, x.eps)
        self.assertEqual(y, 1.5)
        x = flint(1.5)
        y = np.abs(x)
        self.assertIsInstance(y, flint)
        self.assertEqual(y.eps, x.eps)
        self.assertEqual(y, 1.5)
        x = flint(0)
        x.interval = -1,1
        y = np.abs(x)
        self.assertIsInstance(y, flint)
        self.assertEqual(y.a, 0)
        self.assertEqual(y.b, 1)
        self.assertEqual(y.v, 0)

    def test_sqrt(self):
        """Validate the square root"""
        x = flint(4)
        y = np.sqrt(x)
        self.assertIsInstance(y, flint)
        self.assertTrue(y.eps > 0)
        self.assertEqual(y, 2)
        x = flint(0)
        x.interval = -1,3 # v = 1
        y = np.sqrt(x)
        self.assertIsInstance(y, flint)
        self.assertEqual(y.a, 0)
        self.assertEqual(y.v, 1)
        x = flint(0)
        x.interval = -3,1 # v = 1
        y = np.sqrt(x)
        self.assertIsInstance(y, flint)
        self.assertEqual(y.a, 0)
        self.assertEqual(y.v, 0)
        x = flint(-4)
        y = np.sqrt(x)
        self.assertIsInstance(y, flint)
        self.assertTrue(np.isnan(y))

    def test_cbrt(self):
        """Validate the cube root"""
        x = flint(8)
        y = np.cbrt(x)
        self.assertIsInstance(y, flint)
        self.assertTrue(y.eps > 0)
        self.assertEqual(y, 2)

    def test_hypot(self):
        """Validate the hypotenuse"""
        a = flint(0.6)
        b = flint(0.8)
        c = np.hypot(a,b)
        self.assertIsInstance(c, flint)
        self.assertTrue(c.eps > 0)
        self.assertEqual(c, 1)
        a = flint(-0.6)
        b = flint(0.8)
        c = np.hypot(a,b)
        self.assertEqual(c, 1)
        a = flint(-0.6)
        b = flint(-0.8)
        c = np.hypot(a,b)
        self.assertEqual(c, 1)
        a = flint(0.6)
        b = flint(-0.8)
        c = np.hypot(a,b)
        self.assertEqual(c, 1)
        a = flint(0)
        a.interval = -1,1
        b = flint(1)
        c = np.hypot(a, b)
        self.assertEqual(c, np.sqrt(2))
        self.assertAlmostEqual(c.a, 1)
        self.assertTrue(c.b> np.sqrt(2))
        self.assertAlmostEqual(c.b, np.sqrt(2))
        a = flint(0)
        a.interval = -1,1
        b = flint(-1)
        c = np.hypot(a, b)
        self.assertEqual(c, np.sqrt(2))
        self.assertAlmostEqual(c.a, 1)
        self.assertTrue(c.b> np.sqrt(2))
        self.assertAlmostEqual(c.b, np.sqrt(2))
        a = flint(1)
        b = flint(0)
        b.interval = -1,1
        c = np.hypot(a, b)
        self.assertEqual(c, np.sqrt(2))
        self.assertAlmostEqual(c.a, 1)
        self.assertTrue(c.b> np.sqrt(2))
        self.assertAlmostEqual(c.b, np.sqrt(2))
        a = flint(-1)
        b = flint(0)
        b.interval = -1,1
        c = np.hypot(a, b)
        self.assertEqual(c, np.sqrt(2))
        self.assertAlmostEqual(c.a, 1)
        self.assertTrue(c.b> np.sqrt(2))
        self.assertAlmostEqual(c.b, np.sqrt(2))
        a = flint(0)
        b = flint(0)
        a.interval = -1,1
        b.interval = -1,1
        c = np.hypot(a, b)
        self.assertEqual(c, np.sqrt(2))
        self.assertEqual(c.a, 0)
        self.assertTrue(c.b > np.sqrt(2))
        self.assertAlmostEqual(c.b, np.sqrt(2))


class TestExponentialMath(unittest.TestCase):
    """Test the exponential and log based math functions"""

    def test_exp(self):
        """Validate the exponential function"""
        x = flint(1)
        y = np.exp(x)
        self.assertIsInstance(y, flint)
        self.assertTrue(y.eps > 0)
        self.assertEqual(y, np.e)

    def test_exp2(self):
        """Validate the 2^x function"""
        x = flint(0.5)
        y = np.exp2(x)
        self.assertIsInstance(y, flint)
        self.assertTrue(y.eps > 0)
        self.assertEqual(y, np.sqrt(2))

    def test_expm1(self):
        """Validate the e^x-1 function"""
        x = flint(1.0e-15)
        y = np.expm1(x)
        self.assertIsInstance(y, flint)
        self.assertTrue(y.eps > 0)
        self.assertEqual(y, 1.0e-15)

    def test_log(self):
        """Validate the natural logarithm"""
        x = flint(np.e)
        y = np.log(x)
        self.assertIsInstance(y, flint)
        self.assertTrue(y.eps > 0)
        self.assertEqual(y, 1)
        x = flint(0)
        x.interval = -1,3 # midpoint of 1
        y = np.log(x)
        self.assertIsInstance(y, flint)
        self.assertTrue(np.isinf(y))
        self.assertEqual(y.v, 0)
        x = flint(-1)
        y = np.log(x)
        self.assertIsInstance(y, flint)
        self.assertTrue(np.isnan)

    def test_log10(self):
        """Validate the log base 10"""
        x = flint(1.0e7)
        y = np.log10(x)
        self.assertIsInstance(y, flint)
        self.assertTrue(y.eps > 0)
        self.assertEqual(y, 7)
        x = flint(0)
        x.interval = -1,3 # midpoint of 1
        y = np.log10(x)
        self.assertIsInstance(y, flint)
        self.assertTrue(np.isinf(y))
        self.assertEqual(y.v, 0)
        x = flint(-1)
        y = np.log10(x)
        self.assertIsInstance(y, flint)
        self.assertTrue(np.isnan)

    def test_log2(self):
        """Validate the log base 2"""
        x = flint(256)
        y = np.log2(x)
        self.assertIsInstance(y, flint)
        self.assertTrue(y.eps > 0)
        self.assertEqual(y, 8)
        x = flint(0)
        x.interval = -1,3 # midpoint of 1
        y = np.log2(x)
        self.assertIsInstance(y, flint)
        self.assertTrue(np.isinf(y))
        self.assertEqual(y.v, 0)
        x = flint(-1)
        y = np.log2(x)
        self.assertIsInstance(y, flint)
        self.assertTrue(np.isnan)

    def test_log1p(self):
        """Validate the log(1+x)"""
        x = flint(1.0e-15)
        y = np.log1p(x)
        self.assertIsInstance(y, flint)
        self.assertTrue(y.eps > 0)
        self.assertEqual(y, 1.0e-15)
        x = flint(0)
        x.interval = -2,2 # midpoint of 0
        y = np.log1p(x)
        self.assertIsInstance(y, flint)
        self.assertTrue(np.isinf(y))
        self.assertEqual(y.v, 0)
        x = flint(-2)
        y = np.log1p(x)
        self.assertIsInstance(y, flint)
        self.assertTrue(np.isnan)


class TestTrigMath(unittest.TestCase):
    """Test trig functions"""

    def test_sin(self):
        """Validate sin"""
        # Check if sin(pi) = 0
        x = flint(np.pi)
        y = np.sin(x)
        self.assertIsInstance(y, flint)
        self.assertTrue(y.eps > 0)
        self.assertEqual(y, 0)
        # Make sure invariant for a and b is held true
        x = flint(0.5)
        y = np.sin(x)
        self.assertTrue(y.b > y.a)
        x = flint(3.5)
        y = np.sin(x)
        self.assertTrue(y.b > y.a)
        # Check when not monotonic
        x = flint(0)
        x.interval = 1,2 # covers pi/2
        y = np.sin(x)
        self.assertEqual(y.b, 1.0)
        self.assertAlmostEqual(y.a, np.sin(1.0))
        x.interval = 4,5 # covers 3 pi/2
        y = np.sin(x)
        self.assertEqual(y.a, -1.0)
        self.assertAlmostEqual(y.b, np.sin(4.0))
        x.interval = 6,8 # covers 5 pi/2
        y = np.sin(x)
        self.assertEqual(y.b, 1.0)
        self.assertAlmostEqual(y.a, np.sin(6.0))
        x.interval = 4,8 # covers 3 pi/2 and 5 pi/2
        y = np.sin(x)
        self.assertEqual(y.a, -1.0)
        self.assertEqual(y.b, 1.0)
        x.interval = 6,11 # covers 5 pi/2 and 7 pi/2
        y = np.sin(x)
        self.assertEqual(y.a, -1.0)
        self.assertEqual(y.b, 1.0)

    def test_cos(self):
        """Validate cos"""
        # Check if cos(pi/2) = 0
        x = flint(np.pi/2)
        y = np.cos(x)
        self.assertIsInstance(y, flint)
        self.assertTrue(y.eps > 0)
        self.assertEqual(y, 0)
        # Check invariant for a and b held true
        x = flint(1.5)
        y = np.cos(x)
        self.assertTrue(y.a < y.b)
        x = flint(4.5)
        y = np.cos(x)
        self.assertTrue(y.a < y.b)
        # Check for extreme values
        x = flint(0)
        x.interval = -0.5, 1 # over zero
        y = np.cos(x)
        self.assertEqual(y.b, 1.0)
        self.assertAlmostEqual(y.a, np.cos(1))
        x = flint(0)
        x.interval = 3, 4 # over pi
        y = np.cos(x)
        self.assertEqual(y.a, -1.0)
        self.assertAlmostEqual(y.b, np.cos(4))
        x = flint(0)
        x.interval = 3, 7 # over pi and 2pi
        y = np.cos(x)
        self.assertEqual(y.a, -1.0)
        self.assertEqual(y.b, 1.0)
        x.interval = 6, 10 # over 2pi and 3pi
        y = np.cos(x)
        self.assertEqual(y.a, -1.0)
        self.assertEqual(y.b, 1.0)

    def test_tan(self):
        """Validate tangent"""
        x = flint(np.pi/4)
        y = np.tan(x)
        self.assertIsInstance(y, flint)
        self.assertTrue(y.eps > 0)
        self.assertEqual(y, 1)
        x = flint(0)
        x.interval = 1.5,1.6
        y = np.tan(x)
        self.assertTrue(np.isinf(y.a))
        self.assertTrue(np.isinf(y.b))
        x.interval = -0.5, np.pi
        y = np.tan(x)
        self.assertTrue(np.isinf(y.a))
        self.assertTrue(np.isinf(y.b))
    

class TestInverseTrigMath(unittest.TestCase):
    """Test inverse trig functions"""

    def test_asin(self):
        """Validate the inverse sin"""
        x = flint(0.5)
        y = np.arcsin(x)
        self.assertIsInstance(y, flint)
        self.assertTrue(y.eps > 0)
        self.assertEqual(y, np.pi/6)
        x = flint(0)
        x.interval = -1.2, -0.9
        y = np.arcsin(x)
        self.assertEqual(y, -np.pi/2)
        self.assertAlmostEqual(y.a, -np.pi/2)
        self.assertAlmostEqual(y.v, -np.pi/2)
        self.assertAlmostEqual(y.b, np.arcsin(-0.9))
        x = flint(0)
        x.interval = 0.8, 1.1
        y = np.arcsin(x)
        self.assertEqual(y, np.pi/2)
        self.assertAlmostEqual(y.a, np.arcsin(0.8))
        self.assertAlmostEqual(y.v, np.arcsin(0.95))
        self.assertAlmostEqual(y.b, np.pi/2)
        x = flint(0)
        x.interval = -1.1, 1.1
        y = np.arcsin(x)
        self.assertAlmostEqual(y.a, -np.pi/2)
        self.assertEqual(y.v, 0)
        self.assertAlmostEqual(y.b, np.pi/2)
        x = flint(1.1)
        y = np.arcsin(x)
        self.assertTrue(np.isnan(y.a))
        self.assertTrue(np.isnan(y.b))
        self.assertTrue(np.isnan(y.v))
        x = flint(-1.1)
        y = np.arcsin(x)
        self.assertTrue(np.isnan(y.a))
        self.assertTrue(np.isnan(y.b))
        self.assertTrue(np.isnan(y.v))

    def test_acos(self):
        """Validate the inverse cos"""
        x = flint(0.5)
        y = np.arccos(x)
        self.assertIsInstance(y, flint)
        self.assertTrue(y.eps > 0)
        self.assertEqual(y, np.pi/3)
        x = flint(0)
        x.interval = -1.2, -0.9
        y = np.arccos(x)
        self.assertEqual(y, np.pi)
        self.assertAlmostEqual(y.b, np.pi)
        self.assertAlmostEqual(y.v, np.pi)
        self.assertAlmostEqual(y.a, np.arccos(-0.9))
        x = flint(0)
        x.interval = 0.8, 1.1
        y = np.arccos(x)
        self.assertEqual(y, 0)
        self.assertAlmostEqual(y.b, np.arccos(0.8))
        self.assertAlmostEqual(y.v, np.arccos(0.95))
        self.assertEqual(y.a, 0)
        x.interval = -1.1, 1.1
        y = np.arccos(x)
        self.assertAlmostEqual(y.b, np.pi)
        self.assertAlmostEqual(y.v, np.pi/2)
        self.assertEqual(y.a, 0)
        x = flint(1.1)
        y = np.arccos(x)
        self.assertTrue(np.isnan(y.a))
        self.assertTrue(np.isnan(y.b))
        self.assertTrue(np.isnan(y.v))
        x = flint(-1.1)
        y = np.arccos(x)
        self.assertTrue(np.isnan(y.a))
        self.assertTrue(np.isnan(y.b))
        self.assertTrue(np.isnan(y.v))

    def test_atan(self):
        """Validate the inverse tan"""
        x = flint(1.0)
        y = np.arctan(x)
        self.assertIsInstance(y, flint)
        self.assertTrue(y.eps > 0)
        self.assertEqual(y, np.pi/4)

    def test_atan2(self):
        """Validate two-input arctan"""
        x = flint(1)
        y = flint(1)
        z = np.arctan2(y,x)
        self.assertIsInstance(z, flint)
        self.assertTrue(z.eps > 0)
        self.assertEqual(z, np.pi/4)
        x = flint(-1)
        y = flint(1)
        z = np.arctan2(y,x)
        self.assertEqual(z, 3*np.pi/4)
        x = flint(1)
        y = flint(-1)
        z = np.arctan2(y,x)
        self.assertEqual(z, -np.pi/4)
        x = flint(-1)
        y = flint(-1)
        z = np.arctan2(y,x)
        self.assertEqual(z, -3*np.pi/4)
        x = flint(1)
        y = flint(0)
        y.interval = -1,1
        z = np.arctan2(y,x)
        self.assertAlmostEqual(z.a, -np.pi/4)
        self.assertAlmostEqual(z.b, np.pi/4)
        self.assertEqual(z.v, 0)
        x = flint(0)
        x.interval = -1,1
        y = flint(1)
        z = np.arctan2(y,x)
        self.assertAlmostEqual(z.a, np.pi/4)
        self.assertAlmostEqual(z.b, 3*np.pi/4)
        self.assertAlmostEqual(z.v, np.pi/2)
        x = flint(0)
        x.interval = -1,1
        y = flint(-1)
        z = np.arctan2(y,x)
        self.assertAlmostEqual(z.a, -3*np.pi/4)
        self.assertAlmostEqual(z.b, -np.pi/4)
        self.assertAlmostEqual(z.v, -np.pi/2)
        x = flint(-1)
        y = flint(0)
        y.interval = -1.000000001,1
        z = np.arctan2(y,x)
        self.assertAlmostEqual(z.a, -5*np.pi/4)
        self.assertAlmostEqual(z.b, -3*np.pi/4)
        self.assertAlmostEqual(z.v, -np.pi)
        x = flint(-1)
        y = flint(0)
        y.interval = -1,1.0000000001
        z = np.arctan2(y,x)
        self.assertAlmostEqual(z.a, 3*np.pi/4)
        self.assertAlmostEqual(z.b, 5*np.pi/4)
        self.assertAlmostEqual(z.v, np.pi)


class TestHyperbolicTrigMath(unittest.TestCase):
    """Test inverse trig functions"""

    def test_sinh(self):
        """Validate sinh"""
        res = 0.5*(np.e-1/np.e)
        x = flint(1.0)
        y = np.sinh(x)
        self.assertIsInstance(y, flint)
        self.assertTrue(y.eps > 0)
        self.assertEqual(y, res)
        x = flint(-1.0)
        y = np.sinh(x)
        self.assertTrue(y.eps > 0)
        self.assertEqual(y, -res)

    def test_cosh(self):
        """Validate cosh"""
        res = 0.5*(np.e+1/np.e)
        x = flint(1.0)
        y = np.cosh(x)
        self.assertIsInstance(y, flint)
        self.assertTrue(y.eps > 0)
        self.assertEqual(y, res)
        x = flint(-1.0)
        y = np.cosh(x)
        self.assertTrue(y.eps > 0)
        self.assertEqual(y, res)
        x = flint(0)
        x.interval = -0.5,1
        y = np.cosh(x)
        self.assertEqual(y.a, 1)
        self.assertAlmostEqual(y.b, res)
        x = flint(0)
        x.interval = -1,0.5
        y = np.cosh(x)
        self.assertEqual(y.a, 1)
        self.assertAlmostEqual(y.b, res)

    def test_tanh(self):
        """Validate tanh"""
        res = (np.e-1/np.e)/(np.e+1/np.e)
        x = flint(1.0)
        y = np.tanh(x)
        self.assertIsInstance(y, flint)
        self.assertTrue(y.eps > 0)
        self.assertEqual(y, res)
        x = flint(-1.0)
        y = np.tanh(x)
        self.assertTrue(y.eps > 0)
        self.assertEqual(y, -res)


class TestInvHyperTrigMath(unittest.TestCase):
    """Test inverse hyperbolic trig functions"""

    def test_asinh(self):
        """Validate asinh"""
        e = flint(np.e)
        x = 0.5*(e-1/e)
        y = np.arcsinh(x)
        self.assertIsInstance(y, flint)
        self.assertTrue(y.eps > 0)
        self.assertEqual(y, 1)
        y = np.arcsinh(-x)
        self.assertTrue(y.eps > 0)
        self.assertEqual(y, -1)

    def test_acosh(self):
        """Validate acosh"""
        e = flint(np.e)
        x = 0.5*(e+1/e)
        y = np.arccosh(x)
        self.assertIsInstance(y, flint)
        self.assertTrue(y.eps > 0)
        self.assertEqual(y, 1)
        x = flint(1)
        x.interval = 0.5,2.5
        y = np.arccosh(x)
        self.assertEqual(y.a, 0)
        self.assertAlmostEqual(y.b, np.arccosh(2.5))
        self.assertAlmostEqual(y.v, np.arccosh(1.5))
        x = flint(0)
        y = np.arccosh(x)
        self.assertTrue(np.isnan(y.a))
        self.assertTrue(np.isnan(y.b))
        self.assertTrue(np.isnan(y.v))

    def test_atanh(self):
        """Validate atanh"""
        e = flint(np.e)
        x = (e-1/e)/(e+1/e)
        y = np.arctanh(x)
        self.assertIsInstance(y, flint)
        self.assertTrue(y.eps > 0)
        self.assertEqual(y, 1)
        y = np.arctanh(-x)
        self.assertTrue(y.eps > 0)
        self.assertEqual(y, -1)
        x = flint(-1)
        x.interval = -1.2,-0.9
        y = np.arctanh(x)
        self.assertTrue(np.isinf(y.a))
        self.assertTrue(np.isinf(y.v))
        self.assertFalse(np.isinf(y.b))
        x = flint(1)
        x.interval = 0.8,1.1
        y = np.arctanh(x)
        self.assertTrue(np.isinf(y.b))
        self.assertFalse(np.isinf(y.v))
        self.assertFalse(np.isinf(y.a))
        x = flint(0)
        x.interval = -1.1,1.1
        y = np.arctanh(x)
        self.assertTrue(np.isinf(y.b))
        self.assertFalse(np.isinf(y.v))
        self.assertTrue(np.isinf(y.a))
        x = flint(-1.1)
        y = np.arctanh(x)
        self.assertTrue(np.isnan(y.a))
        self.assertTrue(np.isnan(y.b))
        self.assertTrue(np.isnan(y.v))
        x = flint(1.1)
        y = np.arctanh(x)
        self.assertTrue(np.isnan(y.a))
        self.assertTrue(np.isnan(y.b))
        self.assertTrue(np.isnan(y.v))


class TestNumpyArray():

    def test_array(self):
        a = np.array([1], dtype=flint)
        assert isinstance(a, np.ndarray)
        assert a.dtype == flint
        assert isinstance(a[0], flint)
        assert a[0].a == np.nextafter(1,-np.inf)
        assert a[0].b == np.nextafter(1, np.inf)
        assert a[0].v == 1.0

    def test_zeros(self):
        a = np.zeros((1,), dtype=flint)
        assert isinstance(a, np.ndarray)
        assert a.dtype == flint
        assert isinstance(a[0], flint)
        assert a[0].a == 0.0
        assert a[0].b == 0.0
        assert a[0].v == 0.0

    def test_fill(self):
        x = flint(1.5)
        x.interval = 1,2
        a = np.full((3,4), x, dtype=flint)
        assert isinstance(a, np.ndarray)
        assert a.dtype == flint
        for val in np.nditer(a):
            item = val.item()
            assert isinstance(item, flint)
            assert item.a == x.a
            assert item.b == x.b
            assert item.v == x.v

    def test_copy_row(self):
        a = np.zeros((3,3), dtype=flint)
        b = np.arange(1,4, dtype=flint)
        zero_row = np.zeros((3,))
        assert np.alltrue( a[0] == zero_row )
        assert np.alltrue( a[1] == zero_row )
        assert np.alltrue( a[2] == zero_row )
        a[1] = b
        assert np.alltrue( a[0] == zero_row )
        assert np.alltrue( a[1] == b )
        assert np.alltrue( a[2] == zero_row )

    def test_copy_col(self):
        a = np.zeros((3,3), dtype=flint)
        b = np.arange(1,4, dtype=flint)
        zero_row = np.zeros((3,))
        assert np.alltrue( a[:,0] == zero_row )
        assert np.alltrue( a[:,1] == zero_row )
        assert np.alltrue( a[:,2] == zero_row )
        a[:,1] = b
        assert np.alltrue( a[:,0] == zero_row )
        assert np.alltrue( a[:,1] == b )
        assert np.alltrue( a[:,2] == zero_row )
