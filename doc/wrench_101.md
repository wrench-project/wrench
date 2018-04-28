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

A WRENCH-based simulator can be as simple as a single `main()` function that first creates 
a platform to be simulated (the hardware) and a set of services that turns on the platform
(the software).  These services correspond to software that knows how to store data, 
perform computation, and many other useful things that real-world cyberinfrastructures
can do.  

The simulator then needs to create a workflow to be executed, which is a set
of compute tasks each with input and output files, and thus data-dependencies.  A special
service is then created, called a Workflow Management System (WMS),  that will be in charge
of executing the workflow on the platform. (This service must have been implemented by a WRENCH "developer", 
i.e., a user that has used the Developer API.)  The set of input files to the workflow, if any, are then
staged on the platform at some locations. 
 
The simulation is then simply launched via a single call. When this call returns, the WMS
has completed (typically after completing the execution of the workflow, or failing to executed it) 
and one can analyze simulation output. 


# Blueprint for a WRENCH-based simulator #         {#wrench-101-simulator-blueprint}

Let's dive down into more details.   Here are the steps that a WRENCH-based simulator typically follows:

-# **Create and initialize a simulation** -- In WRENCH, a user simulation is defined via the `wrench::Simulation` class. And instance of this class
must be created, and the `wrench::Simulation::init(int *, char **)` method is called to initialize the simulation (and parse WRENCH-specific 
and [SimGrid-specific](http://simgrid.gforge.inria.fr/simgrid/3.19/doc/options.html) 
command-line arguments and removed them from the command-line argument list).

-# **Instantiate a simulated platform** --  This is done with the `wrench::Simulation::instantiatePlatform(std::string &)`
method which takes as argument a 
[SimGrid virtual platform description file](http://simgrid.gforge.inria.fr/simgrid/3.17/doc/platform.html).
Any [SimGrid](http://simgrid.gforge.inria.fr) simulation must be provided with the description 
of the platform on which an application/system execution is to be simulated (compute hosts, clusters of hosts, 
storage resources, network links, routers, routes between hosts, etc.)

-# **Instantiate services on the platform** -- The `wrench::Simulation::add(wrench::Service *)` method is used
 to add services to the simulation. Each class of service is created with a particular 
 constructor, which also specifies host(s) on which the service is to be started. Typical kinds of services
 are compute services, storage services, network proximity services, file registry services.  The only service that's necessary
 is a WMS...
 
 -# **Create at least one workflow** --  This is done by creating an instance of the `wrench::Workflow` class, which has
 methods to manually add tasks and files to the workflow application, but also methods to import workflows
 from workflow description files ([DAX](http://workflowarchive.org) and [JSON](http://workflowhub.org/traces/). 
 If there are input files to the workflow's entry tasks, these must be staged on instantiated storage
 services. 
 
-# **Instantiate at least one WMS per workflow** -- One of the services instantiated must be a `wrench::WMS` instance, i.e., a service that is
 in charge of executing the workflow, as implemented by a WRENCH "developer" using the Developer API.  

-# **Launch the simulation** -- This is done via the `wrench::Simulation::launch()` call which first
      sanity checks the simulation setup and then launches and simulators all services, until all WMS services
      have exited (after they have completed or failed to complete workflows).
      
-# **Process simulation output** -- The `wrench::Simulation` class has an `output` field that is a collection of 
   time-stamped traces of simulation events. These traces can be processed/analyzed at will.  
      

The above steps are depicted in the figure below: 

![Overview of the WRENCH simulation setup.](images/wrench-simulation.png)

# Available services #      {#wrench-101-simulator-services}

To date, these are the (simulated) services that can be instantiated on the
simulated platform:


- **Compute Services** (classes that derive `wrench::ComputeService`): These are services
that know how to compute workflow tasks. These include bare-metal servers (`wrench::MultihostMulticoreComputeService`), cloud
platforms (`wrench::CloudService`), batch-scheduled clusters (`wrench::BatchService`), etc.
It is not technically required to instantiate a compute service, but then no workflow task
can be executed by the WMS. 

- **Storage Services** (classes that derive `wrench::StorageService`): 
These are services that know how to store workflow files, which can then be
accessed in reading/writing but compute services when the execute tasks that
read/write files. 
It is not technically required to instantiate a storage service, but then no workflow task
can have an input or an output file. 

- **File Registry Services** (the `wrench::FileRegistryService` class): 
This is a service that is often known as a _replica catalog_, and which is simply
a database of <filename, list of locations> key-value pairs of the storage services
on which a copy of file are available.  It is used during simulation to decide where
input files for tasks can be acquired. 
It is not required to instantiate a file registry service, unless the workflow's
entry tasks have input files (because in this case these files have to be somewhere
before execution can start). But some WMSs may complain if none is available.


- **Network Proximity Services ** (the class `wrench::NetworkProximityService`): 
These are services that monitor the network and provide a database of 
host-to-host network distances. This database can be queried by WMSs to make informed
decisions, e.g., to pick from which storage service a file should be retrieved
so as to reduce communication time.  Typically, network distances are estimated
as or based on roud-trip-times between hosts. 
It is not required to instantiate a network proximity service, but some WMSs may complain if none
is available.


- **Worflow Management Systems (WMSs) ** (classes that derive `wrench::WMS`): 
A workflow management system provides the mechanisms for executing a workflow
applications, include decision-making for optimizing various objectives (the most
common one between to minimize workflow execution time).  By default,
WRENCH does not provide a WMS implementation as part of its core components, however a
simple implementation (`wrench::SimpleWMS`) is available in the `examples/simple-example` folder. Please,
refer to the [Creating your own WMS](@ref wrench-101-wms) section for further information
on how to develop a WMS. 
At least **one** WMS should be provided for running a simulation.




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





