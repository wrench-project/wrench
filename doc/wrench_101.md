WRENCH 101                        {#wrench-101}
============

@WRENCHUserDoc <div class="doc-type">User Documentation</div><div class="doc-link">Other: <a href="../developer/getting-started.html">Developer</a> - <a href="../internal/getting-started.html">Internal</a></div> @endWRENCHDoc
@WRENCHDeveloperDoc  <div class="doc-type">Developer Documentation</div><div class="doc-link">Other: <a href="../user/getting-started.html">User</a> - <a href="../internal/getting-started.html">Internal</a></div> @endWRENCHDoc
@WRENCHInternalDoc  <div class="doc-type">Internal Documentation</div><div class="doc-link">Other: <a href="../user/getting-started.html">User</a> -  <a href="../developer/getting-started.html">Developer</a></div> @endWRENCHDoc

[TOC]

WRENCH 101 provides detailed information for WRENCH's [classes of users](@ref overview-users) 
seeking to obtain the best experience from WRENCH's capabilities. For instructions on how to
[install](@ref install), run a [first example](@ref getting-started), or 
[prepare the environment](@ref getting-started-prep), please refer to their respective sections 
in the documentation.


<!--################ 101 Guide Description ################ -->

@WRENCHUserDoc
This **User 101** guide describes WRENCH's simulation components (building blocks) 
necessary to build a simulator and run simulation scenarios. 
@endWRENCHDoc

@WRENCHDeveloperDoc 
This **Developer 101** guide describes WRENCH's architectural components necessary
to build your own workflow management systems (including schedulers, optimizations,
etc.), and services (e.g., compute, storage, network, etc.).  
@endWRENCHDoc

@WRENCHInternalDoc
This **Internal 101** guide provides detailed description of WRENCH's architecture
and modules for users who wish to contribute to WRENCH's core code. 
@endWRENCHDoc



# Building a WRENCH's Simulation #         {#wrench-101-simulation}

In WRENCH, a user simulation is defined via the wrench::Simulation class. Briefly,
a simulation is described by the following actions: 

-# `init(int *, char **)`: Initialize the simulation, which parses out WRENCH-specific 
and [SimGrid-specific](http://simgrid.gforge.inria.fr/simgrid/3.18/doc/options.html) 
command-line arguments.

-# `instantiatePlatform(std::string &)`: Instantiate a simulated platform. It requires a 
[SimGrid virtual platform description file](http://simgrid.gforge.inria.fr/simgrid/3.17/doc/platform.html).
Any [SimGrid](http://simgrid.gforge.inria.fr) simulation must be provided with the description 
of the platform on which an application execution is to be simulates. This is done via
a platform description file that includes definitions of compute hosts, clusters of hosts, 
storage resources, network links, routes between hosts, etc.

-# `add(std::unique_ptr<wrench::ComputeService>)`, `add(std::unique_ptr<wrench::StorageService>)`, 
and `add(std::unique_ptr<wrench::WMS>)`: Add a compute service (jobs can be scheduled to these
resources), a storage service (may represent a disk in a compute service or an external storage
system), or a workflow management system.

-# `setFileRegistryService(std::unique_ptr<wrench::FileRegistryService>` or 
`setNetworkProximityService(std::unique_ptr<wrench::NetworkProximityService>`: Set the
simulation's file registry service (required) and network proximity service (optional). 

-# `launch()`: Sanity check of simulation elements and simulation execution.

![Overview of the WRENCH simulation components.](images/wrench-simulation.png)

A simulation is composed by the following elements:

- wrench::ComputeService: A simulation is composed by a set of compute services, which
may represent bare-metal servers (wrench::MultihostMulticoreComputeService), cloud
platforms (wrench::CloudService), or batch-scheduled clusters (wrench::BatchService).
At least **one** compute service should be provided for running a simulation.

- wrench::StorageService: In a simulation, data is stored in a set of storage services, 
which may represent a hard disk (wrench::SimpleStorageService).
At least **one** storage service should be provided for running a simulation.

- wrench::WMS: A workflow management system provides the mechanisms for running and
monitoring a defined sequence of tasks, described as workflow applications. By default,
WRENCH does not provide a WMS implementation as part of its core components, however a
simple implementation (`wrench::SimpleWMS`) is available in the examples folder. Please,
refer to the [Creating your own WMS](@ref wrench-101-wms) section for further information
on how to develop a WMS. 
At least **one** WMS should be provided for running a simulation.

- wrench::FileRegistryService: The file registry keeps track of files stored in
different storage services. It is used during simulation to unveil the route
between data transfers.
Only **one** instance of a file registry service is allowed per simulation.

- wrench::NetworkProximityService: Proximity represents a topological relationship 
between services that significantly improves network performance. In WRENCH, 
proximity refers to connecting a client to the most proximate service based 
on a measurement of the round-trip time (RTT).
Only **one** instance of a network proximity service is allowed per simulation. 

WRENCH's wrench::ComputeService, wrench::StorageService, and wrench::WMS are defined
in a simulation via the `add()` function (recall that a simulation can be composed 
by several of these components), while wrench::FileRegistryService and 
wrench::NetworkProximityService are defined by their own `set()` functions. 



# Creating your own WMS #         {#wrench-101-wms}



@WRENCHNotUserDoc
# Creating a Compute Service #         {#wrench-101-cs}

# Creating a Storage Service #         {#wrench-101-ss}
@endWRENCHDoc



