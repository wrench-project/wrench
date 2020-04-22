WRENCH 102                        {#wrench-102}
============


<!--
@WRENCHUserDoc <div class="doc-type">User Documentation</div><div class="doc-link">Other: <a href="../developer/wrench-101.html">Developer</a> - <a href="../internal/wrench-101.html">Internal</a></div> @endWRENCHDoc
@WRENCHDeveloperDoc  <div class="doc-type">Developer Documentation</div><div class="doc-link">Other: <a href="../user/wrench-101.html">User</a> - <a href="../internal/wrench-101.html">Internal</a></div> @endWRENCHDoc
@WRENCHInternalDoc  <div class="doc-type">Internal Documentation</div><div class="doc-link">Other: <a href="../user/wrench-101.html">User</a> -  <a href="../developer/wrench-101.html">Developer</a></div> @endWRENCHDoc
-->



XXXX CHANGE  FROM 101 XXXX
This page is meant to provide high-level and detailed information about
what WRENCH simulators can simulate and how they do it. Full API details
are provided in the [User API Reference](../user/annotated.html).  See the
relevant pages for instructions on how to [install WRENCH](@ref install)
and how to [setup a simulator project](@ref getting-started).

[TOC]


---


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
[SimGrid logging documentation](https://simgrid.org/doc/latest/outcomes.html) for 
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

  




