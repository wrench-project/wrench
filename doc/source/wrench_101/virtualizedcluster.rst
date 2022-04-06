.. _guide-101-virtualizedcluster:

Creating a virtualized cluster compute service
==============================================

.. _guide-virtualizedcluster-overview:

Overview
========

A virtualized cluster service is an abstraction of a compute service
that corresponds to a platform of physical resources on which Virtual
Machine (VM) instances can be created. A virtualized cluster service is
very similar to a a :ref:`cloud service <guide-101-cloud>`, the only
difference being that the former exposes the underlying physical
resources, while the latter does not. More specifically, it is possible
to instantiate a VM on a particular physical host, and to migrate a VM
between two physical hosts.

.. _guide-virtualizedcluster-creating:

Creating a virtualized cluster compute service
==============================================

In WRENCH, a virtualized cluster service is defined by the
wrench::VirtualizedClusterComputeService class. An instantiation of a
virtualized cluster service requires the following parameters:

-  The name of a host on which to start the service;
-  A list (``std::vector``) of hostnames (all cores and all RAM of each
   host is available to the virtualized cluster service);
-  A mount point (corresponding to a disk attached to the host) for the
   scratch space, i.e., storage local to the virtualized cluster service
   (used to store workflow files, as needed, during job executions); and
-  Maps (``std::map``) of configurable properties
   (:cpp:class:`wrench::VirtualizedClusterComputeServiceProperty`) and
   configurable message payloads
   (:cpp:class:`wrench::VirtualizedClusterComputeServiceMessagePayload`).

The example below creates an instance of a virtualized cluster service
that runs on host ``vc_gateway``, provides access to 4 execution hosts,
and has a scratch space on the disk mounted at path ``/scratch`` at host
``vc_gateway``. Furthermore, the VM boot time is configured to be 10
second, and the message with which the service answers resource
description requests is configured to be 1KiB:

.. code:: cpp

   auto virtualized_cluster_cs = simulation.add(
             new wrench::VirtualizedClusterComputeService(
                                                   "vc_gateway", 
                                                   {"host1", "host2", "host3", "host4"}, 
                                                   "/scratch/", 
                                                   {{wrench::VirtualClusterComputeServiceProperty::VM_BOOT_OVERHEAD_IN_SECONDS, "10"}}, 
                                                   {{wrench::VirtualClusterComputeServiceMessagePayload::RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD, 
                                                   1024}}));

See the documentation of
:cpp:class:`wrench::VirtualizedClusterComputeServiceProperty` and
:cpp:class:`wrench::VirtualizedClusterComputeServiceMessagePayload` for all
possible configuration options.

Also see the simulators in the
``examples/basic-examples/virtualized-cluster-*/`` directories, which
use virtualized cluster services.
