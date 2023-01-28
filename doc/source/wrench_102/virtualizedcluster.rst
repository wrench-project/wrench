.. _guide-102-virtualizedcluster:

Interacting with a virtualized cluster service
==============================================

The :cpp:class:`wrench::VirtualizedClusterComputeService` derives the
:cpp:class:`wrench::CloudComputeService` class. One interacts with it in almost
the same way as one :ref:`interacts with a cloud
servicde <guide-102-cloud>`. The one difference between a
virtualized cluster service and a cloud service is that the former
exposes underlying physical resources, while the latter does not. More
simply put, with a virtualized cluster service one can create VM
instances on specific hosts, and migrate VM instances between hosts.

Here is an example interaction with a virtualized cluster service, in
which VM instances are created and (live) migrated:

.. code:: cpp

   std::shared_ptr<wrench::VirtualizedClusterComputeService> virtualized_cluster_cs;


   // Create a VM with 2 cores and 1 GiB of RAM on Host1, which could fail
   // if not enough resources are available
   auto vm1_name = virtualized_cluster_cs->createVM(2, pow(2,30), "Host1");

   // Create a VM with 4 cores and 2 GiB of RAM on Host2 , which could fail
   // if not enough resources are available
   auto vm2_name = virtualized_cluster_cs->createVM(4, pow(2,31), "Host2");

   [...]

   // Start the first VM on Host1
   virtualized_cluster_cs->startVM(vm1_name);

   // Start the second VM on Host2
   virtualized_cluster_cs->startVM(vm2_name);

   [...]

   // Live migrate vm1 to Host3
   virtualized_cluster_cs->migrateVM(vm1_name, "Host3");

   // Live migrate vm2 to Host4
   virtualized_cluster_cs->migrateVM(vm2_name, "Host4");

   [...]

   // Shutdown the VMs
   virtualized_cluster_cs->shutdownVM(vm1_name);
   virtualized_cluster_cs->shutdownVM(vm2_name);

   // Destroy the VMs, which releases resources
   virtualized_cluster_cs->destroyVM(vm1_name);
   virtualized_cluster_cs->destroyVM(vm2_name);

In the code above the VM instances are not used for anything. See the
:ref:`interacting with a cloud service page <guide-102-cloud>` for
an example in which jobs are submitted to the VM instances.

See the execution controller implementation in
``examples/workflow_api/basic-examples/virtualized-cluster-bag-of-tasks/TwoTasksAtATimeVirtualizedClusterWMS.cpp``
for a more complete example.
