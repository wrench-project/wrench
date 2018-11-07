Batch                        {#guide-batch}
============

@WRENCHUserDoc <div class="doc-type">User Documentation</div><div class="doc-link">Other: <a href="../developer/wrench-101.html">Developer</a> - <a href="../internal/wrench-101.html">Internal</a></div> @endWRENCHDoc
@WRENCHDeveloperDoc  <div class="doc-type">Developer Documentation</div><div class="doc-link">Other: <a href="../user/wrench-101.html">User</a> - <a href="../internal/wrench-101.html">Internal</a></div> @endWRENCHDoc
@WRENCHInternalDoc  <div class="doc-type">Internal Documentation</div><div class="doc-link">Other: <a href="../user/wrench-101.html">User</a> -  <a href="../developer/wrench-101.html">Developer</a></div> @endWRENCHDoc

[TOC]

# Overview #            {#guide-batch-overview}

A Batch service is a service that makes it possible to run workflow jobs on
a homogeneous cluster managed by a batch scheduler. The batch scheduler
receives requests that ask for a number of compute nodes, with a number of
cores per compute node, and a duration. Requests wait in a queue and, using
a range of possible batch scheduling algorithms, are dispatched to the
requested compute resources in a space-sharing manner. Therefore, a job
submitted to the service experiences a "queue waiting time" period (the
length of which depends on the load on the service) followed by an
"execution time" period.  In typical batch-scheduler fashion, a running job
is terminated when it reaches its requested duration, which may cause the
job to fail. If, instead, the job completes before the requested duration
the job succeeeds. In both cases, the job's allocated compute resources are
reclaimed by the batch scheduler.

A Batch service also supports so-called "pilot jobs", i.e., jobs that are 
submitted to the service, with requested resources and duration, but without
specifying at submission time which workflow tasks/operations should be performed
by the job. Instead, once the job starts it exposes to its submitter a 
[BareMetal](@ref guide-baremetal) service. This service is available only
for the requested duration, and can be used in any manner by the submitter. 
This allows late binding of workflow tasks to compute resources. 


# Creating a Batch compute service #        {#guide-batch-creating}

In WRENCH, a Batch service represents a compute service
(`wrench::ComputeService`), which is defined by the `wrench::BatchService`
class. An instantiation of a Batch service requires the following
parameters:

- A hostname on which to start the service (this is the entry point to the service);
- A list (`std::vector`) of hostnames (all cores and all RAM of each host is available to
  the Batch service). 
- A scrach space size, i.e., the size in bytes of storage local to the Batch service (used to store
  worfklow files, as needed, during job executions) 
- Maps (`std::map`) of configurable properties (`wrench::BatchServiceProperty`) and configurable message 
  payloads (`wrench::BatchServiceMessagePayload`).
  
The example below shows how to create an instance of a Batch service
that runs on host "Gateway" and provides access to 4 hosts (using all their
cores and RAM), with 1 TiB scratch space.  Furthermore, the batch scheduling algorithm is configured to
the FCFS (First-Come-First-Serve):

~~~~~~~~~~~~~{.cpp}
auto compute_service = simulation->add(
          new wrench::BatchService("Gateway", 
                                  {"Node1", "Node2", "Node3", "Node4"},
                                  pow(2,40),
                                       {{wrench::BatchServiceProperty::BATCH_SCHEDULING_ALGORITHM, "FCFS"}}
                                      );
~~~~~~~~~~~~~

