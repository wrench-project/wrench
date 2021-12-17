WRENCH 101                        {#wrench-101}
============

This page provides high-level and detailed information about what WRENCH 
simulators can simulate and how they do it. Full API details are provided 
in the [User API Reference](./user/annotated.html). See the relevant pages 
for instructions on how to [install WRENCH](@ref install) and how to 
[setup a simulator project](@ref getting-started).

[TOC]


# 10,000-ft view of a WRENCH simulator #      {#wrench-101-simulator-10000ft}

A WRENCH simulator can be as simple as a single `main()` function that
creates a platform to be simulated (the hardware) and a set of
services that run on the platform (the software).  These services
correspond to software that knows how to store data, perform computation,
and many other useful things that real-world cyberinfrastructure services
can do.

The simulator then creates a special (simulated) process called an *execution controller*. An 
execution controller interacts with the services running on the platform to execute some application
workload of interest, whatever that workflow is. The execution controller is implemented using
the [WRENCH Developer API](./developer/annotated.html), as
discussed in the [WRENCH 102](@ref wrench-102) page. 

The simulation is then launched via a single call
(`wrench::Simulation::launch()`), and returns only once the execution controller has terminated 
(after completing or failing to complete whatever it wanted to accomplish).  


# 1,000-ft view of a WRENCH simulator #         {#wrench-101-simulator-1000ft}

In this section, we dive deeper into what it takes to implement a WRENCH 
simulator. _To provide context, we refer to the example simulator in the_
`examples/basic-examples/action_api/multi-action-multi-job` _directory of the WRENCH 
distribution_. This simulator simulates the execution of a few jobs, each of which consists of
one or more actions, on a 4-host platform that runs a couple of compute services and storage
services. Although other examples are available (see `examples/README.md`), 
this simple example is sufficient to showcase most of what a WRENCH simulator 
does, which consists in going through the steps below. Note that all simulator
codes in the `examples` directory contain extensive comments. 

## Step 0: Include wrench.h #                   {#wrench-101-simulator-1000ft-step-0}

For ease of use, all WRENCH abstractions in the 
[WRENCH User API](./user/annotated.html) are available through a single 
header file:

~~~~~~~~~~~~~{.cpp}
#include <wrench.h>
~~~~~~~~~~~~~

## Step 1: Create and initialize a simulation #   {#wrench-101-simulator-1000ft-step-1}

The state of a WRENCH simulation is defined by the `wrench::Simulation` 
class. A simulator must create an instance of this class by calling 
`wrench::Simulation::createSimulation()` and initialize it
with the `wrench::Simulation::init()` member function.  The `multi-action-multi-job`
simulator does this as follows:

~~~~~~~~~~~~~{.cpp}
auto simulation = wrench::Simulation::createSimulation();
    simulation->init(&argc, argv);
~~~~~~~~~~~~~


Note that this member function takes in the command-line arguments passed to the main
function of the simulator. This is so that it can parse WRENCH-specific and
[SimGrid-specific](https://simgrid.org/doc/latest/Configuring_SimGrid.html)
command-line arguments. (Recall that WRENCH is based on
[SimGrid](https://simgrid.org).) Two useful such arguments are `--wrench-help`,
which displays a WRENCH help message, and `--help-simgrid`, which displays
an extensive SimGrid help message. Another one is `--wrench-full-log`, which displays full simulation logs (see below for more details).

## Step 2: Instantiate a simulated platform #     {#wrench-101-simulator-1000ft-step-2}

This is done with the `wrench::Simulation::instantiatePlatform()` method. There are two
versions of this method. The **first version** takes as argument a [SimGrid virtual platform description
file](https://simgrid.org/doc/latest/platform.html), we defines all the 
simulated hardware (compute hosts, clusters 
of hosts, storage resources, network links, routers, routes between 
hosts, etc.). The bare-metal-chain simulator comes with a platform 
description file, `examples/action_api/multi-action-multi-job/four_hosts.xml`, 
which we include here:

~~~~~~~~~~~~~{.xml}
<?xml version='1.0'?>
<!DOCTYPE platform SYSTEM "http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd">
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
~~~~~~~~~~~~~

This file defines a platform with several hosts, each with some number of cores and a core speed. Some hosts have
a disk attached to them, some declare a RAM capacity.  The platform also declares a single network link with a particular
latency and bandwidth, and routes between some of the hosts (over that one link).
We refer the reader 
to platform description files in other examples in the  `examples` directory 
and to the [SimGrid documentation](https://simgrid.org/doc/latest/platform.html) 
for more information on how to create platform description files.  There are many possibilities
for defining complex platforms at will. 
The bare-metal-chain simulator takes the path to the platform description as 
its 1st (and only) command-line argument and thus instantiates the simulated platform as:

~~~~~~~~~~~~~{.cpp}
simulation.instantiatePlatform(argv[1]);
~~~~~~~~~~~~~

The **second version** of the `wrench::Simulation::instantiatePlatform()` method takes as input 
a function that creates the platform description programmatically using the 
[SimGrid platform description API](https://simgrid.org/doc/latest/Platform_cpp.html). The 
example in `examples/workflow_api/basic-examples/bare-metal-bag-of-tasks-programmatic-platform` shows how
the XML platform description in `examples/workflow_api/basic-examples/bare-metal-bag-of-tasks/two_hosts.xml` can
be implemented programmatically. (Note that this example passes a functor to `wrench::Simulation::instantiatePlatform()`
rather than a plain lambda.)

XXX BARF XXX

## Step 3: Instantiate services on the platform #    {#wrench-101-simulator-1000ft-step-3}

While the previous step defines the hardware platform, this step defines
what software services run on that hardware. 
The `wrench::Simulation::add()` member function is used
to add services to the simulation. Each class of service is created with a
particular constructor, which also specifies host(s) on which the service
is to be started. Typical kinds of services include compute services,
storage services, and file registry services  (see 
[below](@ref wrench-101-simulator-services) for more details).

The bare-metal-chain simulator instantiates four services. The first one
is a compute service:

~~~~~~~~~~~~~{.cpp}
 auto baremetal_service = simulation->add(new wrench::BareMetalComputeService("ComputeHost1", {{"ComputeHost1"}, {"ComputeHost2"}}, "", {}, {}));
 ~~~~~~~~~~~~~

The `wrench::BareMetalComputeService` class implements a simulation of a
compute service that greedily runs jobs submitted to it. You can think of
it as a compute server that simply fork-execs (possibly multi-threaded)
processes upon  request, only ensuring that physical RAM capacity is not
exceeded. In this particular case, the compute service is started on host 
`ComputeHost1`. It has access to the compute resources of that same 
host  as well as that of a second host `ComputeHost2` (2nd argument is a list of available
compute hosts). The third argument corresponds to the path of some 
scratch storage, i.e., storage in which data can be stored temporarily while 
a job runs. 
In this case, the scratch storage specification is empty as host `ComputeHost1` 
has no disk attached to it.
The last two
arguments are `std::map` objects (in this case both empty), that are used 
to configure properties of the service (see details in 
[this section below](@ref wrench-101-customizing-services)).

The second service is a cloud compute service:

~~~~~~~~~~~~~{.cpp}
auto cloud_service = simulation->add(new wrench::CloudComputeService("CloudHeadHost", {"CloudHost"}, "/scratch/", {}, {}));
~~~~~~~~~~~~~

The `wrench::CloudComputeService` implements a simulation of a cloud platform on which virtual machine (VM) instances
can be created, started, used, and shutdown. The service runs on host `CloudHeadHost` and has access to the 
compute resources on host `CloudHost`. Unlike the previous service, this service has scratch space,
at path `/data` on the disk attached to host `CloudHost` (as seen in the XML platfom description).
Here again, the last two arguments are used
to configure properties of the service.

The third service is a storage service:

~~~~~~~~~~~~~{.cpp}
auto storage_service_1 = simulation->add(new wrench::SimpleStorageService("StorageHost1", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "50000000"}}, {}));
~~~~~~~~~~~~~

The `wrench::SimpleStorageService` class implements a  simulation of a
remotely-accessible storage service on which files can be stored, copied,
deleted, read, and written. In this particular case, the storage service
is  started on host `StorageHost1`. It uses storage mounted at `/` on that host (which corresponds to the mount path of a disk, as
seen in the XML platform description).  The
last two arguments, as for the compute services, are used to configure
particular properties of the service. In this case, the service is
configured to use a 50-MB buffer size to pipeline network and disk accesses
(see details in [this section below](@ref wrench-101-customizing-services)).

The fourth service is a another storage service that runs on host `StorageHost2`.

[comment]: <> (~~~~~~~~~~~~~{.cpp})

[comment]: <> (auto file_registry_service = new wrench::FileRegistryService&#40;"WMSHost"&#41;; )

[comment]: <> (simulation.add&#40;file_registry_service&#41;; )

[comment]: <> (~~~~~~~~~~~~~)

[comment]: <> (The `wrench::FileRegistryService`  class implements a simulation of a)

[comment]: <> (key-values pair  service that stores for each file &#40;the key&#41;  the locations)

[comment]: <> (where  the file is  available for read/write access &#40;the values&#41;. This service)

[comment]: <> (can be used by a WMS to find out where  workflow files)

[comment]: <> (are located &#40;and is often required - see Step #4 hereafter&#41;. )


XXX BARF XXXX

## Step 4: Create at least one workflow #     {#wrench-101-simulator-1000ft-step-4}

Every WRENCH simulator simulates the execution of a workflow, and thus
must create an instance of the `wrench::Workflow` class. This class has
member functions to manually create tasks and files and add them to the workflow.
For instance, the bare-metal-chain simulator does this as follows:

~~~~~~~~~~~~~{.cpp}
wrench::Workflow workflow;

/* Add workflow tasks */
for (int i=0; i < num_tasks; i++) {
  /* Create a task1: 10GFlop, 1 to 10 cores, 10MB memory footprint */
  auto task1 = workflow.addTask("task_" + std::to_string(i), 10000000000.0, 1, 10, 10000000);
}

/* Add workflow files */
for (int i=0; i < num_tasks+1; i++) {
  /* Create a 100MB file */
  workflow.addFile("file_" + std::to_string(i), 100000000);
}

/* Set input/output files for each task1 */
for (int i=0; i < num_tasks; i++) {
  auto task1 = workflow.getTaskByID("task_" + std::to_string(i));
  task1->addInputFile(workflow.getFileByID("file_" + std::to_string(i)));
  task1->addOutputFile(workflow.getFileByID("file_" + std::to_string(i + 1)));
}
~~~~~~~~~~~~~

The above creates a "chain" workflow (hence the name of the simulator), in which the
output from one task1 is input to the next task1. The number of tasks is obtained 
from a command-line argument. In the above code, each task1 has 100% parallel efficiency
(e.g., will run 10 times faster when running on 10 cores than when running on 1 core). It is
possible to customize the parallel efficiency behavior of a task1, as demonstrated
in `examples/basic-examples/bare-metal-multicore-tasks` for an example
simulator in which tasks with different parallel efficiency models are created and executed. 

The `wrench::Workflow` class also provides member functions to import workflows from
workflow description files in standard [JSON format](https://github.com/workflowhub/workflow-schema) and 
[DAX format](http://workflowarchive.org).

The input files to the workflow must be available (at some storage service) before
the simulated workflow execution begins.  These are the files that are input 
to some tasks, but not output from any task1. They must be "staged" on some
storage service, and the bare-metal-chain simulator does it as:

~~~~~~~~~~~~~{.cpp}
for (auto const &f : workflow.getInputFiles()) {
  simulation.stageFile(f, storage_service);
}
~~~~~~~~~~~~~

Note that in this particular case there is a single input file. But the code
above is more general, as it iterates over all workflow input files. The above
code will throw an exception if no `wrench::FileRegistryService` instance
has been added to the simulation. 

## Step 5: Instantiate at least one WMS per workflow #     {#wrench-101-simulator-1000ft-step-5}

One special service that must be started is a Workflow Management System (WMS)
service, i.e., software that is in charge of executing the workflow given
available software and hardware resources. The bare-metal-chain simulator does 
this as:

~~~~~~~~~~~~~{.cpp}
auto wms = simulation.add(new wrench::OneTaskAtATimeWMS({baremetal_service}, {storage_service}, "WMSHost"));
~~~~~~~~~~~~~

Class `wrench::OneTaskAtATimeWMS`, which is part of this example simulator,
is implemented using the [WRENCH Developer
API](./developer/annotated.html). See the [WRENCH 102](@ref wrench-102)
page for information on how to implement a WMS with WRENCH. The code above
passes the list of compute services (1st argument) and the list
of storage services (2nd argument) to the WMS constructor. The 3rd
argument specifies that the WMS should run on host `WMSHost`. 

The previously created workflow is then associated to the WMS:

~~~~~~~~~~~~~{.cpp}
wms->addWorkflow(&workflow);
~~~~~~~~~~~~~

## Step 6: Launch the simulation #           {#wrench-101-simulator-1000ft-step-6}

This is the easiest step, and is done by simply calling 
`wrench::Simulation::launch()`:

~~~~~~~~~~~~~{.cpp}
simulation.launch();
~~~~~~~~~~~~~

This call checks the simulation setup, and blocks until the WMS terminates. 

## Step 7: Process simulation output #       {#wrench-101-simulator-1000ft-step-7}

Once `wrench::Simulation::launch()` has returned, simulation output can be 
processed  programmatically. The `wrench::Simulation::getOutput()` member function 
returns an instance of class `wrench::SimulationOutput`.
Note that there are member functions to configure the type and amount of output generated 
(see the `wrench::SimulationOutput::enable*Timestamps()` member functions).
The bare-metal-chain simulator does minimal output processing as:

~~~~~~~~~~~~~{.cpp}
auto trace = simulation.getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>();
for (auto const &item : trace) {
   std::cerr << "Task "  << item->getContent()->getTask()->getID() << " completed at time " << item->getDate()  << std::endl;
~~~~~~~~~~~~~

Specifically, class `wrench::SimulationOutput` has a templated
`wrench::SimulationOutput::getTrace()` member function to retrieve traces for
various information types. The first line of code above returns a 
`std::vector` of time-stamped task1 completion events.  
The second line of code iterates through this vector and prints task1 
names and task1 completion dates (in seconds). The classes that 
implement time-stamped events are all classes named 
`wrench::SimulationTimestampSomething`, where "_Something_" is
self-explanatory (e.g., TaskCompletion, TaskFailure).

Another kind of output is (simulated) energy consumption. WRENCH leverages 
[SimGrid's energy plugin](https://simgrid.org/doc/latest/Plugins.html#existing-plugins), 
which provides accounting for computing time and dissipated energy in 
the simulated platform. SimGrid's energy plugin requires host `pstate` 
definitions (levels of performance, CPU frequency) in the
[XML platform description file](https://simgrid.org/doc/latest/platform.html). 
The `wrench::Simulation::getEnergyConsumed()` member function returns energy consumed 
by all hosts in the platform.  **Important:** The energy plugin is NOT
enabled by default in WRENCH simulations. To enable it, pass the
`--wrench-energy-simulation` command line option to the simulator.  See
`examples/basic-examples/cloud-bag-of-tasks-energy` for an example
simulator that makes use of this plugin  (and an example platform
description file that defines host power consumption profiles).

It is also possible to dump all simulation output to a JSON file.
This is done with the `wrench::SimulationOutput::dump*JSON()`
member functions. The documentation of each member function details the structure of the
JSON output, in case you want to parse/process the JSON by hand. See the API
documentation of the `wrench::SimulationOutput`  class for all details. 

Alternatively, you can run the installed `wrench-dashboard` tool, which
provides interactive visualization/inspection of the generated JSON simulation output.
You can run the dashboard for the  JSON output generated by the example simulators
in `examples/basic-examples/bare-metal-bag-of-task1` and
`examples/basic-examples/cloud-bag-of-task1`. These simulators produce a JSON file
in `/tmp/wrench.json`. Simply run the command `wrench-dashboard`, which pops up a Web
browser window in which you simply upload the `/tmp/wrench.json` file. 

# Available services #      {#wrench-101-simulator-services}

Below is the list of services available to-date in WRENCH. Click on the corresponding
links for more information on what these services are and on how to create them.

- **Compute Services**: These are services that know how to compute workflow tasks: 

  - [Bare-metal Servers](@ref guide-101-baremetal)
  - [Cloud Platforms](@ref guide-101-cloud)
  - [Virtualized Cluster Platforms](@ref guide-101-virtualizedcluster)
  - [Batch-scheduled Clusters](@ref guide-101-batch)
  - [HTCondor](@ref guide-101-htcondor)

- **Storage Services**: These are services that know how to store and give access to workflow files:

  - [Simple Storage Service](@ref guide-101-simplestorage)

- **File Registry Services**: These services, also known as _replica catalogs_, 
  are simply databases of `<filename, list of locations>` key-values pairs of 
  the storage services on which copies of files are available. 

   - [File Registry Service](@ref guide-101-fileregistry)

- **Network Proximity Services**: These are services that monitor the network 
  and maintain a database of host-to-host network distances: 

   - [Network Proximity Service](@ref guide-101-networkproximity)

- **EnergyMeter Services**: These services are used to periodically measure host
  energy consumption and include these measurements in the simulation output  
  (see [this section](@ref wrench-101-simulator-1000ft-step-7)).

   - [Energy Meter Service](@ref guide-101-energymeter)

- **Workflow Management Systems (WMSs)** (derives `wrench::WMS`): 
  A WMS provides the mechanisms for executing workflow applications, include 
  decision-making for optimizing various objectives (often attempting to 
  minimize workflow execution time).  
  At least **one** WMS should be provided for running a simulation.  By
  default, WRENCH does **not** provide a WMS implementation as part of its
  core components.  Each example simulator in the `examples/`
  directory implements its own WMS.  Additional WMS implementations may
  also be found on the [WRENCH project website](https://wrench-project.org/usages.html).
  See [WRENCH 102](@ref wrench-102) for information on how to implement a WMS.

# Customizing services #         {#wrench-101-customizing-services}

Each service is customizable by passing to its constructor a _property list_, 
i.e., a key-value map where each key is a property and each value is a string. 
Each service defines a property class. For instance, the `wrench::Service` 
class has an associated `wrench::ServiceProperty` class, the 
`wrench::ComputeService` class has an associated `wrench::ComputeServiceProperty` 
class, and so on at all levels of the service class hierarchy. 

**The API documentation for these property classes explains what each property 
means, what possible values are, and what default values are.** Other properties 
have more to do with what the service can or should do when in operation. 
For instance, the `wrench::BatchComputeServiceProperty` class defines a
`wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM` which specifies 
what scheduling algorithm a batch service should use for prioritizing jobs. All 
property classes inherit from the `wrench::ServiceProperty` class, and one can 
explore that hierarchy to discover all possible (and there are many) service 
customization opportunities. 

Finally, each service exchanges messages on the network with other services
(e.g., a WMS service sends a "do some work" message to a compute service).
The size in bytes, or payload, of all messages can be customized similarly
to the properties, i.e., by passing a key-value map to the service's
constructor. For instance, the `wrench::ServiceMessagePayload` class
defines a `wrench::ServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD`
property which can be used to customize the size, in bytes, of the control
message sent to the service daemon (that is the entry point to the service)
to tell it to terminate.  Each service class has a corresponding message
payload class, and the API documentation for these message payload classes
details all messages whose payload can be customized.

# Customizing logging  #        {#wrench-101-logging}

When running a WRENCH simulator you may notice that there is no
logging output. By default logging output is disabled, but it is often
useful to enable it (remembering that it can slow down the simulation). WRENCH's logging system 
is a thin layer on top of SimGrid's logging system, and as such is controlled 
via command-line arguments. 

The `bare-metal-chain` example simulator can be executed as follows in the
`examples/basic-examples/bare-metal-chain` directory:

~~~~~~~~~~~~~{.sh}
./wrench-example-bare-metal-chain 10 ./two_hosts.xml
~~~~~~~~~~~~~

The above generates no output whatsoever. 
It is possible to enable some logging to the terminal. 
It turns out the WMS class in that example (`OneTaskAtATimeWMS.cpp`) defines a logging
category named  `custom_wms`
(see one of the first lines of `examples/basic-examples/bare-metal-chain/OneTaskAtATimeWMS.cpp`),
which can be enabled as:

~~~~~~~~~~~~~{.cpp}
./wrench-example-bare-metal-chain 10 ./two_hosts.xml --log=custom_wms.threshold=info
~~~~~~~~~~~~~

You will now see some (green) logging output that is generated by  the WMS implementation. 
It is typical to want
to see these messages as the WMS is the brain of the workflow execution. 
They can be enabled while other messages are disabled as follows: 


One can disable the coloring of the logging output with the
`--wrench-no-color` argument:

~~~~~~~~~~~~~{.cpp}
./wrench-example-bare-metal-chain 10 ./two_hosts.xml --log=custom_wms.threshold=info --wrench-no-color
~~~~~~~~~~~~~

Disabling color can be useful when redirecting the logging  output to a file. 

Enabling all logging is done with the argument `--wrench-full-log`:

~~~~~~~~~~~~~{.cpp}
./wrench-example-bare-metal-chain 10 ./two_hosts.xml --wrench-full-log
~~~~~~~~~~~~~

The logging output now contains output produced by all the simulated
running processed.  More details on logging capabilities are displayed when
passing the `--help-logs` command-line argument to your simulator.  Log
category names are attached to `*.cpp` files in the simulator code, the
WRENCH code, and the SimGrid code. Using the `--help-log-categories`
command-line argument shows the entire log category hierarchy (which is huge).  


See the [Simgrid logging
documentation](https://simgrid.org/doc/latest/outcomes.html) for all
details.
