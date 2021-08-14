Creating a HTCondor compute service                        {#guide-101-htcondor}
============

[TOC]

# Overview #            {#guide-htcondor-overview}

[HTCondor](http://htcondor.org) is a workload management framework that supervises 
task executions on various sets of resources.
HTCondor is composed of six main service daemons (`startd`, `starter`, 
`schedd`, `shadow`, `negotiator`, and `collector`). In addition, 
each host on which one or more of these daemons is spawned must also 
run a `master` daemon, which controls the execution of all other 
daemons (including initialization and completion).


# Creating an HTCondor Service #        {#guide-htcondor-creating}

HTCondor is composed of a pool of resources in which jobs are submitted to
perform their computation. In WRENCH, an HTCondor service represents a 
compute service (`wrench::ComputeService`), which is defined by the 
`wrench::HTCondorComputeService` class. An instantiation of an HTCondor 
service requires the following parameters:

- The name of a host on which to start the service;
- A `std::set` of 'child' `wrench::ComputeService` instances available to the HTCondor pool; and
- A `std::map` of properties (`wrench::HTCondorComputeServiceProperty`) and message 
  payloads (`wrench::HTCondorComputeServiceMessagePayload`).
  
The set of compute services may include compute service instances that are either
`wrench::BareMetalComputeService` or `wrench::BatchComputeService` instances.
The example below creates an instance of an HTCondor service
with a pool of resources containing a [Bare-metal](@ref guide-baremetal) server:

~~~~~~~~~~~~~{.cpp}
// Simulation 
wrench::Simulation simulation;
simulation.init(&argc, argv);

// Create a bare-metal service

auto baremetal_service = simulation.add(
    new wrench::BareMetalComputeService(
          "execution_hostname",
          {std::make_pair(
                  "execution_hostname",
                  std::make_tuple(wrench::Simulation::getHostNumCores("execution_hostname"),
                                  wrench::Simulation::getHostMemoryCapacity("execution_hostname")))},
          "/scratch/"));

std::set<std::shared_ptr<wrench::ComputeService>> compute_services;
compute_services.insert(baremetal_service);

auto htcondor_compute_service = simulation->add(
          new wrench::HTCondorComputeService(hostname, 
                                      std::move(compute_services),
                                      {{wrench::HTCondorComputeServiceProperty::SUPPORTS_PILOT_JOBS, "false"}}
                                      ));
~~~~~~~~~~~~~

Jobs submitted to the `wrench::HTCondorComputeService` instance will be dispatched to one
of the 'child' compute services available to that instance (only one in the above example). 


# Anatomy of the HTCondor Service #        {#guide-htcondor-anatomy}

In WRENCH, we implement the 3 fundamental HTCondor services, implemented 
as particular sets of daemons. The _Job Execution Service_ consists of a 
`startd` daemon, which adds the host on which it is running to the HTCondor 
pool, and of a `starter` daemon, which manages task executions on this host.
The _Central Manager Service_ consists of a `collector` daemon, which collects 
information about all other daemons, and of a `negotiator` daemon, which 
performs task/resource matchmaking. The _Job Submission Service_ consists 
of a `schedd` daemon, which maintains a queue of tasks, and of several 
instances of a `shadow` daemon, each of which corresponds to a task submitted 
to the Condor pool for execution.

![](images/htcondor-architecture.png)


