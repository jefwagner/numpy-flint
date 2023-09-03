Building and Testing from Source
================================

There are several reason you might want to build the project from source instead of
using the prebuild binarys from PyPI.

1. You might want to add your own awesome feature.
2. You might want to run the project on one of the unsuppoted architechure. 
3. You might just want to mess around.

In any case following the following steps to build the project locally.

Cloning the repo and setting up development environment
-------------------------------------------------------

Numpy-flint is a python C-Extension project, so it builds using a C compiler to compile
source code written in the C programming language. So the first step is getting a a C
compiler installed and accessable through the command-line, usually mapped to the `CC`
`sysconfig` variable or environment variable.

Once you have a C compiler accessable, you need to clone the source code and change
directorys into the newly created folder. 

.. prompt:: bash $

    git clone https://github.com/jefwanger/numpy-flint.git
    cd numpy-flint

It is strongly recommended that for all development work in python that you work in a
separate development virtual environment. Using the built-in python venv package you
can create a folder specific environment and activate it.

.. prompt:: bash $

    python -m venv .venv
    source .venv/bin/activate

Finally, you need to install the `build` and `pytest` packages in the development
virtual environment using pip.

.. prompt:: bash (.venv) $

    pip install build pytest

.. note::

    These commands are all assuming a bash terminal in linux or unix like environment,
    please substitute the appropriate commands for your specific terminal.


Building and making a local install
-----------------------------------

Now your development environment is set up, just make sure to activate the `.venv`
virtual environment any time you take a break and come back to work on the project. To
build the project you simply call python's `build` module.

.. prompt:: bash (.venv) $

    python -m build

If this works, it will place a wheel file in the `dist` folder. If it doesn't work it
will show the output from the compiler showing you what step failed. You can install the
wheel file directly with pip, but I have found it more convenient to do an editable
install of the project in your build environment.

.. prompt:: bash (.venv) $

    pip install -e .

This should place a dynamically linked library (.dll on windows, .so in linux, and
.dynlib in macos) in the flint `src\flint` folder. You can then run the test cases
from the command line with pytest.

.. prompt:: bash (.venv) $

    pytest


Building the documentation
--------------------------

The `sphinx <https://www.sphinx-doc.org/>`_ package is used to build the documentation.
To rebuild the documentation you need to install sphinx (and the furo theme) into the
development environment.

.. prompt:: bash (.venv) $

    pip install sphinx furo

All of the documentation is either in the comments or docstrings in the source files or
in the .rst files in the `docs` folder. To build the documentaiton you can use the
`sphinx-build``

.. prompt:: bash (.venv) $

    sphinx-build -b html docs dist/html

which should create a copy of the documentation webpages in the `dist/html` subfolder.


Contributing to the project
---------------------------

Numpy-flint is an open source project that was created and is maintained by a single
person. If you would like to contribute, I encourage you to follow standard open source
procedure:

1. Open up an issue on the `Bug Tracker <https://github.com/jefwagner/numpy-flint/issues>`_ with your proposal.
2. Fork the project and create a new branch for your contribution.
3. After testing, add create a pull request to merge your changes back into the project.

A good resource I found is `Open Source Guide's how-to article
<https://opensource.guide/how-to-contribute/>`_.
