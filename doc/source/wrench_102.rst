.. _wrench-102-header:

WRENCH 102
**********

In WRENCH's terminology, and *execution controller* is software that
makes all decisions and takes all actions for executing some application
workflow using cyberinfrastructure services. It is thus a crucial
component in every WRENCH simulator. WRENCH does not provide any
execution controller implementation, but provides the means for
developing custom ones. This page is meant to provide high-level and
detailed information about implementing an execution controller in
WRENCH. Full API details are provided in the :ref:`Developer API
Reference <developer-api>`.

.. _wrench-102-execution-controller-10000ft:

Basic blueprint for an execution controller implementation
==========================================================

An execution controller implementation needs to use many WRENCH classes,
which are accessed by including a single header file:

.. code:: cpp

   #include <wrench-dev.h>

An execution controller implementation must derive the
:cpp:class:`wrench::ExecutionController` class, which means that it must override
several the virtual ``main()`` member function. A typical such
implementation of this function goes through a simple loop as follows:

.. code:: sh

   // A) create/retrieve application workload to execute
   // B) obtain information about running services
   while (application workload execution has not completed/failed) {
     // C) interact with services 
     // D) wait for an event and react to it
   }

In the next three sections, we give details on how to implement the
above. To provide context, we make frequent references to the execution
controllers implemented as part of the example simulators in the
``examples/`` directory. Afterwards are a few sections that highlight
features and functionality relevant to execution controller development.

.. _wrench-102-obtain-information:

A) Finding out information about running services
=================================================

Services that the execution controller can use are typically passed to
its constructor. Most service classes provide member functions to get
information about the capabilities and properties of the services. For
instance, a :cpp:class:`wrench::ComputeService` has a
:cpp:class:`wrench::ComputeService::getNumHosts()` member function that returns
how many compute hosts the service has access to in total. A
:cpp:class:`wrench::StorageService` has a
:cpp:class:`wrench::StorageService::getFreeSpace()` member function to find out
how many bytes of free space are available on it. And so on…

To take a concrete example, consider the execution controller
implementation in
``examples/workflow_api/basic-examples/batch-bag-of-tasks/TwoTasksAtATimeBatchWMS.cpp``.
This WMS finds out the compute speed of the cores of the compute nodes
available to a :cpp:class:`wrench::BatchComputeService` as:

.. code:: cpp

   double core_flop_rate = (*(batch_service->getCoreFlopRate().begin())).second;

Member function :cpp:class:`wrench::ComputeService::getCoreFlopRate()` returns a
map of core compute speeds indexed by hostname (the map thus has one
element per compute node available to the service). Since the compute
nodes of a batch compute service are homogeneous, the above code simply
grabs the core speed value of the first element in the map.

It is important to note that these member functions actually involve
communication with the service, and thus incur overhead that is part of
the simulation (as if, in the real-world, you would contact a running
service with a request for information over the network). This is why
the line of code above, in that example execution controller, is
executed once and the core compute speed is stored in the
``core_flop_rate`` variable to be re-used by the execution controller
repeatedly throughout its execution.

.. _wrench-102-controller-services:

B) Interacting with services
============================

An execution controller can have many and complex interactions with
services, especially with compute and storage services. In this section,
we describe how WRENCH makes these interactions relatively easy,
providing examples for each kind of interaction for each kind of
service.

.. _wrench-102-controller-services-managers:

Job Manager and Data Movement Manager
-------------------------------------

