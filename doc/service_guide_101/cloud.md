CloudComputeService                        {#guide-101-cloud}
============

[TOC]

# Overview #            {#guide-cloud-overview}

A cloud service is an abstraction of a compute service that corresponds to a
cloud platform that provides access to virtualized compute resources, i.e., 
virtual machines (VMs). The cloud service provides all necessary abstractions 
to manage VMs, including creation, suspension, resume, etc. Compute jobs 
submitted to the cloud service run on previously created VM instances.
If a VM that meets a job's requirements cannot be found, the 
service will throw an exception. In the cloud service, a VM instance behaves as a
[bare-metal](@ref guide-baremetal) service.

The main difference between a cloud service and a 
[virtualized cluster service](@ref guide-virtualizedcluster) 
is that the latter does expose the underlying
physical infrastructure (e.g., it is possible to instantiate a VM on a
particular physical host, or to migrate a VM between two particular
physical hosts).


# Creating a cloud compute service #        {#guide-cloud-creating}

In WRENCH, a cloud service represents a compute service (wrench::ComputeService), 
which is defined by the wrench::CloudComputeService class. An instantiation of a cloud
service requires the following parameters:

- A hostname on which to start the service (this is the entry point to the service)
- A list (`std::vector`) of hostnames (all cores and all RAM of each host is available to the cloud service) 
- A mount point (corresponding to a disk attached to the host) for the scratch space, i.e., storage local to the cloud service (used to store
  workflow files, as needed, during job executions) 
- Maps (`std::map`) of configurable properties (`wrench::CloudServiceProperty`) and configurable message 
  payloads (`wrench::CloudComputeServiceMessagePayload`).

The example below shows how to create an instance of a cloud service that runs 
on host "cloud_gateway", provides access to 4 execution hosts, and has a scratch 
space of 1TiB:

~~~~~~~~~~~~~{.cpp}
auto cloud_cs = simulation.add(
          new wrench::CloudComputeService("cloud_gateway", {"host1", "host2", "host3", "host4"}, "/scratch/",
                                   {{wrench::CloudServiceProperty::SUPPORTS_PILOT_JOBS, "false"}}));
~~~~~~~~~~~~~

## Cloud service properties             {#guide-cloud-creating-properties}

In addition to properties inherited from `wrench::ComputeServiceProperty`, a cloud 
service supports the following properties:

- `wrench::CloudServiceProperty::VM_BOOT_OVERHEAD_IN_SECONDS`

