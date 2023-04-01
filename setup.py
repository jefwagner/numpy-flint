from setuptools import setup, Extension
import numpy as np

setup_args = dict(
    ext_modules = [
        Extension(
            name='flint.numpy_flint',
            sources=['src/numpy_flint.c'],
            depends=[
                'src/flint.h',
                'src/numpy_flint.c',
            ],
            include_dirs=[np.get_include(),'src'],
        )
    ]
)

setup(**setup_args)
