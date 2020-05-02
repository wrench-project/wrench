Interacting with a virtualized cluster service     {#guide-102-virtualizedcluster}
============

The `wrench::VirtualizedClusterComputeService` derives the
`wrench::CloudComputeService` class. One interacts  with it in almost
the same way as one [interacts with a  cloud servicde](@ref guide-102-cloud).
The one difference between a virtualized
cluster service and a cloud service is that the former exposes underlying
physical resources, while the latter does not.  More simply put, with  a
virtualized cluster service one can create VM instances on specific hosts,
and migrate VM instances between hosts. 

Here is an example interaction with a virtualized cluster service,  in which 
VM instances are created and (live) migrated:

~~~~~~~~~~~~~{.cpp}
// Get the first virtualized cluster compute service (assuming there's at least one)
auto virtualized_cluster_cs = *(this->getAvailableComputeServices<wrench::VirtualizedClusterComputeService>().begin());


// Create a VM with 2 cores and 1 GiB of RAM
auto vm1 = virtualized_cluster_cs->createVM(2, pow(2,30));

// Create a VM with 4 cores and 2 GiB of RAM
auto vm2 = virtualized_cluster_cs->createVM(4, pow(2,31));

[...]

// Start the first VM on Host1
virtualized_cluster_cs->startVM(vm1, "Host1");

// Start the second VM on Host2
virtualized_cluster_cs->startVM(vm1, "Host2");

[...]

// Live migrate vm1 to host3
virtualized_cluster_cs->migrateVM(vm1, "host3");

// Live migrate vm2 to host4
virtualized_cluster_cs->migrateVM(vm2, "host4");

[...]

// Shutdown the VMs
virtualized_cluster_cs->shutdownVM(vm1);
virtualized_cluster_cs->shutdownVM(vm2);
~~~~~~~~~~~~~


In the code above the VM instances are not used for anything. See the
[interacting with a cloud service page](@ref guide-102-cloud)
for an example in which jobs are submitted to the VM instances. 

See the WMS implementation in `examples/basic-examples/virtualized-cluster-bag-of-tasks/TwoTasksAtATimeVirtualizedClusterWMS.cpp` for a more complete example.

