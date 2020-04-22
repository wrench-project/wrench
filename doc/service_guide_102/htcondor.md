HTCondor                        {#guide-htcondor}
============

@WRENCHUserDoc <div class="doc-type">User Documentation</div><div class="doc-link">Other: <a href="../developer/guide-htcondor.html">Developer</a> - <a href="../internal/guide-htcondor.html">Internal</a></div> @endWRENCHDoc
@WRENCHDeveloperDoc  <div class="doc-type">Developer Documentation</div><div class="doc-link">Other: <a href="../user/guide-htcondor.html">User</a> - <a href="../internal/guide-htcondor.html">Internal</a></div> @endWRENCHDoc
@WRENCHInternalDoc  <div class="doc-type">Internal Documentation</div><div class="doc-link">Other: <a href="../user/guide-htcondor.html">User</a> -  <a href="../developer/guide-htcondor.html">Developer</a></div> @endWRENCHDoc

[TOC]

# Overview #            {#guide-htcondor-overview}

[HTCondor](http://htcondor.org) is a workload management framework that supervises 
task executions on local and remote resources.
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

- A hostname on which to start the service;
- The HTCondor pool name;
- A `std::set` of `wrench::ComputeService` available to the HTCondor pool; and
- A `std::map` of properties (`wrench::HTCondorComputeServiceProperty`) and message 
  payloads (`wrench::HTCondorComputeServiceMessagePayload`).
  
The set of compute services may represent any computing instance natively 
provided by WRENCH (e.g., bare-metal servers, cloud platforms, batch-scheduled
clusters, etc.) or additional services derived from the `wrench::ComputeService`
base class. The example below shows how to create an instance of an HTCondor service
with a pool of resources containing a [Bare-metal](@ref guide-baremetal) server:

~~~~~~~~~~~~~{.cpp}
// Simulation 
wrench::Simulation simulation;
simulation.init(&argc, argv);

// Create bare-metal service
std::set<wrench::ComputeService *> compute_services;
compute_services.insert(new wrench::BareMetalComputeService(
          "execution_hostname",
          {std::make_pair(
                  "execution_hostname",
                  std::make_tuple(wrench::Simulation::getHostNumCores("execution_hostname"),
                                  wrench::Simulation::getHostMemoryCapacity("execution_hostname")))},
          "/scratch/"));

auto compute_service = simulation->add(
          new wrench::HTCondorComputeService(hostname, 
                                      "local", 
                                      std::move(compute_services),
                                      {{wrench::HTCondorComputeServiceProperty::SUPPORTS_PILOT_JOBS, "false"}}
                                      ));
~~~~~~~~~~~~~


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


@WRENCHNotUserDoc  

<p>&nbsp;</p>
WRENCH HTCondor service implementation spawns two additional services during 
execution: wrench::HTCondorCentralManagerService and wrench::HTCondorNegotiatorService.

The wrench::HTCondorCentralManagerService coordinates the execution of jobs
submitted to the HTCondor pool. Jobs submitted to the wrench::HTCondorComputeService
are then queued in a `std::vector<wrench::StandardJob *>`, which are then 
consumed as resources become available. The Central Manager also spawns the
execution of the wrench::HTCondorNegotiatorService, which performs matchmaking
between jobs and compute resources available in the pool. Note that job submission
in HTCondor is asynchronous, thus our simulated services operates independent 
from each other (see the 
@WRENCHDeveloperDoc [Interacting with services](#wrench-101-WMS-services) section@endWRENCHDoc 
@WRENCHInternalDoc Interacting with services section from the 
Developer</a> Documentation @endWRENCHDoc
).


@endWRENCHDoc