BareMetal                        {#guide-baremetal}
============

@WRENCHUserDoc <div class="doc-type">User Documentation</div><div class="doc-link">Other: <a href="../developer/wrench-101.html">Developer</a> - <a href="../internal/wrench-101.html">Internal</a></div> @endWRENCHDoc
@WRENCHDeveloperDoc  <div class="doc-type">Developer Documentation</div><div class="doc-link">Other: <a href="../user/wrench-101.html">User</a> - <a href="../internal/wrench-101.html">Internal</a></div> @endWRENCHDoc
@WRENCHInternalDoc  <div class="doc-type">Internal Documentation</div><div class="doc-link">Other: <a href="../user/wrench-101.html">User</a> -  <a href="../developer/wrench-101.html">Developer</a></div> @endWRENCHDoc

[TOC]

# Overview #            {#guide-baremetal-overview}

A BareMetal service is a service that makes it possible to run workflow
jobs on hardware resources. Think of it as a set of multi-core hosts on
which multi-threaded processes can be started using something like Ssh. The
service does not perform any space-sharing among the jobs. In other words,
jobs submitted to the service execute concurrently in a time-shared manner.
It is the responsibility of the job submitter to pick hosts and/or numbers
of cores for each task, e.g., to enforce space-sharing of cores.  The only
resource allocation performed by the service is that it ensures that the
RAM capacity of a host is not exceeded. Workflow tasks that have non-zero
RAM requirements are queued in FCFS fashion at each host until there is
enough RAM to execute them (think of this as each host running an OS that
disallows swapping).

# Creating a BareMetal compute service #        {#guide-baremetal-creating}

In WRENCH, a BareMetal service represents a compute service
(`wrench::ComputeService`), which is defined by the `wrench::BareMetalService`
class. An instantiation of a BareMetal service requires the following
parameters:

- A hostname on which to start the service (this is the entry point to the service);
- A set of compute hosts in a map (`std::map`), where each key is a hostname
  and each value is a tuple (`std::tuple`) with a number of cores and a RAM capacity. 
- A scrach space size, i.e., the size in bytes of storage local to the Batch service (used to store
  worfklow files, as needed, during job executions) 
- Maps (`std::map`) of configurable properties (`wrench::BareMetalServiceProperty`) and configurable message 
  payloads (`wrench::BareMetalServiceMessagePayload`).
  
The example below shows how to create an instance of a BareMetal service
that runs on host "Gateway", provides access to 4 cores and 1GiB of RAM on host "Node1"
and to 8 cores and 4GiB of RAM on host "Node2", and has a scratch space of 1TiB. Furthermore, the thread startup overhead is
configured to be one hundredth of a second:

~~~~~~~~~~~~~{.cpp}
auto compute_service = simulation->add(
          new wrench::BareMetalService("Gateway", 
                                       {{"Node1", std::make_tuple(4, pow(2,30))}, {"Node2", std::make_tuple(8, pow(2,32)}},
                                        pow(2,40),
                                       {{wrench::BareMetalServiceProperty::THREAD_STARTUP_OVERHEAD, "0.01"}}
                                      );
~~~~~~~~~~~~~

