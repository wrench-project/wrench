VirtualizedClusterComputeService                        {#guide-101-virtualizedcluster}
============


[TOC]

# Overview #            {#guide-virtualizedcluster-overview}

A virtualized cluster service is an abstraction of
a compute service that
corresponds to a platform of physical resources on which Virtual Machine
(VM) instances can be created.  The virtualized cluster service provides
all necessary abstractions to manage VMs and their allocation to physical
resources, including creation on specific physical hosts, suspend/resume,
migration, etc. Compute jobs submitted to the virtualized cluster service
run on previously created VM instances.  If a VM that meets a job's
requirements cannot be found, the service will throw an exception.  In the
virtualized cluster service, a VM behaves as a [bare-metal](@ref guide-baremetal) 
service.


The main difference between a [cloud service](@ref guide-cloud) and 
a virtualized cluster service is that the latter does expose the 
underlying physical infrastructure (e.g., it is possible to instantiate 
a VM on a particular physical host, or to migrate a VM between two 
particular physical hosts).


# Creating a virtualized cluster compute service #        {#guide-virtualizedcluster-creating}

In WRENCH, a virtualized cluster service represents a cloud service (wrench::CloudComputeService,
which itself represents a wrench::ComputeService), 
which is defined by the wrench::VirtualizedClusterComputeService class. An instantiation of a
virtualized cluster service requires the following parameters:

- A hostname on which to start the service (this is the entry point to the service)
- A list (`std::vector`) of hostnames (all cores and all RAM of each host is available to the virtualized cluster service) 
- A mount point (corresponding to a disk attached to the host) for the scratch space, i.e., storage local to the virtualized cluster service (used to store
  workflow files, as needed, during job executions)  
- Maps (`std::map`) of configurable properties (`wrench::VirtualizedClusterComputeServiceProperty`) and configurable message
  payloads (`wrench::VirtualizedClusterComputeServiceMessagePayload`).

The example below shows how to create an instance of a virtualized cluster service 
that runs on host "vc_gateway", provides access to 4 execution hosts, and has a scratch 
space of 1TiB:

~~~~~~~~~~~~~{.cpp}
auto virtualized_cluster_cs = simulation.add(
          new wrench::VirtualizedClusterComputeService("vc_gateway", {"host1", "host2", "host3", "host4"}, 
                                                "/scratch/",
                                                {{wrench::VirtualizedClusterComputeServiceProperty::SUPPORTS_PILOT_JOBS, "false"}}));
~~~~~~~~~~~~~

## Virtualized cluster service properties             {#guide-virtualizedcluster-creating-properties}

All properties for a virtualized cluster service are inherited from `wrench::CloudServiceProperty` 
(see the [Cloud service properties](@ref guide-cloud-creating-properties) section).

