.. _guide-101-baremetal:

Creating a bare-metal compute service
=====================================

.. _guide-baremetal-overview:

Overview
========

A bare-metal compute service makes it possible to run tasks directly on
hardware resources. Think of it as a set of multi-core hosts on which
multi-threaded processes can be started using something like Ssh. The
service does not perform any space-sharing among the jobs. In other
words, jobs submitted to the service execute concurrently in a
time-shared manner. It is the responsibility of the job submitter to
pick hosts and/or numbers of cores for each task, e.g., to enforce
space-sharing of cores. But by default the compute service operates as a
“jungle” in which tasks share cores at will. The only resource
allocation performed by the service is that it ensures that the RAM
capacity of a hosts are not exceeded. Tasks that have non-zero RAM
requirements are queued in FCFS fashion at each host until there is
enough RAM to execute them (think of this as each host running an OS
that disallows swapping and implements a FCFS access policy for RAM
allocation).

.. _guide-baremetal-creating:

Creating a bare-metal compute service
=====================================

In WRENCH, a bare-metal service is defined in the
:cpp:class:`wrench::BareMetalComputeService` class. An instantiation of a
bare-metal service requires the following parameters:

-  The name of a host on which to start the service;
-  A set of compute hosts in a map (``std::map``), where each key is a
   hostname and each value is a tuple (``std::tuple``) with a number of
   cores and a RAM capacity;
-  A mount point (corresponding to a disk attached to the host) for the
   scratch space, i.e., storage local to the bare-metal service (used to
   store workflow files, as needed, during job executions); and
-  Maps (``std::map``) of configurable properties
   (:cpp:class:`wrench::BareMetalComputeServiceProperty`) and configurable
   message payloads (:cpp:class:`wrench::BareMetalComputeServiceMessagePayload`).

The example below creates an instance of a bare-metal service that runs
on host ``Gateway``, provides access to all cores and 1GiB of RAM on
host ``Node1`` and to 8 cores and all RAM on host ``Node2``, and has a
scratch space on the disk mounted at path ``/scratch`` on host
``Gateway``. Furthermore, the thread startup overhead is configured to
be one hundredth of a second, and the message with which the service
answers resource request description requests is configured to be 1KiB:

.. code:: cpp

   auto baremetal_cs = simulation->add(
             new wrench::BareMetalComputeService("Gateway", 
                                          {{"Node1", std::make_tuple(wrench::ComputeService::ALL_CORES, pow(2,30))}, 
                                          {"Node2", std::make_tuple(8, wrench::ComputeService::ALL_RAM}},
                                          "/scratch/",
                                          {{wrench::BareMetalComputeServiceProperty::THREAD_STARTUP_OVERHEAD, "0.01"}}, 
                                          {{wrench::BareMetalComputeServiceMessagePayload::RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD, 1024});

See the documentation of :cpp:class:`wrench::BareMetalComputeServiceProperty` and
:cpp:class:`wrench::BareMetalComputeServiceMessagePayload` for all possible
configuration options.

Also see the simulators in the ``examples/workflow_api/basic-examples/bare-metal-*/`` and
``examples/action_api/bare-metal-*/``
directories, which use bare-metal compute services.
