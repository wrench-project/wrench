WRENCH 101                        {#wrench-101}
============


@WRENCHUserDoc <div class="doc-type">User Documentation</div><div class="doc-link">Other: <a href="../developer/wrench-101.html">Developer</a> - <a href="../internal/wrench-101.html">Internal</a></div> @endWRENCHDoc
@WRENCHDeveloperDoc  <div class="doc-type">Developer Documentation</div><div class="doc-link">Other: <a href="../user/wrench-101.html">User</a> - <a href="../internal/wrench-101.html">Internal</a></div> @endWRENCHDoc
@WRENCHInternalDoc  <div class="doc-type">Internal Documentation</div><div class="doc-link">Other: <a href="../user/wrench-101.html">User</a> -  <a href="../developer/wrench-101.html">Developer</a></div> @endWRENCHDoc


WRENCH 101 is a page and a set of documents that provide detailed information 
for each WRENCH's [classes of users](@ref overview-users),
and higher-level content than the [API Reference](./annotated.html). 
For instructions on how to
[install](@ref install), run a [first example](@ref getting-started),
 or 
[create a basic WRENCH-based simulator](@ref getting-started-prep), 
please refer to their respective sections 
in the documentation.


<!--################################################ -->
<!--################ 101 FOR USERS  ################ -->
<!--################################################ -->


@WRENCHUserDoc


This **User 101** guide describes all the WRENCH simulation components (building blocks) 
necessary to build a custom simulator and run simulation scenarios. 

[TOC]

---

# 10,000-ft view of a WRENCH-based simulator #      {#wrench-101-simulator-10000ft}

A WRENCH-based simulator can be as simple as a single `main()` function that first creates 
a platform to be simulated (the hardware) and a set of services that run on the platform
(the software).  These services correspond to software that knows how to store data, 
perform computation, and many other useful things that real-world cyberinfrastructure services
can do.  

The simulator then needs to create a workflow (or a set of workflows) to be executed, which consists of a set
of compute tasks each with input and output files, and thus data-dependencies.  A special
service is then created, called a Workflow Management System (WMS),  that will be in charge
of executing the workflow on the platform. (This service must have been implemented by a WRENCH "developer", 
i.e., a user that has used the Developer API).  The set of input files to the workflow, if any, are
staged on the platform at particular storage locations. 
 
The simulation is then launched via a single call. When this call returns, the WMS
has cleanly_terminated (typically after completing the execution of the workflow, or failing to executed it)
and the simulation output can be analyzed. 


# Blueprint for a WRENCH-based simulator #         {#wrench-101-simulator-blueprint}

Here are the steps that a WRENCH-based simulator typically follows:

-# **Create and initialize a simulation** -- In WRENCH, a user simulation is defined via the `wrench::Simulation` class. 
 An instance of this class must be created, and the `wrench::Simulation::init()` method is called to initialize the 
 simulation (and parse WRENCH-specific and [SimGrid-specific](http://simgrid.gforge.inria.fr/simgrid/3.19/doc/options.html) 
 command-line arguments).  Two useful such arguments are `--help-wrench`, which displays help messages about 
 optional WRENCH-specific command-line arguments, and `--help-simgrid`, which displays help messages about optional
 Simgrid-specific command-line arguments. 

-# **Instantiate a simulated platform** --  This is done with the `wrench::Simulation::instantiatePlatform()`
 method which takes as argument a 
 [SimGrid virtual platform description file](http://simgrid.gforge.inria.fr/simgrid/3.17/doc/platform.html).
 Any [SimGrid](http://simgrid.gforge.inria.fr) simulation must be provided with the description 
 of the platform on which an application/system execution is to be simulated (compute hosts, clusters of hosts, 
 storage resources, network links, routers, routes between hosts, etc.)

-# **Instantiate services on the platform** -- The `wrench::Simulation::add()` method is used
 to add services to the simulation. Each class of service is created with a particular 
 constructor, which also specifies host(s) on which the service is to be started. Typical kinds of services
 include compute services, storage services, network proximity services, and file registry services.  
 
-# **Create at least one workflow** --  This is done by creating an instance of the `wrench::Workflow` class, which has
 methods to manually add tasks and files to the workflow application, but also methods to import workflows
 from standard workflow description files ([DAX](http://workflowarchive.org) and 
 [JSON](https://github.com/wrench-project/wrench/tree/master/doc/schemas)). 
 If there are input files to the workflow's entry tasks, these must be staged on instantiated storage
 services. 
 
-# **Instantiate at least one WMS per workflow** -- At least one of the services instantiated must be a `wrench::WMS` 
 instance, i.e., a service that is in charge of executing the workflow, as implemented by a WRENCH "developer" using 
 the [Developer](../developer/wrench-101.html) API. Associating a workflow to a WMS is done via the 
 `wrench::WMS::addWorkflow()` method.

-# **Launch the simulation** -- This is done via the `wrench::Simulation::launch()` call which first
   sanity checks the simulation setup and then launches all simulated services, until all WMS services
   have exited (after they have completed or failed to complete workflows).
      
-# **Process simulation output** -- The `wrench::Simulation::getOutput()` method returns an object that is a 
  collection of time-stamped traces of simulation events. These traces can be processed/analyzed at will.  
      

<!-- The above steps are depicted in the figure below: 

![Overview of the WRENCH simulation setup.](images/wrench-simulation.png)
-->

# Available services #      {#wrench-101-simulator-services}

To date, these are the (simulated) services that can be instantiated on the
simulated platform:


- **Compute Services** (classes that derive `wrench::ComputeService`): These are services
  that know how to compute workflow tasks. These include bare-metal servers (`wrench::BareMetalComputeService`), cloud
  platforms (`wrench::CloudComputeService`), virtualized cluster platforms (`wrench::VirtualizedClusterComputeService`),
  batch-scheduled clusters (`wrench::BatchComputeService`).
  It is not technically required to instantiate a compute service, but then no workflow task
  can be executed by the WMS. 

- **Storage Services** (classes that derive `wrench::StorageService`): 
  These are services that know how to store workflow files, which can then be
  accessed in reading/writing by the compute services when executing tasks that
  read/write files. 
  It is not technically required to instantiate a storage service, but then no workflow task
  can have an input or an output file. 

- **File Registry Services** (the `wrench::FileRegistryService` class): 
  These services, often known as _replica catalogs_, are simply
  databases of <filename, list of locations> key-value pairs of the storage services
  on which a copies of files are available.  They are used during workflow execution to decide where
  input files for tasks can be acquired. 
  It is not required to instantiate a file registry service, unless the workflow's
  entry tasks have input files (because in this case these files have to be stored at
  some storage services
  before the execution can start, and all file registry service are then automatically made
  aware of where these files are stored). Note that some WMS implementations
  may complain if no file registry service is available.

- **Network Proximity Services** (the class `wrench::NetworkProximityService`): 
  These are services that monitor the network and maintain a database of 
  host-to-host network distances. This database can be queried by WMSs to make informed
  decisions, e.g., to pick from which storage service a file should be retrieved
  so as to reduce communication time.  Typically, network distances are estimated
  based on round-trip-times between hosts. 
  It is not required to instantiate a network proximity service, but some WMS implementations
  may complain if none is available.

- **Workflow Management Systems (WMSs)** (classes that derive `wrench::WMS`): 
  A workflow management system provides the mechanisms for executing workflow
  applications, include decision-making for optimizing various objectives (the most
  common one is to minimize workflow execution time).  By default,
  WRENCH does not provide a WMS implementation as part of its core components, however a
  simple implementation (`wrench::SimpleWMS`) is available in the `examples/simple-example` folder. Please,
  refer to the [Developer 101 Guide](../developer/wrench-101.html) section for further information
  on how to develop a WMS.  At least **one** WMS should be provided for running a simulation.
  Additional WMSs implementations may also be found in the [WRENCH project website](http://wrench-project.org).



# Customizing Services #         {#wrench-101-customizing-services}

Each service is customizable by passing to its constructor a _property list_, i.e., a key-value map
where each key is a property and each value is a string.  Each service defines a property class.
For instance, the `wrench::Service` class has an associated `wrench::ServiceProperty` class, 
the `wrench::ComputeService` class has an associated `wrench::ComputeServiceProperty` class, and
so on at all levels of the service class hierarchy. 

The API documentation for these property classes explains what each property means, what possible 
values are, and what default values are. Other properties have more to do with what the service 
can or should do when in operation. For instance, the `wrench::BatchComputeServiceProperty` class defines a
`wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM` which specifies what scheduling algorithm
a batch service should use for prioritizing jobs. All property classes inherit from the 
`wrench::ServiceProperty` class, and one can explore that hierarchy to discover
all possible (and there are many) service customization opportunities. 

Finally, each service exchanges messages on the network with other services (e.g., a WMS service sends 
a "do some work" message to a compute service). The size in bytes, or payload, of all messages can be 
customized similarly to the properties, i.e., by passing a key-value map to the service's constructor. For instance, 
the `wrench::ServiceMessagePayload` class defines a `wrench::ServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD`
property which can be used to customize the size, in bytes, of the control message sent to the
service daemon (that is the entry point to the service) to tell it to terminate. 
Each service class has a corresponding message payload class, and the API documentation for these
message payload classes details all messages whose payload can be customized. 


# Customizing logging  #        {#wrench-101-logging}

When running a WRENCH simulator you will notice that there is quite a bit of logging output. While logging
output can be useful to inspect visually the way in which the simulation proceeds, it often becomes necessary
to disable it.  WRENCH's logging system is a thin layer on top of SimGrid's logging system, and as such
is controlled via command-line arguments. The simple example in `examples/simple-example` is executed 
as follows, assuming the working directory is `examples/simple-example`:

~~~~~~~~~~~~~{.cpp}
./wrench-simple-example-cloud  platform_files/cloud_hosts.xml workflow_files/genome.dax
~~~~~~~~~~~~~

One first way in which to modify logging is to disable colors, which can be useful to redirect output
to a file, is to use the `--wrench-no-color` command-line option, anywhere in the argument list, for instance:

~~~~~~~~~~~~~{.cpp}
./wrench-simple-example-cloud  --wrench-no-color platform_files/cloud_hosts.xml workflow_files/genome.dax
~~~~~~~~~~~~~

Disabling all logging is done with the SimGrid option `--wrench-no-log`:

~~~~~~~~~~~~~{.cpp}
./wrench-simple-example-cloud  --wrench-no-log platform_files/cloud_hosts.xml workflow_files/genome.dax
~~~~~~~~~~~~~


The above `--wrench-no-log` option is a simple wrapper around the sophisticated Simgrid logging
capabilities (it is equivalent to the Simgrid argument `--log=root.threshold:critical`). 
Details on these capabilities are displayed when passing the
 `--help-logs` command-line argument to your simulator. In a nutshell particular "log categories" 
 can be toggled on and off. Log category names are attached to `*.cpp` files in the 
 WRENCH and SimGrid code. Using the `--help-log-categories` command-line
 argument shows the
entire log category hierarchy. For instance, there is a log category that is called `wms` for the
WMS, i.e., those logging messages in the `wrench:WMS` class and a log category that is called
`simple_wms` for logging message in the `wrench::SimpleWMS` class, which inherits from `wrench::WMS`. 
These messages are thus logging output produced by the WMS in the simple example. They can be enabled
while other messages are disabled as follows: 

~~~~~~~~~~~~~{.cpp}
./wrench-simple-example-cloud   platform_files/cloud_hosts.xml workflow_files/genome.dax --log=root.threshold:critical --log=simple_wms.threshold=debug --log=wms.threshold=debug
~~~~~~~~~~~~~

Use the `--help-logs` option displays information on the way SimGrid logging works. See the 
[full SimGrid logging documentation](http://simgrid.gforge.inria.fr/simgrid/latest/doc/outcomes_logs.html) for 
all details.


# Analyzing Simulation Output #   {#wrench-101-simulation-output}

Once the `wrench::Simulation::launch()` method has returned, it is possible to process time-stamped traces
to analyze simulation output. The `wrench::Simulation::getOutput()` method returns an instance of 
`wrench::SimulationOutput`. This object has a templated `wrench::SimulationOutput::getTrace()` method to 
retrieve traces for various information types. For instance, the call
```
simulation.getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>()
```
returns a vector of time-stamped task completion events. The classes that implement time-stamped events
are all classes named `wrench::SimulationTimestampSomething`, where `Something` is pretty self-explanatory
(e.g., `TaskCompletion`).


# Measuring Energy Consumption #                 {#wrench-101-energy}

WRENCH leverages [SimGrid's energy plugin](https://simgrid.org/doc/latest/Plugins.html#host-energy),
which provides accounting for computing time and dissipated energy in the simulated platform. SimGrid's energy plugin 
requires host **pstate** definitions (levels of performance, CPU frequency) in the
[XML platform description file](https://simgrid.org/doc/latest/platform.html). The following is a 
list of current available information provided by the plugin: 

- `wrench::Simulation::getNumberofPstates()`
- `wrench::Simulation::getMinPowerConsumption()`
- `wrench::Simulation::getMaxPowerConsumption()`
- `wrench::Simulation::getListOfPstates()`

**Note:** The energy plugin is NOT enabled by default in WRENCH simulation. To enable the plugin, the 
`--activate-energy` command line option should be provided when running a simulator.



@endWRENCHDoc






<!--################################################ -->
<!--############# 101 FOR DEVELOPERS  ############## -->
<!--################################################ -->


@WRENCHDeveloperDoc 


This **Developer 101** guide describes WRENCH's architectural components necessary
to build your own WMS (Workflow Management Systems).

---

[TOC]


# 10,000-ft view of a simulated WMS #           {#wrench-101-WMS-10000ft}

A Workflow Management System (WMS), i.e., the software that makes all decisions
 and takes all actions for executing a workflow, is implemented in WRENCH as 
 a simulated process. This process has a `main()` function that goes through 
 a simple loop as follows:
 
~~~~~~~~~~~~~{.cpp}
  while ( workflow_execution_hasnt_completed_or_failed ) {
    // interact with services
    // wait for an event
  }
~~~~~~~~~~~~~


# Blueprint for a WMS in WRENCH #               {#wrench-101-WMS-blueprint}

A WMS implementation in WRENCH must derive the `wrench::WMS` class, and
typically follows the following steps:

-# **Get references to running services:** The `wrench::WMS` base class implements
   a set of methods named `wrench::WMS::getAvailableComputeServices()`, `wrench::WMS::getAvailableStorageServices()`, etc. 
   These methods return sets of services that can be used by the WMS to execute its workflow. 
   
-# **Acquire information about the services:** Some service classes provide methods to get
   information about the capabilities of the services. For instance, a `wrench::ComputeService()`
   has a `wrench::ComputeService::getNumHosts()` method that makes it possible to find out how many compute hosts the service
   has access to in total.  A `wrench::StorageService` has a `wrench::StorageService::getFreeSpace()` method to find
   out have many bytes of free space are available to it.  Note that these methods actually involve
   communication with the service, and thus incur (simulated) overhead. 
   
-# **Go through a main loop:** The heart of the WMS's execution consists in
   going through a loop until the workflow is executed or has failed to execute. This loop
   consists of two main steps:
   
   - **Interact with services:** This is where the WMS can cause workflow files to be copied
      between storage services, and where workflow tasks can be submitted to compute services.
      See the [Interacting with services](#wrench-101-WMS-services) section below for more details.
   
   - **Wait for an event:** This is where the WMS is waiting for services to reply with 
      "work done" or "work failed" events. See the [Workflow execution events](#wrench-101-WMS-events) section for more details.


# Interacting with services  #                  {#wrench-101-WMS-services}

Each service type provides its own API. For instance, a network proximity service provides methods to query the service's
host distance databases. The [API Reference](./annotated.html) provides all necessary
documentation, which also explains which methods are synchronous and which are asynchronous (in which
case some [event](#wrench-101-WMS-events) will likely occur in the future).
 _However_, the WRENCH developer will find that many methods that one would
expect are nowhere to be found. For instance, the compute services do not have methods for compute job submissions!

The rationale for the above is that many methods need to be asynchronous so that the WMS can
use services concurrently. For instance, a WMS could submit a compute job to two distinct
compute services asynchronously, and then wait for the service which
completes its job first and cancel the job on the other service.  Exposing this asynchronicity to the WMS 
would require that the WRENCH developer uses data structures to perform the
necessary bookkeeping of ongoing service interactions, and process incoming control messages
from the services on the (simulated) network or register many callbacks. 
Instead, WRENCH provides **managers**. One can think of managers are separate threads
that handle all asynchronous interactions with services, and which have been implemented for your convenience
 to make service interactions easy.  

For now there are two possible managers: a **job manager** manager (class `wrench::JobManager`) and a 
**data movement manager** (class `wrench::DataMovementManager`). The base `wrench::WMS` class provides two methods
for instantiating and starting these managers: `wrench::WMS::createJobManager()` and `wrench::WMS::createDataMovementManager()`. 
 Creating these managers typically is the first thing a WMS does.
Each manager has its own documented API, and is discussed further in sections below. 


# Copying workflow data #                        {#wrench-101-WMS-data}

The WMS may need to explicitly copy files from one storage service to another storage service, e.g., 
to improve data locality when executing workflow tasks.
File copies are accomplished through the data movement manager, which provides two methods:

  - `wrench::DataMovementManager::doSynchronousFileCopy()`: performs a synchronous file copy (i.e., the call will block until the operation completes or fails)
  - `wrench::DataMovementManager::initiateAsynchronousFileCopy()`: performs and asynchronous file copy (i.e., the call returns almost immediately, and
  an [event](#wrench-101-WMS-events) will be generated later on)
  
Both methods take an optional `wrench::FileRegistryService` argument, in which case they will also update this file registry service
with a new entry once the file copy has been completed. 


# Running workflow tasks #                      {#wrench-101-WMS-tasks}

A workflow comprises tasks, and a WMS must pack tasks into _jobs_ to execute them. There are two kinds of jobs in WRENCH:
`wrench::PilotJob` and `wrench::StandardJob`.   A pilot job (sometimes called a "placeholder job") is a concept that is
mostly relevant for batch scheduling. In a nutshell, it is a job that allows late binding of tasks to resources. It is submitted
to a compute service (provided that service supports pilot jobs), and when a pilot job starts it just looks to the WMS like a short-lived compute service to which standard jobs can be
submitted.  

The most common kind of jobs is the standard job. A standard job is a unit of execution by which a WMS tells a compute service
to do things. More specifically, in its most complete form, a standard job specifies:
    
  - A vector of `wrench::WorkflowTask` to execute;
  - A `std::map` of `<wrench::WorkflowFile*, wrench::StorageService *>` values which specifies from which storage services particular input files should be read and to
    which storage services output files should be written. (Note that a compute service can be associated to a "by default" storage service
    upon instantiation);
  - A set of file copy operations to be performed before executing the tasks;
  - A set of file copy operations to be performed after executing the tasks; and
  - A set of file deletion operations to be performed after executing the tasks.
  
Any of the above can actually be empty, and in the extreme a standard job does nothing.  

Standard jobs and pilot jobs are created via the job manager (see multiple versions of the `wrench::JobManager::createStandardJob()` and
 `wrench::JobManager::createPilotJob()` methods). The job manager thus 
acts as a job factory, and provides job management methods:

  - `wrench::JobManager::submitJob()`: used to submit a job to a compute service.
  - `wrench::JobManager::terminateJob()`: used to terminate a previously submitted job.
  - `wrench::JobManager::forgetJob()`: used to completely remove all data regarding a completed/failed job.
  - `wrench::JobManager::getPendingPilotJobs()`: used to retrieve a list of previously submitted jobs that have yet to begin executing.
  - `wrench::JobManager::getRunningPilotJobs()`: used to retrieve a list of currently running jobs.


# Workflow execution events #                   {#wrench-101-WMS-events}

Because the WMS, in part via the managers, performs many asynchronous operations, it needs to act as an event handler. 
This is called by calling the `wrench::WMS::waitForAndProcessNextEvent()` method implemented by the base
`wrench::WMS` class. A call to this method blocks until some event occurs. The possible event classes all derive
the `wrench::WorkflowExecutionEvent` class. A WMS can override a method to react to each possible event (the default
method does nothing but print some log message). At the time this
documentation is being written, these overridable methods are:

  - `wrench::WMS::processEventStandardJobCompletion()`: react to a standard job completion
  - `wrench::WMS::processEventStandardJobFailure()`: react to a standard job failure
  - `wrench::WMS::processEventPilotJobStart()`: react to a pilot job beginning execution
  - `wrench::WMS::processEventPilotJobExpiration()`: react to a pilot job expiration
  - `wrench::WMS::processEventFileCopyCompletion()`: react to a file copy completion
  - `wrench::WMS::processEventFileCopyFailure()`: react to a file copy failure
  
Each method above takes in an event object as parameter, and each even class offers
several methods to inspect the meaning of the event. 
In the case of failure, the event includes  a `wrench::FailureCause` object, which can be accessed to
understand the root cause of the failure. 


# Exceptions #                                  {#wrench-101-WMS-exceptions}

Most methods in the WRENCH Developer API throw `wrench::WorkflowExecutionException` instances when exceptions occur. 
These are exceptions that corresponds to failures during the simulated workflow executions (i.e., errors that would occur in a real-world
execution). Each such exception contains a `wrench::FailureCause` object, 
which can be accessed to understand the root cause of the execution failure.  
Other exceptions (e.g., `std::invalid_arguments`, `std::runtime_error`) are thrown as well, which are used for detecting mis-uses
of the WRENCH API or internal WRENCH errors. 


# Schedulers for decision-making #              {#wrench-101-WMS-schedulers}

A large part of what a WMS does is make decisions. It is often a good idea for decision-making algorithms
(often simply called "scheduling algorithms") to be re-usable across multiple WMS implementations, or 
plug-and-play-able for a single WMS implementation. For this reason, the `wrench::WMS` constructor takes
as parameters two objects (or null pointers if not needed):

  - `wrench::StandardJobScheduler`: A class that has a `wrench::PilotJobScheduler::schedulePilotJobs()` 
     method (to be overwritten) that can be invoked at any time by the WMS to submit pilot jobs to compute services. 
  - `wrench::PilotJobScheduler`: A class that has a `wrench::StandardJobScheduler::scheduleTasks()` 
     method (to be overwritten) that can be invoked at any time by the WMS to submit tasks (inside standard jobs) to compute services. 

Although not required, it is possible to implement most (or even all) decision-making in these two methods so at to have a clean
separation of concern between the decision-making part of the WMS and the rest of its functionality.  This kind of design is used
in the simple example provided in the `examples/simple-example` directory. 


# Logging #                                     {#wrench-101-WMS-logging}

It is often desirable for the WMS to print log output to the terminal. This is easily accomplished using the 
`wrench::WRENCH_INFO`, `wrench::WRENCH_DEBUG`, and `wrench::WRENCH_WARN` macros, which are used just like `printf`. Each of these
macros corresponds to a different logging level in SimGrid. See the 
[SimGrid logging documentation](http://simgrid.gforge.inria.fr/simgrid/latest/doc/outcomes_logs.html) for 
all details. 

Furthermore, one can change the color of the log messages with the
`wrench::TerminalOutput::setThisProcessLoggingColor()` method, which takes as
parameter a color specification:

~~~~~~~~~~~~~{.cpp}
wrench::TerminalOutput::COLOR_BLACK
wrench::TerminalOutput::COLOR_RED
wrench::TerminalOutput::COLOR_GREEN
wrench::TerminalOutput::COLOR_YELLOW
wrench::TerminalOutput::COLOR_BLUE
wrench::TerminalOutput::COLOR_MAGENTA
wrench::TerminalOutput::COLOR_CYAN
wrench::TerminalOutput::COLOR_WHITE
~~~~~~~~~~~~~

  
@endWRENCHDoc






<!--################################################ -->
<!--############### 101 FOR INTERNAL  ############## -->
<!--################################################ -->


@WRENCHInternalDoc


This **Internal 101** guide is intended for users who want to contribute code to WRENCH to
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
  - A WRENCH simulation, like any SimGrid simulation, is comprised of simulated processes
    that communicate via messages with put/get-like operations into mailboxes. These
    processes are essentially implemented as threads, but they are not scheduled by the OS. Instead,
    they are scheduled by SimGrid in round-robin fashion. Each thread runs without interruption in between
    two SimGrid API calls. Therefore, although in principle WRENCH is massively multi-threaded, there are
    _almost_ no need for locks when sharing data among threads.
  


@endWRENCHDoc