As expected, each service type provides its own API. For instance, a
network proximity service provides member functions to query the
service's host distance databases. The :ref:`Developer API
Reference <developer-api>` provides all necessary
documentation, which also explains which member functions are
synchronous and which are asynchronous (in which case some
:ref:`event <wrench-102-controller-events>` will occur in the
future). **However, the WRENCH developer will find that many member
functions that one would expect are nowhere to be found. For instance,
the compute services do not have (public) member functions for
submitting jobs for execution!**

The rationale for the above is that many member functions need to be
asynchronous so that the execution controller can use services
concurrently. For instance, an execution controller could submit a job
to two distinct compute services asynchronously, and then wait for the
service which completes its job first and cancel the job on the other
service. Exposing this asynchronicity to the execution controller would
require that the WRENCH developer use data structures to perform the
necessary bookkeeping of ongoing service interactions, and process
incoming control messages from the services on the (simulated) network
or alternately register many callbacks. Instead, WRENCH provides
**managers**. One can think of managers as separate threads that handle
all asynchronous interactions with services, and which have been
implemented for your convenience to make interacting with services easy.

There are two managers: a **job manager**
(class :cpp:class:`wrench::JobManager`) and a **data movement manager** (class
:cpp:class:`wrench::DataMovementManager`). The base
:cpp:class:`wrench::ExecutionController` class provides two member functions for
instantiating and starting these managers:
:cpp:class:`wrench::ExecutionController::createJobManager()` and
:cpp:class:`wrench::ExecutionController::createDataMovementManager()`.

Creating one or two of these managers typically is the first thing an
execution controller does. For instance, the execution controller in
``examples/workflow_api/basic-examples/bare-metal-data-movement/DataMovementWMS.cpp``
starts by doing:

.. code:: cpp

   auto job_manager = this->createJobManager();
   auto data_movement_manager = this->createDataMovementManager();

Each manager has its own documented API, and is discussed further in
sections below.

.. _wrench-102-controller-services-storage:

Interacting with storage services
---------------------------------

Typical interactions between an execution controller and a storage
service include locating, reading, writing, and copying files. Different storage
service implementations may or not implement some of of these operations.
Click on the following links to see concrete examples
of interactions with the currently available storage service type:

- :ref:`Simple storage service <guide-102-simplestorage>`
- :ref:`XRootD storage service <guide-102-xrootd>`

.. _wrench-102-controller-services-compute:

Interacting with compute services
---------------------------------

.. _wrench-102-controller-services-compute-job:

The Job abstraction
~~~~~~~~~~~~~~~~~~~

The main activity of an execution controller is to execute workflow
tasks on compute services. Rather than submitting tasks directly to
compute services, an execution controller must create “jobs”, which can
comprise multiple tasks and involve data copy/deletion operations. The
job abstraction is powerful and greatly simplifies the task of an
execution controller while affording flexibility.

**There are three kinds of jobs in WRENCH**: :cpp:class:`wrench::CompoundJob`,
:cpp:class:`wrench::StandardJob`, and :cpp:class:`wrench::PilotJob`.

A **Compound Job** is simply set of actions to be performed, with
possible control dependencies between actions. It is the most generic,
flexible, and expressive kind of job. See the API documentation for the
:cpp:class:`wrench::CompoundJob` class and the examples in the
``examples/action_api`` directory. The other types of jobs below are
actually implemented internally as compound jobs. The Compound Job
abstraction is the most recent addition to the WRENCH API, and vastly
expands the list of possible things that an execution controller can do.
But because it is more recent, the reader will find that there are more
examples in these documents and in the ``examples`` directory that use
standard jobs (described below). But all these examples could be easily
rewritten using the more generic compound job abstraction.

A **Standard Job** is a specific kind of job designed for **workflow**
applications. In its most complete form, a standard job specifies: - A
set (in fact a vector) of ``std::shared_ptr<wrench::WorkflowTask>`` to
execute, so that each task without all its predecessors in the set is
ready;

-  A ``std::map`` of
   ``<std::shared_ptr<wrench::DataFile>>, std::shared_ptr<wrench::StorageService>>``
   pairs that specifies from which storage services particular input
   files should be read and to which storage services output files
   should be written;

-  A set of file copy operations to be performed before executing the
   tasks;

-  A set of file copy operations to be performed after executing the
   tasks; and

-  A set of file deletion operations to be performed after executing the
   tasks and file copy operations.

Any of the above can actually be empty, and in the extreme a standard
job can do nothing.

A **Pilot Job** (sometimes called a “placeholder job” in the literature)
is a concept that is mostly relevant for batch scheduling. In a
nutshell, it is a job that allows late binding of tasks to resources. It
is submitted to a compute service (provided that service supports pilot
jobs), and when it starts it just looks to the execution controller like
a short-lived :cpp:class:`wrench::BareMetalComputeService` to which compound
and/or standard jobs can be submitted.

All jobs are created via the job manager, which provides
:cpp:class:`wrench::JobManager::createCompoundJob()`,
:cpp:class:`wrench::JobManager::createStandardJob()`, and
:cpp:class:`wrench::JobManager::createPilotJob()` member functions (the job
manager is thus a job factory).

In addition to member functions for job creation, the job manager also
provides the following:

-  :cpp:class:`wrench::JobManager::submitJob()`: asynchronous submission of a job
   to a compute service.

-  :cpp:class:`wrench::JobManager::terminateJob()`: synchronous termination of a
   previously submitted job.

The next section gives examples of interactions with each kind of
compute service.

Click on the following links to see detailed descriptions and examples
of how jobs are submitted to each compute service type:

- :ref:`Bare-metal compute service <guide-102-baremetal>`
- :ref:`Batch compute service <guide-102-batch>`
- :ref:`Cloud compute service <guide-102-cloud>`
- :ref:`Virtualized cluster compute service <guide-102-virtualizedcluster>`
- :ref:`HTCondor compute service <guide-102-htcondor>`

.. _wrench-102-controller-services-registry:

Interacting with file registry services
---------------------------------------

Interaction with a file registry service is straightforward and done by
directly calling member functions of the :cpp:class:`wrench::FileRegistryService`
class. Note that often file registry service entries are managed
automatically, e.g., via calls to :cpp:class:`wrench::DataMovementManager` and
:cpp:class:`wrench::StorageService` member functions. So often an execution
controller does not need to interact with the file registry service.

Adding/removing an entry to a file registry service is done as follows:

.. code:: sh

   std::shared_ptr<wrench::FileRegistryService> file_registry;
   std::shared_ptr<wrench::DataFile> some_file;
   std::shared_ptr<wrench::StorageService> some_storage_service;

   [...]

   file_registry->addEntry(some_file, wrench::FileLocation::LOCATION(some_storage_service));
   file_registry->removeEntry(some_file, wrench::FileLocatio::LOCATION(some_storage_service));

The :cpp:class:`wrench::FileLocation` class is a convenient abstraction for a
file copy available at some storage service (with optionally a directory
path at that service).

Retrieving all entries for a given file is done as follows:

.. code:: cpp

   std::shared_ptr<wrench::FileRegistryService> file_registry;
   std::shared_ptr<wrench::DataFile> some_file;

   [...]

   std::set<std::shared_ptr<wrench::FileLocation>> entries;
   entries = file_registry->lookupEntry(some_file);

If a network proximity service is running, it is possible to retrieve
entries for a file sorted by non-decreasing proximity from some
reference host. Returned entries are stored in a (sorted) ``std::map``
where the keys are network distances to the reference host. For
instance:

.. code:: cpp

   std::shared_ptr<wrench::FileRegistryService> file_registry;
   std::shared_ptr<wrench::DataFile> some_file;
   std::shared_ptr<wrench::NetworkProximityService> np_service;

   [...]

   auto entries = fr_service->lookupEntry(some_file, "ReferenceHost", np_service);

See the documentation of :cpp:class:`wrench::FileRegistryService` 
for more API member functions.

.. _wrench-102-controller-services-network:

Interacting with network proximity services
-------------------------------------------

Querying a network proximity service is straightforward. For instance,
to obtain a measure of the network distance between hosts “Host1” and
“Host2”, one simply does:

.. code:: cpp

   std::shared_ptr<wrench::NetworkProximityService> np_service;

   double distance = np_service->query(std::make_pair("Host1","Host2"));

This distance corresponds to half the round-trip-time, in seconds,
between the two hosts. If the service is configured to use the Vivaldi
coordinate-based system, as in our example above, this distance is
actually derived from network coordinates, as computed by the Vivaldi
algorithm. In this case, one can actually ask for these coordinates for
any given host:

.. code:: cpp

   std::pair<double,double> coords = np_service->getCoordinates("Host1");

See the documentation of :cpp:class:`wrench::NetworkProximityService` 
for more API member functions.

.. _wrench-102-controller-events:

C) Workflow execution events
============================

Because the execution controller performs asynchronous operations, it
needs to wait for and re-act to events. This is done by calling the
:cpp:class:`wrench::ExecutionController::waitForAndProcessNextEvent()` member
function implemented by the base ``wrench::ExecutionController`` class.
A call to this member function blocks until some event occurs and then
calls a callback member function. The possible event classes all derive
from the :cpp:class:`wrench::ExecutionEvent` class, and an execution controller
can override the callback member function for each possible event (the
default member function does nothing but print some log message). These
overridable callback member functions are:

-  :cpp:class:`wrench::ExecutionController::processEventCompoundJobCompletion()`:
   react to a compound job completion
-  :cpp:class:`wrench::ExecutionController::processEventCompoundJobFailure()`:
   react to a compound job failure
-  :cpp:class:`wrench::ExecutionController::processEventStandardJobCompletion()`:
   react to a standard job completion
-  :cpp:class:`wrench::ExecutionController::processEventStandardJobFailure()`:
   react to a standard job failure
-  :cpp:class:`wrench::ExecutionController::processEventPilotJobStart()`: react
   to a pilot job beginning execution
-  :cpp:class:`wrench::ExecutionController::processEventPilotJobExpiration()`:
   react to a pilot job expiration
-  :cpp:class:`wrench::ExecutionController::processEventFileCopyCompletion()`:
   react to a file copy completion
-  :cpp:class:`wrench::ExecutionController::processEventFileCopyFailure()`: react
   to a file copy failure

Each member function above takes in an event object as parameter. In the
case of failure, the event includes a :cpp:class:`wrench::FailureCause` object,
which can be accessed to analyze (or just display) the root cause of the
failure.

Consider the execution controller in
``examples/workflow_api/basic-examples/bare-metal-bag-of-tasks/TwoTasksAtATimeWMS.cpp``.
At each each iteration of its main loop it does:

.. code:: cpp

   // Submit some standard job to some compute  service
   job_manager->submitJob(...);

   // Wait for and process next event
   this->waitForAndProcessNextEvent();

In this simple example, only one of two events could occur at this
point: a standard job completion or a standard job failure. As a result,
this execution controller overrides the two corresponding member
functions as follows:

.. code:: cpp

   void TwoTasksAtATimeWMS::processEventStandardJobCompletion(
                  std::shared_ptr<StandardJobCompletedEvent> event) {
     // Retrieve the job that this event is for 
     auto job = event->job;
     // Print some message for each task in the job
     for (auto const &task : job->getTasks()) {
       std::cerr  << "Notified that a standard job has completed task " << task->getID() << std::endl;
     }
   }

   void TwoTasksAtATimeWMS::processEventStandardJobFailure(
                  std::shared_ptr<StandardJobFailedEvent> event) {
     // Retrieve the job that this event is for 
     auto job = event->job;
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

You may note some difference between the above code and that in
``examples/workflow_api/basic-examples/bare-metal-bag-of-tasks/TwoTasksAtATimeWMS.cpp``.
This is for clarity purposes, and especially because we have not yet
explained how WRENCH does message logging. See :ref:`an upcoming section
about logging <wrench-102-controller-logging>`.

While the above callbacks are convenient, sometimes it is desirable to
do things more manually. That is, wait for an event and then process it
in the code of the main loop of the execution controller rather than in
a callback member function. This is done by calling the
:cpp:class:`wrench::waitForNextEvent()` member function. For instance, the
execution controller in
``examples/workflow_api/basic-examples/bare-metal-data-movement/DataMovementWMS.cpp``
does it as:

.. code:: cpp

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

.. _wrench-102-controller-exceptions:

Exceptions
==========

Most member functions in the WRENCH Developer API throw exceptions. In
fact, most of the code fragments above should be in try-catch clauses,
catching these exceptions.

Some exceptions correspond to failures during the simulated workflow
executions (i.e., errors that would occur in a real-world execution and
are thus part of the simulation). Each such exception contains a
:cpp:class:`wrench::FailureCause` object, which can be accessed to understand the
root cause of the execution failure. Other exceptions (e.g.,
``std::invalid_arguments``, ``std::runtime_error``) are thrown as well,
which are used for detecting misuses of the WRENCH API or internal
WRENCH errors.

.. _wrench-102-controller-hardware:

Finding information and interacting with hardware resources
===========================================================

The :cpp:class:`wrench::Simulation` class provides many member functions to
discover information about the (simulated) hardware platform and
interact with it. It also provides other useful information about the
simulation itself, such as the current simulation date. Some of these
member functions are static, but others are not. The
:cpp:class:`wrench::ExecutionController` class includes a ``simulation`` object.
Thus, the execution controller can call member functions on the
``this->simulation`` object. For instance, this fragment of code shows
how an execution controller can figure out the current simulated date
and then check that a host exists (given a hostname) and, if so, set its
``pstate`` (power state) to the highest possible setting.

.. code:: cpp

   auto now = wrench::Simulation::getCurrentSimulatedDate();
   if (wrench::Simulation::doesHostExist("SomeHost"))  {
     this->simulation->setPstate("SomeHost", wrench::Simulation::getNumberofPstates("SomeHost")-1);
   }

See the documentation of the :cpp:class:`wrench::Simulation` class for all
details. Specifically regarding host pstates, see the example execution
controller in
``examples/workflow_api/basic-examples/cloud-bag-of-tasks-energy/TwoTasksAtATimeCloudWMS.cpp``,
which interacts with host pstates (and the
``examples/workflow_api/basic-examples/cloud-bag-of-tasks-energy/four_hosts_energy.xml``
platform description file which defines pstates).

.. _wrench-102-controller-logging:

Logging
=======

It is typically desirable for the execution controller to print log
output to the terminal. This is easily accomplished using the
:cpp:class:`wrench::WRENCH_INFO()`, :cpp:class:`wrench::WRENCH_DEBUG()`, and
:cpp:class:`wrench::WRENCH_WARN()` macros, which are used just like C's
``printf()``. Each of these macros corresponds to a different logging
level in SimGrid. See the :ref:`SimGrid logging
documentation <https://simgrid.org/doc/latest/Outcomes.html>` for all
details.

