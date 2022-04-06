.. _wrench-101-header:

WRENCH 101
**********

This page provides high-level and detailed information about what WRENCH
simulators can simulate and how they do it. Full API details are
provided in the :ref:`User API Reference <user-api>`. See the
relevant pages for instructions on how to :ref:`install
WRENCH <install>` and how to :ref:`setup a simulator
project <getting-started>`.

.. _wrench-101-simulator-10000ft:

10,000-ft view of a WRENCH simulator
====================================

A WRENCH simulator can be as simple as a single ``main()`` function that
creates a platform to be simulated (the hardware) and a set of services
that run on the platform (the software). These services correspond to
software that knows how to store data, perform computation, and many
other useful things that real-world cyberinfrastructure services can do.

The simulator then creates a special (simulated) process called an
*execution controller*. An execution controller interacts with the
services running on the platform to execute some application workload of
interest, whatever that workflow is. The execution controller is
implemented using the :ref:`WRENCH Developer
API <developer-api>`, as discussed in the :ref:`WRENCH
102 <wrench-102-header>` page.

The simulation is then launched via a single call
(:cpp:class:`wrench::Simulation::launch()`), and returns only once the execution
controller has terminated (after completing or failing to complete
whatever it wanted to accomplish).

.. _wrench-101-simulator-1000ft:

1,000-ft view of a WRENCH simulator
===================================

In this section, we dive deeper into what it takes to implement a WRENCH
simulator. *To provide context, we refer to the example simulator in
the* ``examples/basic-examples/action_api/multi-action-multi-job``
*directory of the WRENCH distribution*. This simulator simulates the
execution of a few jobs, each of which consists of one or more actions,
on a 4-host platform that runs a couple of compute services and storage
services. Although other examples are available (see
``examples/README.md``), this simple example is sufficient to showcase
most of what a WRENCH simulator does, which consists in going through
the steps below. Note that all simulator codes in the ``examples``
directory contain extensive comments.

.. _wrench-101-simulator-1000ft-step-0:

Step 0: Include wrench.h
------------------------

For ease of use, all WRENCH abstractions in the :ref:`WRENCH User
API <user-api>` are available through a single header file:

.. code:: cpp

   #include <wrench.h>

.. _wrench-101-simulator-1000ft-step-1:

Step 1: Create and initialize a simulation
------------------------------------------

The state of a WRENCH simulation is defined by the
:cpp:class:`wrench::Simulation` class. A simulator must create an instance of
this class by calling :cpp:class:`wrench::Simulation::createSimulation()` and
initialize it with the :cpp:class:`wrench::Simulation::init()` member function.
The ``multi-action-multi-job`` simulator does this as follows:

.. code:: cpp

   auto simulation = wrench::Simulation::createSimulation();
       simulation->init(&argc, argv);

Note that this member function takes in the command-line arguments
passed to the main function of the simulator. This is so that it can
parse WRENCH-specific and
:ref:`SimGrid-specific <https://simgrid.org/doc/latest/Configuring_SimGrid.html>`
command-line arguments. (Recall that WRENCH is based on
:ref:`SimGrid <https://simgrid.org>`.) Two useful such arguments are
``--wrench-help``, which displays a WRENCH help message, and
``--help-simgrid``, which displays an extensive SimGrid help message.
Another one is ``--wrench-full-log``, which displays full simulation
logs (see below for more details).

.. _wrench-101-simulator-1000ft-step-2:

Step 2: Instantiate a simulated platform
----------------------------------------

This is done with the :cpp:class:`wrench::Simulation::instantiatePlatform()`
method. There are two versions of this method. The **first version**
takes as argument a :ref:`SimGrid virtual platform description
file <https://simgrid.org/doc/latest/Platform.html>`, we defines all
the simulated hardware (compute hosts, clusters of hosts, storage
resources, network links, routers, routes between hosts, etc.). The
bare-metal-chain simulator comes with a platform description file,
``examples/action_api/multi-action-multi-job/four_hosts.xml``, which we
include here:

.. code:: xml

   <?xml version='1.0'?>
   <!DOCTYPE platform SYSTEM "https://simgrid.org/simgrid.dtd">
   <platform version="4.1">
       <zone id="AS0" routing="Full">

           <!-- The host on which the Controller will run -->
           <host id="UserHost" speed="10Gf" core="1">
           </host>

           <!-- The host on which the bare-metal compute service will run and also run jobs-->
           <host id="ComputeHost1" speed="35Gf" core="10">
               <prop id="ram" value="16GB" />
           </host>

           <!-- Another host on which the bare-metal compute service will be able to run jobs -->
           <host id="ComputeHost2" speed="35Gf" core="10">
               <prop id="ram" value="16GB" />
           </host>

           <!-- The host on which the first storage service will run -->
           <host id="StorageHost1" speed="10Gf" core="1">
               <disk id="hard_drive" read_bw="100MBps" write_bw="100MBps">
                   <prop id="size" value="5000GiB"/>
                   <prop id="mount" value="/"/>
               </disk>
           </host>

           <!-- The host on which the second storage service will run -->
           <host id="StorageHost2" speed="10Gf" core="1">
               <disk id="hard_drive" read_bw="200MBps" write_bw="200MBps">
                   <prop id="size" value="5000GiB"/>
                   <prop id="mount" value="/"/>
               </disk>
           </host>

           <!-- The host on which the cloud compute service will run -->
           <host id="CloudHeadHost" speed="10Gf" core="1">
               <disk id="hard_drive" read_bw="100MBps" write_bw="100MBps">
                   <prop id="size" value="5000GiB"/>
                   <prop id="mount" value="/scratch/"/>
               </disk>
           </host>

           <!-- The host on which the cloud compute service will start VMs -->
           <host id="CloudHost" speed="25Gf" core="8">
               <prop id="ram" value="16GB" />
           </host>

           <!-- A network link shared by EVERY ONE-->
           <link id="network_link" bandwidth="50MBps" latency="1ms"/>

           <!-- The same network link connects all hosts together -->
           <route src="UserHost" dst="ComputeHost1"> <link_ctn id="network_link"/> </route>
           <route src="UserHost" dst="ComputeHost2"> <link_ctn id="network_link"/> </route>
           <route src="UserHost" dst="StorageHost1"> <link_ctn id="network_link"/> </route>
           <route src="UserHost" dst="StorageHost2"> <link_ctn id="network_link"/> </route>
           <route src="UserHost" dst="CloudHeadHost"> <link_ctn id="network_link"/> </route>
           <route src="ComputeHost1" dst="StorageHost1"> <link_ctn id="network_link"/> </route>
           <route src="ComputeHost2" dst="StorageHost2"> <link_ctn id="network_link"/> </route>
           <route src="CloudHeadHost" dst="CloudHost"> <link_ctn id="network_link"/> </route>
           <route src="StorageHost1" dst="CloudHost"> <link_ctn id="network_link"/> </route>
           <route src="StorageHost2" dst="CloudHost"> <link_ctn id="network_link"/> </route>

       </zone>
   </platform>

This file defines a platform with several hosts, each with some number
of cores and a core speed. Some hosts have a disk attached to them, some
declare a RAM capacity. The platform also declares a single network link
with a particular latency and bandwidth, and routes between some of the
hosts (over that one link). We refer the reader to platform description
files in other examples in the ``examples`` directory and to the
:ref:`SimGrid documentation <https://simgrid.org/doc/latest/Platform.html>`
for more information on how to create platform description files. There
are many possibilities for defining complex platforms at will. The
bare-metal-chain simulator takes the path to the platform description as
its 1st (and only) command-line argument and thus instantiates the
simulated platform as:

.. code:: cpp

   simulation.instantiatePlatform(argv[1]);

The **second version** of the
:cpp:class:`wrench::Simulation::instantiatePlatform()` method takes as input a
function that creates the platform description programmatically using
the :ref:`SimGrid platform description
API <https://simgrid.org/doc/latest/Platform_cpp.html>`. The example
in
``examples/workflow_api/basic-examples/bare-metal-bag-of-tasks-programmatic-platform``
shows how the XML platform description in
``examples/workflow_api/basic-examples/bare-metal-bag-of-tasks/two_hosts.xml``
can be implemented programmatically. (Note that this example passes a
functor to :cpp:class:`wrench::Simulation::instantiatePlatform()` rather than a
plain lambda.)

.. _wrench-101-simulator-1000ft-step-3:

Step 3: Instantiate services on the platform
--------------------------------------------

While the previous step defines the hardware platform, this step defines
what software services run on that hardware. The
:cpp:class:`wrench::Simulation::add()` member function is used to add services to
the simulation. Each class of service is created with a particular
constructor, which also specifies host(s) on which the service is to be
started. Typical kinds of services include compute services, storage
services, and file registry services (see
:ref:`below <wrench-101-simulator-services>` for more details).

