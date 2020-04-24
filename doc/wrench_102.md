WRENCH 102                        {#wrench-102}
============

A Workflow Management System (WMS) is a software that makes all decisions
and takes all actions for executing a workflow using cyberinfrastructure 
services. It is thus a crucial component in every WRENCH simulator. 
WRENCH does not provide any WMS implementation, but provides the means for 
developing custom WMSs. This page is meant to provide high-level and 
detailed information about implementing a WMS in WRENCH. Full API details 
are provided in the [Developer API Reference](../developer/annotated.html).

[TOC]

---

# Basic blueprint for a WMS implementation #        {#wrench-102-WMS-10000ft}

A WMS implementation needs to use many WRENCH classes, which are accessed
by including a single header file:

~~~~~~~~~~~~~{.cpp}
#include <wrench-dev.h>
~~~~~~~~~~~~~

A WMS implementation must derive the `wrench::WMS` class, which means that
it can override several virtual methods, but also that a WMS is a service. 
As such, it has a `main()` function that goes through a simple loop as follows:

~~~~~~~~~~~~~{.sh}
// A) obtain information about running services
while (workflow execution is not completed/failed) {
  // B) interact with services 
  // C) wait for an event and react to it
}
~~~~~~~~~~~~~

In the next three sections, we give details on how to implement A, B, and C in 
the code above. To provide context, we make frequent references to the WMS
implementations in the example simulators in the `examples/` directory. 
Afterwards are a few sections that highlight features and functionality relevant 
to WMS development.

# A) Obtaining information about services #      {#wrench-102-obtain-information}

## Discovering running services #                {#wrench-102-obtain-information-discovering}

The `wrench::WMS` base class implements a set of methods named
`wrench::WMS::getAvailableComputeServices()`,
`wrench::WMS::getAvailableStorageServices()`,
`wrench::WMS::getAvailableNetworkProximityServices()`, etc.  These methods
return sets of services  that can be used by the WMS to execute its
workflow.  Some of these methods are templated to retrieve only a
particular kind of services. For instance, the
`wrench::WMS::getAvailableComputeServices()`  takes a  template argument
to  retrieve  particular kinds of compute services. In the example
simulator in `examples/basic-examples/bare-metal-chain`, the  WMS
implementation  in `OneTaskAtATimeWMS.cpp`  includes  the  following call:

~~~~~~~~~~~~~{.cpp}
auto compute_service = *(this->getAvailableComputeServices<BareMetalComputeService>().begin());
~~~~~~~~~~~~~

This call stores the first of the bare-metal compute services available to the WMS
for executing workflow tasks in the  `compute_service` variable. In  this
example the simulator always passes exactly one bare-metal service  to the
WMS, so this code is valid. However, `wrench::WMS::getAvailableComputeServices<T>()` can return an empty set. 

The above methods (as  well as, for instance, `wrench::Simulation::add()`)
return shared pointers (i.e., `std::shared_ptr<>`) to the service
instances. This is to free the developer from the responsibility of freeing
memory.


## Finding out information about running services #        {#wrench-102-obtain-information-finding}
   
Most service classes provide methods to get information about the
capabilities and properties of the services.  For instance, a
`wrench::ComputeService` has a `wrench::ComputeService::getNumHosts()`
method that returns how many compute hosts the
service has access to in total.  A `wrench::StorageService` has a
`wrench::StorageService::getFreeSpace()` method to find out have many bytes
of free space are available on it. And so on...

To take a concrete example, consider the WMS implementation in `examples/basic-examples/batch-bag-of-tasks/TwoTasksAtATimeBatchWMS.cpp`. This WMS finds out the compute speed of the cores of the compute nodes
available to  a `wrench::BatchComputeService` as:

~~~~~~~~~~~~~{.cpp}
double core_flop_rate = (*(batch_service->getCoreFlopRate().begin())).second;
~~~~~~~~~~~~~

Method `wrench::ComputeService::getCoreFlopRate()` returns a map of core
compute speeds indexed by hostname (the map thus has  one element per
compute node available to the service). Since the  compute nodes of a batch
compute service are homogeneous, the above code simply grabs the core
speed value of the first element in the map.

It is important to note that these methods actually
involve communication with the service, and thus incur overhead 
that is part of the simulation (as if,  in  the  real-world, you would
contact a running service  with a request for information over the network). 
This is why the line of code above, in that example WMS, is executed once 
and the core compute speed is stored in the `core_flop_rate` variable
to be  re-used by  the WMS repeatedly throughout  its execution.


# B)  Interacting with services  #                  {#wrench-102-WMS-services}

A WMS can have many and complex interactions with services, especially
with compute and storage services. In this section we describe how WRENCH
makes these  interactions relatively easy, providing examples for
each kind  of interaction for each kind of service. 

## Job Manager and Data Movement Manager #           {#wrench-102-WMS-services-managers}

As expected, each service type provides its own API. For instance, a network proximity
service provides methods to query the service's host distance databases.
The [Developer API Reference](../developer/annotated.html) provides all
necessary documentation, which also explains which methods are synchronous
and which are asynchronous (in which case some
[event](@ref wrench-102-WMS-events) will occur in the future).
**However, the WRENCH developer will find that many methods that one would
expect are nowhere to be found. For instance, the compute services do not
have methods for submitting workflow tasks for execution!**

The rationale for the above is that many methods need to be asynchronous so
that the WMS can use services concurrently. For instance, a WMS could
submit a compute job to two distinct compute services asynchronously, and
then wait for the service which completes its job first and cancel the job
on the other service.  Exposing this asynchronicity to the WMS would
require that the WRENCH developer use data structures to perform the
necessary bookkeeping of ongoing service interactions, and process incoming
control messages from the services on the (simulated) network or alternately register
many callbacks.  Instead, WRENCH provides **managers**. One can think of
managers are separate threads that handle all asynchronous interactions
with services, and which have been implemented for your convenience
 to make interacting with services easy.

There are two managers: a **job manager** (class
`wrench::JobManager`) and a **data movement manager** (class
`wrench::DataMovementManager`). The base `wrench::WMS` class provides two
methods for instantiating and starting these managers:
`wrench::WMS::createJobManager()` and
`wrench::WMS::createDataMovementManager()`.

Creating these managers typically is the first thing a WMS does. For instance, the WMS in
`examples/basic-examples/bare-metal-data-movement/DataMovementWMS.cpp`  starts by doing:

~~~~~~~~~~~~~{.cpp}
auto job_manager = this->createJobManager();
auto data_movement_manager = this->createDataMovementManager();
~~~~~~~~~~~~~


Each manager has its own documented API, and is discussed further in
sections below.

## Interacting with storage services #                 {#wrench-102-WMS-services-storage}

The  possible interactions between a WMS and a storage  service include:

  - Synchronously check that a file exists
  - Synchronously read a file (rarely used by a WMS but included for completeness)
  - Synchronously write a file (rarely used by a WMS but included for completeness)
  - Synchronously delete a file
  - Synchronously copy a file from one storage service to another
  - Asynchronously copy a file from one storage service to another

The first 4 interactions above are done by calling methods of the
`wrench::StorageService` class. The last two are done via a Data Movement
Manager, i.e., by  calling methods of the `wrench::DataMovementManager` class.  Some of
these methods  take an optional `wrench::FileRegistryService` argument, in which case 
they will also update entries in a file registry service (e.g., removing an entry
when a file is deleted). 

See [this page](@ref guide-102-simplestorage) for  concrete examples of interactions
with a `wrench::SimpleStorageService`.


## Interacting with compute services #               {#wrench-102-WMS-services-compute}

### The Job abstraction #                            {#wrench-102-WMS-services-compute-job}

The main activity of a WMS is to execute workflow tasks on compute services. 
Rather than  submitting tasks directly to compute services, a WMS must
create "jobs", which  can comprise multiple tasks and involve data copy/deletion
operations. The job abstraction is powerful and greatly simplifies the task
of a WMS  while affording flexibility. 

There are two kinds of jobs in WRENCH: `wrench::PilotJob` and
`wrench::StandardJob`.   A pilot job (sometimes called a "placeholder job" in the literature)
is a concept that is mostly relevant for batch scheduling. In a nutshell,
it is a job that allows late binding of tasks to resources. It is submitted
to a compute service (provided that service supports pilot jobs), and when
it starts it just looks to the WMS like a temporary (bare-metal) compute
service to which standard jobs can be submitted.

The most common kind of jobs is the standard job. A standard job is a unit
of execution by which a WMS tells a compute service to do a set of operations. More
specifically, in its most complete form, a standard job specifies:

  - A set (in fact a vector) of `wrench::WorkflowTask` to execute, so that each
    task without all its predecessors in the set is ready;

  - A `std::map` of `<wrench::WorkflowFile*, std::shared_ptr<wrench::StorageService>>`
    values that specifies from which storage services particular input files
    should be read and to which storage services output files should be
    written;

  - A set of file copy operations to be performed before executing the tasks; 

  - A set of file copy operations to be performed after executing
    the tasks; and 

  - A set of file deletion operations to be performed after
    executing the tasks and file copy operations.

Any of the above can actually be empty, and in the extreme a standard job
can  do nothing.

Standard jobs and pilot jobs are created via the job manager, which
provides a `wrench::JobManager::createPilotJob()` method and several
versions of a `wrench::JobManager::createStandardJob()` method.  Briefly
put, the job manager is a job factory.

The job manager provides the following expected methods:

  - `wrench::JobManager::submitJob()`: asynchronous submission of a job to a compute service.

  - `wrench::JobManager::terminateJob()`: synchronous termination of a previously submitted job.

  - `wrench::JobManager::getPendingPilotJobs()`: synchronous retrieval of the list of pending  pilot jobs.

  - `wrench::JobManager::getRunningPilotJobs()`: synchronous retrieval of the list of running pilot jobs.

  - `wrench::JobManager::forgetJob()`: free memory for a completed/failed job.

The next section gives many examples of interactions with each kind of compute service.


Click on the following links to see detailed descriptions of
and examples of how jobs are submitted to each compute service type:

  - [Bare-metal compute service](@ref guide-102-baremetal)
  - [Batch compute service](@ref guide-102-batch)
  - [Cloud compute service](@ref guide-102-cloud)
  - [Virtualized cluster compute service](@ref guide-102-virtualizedcluster)
  - [HTCondor compute service](@ref guide-102-htcondor)



## Interacting with file registry services #              {#wrench-102-WMS-services-registry}

Interaction with a file registry service is straightforward and done by directly
calling methods of the `wrench::FileRegistryService` class. Note that often
file registry service entries are managed automatically, e.g.,  via calls to
`wrench::DataMovementManager` and `wrench::StorageService` methods. So often
a WMS  does not need to interact with the file registry service. 

Adding/removing an entry to a file registry service is done as follows:

~~~~~~~~~~~~~{.sh}
fr_service = this->getAvailableFileRegistryService();
wrench::WorkflowFile *some_file  = ...;
std::shared_ptr<wrench::StorageService> some_storage_service = ...;

[...]

fr_service->addEntry(some_file, wrench::FileLocation::LOCATION(some_storage_service));
fr_service->removeEntry(some_file, wrench::FileLocatio::LOCATION(some_storage_service));
~~~~~~~~~~~~~

The `wrench::FileLocation` class is a convenient abstraction
for a file copy  available at some storage service. 

Retrieving all entries for a given file is done as follows:


~~~~~~~~~~~~~{.cpp}
wrench::WorkflowFile *some_file = ...;

[...]

std::set<std::shared_ptr<wrench::FileLocation>> entries;
entries = fr_service->lookupEntry(some_file);
~~~~~~~~~~~~~

If a network proximity service is running, it is possible to retrieve
entries for a file sorted by non-decreasing proximity from some reference
host. Returned entries are stored in a (sorted) `std::map` where the keys
are network distances to the reference host. For instance:

~~~~~~~~~~~~~{.cpp}
wrench::WorkflowFile *some_file = ...;
std::shared_ptr<wrench::NetworkProximityService> np_service = 
  *(this->getAvailableNetworkProximityServices().begin());

[...]

auto entries = fr_service->lookupEntry(some_file, "ReferenceHost", np_service);
~~~~~~~~~~~~~

See the documentation of `wrench::FileRegistryService` for more API methods.

## Interacting with network proximity services #              {#wrench-102-WMS-services-network}

Querying a network proximity service is straightforward. For instance, to
obtain a measure of the network distance between hosts "Host1" and "Host3",
one simply does:

~~~~~~~~~~~~~{.cpp}
std::shared_ptr<wrench::NetworkProximityService> np_service = 
  *(this->getAvailableNetworkProximityServices().begin());

double distance = np_service->query(std::make_pair("Host1","Host2"));
~~~~~~~~~~~~~

This distance corresponds to half the round-trip-time, in seconds, between
the two hosts.  If the service is configured to use the Vivaldi
coordinate-based system, as in our example above, this distance is actually
derived from network coordinates, as computed by the Vivaldi algorithm. In
this case one can actually ask for these coordinates for any given host:

~~~~~~~~~~~~~{.cpp}
std::pair<double,double> coords = np_service->getCoordinates("Host1");
~~~~~~~~~~~~~

See the documentation of `wrench::NetworkProximityService` for more API methods.




# C) Workflow execution events #                     {#wrench-102-WMS-events}


Because the WMS performs asynchronous
operations, it needs to wait for and re-act to events.  This is done by
calling the `wrench::WMS::waitForAndProcessNextEvent()` method implemented
by the base `wrench::WMS` class. A call to this method blocks until some
event occurs  and the calls a callback method. The possible event classes all derive the
`wrench::WorkflowExecutionEvent` class, and a WMS can override the callback method for
each possible event (the default method does nothing but print
some log message). These overridable callback methods are:

  - `wrench::WMS::processEventStandardJobCompletion()`: react to a standard job completion
  - `wrench::WMS::processEventStandardJobFailure()`: react to a standard job failure
  - `wrench::WMS::processEventPilotJobStart()`: react to a pilot job beginning execution
  - `wrench::WMS::processEventPilotJobExpiration()`: react to a pilot job expiration
  - `wrench::WMS::processEventFileCopyCompletion()`: react to a file copy completion
  - `wrench::WMS::processEventFileCopyFailure()`: react to a file copy failure

Each method above takes in an event object as parameter.
In the
case of failure, the event includes  a `wrench::FailureCause` object, which
can be accessed to analyze (or just display) the root cause of the failure.

Consider the WMS in `examples/basic-examples/bare-metal-bag-of-tasks/TwoTasksAtATimeWMS.cpp`. At each
each iteration of its main loop it does:

~~~~~~~~~~~~~{.cpp}
// Submit some standard job to some compute  service
job_manager->submitJob(...);

// Wait for and process next event
this->waitForAndProcessNextEvent();
~~~~~~~~~~~~~

In this simple example, only one of two events could occur at this point: a standard  job completion
or a standard job failure. As a result, this WMS overrides the two corresponding methods as follows:

~~~~~~~~~~~~~{.cpp}
void TwoTasksAtATimeWMS::processEventStandardJobCompletion(
               std::shared_ptr<StandardJobCompletedEvent> event) {
  // Retrieve the job that this event is for 
  auto job = event->standard_job;
  // Print some message for each task in the job
  for (auto const &task : job->getTasks()) {
    std::cerr  << "Notified that a standard job has completed task " << task->getID() << std::endl;
  }
}

void TwoTasksAtATimeWMS::processEventStandardJobFailure(
               std::shared_ptr<StandardJobFailedEvent> event) {
  // Retrieve the job that this event is for 
  auto job = event->standard_job;
  std::cerr  << "Notified that a standard job has failed (failure cause: ";
  std::cerr << event->failure_cause->toString() << ")" <<  std::endl;
  // Print some message for each task in the job if it has failed
  std::cerr << "As a result, the following tasks have failed:";
  for (auto const &task : job->getTasks()) { 
    if (task->getState != WorkflowTask::COMPLETE) { 
      std::cerr  << "  - " << task->getID() << std::endl;
    }       
  }
}
~~~~~~~~~~~~~

You may note some difference between the above code  and that in
`examples/basic-examples/bare-metal-bag-of-tasks/TwoTasksAtATimeWMS.cpp`.
This is for clarity purposes, and especially because we have not yet
explained  how  WRENCH does message logging. See                
[an upcoming section about logging](@ref wrench-102-WMS-logging).


While the above callbacks are convenient, sometimes it is desirable to do
things more manually.  That is, wait for an event and then  process it in
the code of the main loop of the WMS rather than in a callback method. This
is done by calling the `wrench::waitForNextEvent()` method.  For instance, the WMS
in `examples/basic-examples/bare-metal-data-movement/DataMovementWMS.cpp`
does it as:

~~~~~~~~~~~~~{.cpp}
// Initiate an asynchronous file copy
data_movement_manager->initiateAsynchronousFileCopy(...);

// Wait for an event
auto event = this->waitForNextEvent();


//Process the event
if (auto file_copy_completion_event = std::dynamic_pointer_cast<wrench::FileCopyCompletedEvent>(event)) {
  std::cerr << "Notified of a file copy completion for file ";
  std::cerr << file_copy_completion_event->file->getID()<< "as expected" << std::endl;
} else {
   throw std::runtime_error("Unexpected event (" + event->toString() + ")");}
}
~~~~~~~~~~~~~


