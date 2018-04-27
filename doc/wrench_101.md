WRENCH 101                        {#wrench-101}
============


@WRENCHUserDoc <div class="doc-type">User Documentation</div><div class="doc-link">Other: <a href="../developer/getting-started.html">Developer</a> - <a href="../internal/getting-started.html">Internal</a></div> @endWRENCHDoc
@WRENCHDeveloperDoc  <div class="doc-type">Developer Documentation</div><div class="doc-link">Other: <a href="../user/getting-started.html">User</a> - <a href="../internal/getting-started.html">Internal</a></div> @endWRENCHDoc
@WRENCHInternalDoc  <div class="doc-type">Internal Documentation</div><div class="doc-link">Other: <a href="../user/getting-started.html">User</a> -  <a href="../developer/getting-started.html">Developer</a></div> @endWRENCHDoc


WRENCH 101 is a page and a set of documents that provide detailed information for each WRENCH's [classes of users](@ref overview-users),
which serves as higher-level documentation than the [API Reference](./annotated.html). For instructions on how to
[install](@ref install), run a [first example](@ref getting-started), or 
[create a basic WRENCH-based simulator](@ref getting-started-prep), please refer to their respective sections 
in the documentation.


<!--################################################ -->
<!--################ 101 FOR USERS  ################ -->
<!--################################################ -->


@WRENCHUserDoc


This **User 101** guide describes all the WRENCH's simulation components (building blocks) 
necessary to build a custom simulator and run simulation scenarios. 

---

[TOC]

---

# 10,000-ft view of a WRENCH-based simulator #      {#wrench-101-simulator-10000ft}


# Blueprint for a WRENCH-based simulator #         {#wrench-101-simulator-blueprint}

In WRENCH, a user simulation is defined via the wrench::Simulation class. Briefly,
a simulation is described by the following actions: 

-# `init(int *, char **)`: Initialize the simulation, which parses out WRENCH-specific 
and [SimGrid-specific](http://simgrid.gforge.inria.fr/simgrid/3.19/doc/options.html) 
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


# Customizing Services #         {#wrench-101-customizing-services}


# Turning logging on/off #        {#wrench-101-logging-onoff}


# Analyzing Simulation Output #   {#wrench-101-simulation-output}


@endWRENCHDoc






<!--################################################ -->
<!--############# 101 FOR DEVELOPERS  ############## -->
<!--################################################ -->


@WRENCHDeveloperDoc 


This **Developer 101** guide describes WRENCH's architectural components necessary
to build your own WMS (Workflow Management Systems).

---

[TOC]

---


# 10,000-ft view of a simulated WMS #           {#wrench-101-WMS-10000ft}


# Blueprint for a WMS in WRENCH #               {#wrench-101-WMS-blueprint}


# Interacting with services  #                  {#wrench-101-WMS-services}

# Running workflow tasks #                      {#wrench-101-WMS-tasks}

# Moving workflow data #                        {#wrench-101-WMS-data}

# Schedulers for decision-making #              {#wrench-101-WMS-schedulers}

# Workflow execution events #                   {#wrench-101-WMS-events}

# Logging #                                     {#wrench-101-WMS-logging}


  
@endWRENCHDoc






<!--################################################ -->
<!--############### 101 FOR INTERNAL  ############## -->
<!--################################################ -->


@WRENCHInternalDoc


This **Internal 101** guide is intend to users who want to contribute code to WRENCH to
extend its capabilities. 

---

Make sure to read the [User 101 Guide](../user/wrench-101.html)
and the   [Developer 101 Guide](../developer/wrench-101.html) to understand
how WRENCH works from  those perspectives.   The largest portion of the
WRENCH code base is the "Internal" code base, and for now, the way to go
is to look at the [API Reference](./annotated.html). Do not hesitate to contact
the WRENCH team with questions about the internals of WRENCH if you want to contribute.  
Of course, forking [The WRENCH repository](http://github.com/wrench-project/wrench) and
creating pull requests is the preferred way to contribute to WRENCH as an Internal developer. 
  
  
Here is a in-progress misc item list of interest:
  
  - Most (soon to be all) interaction with SimGrid is done by the classes in the
      `src/simgrid_S4U_util` directory
  - A WRENCH simulation, list any SimGrid simulation, is comprised of simulated processes
    that communicate via messages with put/get-like operations into mailboxes. These
    processes are essentially implemented as threads, but they are not scheduled by the OS. Instead,
    they are scheduled by SimGrid in round-robin fashion. Each thread runs without interruption in between
    two SimGrid API calls. Therefore, although in principle WRENCH is massively multi-threaded, there are
    _almost_ no needs to locks when sharing data among threads. 
  


@endWRENCHDoc