The bare-metal-chain simulator instantiates four services. The first one
is a compute service:

.. code:: cpp

    auto baremetal_service = simulation->add(new wrench::BareMetalComputeService("ComputeHost1", {{"ComputeHost1"}, {"ComputeHost2"}}, "", {}, {}));

The :cpp:class:`wrench::BareMetalComputeService` class implements a simulation of
a compute service that greedily runs jobs submitted to it. You can think
of it as a compute server that simply fork-execs (possibly
multi-threaded) processes upon request, only ensuring that physical RAM
capacity is not exceeded. In this particular case, the compute service
is started on host ``ComputeHost1``. It has access to the compute
resources of that same host as well as that of a second host
``ComputeHost2`` (2nd argument is a list of available compute hosts).
The third argument corresponds to the path of some scratch storage,
i.e., storage in which data can be stored temporarily while a job runs.
In this case, the scratch storage specification is empty as host
``ComputeHost1`` has no disk attached to it. The last two arguments are
``std::map`` objects (in this case both empty), that are used to
configure properties of the service (see details in :ref:`this section
below <wrench-101-customizing-services>`).

The second service is a cloud compute service:

.. code:: cpp

   auto cloud_service = simulation->add(new wrench::CloudComputeService("CloudHeadHost", {"CloudHost"}, "/scratch/", {}, {}));

The :cpp:class:`wrench::CloudComputeService` implements a simulation of a cloud
platform on which virtual machine (VM) instances can be created,
started, used, and shutdown. The service runs on host ``CloudHeadHost``
and has access to the compute resources on host ``CloudHost``. Unlike
the previous service, this service has scratch space, at path ``/data``
on the disk attached to host ``CloudHost`` (as seen in the XML platform
description). Here again, the last two arguments are used to configure
properties of the service.

The third service is a storage service:

.. code:: cpp

   auto storage_service_1 = simulation->add(new wrench::SimpleStorageService("StorageHost1", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "50000000"}}, {}));

The :cpp:class:`wrench::SimpleStorageService` class implements a simulation of a
remotely-accessible storage service on which files can be stored,
copied, deleted, read, and written. In this particular case, the storage
service is started on host ``StorageHost1``. It uses storage mounted at
``/`` on that host (which corresponds to the mount path of a disk, as
seen in the XML platform description). The last two arguments, as for
the compute services, are used to configure particular properties of the
service. In this case, the service is configured to use a 50-MB buffer
size to pipeline network and disk accesses (see details in :ref:`this section
below <wrench-101-customizing-services>`).

The fourth service is a another storage service that runs on host
``StorageHost2``.

.. _wrench-101-simulator-1000ft-step-4:

Step 4: Instantiate at least one Execution controller
-----------------------------------------------------

At leave on *execution controller* must be created and added to the
simulation. This is a special service that is in charge of executing an
application workload on the platform. It is implemented as a class that
derives from :cpp:class:`wrench::ExecutionController` and override its
constructor as well as its ``main()`` method. This method is
implementing using the :ref:`WRENCH Developer
API <developer-api>`.

The example in ``examples/action_api/bare-metal-bag-of-actions`` does
this as follows: 

.. code:: cpp

   auto wms = simulation->add(new wrench::TwoTasksAtATimeExecutionController(num_tasks, baremetal_service, storage_service, "UserHost"));

This creates an execution controller and passes to its constructor a
number of tasks to execute, the compute service to use, the storage
service to use, and the host on which it is supposed to execute. Class
``wrench::TwoTasksAtATimeExecutionController`` is of course provided
with the example. See the :ref:`WRENCH 102 <wrench-102-header>` page for
information on how to implement an execution controller.

One important question is how to specify an *application workload* and
tell the execution controller to execute it. This is completely up to
the developer, and in this example the execution controller is simply
given a number of tasks and then creates files, file read actions, file
write actions, and compute actions to be executed as part of various
jobs (see the implementation of
``wrench::TwoTasksAtATimeExecutionController``). All the examples in the
``examples/action_api`` directory do this in different ways. *However*,
many users are interested in **workflow applications**, for this reason,
WRENCH provides a :cpp:class:`wrench::Workflow` class that has member functions
to manually create tasks and files and add them to a workflow. The use
of this class is shown in all the examples in directory
``examples/workflow_api``. The :cpp:class:`wrench::Workflow` class also provides
member functions to import workflows from workflow description files in
standard :ref:`JSON format <https://github.com/wfcommons/wfformat>`. Note
that an execution controller that executes a workflow is often called a
Workflow Management System (WMS). This is why many execution controllers
in the examples in directory ``examples/workflow_api`` have WMS in their
class names.