# Exceptions #                                  {#wrench-102-WMS-exceptions}

Most methods in the WRENCH Developer API throw exceptions. In fact, most of
the code fragments above should be in try-catch clauses, catching these
exceptions.

Some exceptions correspond to failures during the simulated workflow
executions (i.e., errors that would occur in a real-world execution and are
thus part of the simulation). Each such exception contains a
`wrench::FailureCause` object, which can be accessed to understand the root
cause of the execution failure.  Other exceptions (e.g.,
`std::invalid_arguments`, `std::runtime_error`) are thrown as well, which
are used for detecting mis-uses of the WRENCH API or internal WRENCH
errors.


# Schedulers for decision-making #              {#wrench-102-WMS-schedulers}

A large part of what a WMS does is make decisions. It is often a good idea
for decision-making algorithms (often simply called "scheduling
algorithms") to be re-usable across multiple WMS implementations, or
plug-and-play-able for a single WMS implementation. For this reason, the
`wrench::WMS` constructor takes as parameters two objects (or null pointers
if not needed):

  - `wrench::StandardJobScheduler`: A class that has a `wrench::PilotJobScheduler::schedulePilotJobs()` 
     method (to be overwritten) that can be invoked at any time by the WMS to submit pilot jobs to compute services. 
  - `wrench::PilotJobScheduler`: A class that has a `wrench::StandardJobScheduler::scheduleTasks()` 
     method (to be overwritten) that can be invoked at any time by the WMS to submit tasks (inside standard jobs) to compute services. 

Although not required, it is possible to implement most (or even all)
decision-making in these two methods so at to have a clean separation of
concern between the decision-making part of the WMS and the rest of its
functionality.  This kind of design is used in the example simulators  in the 
`examples/real-workflow-example/` directory.


# Logging #                                     {#wrench-102-WMS-logging}

It is typically desirable for the WMS to print log output to the terminal.
This is easily accomplished using the `wrench::WRENCH_INFO()`,
`wrench::WRENCH_DEBUG()`, and `wrench::WRENCH_WARN()` macros, which are used
just like C's `printf()`. Each of these macros corresponds to a different logging
level in SimGrid. See the [SimGrid logging
documentation](https://simgrid.org/doc/latest/outcomes.html) for all
details.

Furthermore, one can change the color of the log messages with the
`wrench::TerminalOutput::setThisProcessLoggingColor()` method, which takes as parameter a color specification:

  - `wrench::TerminalOutput::COLOR_BLACK`
  - `wrench::TerminalOutput::COLOR_RED`
  - `wrench::TerminalOutput::COLOR_GREEN`
  - `wrench::TerminalOutput::COLOR_YELLOW`
  - `wrench::TerminalOutput::COLOR_BLUE`
  - `wrench::TerminalOutput::COLOR_MAGENTA`
  - `wrench::TerminalOutput::COLOR_CYAN`
  - `wrench::TerminalOutput::COLOR_WHITE`

When inspecting the code of the WMSs in the example simulators
you will find many examples of calls to `wrench::WRENCH_INFO()`.
The logging is per .cpp file, each of which corresponds to a declared
logging category. For instance, in
`examples/basic-examples/batch-bag-of-tasks/TwoTasksAtATimeBatchWMS.cpp`,
you will find the typical pattern:

~~~~~~~~~~~~~{.cpp}
// Define a log category name for this file
WRENCH_LOG_CATEGORY(custom_wms, "Log category for TwoTasksAtATimeBatchWMS");

[...]

int TwoTasksAtATimeBatchWMS::main() {

  // Set the logging color to green
  TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);

  [...]

  // Print an info-level message, using printf-like format
  WRENCH_INFO("Submitting the job, asking for %s %s-core nodes for %s minutes",
              service_specific_arguments["-N"].c_str(),
              service_specific_arguments["-c"].c_str(),
              service_specific_arguments["-t"].c_str());

  [...]

  // Print a last info-level message 
  WRENCH_INFO("Workflow execution complete");
  return 0;
}
~~~~~~~~~~~~~

The name of the logging category, in this case `custom_wms`, can then be passed  to the
`--log` command-line argument. For instance, invoking the simulator with additional
arguments `--wrench-no-logs --log=custom_wms.threshold=info` will make it so that
only those `WRENCH_INFO` statements in  `TwoTasksAtATimeBatchWMS.cpp` will be printed (in green!). 
