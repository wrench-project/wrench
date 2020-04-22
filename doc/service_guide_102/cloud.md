Cloud                        {#guide-cloud}
============

@WRENCHUserDoc <div class="doc-type">User Documentation</div><div class="doc-link">Other: <a href="../developer/guide-cloud.html">Developer</a> - <a href="../internal/guide-cloud.html">Internal</a></div> @endWRENCHDoc
@WRENCHDeveloperDoc  <div class="doc-type">Developer Documentation</div><div class="doc-link">Other: <a href="../user/guide-cloud.html">User</a> - <a href="../internal/guide-cloud.html">Internal</a></div> @endWRENCHDoc
@WRENCHInternalDoc  <div class="doc-type">Internal Documentation</div><div class="doc-link">Other: <a href="../user/guide-cloud.html">User</a> -  <a href="../developer/guide-cloud.html">Developer</a></div> @endWRENCHDoc

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

@WRENCHNotUserDoc  

# Managing a cloud compute service #        {#guide-cloud-managing}

The cloud service provides several mechanisms to manage the set of VMs instantiated
on the execution hosts. Currently, it is possible to create, shutdown, start,
suspend, and resume VMs (see a complete list of functions available in the 
wrench::CloudComputeService API documentation). The figure below shows the different states
a VM can be:

![](images/wrench-guide-cloud-state-diagram.png)
<br/>

# Submitting Jobs to a cloud compute service #        {#guide-cloud-using}

As expected, a cloud service provides implementations of the methods
in the `wrench::ComputeService` base class. The
`wrench::ComputeService::submitJob()` method takes as argument
service-specific arguments as a `std::map<std::string, std::string>` of
key-value pairs.  For a cloud service, these arguments are optional and to be specified as follows:

  - key: `-vm`; value: a VM name (as returned by wrench::CloudComputeService::createVM)  on which the job will be executed.

If no argument is specified, the cloud service will pick the VM on which to execute
the job. 

Here is an example job submission to the cloud service:

~~~~~~~~~~~~~{.cpp}
// Create a VM with 2 cores and 1 GiB of RAM
auto vm1 = cloud_cs->createVM(2, pow(2,30));
// Create a VM with 4 cores and 2 GiB of RAM
auto vm2 = cloud_cs->createVM(4, pow(2,31));

// Create a job manager
auto job_manager = this->createJobManager();

// Create a job
auto job = job_manager->createStandardJob(tasks, {});

// Create service-specific arguments so that the job will run on the second vm
std::map<std::string, std::string> service_specific_args;
service_specific_args["-vm"] = vm2;

// Submit the job
job_manager->submitJob(job, cloud_cs, service_specific_args);
~~~~~~~~~~~~~


@endWRENCHDoc
