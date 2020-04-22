Bare-Metal                        {#guide-101-baremetal}
============


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