.. _wrench-101-simulator-1000ft-step-5:

Step 5: Launch the simulation
-----------------------------

This is the easiest step, and is done by simply calling
:cpp:class:`wrench::Simulation::launch()`:

.. code:: cpp

   simulation.launch();

This call checks the simulation setup and blocks until the execution
controller terminates.

.. _wrench-101-simulator-1000ft-step-6:

Step 6: Process simulation output
---------------------------------

The processing of simulation output is up to the user as different users
are interested in different output. For instance, the examples in
directory ``examples/action_api`` merely print some information to the
terminal. But this information could be collected in data structures,
output to files, etc. This said, WRENCH provides a
:cpp:class:`wrench::Simulation::getOutput()` member function that returns an
instance of class :cpp:class:`wrench::SimulationOutput`. Note that there are
member functions to configure the type and amount of output generated
(see the ``wrench::SimulationOutput::enable*Timestamps()`` member
functions). :cpp:class:`wrench::SimulationOutput` has a templated
:cpp:class:`wrench::SimulationOutput::getTrace()` member function to retrieve
traces for various information types. This is exemplified in several of
the example simulators in the ``examples/workflow_api`` directory. Note
that many of the timestamp types have to do with the execution of
workflow tasks, as defined using the :cpp:class:`wrench::Workflow` class.

Another kind of output is (simulated) energy consumption. WRENCH
leverages :ref:`SimGrid's energy
plugin <https://simgrid.org/doc/latest/Plugins.html#existing-plugins>`,
which provides accounting for computing time and dissipated energy in
the simulated platform. SimGrid's energy plugin requires host ``pstate``
definitions (levels of performance, CPU frequency) in the :ref:`XML platform
description file <https://simgrid.org/doc/latest/Platform.html>`. The
:cpp:class:`wrench::Simulation::getEnergyConsumed()` member function returns
energy consumed by all hosts in the platform. **Important:** The energy
plugin is NOT enabled by default in WRENCH simulations. To enable it,
pass the ``--wrench-energy-simulation`` command line option to the
simulator. See ``examples/basic-examples/cloud-bag-of-tasks-energy`` for
an example simulator that makes use of this plugin (and an example
platform description file that defines host power consumption profiles).

It is also possible to dump all simulation output to a JSON file. This
is done with the ``wrench::SimulationOutput::dump*JSON()`` member
functions. The documentation of each member function details the
structure of the JSON output, in case you want to parse/process the JSON
by hand. See the API documentation of the :cpp:class:`wrench::SimulationOutput`
class for all details.

Alternatively, you can run the installed ``wrench-dashboard`` tool,
which provides interactive visualization/inspection of the generated
JSON simulation output. You can run the dashboard for the JSON output
generated by the example simulators in
``examples/workflow_api/basic-examples/bare-metal-bag-of-task`` and
``examples/workflow_api/basic-examples/cloud-bag-of-task``. These
simulators produce a JSON file in ``/tmp/wrench.json``. Simply run the
command ``wrench-dashboard``, which pops up a Web browser window in
which you simply upload the ``/tmp/wrench.json`` file.

We find that most users end up doing their own, custom simulation output
generation since they are the ones who know what they are interested in.

.. _wrench-101-simulator-services:

Available services
==================

Below is the list of services available to-date in WRENCH. Click on the
corresponding links for more information on what these services are and
on how to create them.

-  **Compute Services**: These are services that know how to compute
   workflow tasks:

   -  :ref:`Bare-metal Servers <guide-101-baremetal>`
   -  :ref:`Cloud Platforms <guide-101-cloud>`
   -  :ref:`Virtualized Cluster
      Platforms <guide-101-virtualizedcluster>`
   -  :ref:`Batch-scheduled Clusters <guide-101-batch>`
   -  :ref:`HTCondor <guide-101-htcondor>`

-  **Storage Services**: These are services that know how to store and
   give access to workflow files:

   -  :ref:`Simple Storage Service <guide-101-simplestorage>`

-  **File Registry Services**: These services, also known as *replica
   catalogs*, are simply databases of ``<filename, list of locations>``
   key-values pairs of the storage services on which copies of files are
   available.

   -  :ref:`File Registry Service <guide-101-fileregistry>`

