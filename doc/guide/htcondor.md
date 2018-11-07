HTCondor                        {#guide-htcondor}
============

@WRENCHUserDoc <div class="doc-type">User Documentation</div><div class="doc-link">Other: <a href="../developer/wrench-101.html">Developer</a> - <a href="../internal/wrench-101.html">Internal</a></div> @endWRENCHDoc
@WRENCHDeveloperDoc  <div class="doc-type">Developer Documentation</div><div class="doc-link">Other: <a href="../user/wrench-101.html">User</a> - <a href="../internal/wrench-101.html">Internal</a></div> @endWRENCHDoc
@WRENCHInternalDoc  <div class="doc-type">Internal Documentation</div><div class="doc-link">Other: <a href="../user/wrench-101.html">User</a> -  <a href="../developer/wrench-101.html">Developer</a></div> @endWRENCHDoc

[TOC]

# Overview #            {#guide-htcondor-overview}

[HTCondor](http://htcondor.org) is a workload management framework that supervises 
task executions on local and remote resources.
HTCondor is composed of six main service daemons (`startd`, `starter`, 
`schedd`, `shadow`, `negotiator`, and `collector`). In addition, 
each host on which one or more of these daemons is spawned must also 
run a `master` daemon, which controls the execution of all other 
daemons (including initialization and completion).

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


# Creating an HTCondor Service #        {#guide-htcondor-creating}

HTCondor is composed of a pool of resources in which jobs are submitted to
perform their computation. In WRENCH, an HTCondor service represents a 
compute service (`wrench::ComputeService`), which is defined by the 
`wrench::HTCondorService` class. An instantiation of an HTCondor 
service requires the following parameters:

- A hostname on which to start the service;
- The HTCondor pool name;
- A `std::set` of `wrench::ComputeService` available to the HTCondor pool; and
- Maps (`std::map`) of properties (`wrench::HTCondorServiceProperty`) and message 
  payloads (`wrench::HTCondorServiceMessagePayload`).
  
The set of compute services may represent any of computing instance natively 
provided by WRENCH (e.g., bare-metal servers, cloud platforms, batch-scheduled
clusters, etc.) or additional services derived from the `wrench::ComputeService`
base class. The example below shows how to create an instance of an HTCondor service
with a pool of resources containing a bare-metal server and a cloud platform:

~~~~~~~~~~~~~{.cpp}
auto compute_service = simulation->add(
          new wrench::HTCondorService(hostname, 
                                      "local", 
                                      std::move(compute_services),
                                      {{wrench::HTCondorServiceProperty::SUPPORTS_PILOT_JOBS, "false"}}
                                      ));
~~~~~~~~~~~~~
