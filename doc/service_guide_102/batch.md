Batch                        {#guide-batch}
============

@WRENCHUserDoc <div class="doc-type">User Documentation</div><div class="doc-link">Other: <a href="../developer/guide-batch.html">Developer</a> - <a href="../internal/guide-batch.html">Internal</a></div> @endWRENCHDoc
@WRENCHDeveloperDoc  <div class="doc-type">Developer Documentation</div><div class="doc-link">Other: <a href="../user/guide-batch.html">User</a> - <a href="../internal/guide-batch.html">Internal</a></div> @endWRENCHDoc
@WRENCHInternalDoc  <div class="doc-type">Internal Documentation</div><div class="doc-link">Other: <a href="../user/guide-batch.html">User</a> -  <a href="../developer/guide-batch.html">Developer</a></div> @endWRENCHDoc

[TOC]

# Overview #            {#guide-batch-overview}

A batch service is a service that makes it possible to run workflow jobs on
a homogeneous cluster managed by a batch scheduler. The batch scheduler
receives requests that ask for a number of compute nodes, with a number of
cores per compute node, and a duration. Requests wait in a queue and, using
a range of possible batch scheduling algorithms, are dispatched to the
requested compute resources in a space-sharing manner. Therefore, a job
submitted to the service experiences a "queue waiting time" period (the
length of which depends on the load on the service) followed by an
"execution time" period.  In typical batch-scheduler fashion, a running job
is cleanly_terminated when it reaches its requested duration, which may cause the
job to fail. If, instead, the job completes before the requested duration
the job succeeds. In both cases, the job's allocated compute resources are
reclaimed by the batch scheduler.

A batch service also supports so-called "pilot jobs", i.e., jobs that are 
submitted to the service, with requested resources and duration, but without
specifying at submission time which workflow tasks/operations should be performed
by the job. Instead, once the job starts it exposes to its submitter a 
[Bare-metal](@ref guide-baremetal) service. This service is available only
for the requested duration, and can be used in any manner by the submitter. 
This allows late binding of workflow tasks to compute resources. 


# Creating a batch compute service #        {#guide-batch-creating}

In WRENCH, a batch service represents a compute service
(`wrench::ComputeService`), which is defined by the `wrench::BatchComputeService`
class. An instantiation of a batch service requires the following
parameters:

- A hostname on which to start the service (this is the entry point to the service)
- A list (`std::vector`) of hostnames (all cores and all RAM of each host is available to
  the batch service)
- A mount point (corresponding to a disk attached to the host) for the scratch space, i.e., storage local to the batch service (used to store
  workflow files, as needed, during job executions) 
- Maps (`std::map`) of configurable properties (`wrench::BatchComputeServiceProperty`) and configurable message
  payloads (`wrench::BatchComputeServiceMessagePayload`)
  
The example below shows how to create an instance of a batch service
that runs on host "Gateway" and provides access to 4 hosts (using all their
cores and RAM), with 1 TiB scratch space.  Furthermore, the batch scheduling algorithm is configured to
the FCFS (First-Come-First-Serve):

~~~~~~~~~~~~~{.cpp}
auto batch_cs = simulation->add(
          new wrench::BatchComputeService("Gateway",
                                   {"Node1", "Node2", "Node3", "Node4"},
                                   "/scratch/",
                                   {{wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, "FCFS"}},
                                   {});
~~~~~~~~~~~~~


## Batch compute service properties             {#guide-batch-creating-properties}

In addition to properties inherited from `wrench::ComputeServiceProperty`, a batch compute
service supports the following properties:

- `wrench::BatchComputeServiceProperty::THREAD_STARTUP_OVERHEAD`
- `wrench::BatchComputeServiceProperty::HOST_SELECTION_ALGORITHM`
- `wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM`
- `wrench::BatchComputeServiceProperty::BATCH_QUEUE_ORDERING_ALGORITHM`
- `wrench::BatchComputeServiceProperty::BATCH_RJMS_DELAY`
- `wrench::BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE`
- `wrench::BatchComputeServiceProperty::OUTPUT_CSV_JOB_LOG`
- `wrench::BatchComputeServiceProperty::SIMULATE_COMPUTATION_AS_SLEEP`
- `wrench::BatchComputeServiceProperty::BATSCHED_LOGGING_MUTED`
- `wrench::BatchComputeServiceProperty::BATSCHED_CONTIGUOUS_ALLOCATION`


@WRENCHNotUserDoc

# Submitting jobs to a batch compute service #        {#guide-batch-using}

As expected, a batch service provides implementations of the methods
in the `wrench::ComputeService` base class. The
`wrench::ComputeService::submitJob()` method takes as argument
service-specific arguments as a `std::map<std::string, std::string>` of
key-value pairs.  These **required** arguments are to be specified as follows:

  - key: `-t`; value: a requested runtime in minutes (if the job execution exceeds this limit, then the job is cleanly_terminated)
  - key: `-N`; value: a requested number of compute nodes
  - key: `-c`; value: a requested number of cores per compute nodes

(You may note the the above corresponds to the arguments that must be
provided to [SLURM](https://slurm.schedmd.com/).)

Here is an example job submission to the batch service:

~~~~~~~~~~~~~{.cpp}
// Create a job manager
auto job_manager = this->createJobManager();

// Create a job
auto job = job_manager->createStandardJob(tasks, {});

// Create service-specific arguments so that:
//   The job will run no longer than 1 hour
//   The job will run on 2 compute nodes
//   The job will use 4 cores on each compute nodes
std::map<std::string, std::string> service_specific_args;
service_specific_args["-t"] = "60";
service_specific_args["-N"] = "2";
service_specific_args["-c"] = "4";

// Submit the job
job_manager->submitJob(job, batch_cs, service_specific_args);
~~~~~~~~~~~~~

@endWRENCHDoc

