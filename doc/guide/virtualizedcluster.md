Virtualized Cluster                        {#guide-virtualizedcluster}
============

@WRENCHUserDoc <div class="doc-type">User Documentation</div><div class="doc-link">Other: <a href="../developer/guide-virtualizedcluster.html">Developer</a> - <a href="../internal/guide-virtualizedcluster.html">Internal</a></div> @endWRENCHDoc
@WRENCHDeveloperDoc  <div class="doc-type">Developer Documentation</div><div class="doc-link">Other: <a href="../user/guide-virtualizedcluster.html">User</a> - <a href="../internal/guide-virtualizedcluster.html">Internal</a></div> @endWRENCHDoc
@WRENCHInternalDoc  <div class="doc-type">Internal Documentation</div><div class="doc-link">Other: <a href="../user/guide-virtualizedcluster.html">User</a> -  <a href="../developer/guide-virtualizedcluster.html">Developer</a></div> @endWRENCHDoc

[TOC]

# Overview #            {#guide-virtualizedcluster-overview}

A virtualized cluster service is an abstraction of a 
[cloud service](@ref guide-cloud) that corresponds to a
virtual cluster platform that provides access to virtualized compute 
resources, i.e., virtual machines (VMs).
The main difference between a [cloud service](@ref guide-cloud) and 
a virtualized cluster service is that the latter does expose the 
underlying physical infrastructure (e.g., it is possible to instantiate 
a VM on a particular physical host, or to migrate a VM between two 
particular physical hosts).


# Creating a virtualized cluster compute service #        {#guide-virtualizedcluster-creating}

In WRENCH, a virtualized cluster service represents a cloud service (wrench::CloudService, 
which itself represents a wrench::ComputeService), 
which is defined by the wrench::VirtualizedClusterService class. An instantiation of a 
virtualized cluster service requires the following parameters:

- A hostname on which to start the service (this is the entry point to the service)
- A list (`std::vector`) of hostnames (all cores and all RAM of each host is available to the virtualized cluster service) 
- A scratch space size, i.e., the size in bytes of storage local to the virtualized cluster service (used to store
  workflow files, as needed, during job executions) 
- Maps (`std::map`) of configurable properties (`wrench::VirtualizedClusterServiceProperty`) and configurable message 
  payloads (`wrench::VirtualizedClusterServiceMessagePayload`).

The example below shows how to create an instance of a virtualized cluster service 
that runs on host "vc_gateway", provides access to 4 execution hosts, and has a scratch 
space of 1TiB:

~~~~~~~~~~~~~{.cpp}
auto virtualized_cluster_cs = simulation.add(
          new wrench::VirtualizedClusterService("vc_gateway", {"host1", "host2", "host3", "host4"}, pow(2,40),
                                                {{wrench::VirtualizedClusterServiceProperty::SUPPORTS_PILOT_JOBS, "false"}}));
~~~~~~~~~~~~~

## Virtualized cluster service properties             {#guide-virtualizedcluster-creating-properties}

All properties for a virtualized cluster service are inherited from `wrench::CloudServiceProperty` 
(see the [Cloud service properties](@ref guide-cloud-creating-properties) section).


@WRENCHNotUserDoc

# Migrating a VM to another execution host #        {#guide-virtualizedcluster-migrating}  

As expected, a virtualized cluster service provides access to all mechanisms to
[manage](@ref guide-cloud-managing) the set of VMs instantiated on the execution 
hosts. In addition to such capabilities, the virtualized cluster service provides
an additional wrench::VirtualizedClusterService::createVM function, which instantiates
a VM on a specific execution host; and a wrench::VirtualizedClusterService::migrateVM 
function, which allows VM migration between execution hosts.

Here is an example of VMs migration on a virtualized cluster service:

~~~~~~~~~~~~~{.cpp}
// Create a VM with 2 cores and 1 GiB of RAM on host1
auto vm1 = virtualized_cluster_cs->createVM("host1", 2, pow(2,30));
// Create a VM with 4 cores and 2 GiB of RAM on host2
auto vm2 = virtualized_cluster_cs->createVM("host2", 4, pow(2,31));

// Migrating vm1 to host3
virtualized_cluster_cs->migrateVM(vm1, "host3");
// Migrating vm2 to host4
virtualized_cluster_cs->migrateVM(vm2, "host4");
~~~~~~~~~~~~~


@endWRENCHDoc
