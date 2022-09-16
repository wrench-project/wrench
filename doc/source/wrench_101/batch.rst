.. _guide-101-batch:

Creating a batch compute service
================================

.. _guide-batch-overview:

Overview
========

A batch service is a service that makes it possible to run jobs on a
homogeneous cluster managed by a batch scheduler. The batch scheduler
receives requests that ask for a number of compute nodes, with a number
of cores per compute node, and a duration. Requests wait in a queue and,
using a range of possible batch scheduling algorithms, are dispatched to
the requested compute resources in a space-sharing manner. Therefore, a
job submitted to the service experiences a “queue waiting time” period
(the length of which depends on the load on the service) followed by an
“execution time” period. In typical batch-scheduler fashion, a running
job is forcefully terminated when it reaches its requested duration
(i.e., the job fails). If, instead, the job completes before the
requested duration, it succeeds. In both cases, the job’s allocated
compute resources are reclaimed by the batch scheduler.

A batch service also supports so-called “pilot jobs”, i.e., jobs that
are submitted to the service, with requested resources and duration, but
without specifying at submission time which workflow tasks/operations
should be performed by the job. Instead, once the job starts it exposes
to its submitter a :ref:`bare-metal <guide-101-baremetal>` service.
This service is available only for the requested duration, and can be
used in any manner by the submitter. This allows late binding of
workflow tasks to compute resources.

.. _guide-batch-creating:

Creating a batch compute service
================================

In WRENCH, a batch service is defined by the
:cpp:class:`wrench::BatchComputeService` class. An instantiation of a batch
service requires the following parameters:

-  The name of a host on which to start the service;
-  A list (``std::vector``) of hostnames (all cores and all RAM of each
   host is available to the batch service);
-  A mount point (corresponding to a disk attached to the host) for the
   scratch space, i.e., storage local to the batch service (used to
   store workflow files, as needed, during job executions); and
-  Maps (``std::map``) of configurable properties
   (:cpp:class:`wrench::BatchComputeServiceProperty`) and configurable message
   payloads (:cpp:class:`wrench::BatchComputeServiceMessagePayload`).

The example below creates an instance of a batch service that runs on
host ``Gateway`` and provides access to 4 hosts (using all their cores
and RAM), with scratch space on the disk mounted at path ``/scratch/``
at host ``Gateway``. Furthermore, the batch scheduling algorithm is
configured to use the FCFS (First-Come-First-Serve) algorithm, and the
message with which the service answers resource request description
requests is configured to be 1KiB:

.. code:: cpp

   auto batch_cs = simulation->add(
             new wrench::BatchComputeService("Gateway",
                                      {"Node1", "Node2", "Node3", "Node4"},
                                      "/scratch/",
                                      {{wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, "fcfs"}},
                                      {{wrench::BareMetalComputeServiceMessagePayload::RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD, 1024}});

See the documentation of :cpp:class:`wrench::BatchComputeServiceProperty` and
:cpp:class:`wrench::BatchComputeServiceMessagePayload` for all possible
configuration options.

Also see the simulators in the ``examples/workflow_api/basic-examples/batch-*/`` and
``examples/action_api/batch-*/``
directories, which use batch compute services.
