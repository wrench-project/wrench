WRENCH REST API
***************

WRENCH provides a REST API, so as to provide a language-agnostic way to develop
WRENCH simulators (at the cost of extra overhead). To this end, the WRENCH distribution
comes with a "WRENCH daemon" executable, which
can be built and installed as:

.. code:: sh

   make wrench-daemon
   make install  # try "sudo make install" if you do not have write privileges

The wrench-daemon is to be started on your local machine and comprises an
HTTP server that answers REST API requests. Use ``wrench-daemon --help`` for command-line options.

The full documentation of the REST API is provided on `this page <restapi/index.html>`_

We have developed a `Python API to WRENCH <https://github.com/wrench-project/wrench-python-api/>`__,
which sits on top of the above REST API.
