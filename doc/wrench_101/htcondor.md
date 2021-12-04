Creating a HTCondor compute service                        {#guide-101-htcondor}
============

[TOC]

# Overview #            {#guide-htcondor-overview}

[HTCondor](http://htcondor.org) is a workload management framework that supervises 
task1 executions on various sets of resources.
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

Jobs submitted to the `wrench::HTCondorComputeService` instance will be
dispatched automatically to one of the 'child' compute services available
to that instance (only one in the above example). 