-  **Network Proximity Services**: These are services that monitor the
   network and maintain a database of host-to-host network distances:

   -  :ref:`Network Proximity Service <guide-101-networkproximity>`

-  **EnergyMeter Services**: These services are used to periodically
   measure host energy consumption and include these measurements in the
   simulation output:

   -  :ref:`Energy Meter Service <guide-101-energymeter>`

-  **BandwidthMeter Services**: These services are used to periodically
   measure network links' bandwidth usage and include these measurements
   in the simulation output:

   -  :ref:`Bandwidth Meter Service <guide-101-bandwidthmeter>`

.. _wrench-101-customizing-services:

Customizing services
====================

Each service is customizable by passing to its constructor a *property
list*, i.e., a key-value map where each key is a property and each value
is a string. Each service defines a property class. For instance, the
:cpp:class:`wrench::Service` class has an associated :cpp:class:`wrench::ServiceProperty`
class, the :cpp:class:`wrench::ComputeService` class has an associated
:cpp:class:`wrench::ComputeServiceProperty` class, and so on at all levels of the
service class hierarchy.

**The API documentation for these property classes explains what each
property means, what possible values are, and what default values are.**
Other properties have more to do with what the service can or should do
when in operation. For instance, the
:cpp:class:`wrench::BatchComputeServiceProperty` class defines a
:cpp:class:`wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM`
which specifies what scheduling algorithm a batch service should use for
prioritizing jobs. All property classes inherit from the
:cpp:class:`wrench::ServiceProperty` class, and one can explore that hierarchy to
discover all possible (and there are many) service customization
opportunities.

Finally, each service exchanges messages on the network with other
services (e.g., an execution controller sends a “do some work for me”
messages to compute services). The size in bytes, or payload, of all
messages can be customized similarly to the properties, i.e., by passing
a key-value map to the service's constructor. For instance, the
:cpp:class:`wrench::ServiceMessagePayload` class defines a
:cpp:class:`wrench::ServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD` property
which can be used to customize the size, in bytes, of the control
message sent to the service daemon (that is the entry point to the
service) to tell it to terminate. Each service class has a corresponding
message payload class, and the API documentation for these message
payload classes details all messages whose payload can be customized.

.. _wrench-101-logging:

Customizing logging
===================

When running a WRENCH simulator you may notice that there is no logging
output. By default logging output is disabled, but it is often useful to
enable it (remembering that it can slow down the simulation). WRENCH's
logging system is a thin layer on top of SimGrid's logging system, and
as such is controlled via command-line arguments.

The ``bare-metal-chain`` example simulator can be executed as follows in
the ``examples/action_api/bare-metal-bag-of-actions`` subdirectory of
the build directory (after typing ``make examples`` in the build
directory):

.. code:: sh

   ./wrench-example-bare-metal-bag-of-tasks 10 ./four_hosts.xml

The above generates almost no output to the terminal whatsoever. It is
possible to enable some logging to the terminal. It turns out the
execution controller class in that example
(``TwoTasksAtATimeExecutionController.cpp``) defines a logging category
named ``custom_execution_controller`` (see one of the first lines of
``examples/action_api/bare-metal-bag-of-actions/TwoActionsAtATimeExecutionController.cpp``),
which can be enabled as:

.. code:: cpp

   ./wrench-example-bare-metal-bag-of-tasks 10 ./four_hosts.xml --log=custom_execution_controller.threshold=info

You will now see some (green) logging output that is generated by the
execution controller implementation. It is typical to want to see these
messages as the controller is the brain of the application workload
execution.

One can disable the coloring of the logging output with the
``--wrench-no-color`` argument:

.. code:: cpp

   ./wrench-example-bare-metal-bag-of-tasks 10 ./four_hosts.xml --log=custom_execution_controller.threshold=info --wrench-no-color

Disabling color can be useful when redirecting the logging output to a
file.

Enabling all logging is done with the argument ``--wrench-full-log``:

.. code:: cpp

   ./wrench-example-bare-metal-bag-of-tasks 10 ./four_hosts.xml --wrench-full-log

The logging output now contains output produced by all the simulated
running processed. More details on logging capabilities are displayed
when passing the ``--help-logs`` command-line argument to your
simulator. Log category names are attached to ``*.cpp`` files in the
simulator code, the WRENCH code, and the SimGrid code. Using the
``--help-log-categories`` command-line argument shows the entire log
category hierarchy (which is huge).

See the :ref:`Simgrid logging
documentation <https://simgrid.org/doc/latest/Outcomes.html>` for all
details.
