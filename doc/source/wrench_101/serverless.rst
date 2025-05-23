.. _guide-101-serverless:

Creating a serverless compute service
=====================================

.. _guide-serverless-overview:

Overview
========

A serverless compute service provides users with a "function" abstraction,
as done by providers such as Google Functions, AWS lambdas, etc.  The
serverless compute service provides all necessary functions to register and
invoke functions, which, in practice, run inside containers.

.. _guide-serverless-architecture:

Architecture of a serverless compute service
============================================

A serverless compute service consists of:

- A head node with disk storage
- A set of homogeneous compute nodes, each with its own local disk storage

Once a function is registered at the compute service, and once it's image is downloaded
to the head node's storage, then the function can be invoked.  A function invocation 
on a core of a compute node consists in:

- Copying the function's image from the head node's storage to the compute node's storage, if not already present there
- Loading the function's image from the compute node's storage to the compute node's RAM, if not already present there
- Executing the function's code

The choice of which function invocation to execute on which compute node is done by a
:cpp:class:`wrench::ServerlessScheduler` class.  A key thing to note is that each compute node's
storage and RAM are managed using an LRU (Least Recently Used) scheme.  That is, images
are automatically evicted from disk or RAM. 


.. _guide-serverless-creating:

Creating a serverless compute service
=====================================

In WRENCH, a serverless compute service is defined by the
:cpp:class:`wrench::ServerlessComputeService` class. An instantiation of a serverless
service requires the following parameters:

-  The name of a head host on which to start the service;
-  A list (``std::vector``) of hostnames for homogeneous hosts. All cores and all RAM of each
   host are available to the cloud service), and each host has a disk with mount point '/';
-  A mount point (corresponding to a disk attached to the head host) for the
   storage local to the cloud service (used to
   store container images);
-  A scheduler that defines the resource management algorithms used internally
   to decide which function invocations should be executed and when; and
-  Maps (``std::map``) of configurable properties
   (:cpp:class:`wrench::ServerlessComputeServiceProperty`) and configurable message
   payloads (:cpp:class:`wrench::ServerlessComputeServiceMessagePayload`).


The example below creates an instance of a serverless compute service that runs on
host ``serverless_gateway``, provides access to 4 execution hosts, and has storage
on the disk mounted at path ``/`` at host
``serverless_gateway``. The scheduler is the built-in FCFS (First Come First Serve) scheduler, and
the container startup overhead is set to 1 second:

.. code:: cpp

   auto serverless_cs = simulation->add(
             new wrench::ServerlessComputeService("serverless_gateway", {"host1", "host2", "host3", "host4"}, "/",
                                      {{wrench::ServerlessComputeServiceProperty::CONTAINER_STARTUP_OVERHEAD, "1s"}},
                                      {}));

See the documentation of :cpp:class:`wrench::ServerlessComputeServiceProperty` for all possible
configuration options.   See :ref:`this page <guide-102-serverless>` for more information of how
a serverless compute infrastructure is simulated. 

Also see the simulator in the ``examples/serverless_api/basic/``
directory, which uses a serverless compute service.
