.. _guide-101-htcondor:

Creating a HTCondor compute service
===================================

.. _guide-htcondor-overview:

Overview
========

:ref:`HTCondor <http://htcondor.org>` is a workload management framework
that supervises task executions on various sets of resources. HTCondor
is composed of six main service daemons (``startd``, ``starter``,
``schedd``, ``shadow``, ``negotiator``, and ``collector``). In addition,
each host on which one or more of these daemons is spawned must also run
a ``master`` daemon, which controls the execution of all other daemons
(including initialization and completion).

.. _guide-htcondor-creating:

Creating an HTCondor Service
============================

HTCondor is composed of a pool of resources in which jobs are submitted
to perform their computation. In WRENCH, an HTCondor service represents
a compute service (:cpp:class:`wrench::ComputeService`), which is defined by the
:cpp:class:`wrench::HTCondorComputeService` class. An instantiation of an
HTCondor service requires the following parameters:

-  The name of a host on which to start the service;
-  A ``std::set`` of ‘child’ :cpp:class:`wrench::ComputeService` instances
   available to the HTCondor pool; and
-  A ``std::map`` of properties
   (:cpp:class:`wrench::HTCondorComputeServiceProperty`) and message payloads
   (:cpp:class:`wrench::HTCondorComputeServiceMessagePayload`).

The set of compute services may include compute service instances that
are either :cpp:class:`wrench::BareMetalComputeService` or
:cpp:class:`wrench::BatchComputeService` instances. The example below creates an
instance of an HTCondor service with a pool of resources containing a
:ref:`Bare-metal <guide-101-baremetal>` server:

.. code:: cpp

   // Simulation 
   wrench::Simulation simulation;
   simulation.init(&argc, argv);

   // Create a bare-metal service

   auto baremetal_service = simulation.add(
       new wrench::BareMetalComputeService(
             "execution_hostname",
             {std::make_pair(
                     "execution_hostname",
                     std::make_tuple(wrench::Simulation::getHostNumCores("execution_hostname"),
                                     wrench::Simulation::getHostMemoryCapacity("execution_hostname")))},
             "/scratch/"));

   std::set<std::shared_ptr<wrench::ComputeService>> compute_services;
   compute_services.insert(baremetal_service);

   auto htcondor_compute_service = simulation->add(
             new wrench::HTCondorComputeService(hostname, 
                                         std::move(compute_services),
                                         {{wrench::HTCondorComputeServiceProperty::SUPPORTS_PILOT_JOBS, "false"}}
                                         ));

Jobs submitted to the :cpp:class:`wrench::HTCondorComputeService` instance will
be dispatched automatically to one of the ‘child’ compute services
available to that instance (only one in the above example).
