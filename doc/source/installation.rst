.. _install:

Installing WRENCH
*****************

.. _install-prerequisites:

Prerequisites
=============

WRENCH is developed in ``C++``. The code follows the C++14 standard, and
thus older compilers may fail to compile it. Therefore, we strongly
recommend users to satisfy the following requirements:

-  **CMake** - version 3.10 or higher

And, one of the following: - **g++** - version 7.5 or higher - **clang**
- version 9.0 or higher

.. _install-prerequisites-dependencies:

Required Dependencies
---------------------

-  `SimGrid <https://simgrid.org/>`__ – version 3.31
-  `JSON for Modern C++ <https://github.com/nlohmann/json>`__ – version
   3.9.0 or higher

(See the :ref:`install-troubleshooting` section below if encountering difficulties
installing dependencies)

.. _install-prerequisites-opt-dependencies:

Optional Dependencies
---------------------

-  `Google Test <https://github.com/google/googletest>`__ – version 1.8
   or higher (only required for running tests)
-  `Doxygen <http://www.doxygen.org>`__ – version 1.8 or higher (only
   required for generating documentation)
-  `Sphinx <https://www.sphinx-doc.org/en/master/usage/installation.html>`__ - 
   version 4.5 or higher along with the following Python packages: 
   ``pip3 install sphinx-rtd-theme breathe recommonmark``  (only required 
   for generating documentation)
-  `Batsched <https://gitlab.inria.fr/batsim/batsched>`__ – version 1.4
   - useful for expanded batch-scheduled resource simulation
   capabilities

.. _install-source:

Source Install
==============

.. _install-source-build:

Building WRENCH
---------------

You can download the ``wrench-<version>.tar.gz`` archive from the `GitHub
releases <https://github.com/wrench-project/wrench/releases>`__ page.
Once you have installed dependencies (see above), you can install WRENCH
as follows:

.. code:: sh

      tar xf wrench-<version>.tar.gz
      cd wrench-<version>
      mkdir build
      cd build
      cmake ..
      make -j8
      make install # try "sudo make install" if you do not have write privileges

If you want to see actual compiler and linker invocations, add
``VERBOSE=1`` to the compilation command:

.. code:: sh

   make -j8 VERBOSE=1

To enable the use of Batsched (provided you have installed that package,
see above): 

.. code:: sh

   cmake -DENABLE_BATSCHED=on .

If you want to stay on the bleeding edge, you should get the latest git
version, and recompile it as you would do for an official archive:

.. code:: sh

   git clone https://github.com/wrench-project/wrench

.. _install-unit-tests:

Compiling and running unit tests
--------------------------------

Building and running the unit tests, which requires Google Test, is done
as:

.. code:: sh

   make -j8 unit_tests
   ./unit_tests

.. _install-examples:

Compiling and running examples
------------------------------

Building the examples is done as:

.. code:: sh

   make -j8 examples

All binaries for the examples are then created in subdirectories of
``build/examples/``

.. _install-troubleshooting:

Installation Troubleshooting
----------------------------

Could NOT find PkgConfig (missing: PKG_CONFIG_EXECUTABLE)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  This error on MacOS is because the ``pkg-config`` package is not
   installed
-  Solution: install this package

   -  MacPorts: ``sudo port install pkg-config``
   -  Brew: ``sudo brew install pkg-config``

Could not find libgfortran when building the SimGrid dependency
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  This is an error that sometimes occurs on MacOS
-  A quick fix is to disable the SMPI feature of SimGrid when
   configuring it: ``cmake -Denable_smpi=off .``

.. _install-docker:

Docker Containers
=================

WRENCH is also distributed in Docker containers. Please, visit the
`WRENCH Repository on Docker
Hub <https://hub.docker.com/r/wrenchproject/wrench/>`__ to pull WRENCH’s
Docker images.

The ``latest`` tag provides a container with the latest `WRENCH
release <https://github.com/wrench-project/wrench/releases>`__:

.. code:: sh

   docker pull wrenchproject/wrench 
   # or
   docker run --rm -it wrenchproject/wrench /bin/bash

The ``unstable`` tag provides a container with the (almost) current code
in the GitHub’s ``master`` branch:

.. code:: sh

   docker pull wrenchproject/wrench:unstable
   # or
   docker run --rm -it wrenchproject/wrench:unstable /bin/bash

Additional tags are available for all WRENCH releases.
