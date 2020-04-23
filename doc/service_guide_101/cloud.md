Creating a cloud compute service                        {#guide-101-cloud}
============

[TOC]

# Overview #            {#guide-cloud-overview}

A cloud service is an abstraction of a compute service that corresponds to a
cloud platform that provides access to virtualized compute resources, i.e., 
virtual machines (VMs). The cloud service provides all necessary functions
to manage VMs (create, suspend/resume, shutdown). 
**Jobs are never submitted directly to a cloud service**. Instead, 
a VM instance behaves as a [bare-metal](@ref guide-101-baremetal) service, to
which jobs can be submitted.

The main difference between a cloud service and a [virtualized cluster service](@ref guide-101-virtualizedcluster) is that the latter does expose the
underlying physical infrastructure (e.g., it is possible to instantiate a
VM on a particular physical host, or to migrate a VM between two particular
physical hosts).


# Creating a cloud compute service #        {#guide-cloud-creating}

In WRENCH, a cloud service is 
defined by the `wrench::CloudComputeService` class. An instantiation of a cloud
service requires the following parameters:

- The name of a host on which to start the service;
- A list (`std::vector`) of hostnames (all cores and all RAM of each host are available to the cloud service);
- A mount point (corresponding to a disk attached to the host) for the scratch space, i.e., storage local to the cloud service (used to store workflow files, as needed, during job executions); and
- Maps (`std::map`) of configurable properties (`wrench::CloudComputeServiceProperty`) and configurable message 
  payloads (`wrench::CloudComputeServiceMessagePayload`).

The example below creates an instance of a cloud service that runs 
on host `cloud_gateway`, provides access to 4 execution hosts, and has a scratch 
space on the disk mounted  at path `/scratch` at host `cloud_gateway`. 
Furthermore, the VM boot time  is
configured to be 10 second, and the message with which the service answers resource request description requests is configured to be 1KiB:


~~~~~~~~~~~~~{.cpp}
auto cloud_cs = simulation.add(
          new wrench::CloudComputeService("cloud_gateway", {"host1", "host2", "host3", "host4"}, "/scratch/",
                                   {{wrench::CloudServiceProperty::VM_BOOT_OVERHEAD_IN_SECONDS, "10"}},
                                   {{wrench::BareMetalComputeServiceMessagePayload::RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD, 1024}}));
~~~~~~~~~~~~~

See the documentation of `wrench::CloudComputeServiceProperty` and
`wrench::CloudComputeServiceMessagePayload` for all possible configuration
options.
