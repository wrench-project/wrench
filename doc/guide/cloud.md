Cloud                        {#guide-cloud}
============

@WRENCHUserDoc <div class="doc-type">User Documentation</div><div class="doc-link">Other: <a href="../developer/guide-cloud.html">Developer</a> - <a href="../internal/guide-cloud.html">Internal</a></div> @endWRENCHDoc
@WRENCHDeveloperDoc  <div class="doc-type">Developer Documentation</div><div class="doc-link">Other: <a href="../user/guide-cloud.html">User</a> - <a href="../internal/guide-cloud.html">Internal</a></div> @endWRENCHDoc
@WRENCHInternalDoc  <div class="doc-type">Internal Documentation</div><div class="doc-link">Other: <a href="../user/guide-cloud.html">User</a> -  <a href="../developer/guide-cloud.html">Developer</a></div> @endWRENCHDoc

[TOC]

# Overview #            {#guide-cloud-overview}

A Cloud service is an abstraction of a compute service that corresponds to a
Cloud platform that provides access to virtualized compute resources, i.e., 
virtual machines (VMs). The Cloud service provides all necessary abstractions 
to manage VMs, including creation, suspension, resume, etc. Compute jobs 
submitted to the Cloud service run on VMs previously instantiated during the
simulation. If a VM that meets a job's requirements cannot be found, the 
service will throw an exception. In the Cloud service, a VM denotes a
[Bare-metal](@ref guide-baremetal) service instantiated at an execution host. 


# Creating a Cloud compute service #        {#guide-cloud-creating}

In WRENCH, a Cloud service represents a compute service (wrench::ComputeService), 
which is defined by the wrench::CloudService class. An instantiation of a Cloud 
service requires the following parameters:

- A hostname on which to start the service (this is the entry point to the service)
- A list (`std::vector`) of hostnames (all cores and all RAM of each host is available to the Cloud service) 
- A scratch space size, i.e., the size in bytes of storage local to the Cloud service (used to store
  workflow files, as needed, during job executions) 
- Maps (`std::map`) of configurable properties (`wrench::CloudServiceProperty`) and configurable message 
  payloads (`wrench::CloudServiceMessagePayload`).

~~~~~~~~~~~~~{.cpp}
auto cloud_service = simulation.add(
          new wrench::CloudService("cloud_gateway", {"host1", "host2", "host3", "host4"}, pow(2,42),
                                   {{wrench::CloudServiceProperty::SUPPORTS_PILOT_JOBS, "false"}}));
~~~~~~~~~~~~~


# Managing a Cloud compute service #        {#guide-cloud-managing}

