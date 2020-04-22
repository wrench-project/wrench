Bare-Metal                        {#guide-baremetal}
============

@WRENCHUserDoc <div class="doc-type">User Documentation</div><div class="doc-link">Other: <a href="../developer/guide-baremetal.html">Developer</a> - <a href="../internal/guide-baremetal.html">Internal</a></div> @endWRENCHDoc
@WRENCHDeveloperDoc  <div class="doc-type">Developer Documentation</div><div class="doc-link">Other: <a href="../user/guide-baremetal.html">User</a> - <a href="../internal/guide-baremetal.html">Internal</a></div> @endWRENCHDoc
@WRENCHInternalDoc  <div class="doc-type">Internal Documentation</div><div class="doc-link">Other: <a href="../user/guide-baremetal.html">User</a> -  <a href="../developer/guide-baremetal.html">Developer</a></div> @endWRENCHDoc

[TOC]

# Overview #            {#guide-baremetal-overview}

A bare-metal is a service that makes it possible to run directly jobs on
hardware resources. Think of it as a set of multi-core hosts on which
multi-threaded processes can be started using something like Ssh. The
service does not perform any space-sharing among the jobs. In other words,
jobs submitted to the service execute concurrently in a time-shared manner.
It is the responsibility of the job submitter to pick hosts and/or numbers
of cores for each task, e.g., to enforce space-sharing of cores.  The only
resource allocation performed by the service is that it ensures that the
RAM capacity of a host is not exceeded. Tasks that have non-zero
RAM requirements are queued in FCFS fashion at each host until there is
enough RAM to execute them (think of this as each host running an OS that
disallows swapping and implements a FCFS access policy for RAM allocation).

# Creating a bare-metal compute service #        {#guide-baremetal-creating}

In WRENCH, a bare-metal service represents a compute service
(`wrench::ComputeService`), which is defined by the `wrench::BareMetalComputeService`
class. An instantiation of a bare-metal service requires the following
parameters:

- The name of a host on which to start the service (this is the entry point to the service);
- A set of compute hosts in a map (`std::map`), where each key is a hostname
  and each value is a tuple (`std::tuple`) with a number of cores and a RAM capacity. 
- A mount point (corresponding to a disk attached to the host) for the scratch space, i.e., storage local to the bare-metal service (used to store
  workflow files, as needed, during job executions) 
- Maps (`std::map`) of configurable properties (`wrench::BareMetalComputeServiceProperty`) and configurable message 
  payloads (`wrench::BareMetalComputeServiceMessagePayload`).
  
The example below shows how to create an instance of a bare-metal service
that runs on host "Gateway", provides access to 4 cores and 1GiB of RAM on host "Node1"
and to 8 cores and 4GiB of RAM on host "Node2", and has a scratch space of 1TiB. Furthermore, the thread startup overhead is
configured to be one hundredth of a second:

~~~~~~~~~~~~~{.cpp}
auto baremetal_cs = simulation->add(
          new wrench::BareMetalComputeService("Gateway", 
                                       {{"Node1", std::make_tuple(4, pow(2,30))}, {"Node2", std::make_tuple(8, pow(2,32)}},
                                       "/scratch/",
                                       {{wrench::BareMetalComputeServiceProperty::THREAD_STARTUP_OVERHEAD, "0.01"}}, 
                                       {});
~~~~~~~~~~~~~

## Bare-metal service properties             {#guide-baremetal-creating-properties}

In addition to properties inherited from `wrench::ComputeServiceProperty`, a bare-metal
service supports the following properties:

- `wrench::BareMetalComputeServiceProperty::THREAD_STARTUP_OVERHEAD`


@WRENCHNotUserDoc

# Submitting jobs to a bare-metal compute service #        {#guide-baremetal-using}

As expected, a bare-metal service provides implementations of the methods  in the
`wrench::ComputeService` base class. The `wrench::ComputeService::submitJob()` method
takes as argument service-specific arguments as a `std::map<std::string,
std::string>` of key-value pairs. The key is a task ID, and the value is
the service-specific argument for that task.  When submitting a job to a
bare-metal service, arguments can be specified a follows.

For each task, an optional argument can be provided as a string formatted
as "hostname:num_cores", "hostname", or "num_cores", where "hostname" is the name
of one of the service's compute hosts and "num_cores" is an integer (e.g., "host1:10",
"host1", "10"):

  - If no value is provided for a task, or if the value is the empty string, then the bare-metal
    service will choose the host on which the task should be executed (typically the host with
    the lowest current load), and will execute the task with as many cores as possible on that host. 
  
  - If a "hostname" value is provided for a task, then the bare-metal service will execute the
    task on that host, and will execute the task with as many cores as possible on that host.

  - If a "num_cores" value is provided for a task, then the bare-metal
    service will choose the host on which the task should be executed (typically the host with
    the lowest current load), and will execute the task with the specified number of cores. 

  - If a "hostname:num_cores" value is provided for a task, then the bare-metal service
   will execute the task on that host with the specified number of cores.

Here is an example submission to the bare-metal service created in the above example, for a 4-task job for tasks with IDs "task1", "task2", "task3", and "task4":

~~~~~~~~~~~~~{.cpp}
// Create a job manager
auto job_manager = this->createJobManager();

// Create a job
auto job = job_manager->createStandardJob(
                 {this->getWorklow()->getTaskByID("task1"),
                  this->getWorklow()->getTaskByID("task2"),
                  this->getWorklow()->getTaskByID("task3"),
                  this->getWorklow()->getTaskByID("task4")}, {});

// Create service-specific arguments so that:
//   task1 will run on host Node1 with as many cores as possible
//   task2 will run on host Node2 with 16 cores
//   task3 will run on some host with as many cores as possible
//   task4 will run on some host with 4 cores
std::map<std::string, std::string> service_specific_args;
service_specific_args["task1"] = "Node1";
service_specific_args["task2"] = "Node2:16";
service_specific_args["task4"] = "4";

// Submit the job
job_manager->submitJob(job, baremetal_cs, service_specific_args);
~~~~~~~~~~~~~

@endWRENCHDoc