Furthermore, one can change the color of the log messages with the
:cpp:class:`wrench::TerminalOutput::setThisProcessLoggingColor()` member
function, which takes as parameter a color specification:

-  :cpp:class:`wrench::TerminalOutput::COLOR_BLACK`
-  :cpp:class:`wrench::TerminalOutput::COLOR_RED`
-  :cpp:class:`wrench::TerminalOutput::COLOR_GREEN`
-  :cpp:class:`wrench::TerminalOutput::COLOR_YELLOW`
-  :cpp:class:`wrench::TerminalOutput::COLOR_BLUE`
-  :cpp:class:`wrench::TerminalOutput::COLOR_MAGENTA`
-  :cpp:class:`wrench::TerminalOutput::COLOR_CYAN`
-  :cpp:class:`wrench::TerminalOutput::COLOR_WHITE`

When inspecting the code of the execution controllers in the example
simulators you will find many examples of calls to
:cpp:class:`wrench::WRENCH_INFO()`. The logging is per ``.cpp`` file, each of
which corresponds to a declared logging category. For instance, in
``examples/workflow_api/basic-examples/batch-bag-of-tasks/TwoTasksAtATimeBatchWMS.cpp``,
you will find the typical pattern:

.. code:: cpp

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

The name of the logging category, in this case ``custom_wms``, can then
be passed to the ``--log`` command-line argument. For instance, invoking
the simulator with additional argument
``--log=custom_wms.threshold=info`` will make it so that only those
``WRENCH_INFO`` statements in ``TwoTasksAtATimeBatchWMS.cpp`` will be
printed (in green!).
