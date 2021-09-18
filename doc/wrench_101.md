WRENCH 101                        {#wrench-101}
============

This page provides high-level and detailed information about what WRENCH 
simulators can simulate and how they do it. Full API details are provided 
in the [User API Reference](./user/annotated.html). See the relevant pages 
for instructions on how to [install WRENCH](@ref install) and how to 
[setup a simulator project](@ref getting-started).

[TOC]

---

# 10,000-ft view of a WRENCH simulator #      {#wrench-101-simulator-10000ft}

A WRENCH simulator can be as simple as a single `main()` function that
creates a platform to be simulated (the hardware) and a set of
services that run on the platform (the software).  These services
correspond to software that knows how to store data, perform computation,
and many other useful things that real-world cyberinfrastructure services
can do.

The simulator then creates a workflow to be executed, which consists of 
a set of compute tasks each with input and output files, with control and/or
data dependencies between tasks.

A special service is then created, called a Workflow Management System
(WMS),  that will be in charge of executing the workflow on the platform using
available hardware resources  and software services.  The WMS
is implemented using the [WRENCH Developer API](./developer/annotated.html), as
discussed in the [WRENCH 102](@ref wrench-102) page. 

The simulation is then launched via a single call
(`wrench::Simulation::launch()`), and returns only once the WMS has terminated 
(after completing or failing to complete the execution of the workflow).
Simulation output can be analyzed programmatically and/or dumped to a JSON file. 
This JSON file can be loaded into the *WRENCH dashboard* tool (just run the 
`wrench-dashboard` executable, which should have been installed on your system).

# 1,000-ft view of a WRENCH simulator #         {#wrench-101-simulator-1000ft}

In this section, we dive deeper into what it takes to implement a WRENCH 
simulator. _To provide context, we refer to the example simulator in the_
`examples/basic-examples/bare-metal-chain` _directory of the WRENCH 
distribution_. This simulator simulates the execution of a chain workflow
on a two-host platform that runs one compute service and one storage
service. Although other examples are available (see `examples/README.md`), 
this simple example is sufficient to showcase most of what a WRENCH simulator 
does, which consists in going through the steps below. Note that the simulator's 
code contains extensive comments as well. 

## Step 0: Include wrench.h #                   {#wrench-101-simulator-1000ft-step-0}

For ease of use, all WRENCH abstractions in the 
[WRENCH User API](./user/annotated.html) are available through a single 
header file:

~~~~~~~~~~~~~{.cpp}
#include <wrench.h>
~~~~~~~~~~~~~

## Step 1: Create and initialize a simulation #   {#wrench-101-simulator-1000ft-step-1}

The state of a WRENCH simulation is defined by the `wrench::Simulation` 
class. A simulator must create an instance of this class and initialize it
with the `wrench::Simulation::init()` member function.  The bare-metal-chain
simulator does this as follows:

~~~~~~~~~~~~~{.cpp}
wrench::Simulation simulation;
simulation.init(&argc, argv);
~~~~~~~~~~~~~

Note that this member function takes in the command-line arguments passed to the main
function of the simulator. This is so that it can parse WRENCH-specific and
[SimGrid-specific](https://simgrid.org/doc/latest/Configuring_SimGrid.html)
command-line arguments. (Recall that WRENCH is based on
[SimGrid](https://simgrid.org).) Two useful such arguments are `--wrench-help`,
which displays a WRENCH help message, and `--help-simgrid`, which displays
an extensive SimGrid help message.

## Step 2: Instantiate a simulated platform #     {#wrench-101-simulator-1000ft-step-2}

This is done with the `wrench::Simulation::instantiatePlatform()` method. There are two
versions of this method. The **first version** takes as argument a [SimGrid virtual platform description
file](https://simgrid.org/doc/latest/platform.html), we defines all the 
simulated hardware (compute hosts, clusters 
of hosts, storage resources, network links, routers, routes between 
hosts, etc.). The bare-metal-chain simulator comes with a platform 
description file, `examples/basic-examples/bare-metal-bag-of-tasks/two_hosts.xml`, 
which we include here:

~~~~~~~~~~~~~{.xml}
<?xml version='1.0'?>
<!DOCTYPE platform SYSTEM "http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd">
<platform version="4.1">
    <zone id="AS0" routing="Full">

        <!-- The host on which the WMS will run -->
        <host id="WMSHost" speed="10Gf" core="1">
            <disk id="hard_drive" read_bw="100MBps" write_bw="100MBps">
                <prop id="size" value="5000GiB"/>
                <prop id="mount" value="/"/>
            </disk>
        </host>

        <!-- The host on which the BareMetalComputeService will run -->
        <host id="ComputeHost" speed="1Gf" core="10">
            <prop id="ram" value="16GB" />
       </host>

        <!-- A network link that connects both hosts -->
        <link id="network_link" bandwidth="50MBps" latency="20us"/>
        <!-- WMSHost's local "loopback" link -->
        <link id="loopback_WMSHost" bandwidth="1000EBps" latency="0us"/>
        <!--ComputeHost's local "loopback" link -->
        <link id="loopback_ComputeHost" bandwidth="1000EBps" latency="0us"/>

        <!-- Network routes -->
        <route src="WMSHost" dst="ComputeHost">
            <link_ctn id="network_link"/>
        </route>

        <!-- Each loopback link connects each host to itself -->
        <route src="WMSHost" dst="WMSHost">
            <link_ctn id="loopback_WMSHost"/>
        </route>
        <route src="ComputeHost" dst="ComputeHost">
            <link_ctn id="loopback_ComputeHost"/>
        </route>

    </zone>
</platform>
~~~~~~~~~~~~~

This file defines a platform with two hosts,  `WMSHost` and `ComputeHost`. 
The former is a 1-core host with compute speed 10 Gflop/sec, with a 5000-GiB 
disk with 100 MB/sec read and write bandwidth, which is mounted at `/`. 
The latter is a 10-core host where each core computes at speed 1Gflop/sec and 
with a total RAM capacity of 16 GB.  The platform also declares three network links.
The first one, called `network_link` is an actual network link to interconnect the two
hosts, with 50 MB/sec bandwidth and 20 microsecond latency.
The other two links (`loopback_WMSHost` and `loopback_ComputeHost`) are used to model
inter-process communication (IPC) performance within each host.   Last, network routes are declared.
The route from host `WMSHost` and `ComputeHost` is through `network_link`. Then, there
is a route from each host to itself using each loopback link. Note that these loopback routes
are optional. By default, SimGrid includes a loopback route for each host, with bandwidths and
latencies based on measurements obtained on actual computers. The above XML file  does not
use these defaults, and instead declare loop routes through much faster loopback links (zero latency
and extremely high bandwidth). This is because, for this simulation, we want to model a platform
in  which IPC on a host is essentially free. 

We refer the reader 
to platform description files in other examples in the  `examples` directory 
and to the [SimGrid documentation](https://simgrid.org/doc/latest/platform.html) 
for more information on how to create platform description files. 

The bare-metal-chain simulator takes the path to the platform description as 
its 2nd command-line argument and thus instantiates the simulated platform as:

~~~~~~~~~~~~~{.cpp}
simulation.instantiatePlatform(argv[2]);
~~~~~~~~~~~~~

The **second version** of the `wrench::Simulation::instantiatePlatform()` method takes as input 
a function that creates the platform description programmatically using the 
[SimGrid platform description API](https://simgrid.org/doc/latest/Platform_cpp.html). The 
example in `examples/basic-examples/bare-metal-bag-of-tasks-programmatic-platform` shows how
the XML platform description in `examples/basic-examples/bare-metal-bag-of-tasks/two_hosts.xml` can
be implemented programmatically. (Note that this example passes a functor to `wrench::Simulation::instantiatePlatform()`
rather than a plain method.)


## Step 3: Instantiate services on the platform #    {#wrench-101-simulator-1000ft-step-3}

While the previous step defines the hardware platform, this step defines
what software services run on that hardware. 
The `wrench::Simulation::add()` member function is used
to add services to the simulation. Each class of service is created with a
particular constructor, which also specifies host(s) on which the service
is to be started. Typical kinds of services include compute services,
storage services, and file registry services  (see 
[below](@ref wrench-101-simulator-services) for more details).

The bare-metal-chain simulator instantiates three services. The first one
is a compute service:

~~~~~~~~~~~~~{.cpp}
auto bare_metal_service = simulation.add(new wrench::BareMetalComputeService("ComputeHost", {"ComputeHost"}, "", {}, {}));
~~~~~~~~~~~~~

The `wrench::BareMetalComputeService` class implements a simulation of a
compute service that greedily runs jobs submitted to it. You can think of
it as a compute server that simply fork-execs (possibly multi-threaded)
processes upon  request, only ensuring that physical RAM capacity is not
exceeded. In this particular case, the compute service is started on host 
`ComputeHost`. It has access to the compute resources of that same 
host (2nd argument). The third argument corresponds to the path of some 
scratch storage, i.e., storage in which data can be stored temporarily while 
a job runs. 
In this case, the scratch storage specification is empty as host `ComputeHost` 
has no disk attached to it.
(See the `examples/basic-examples/bare-metal-chain-scratch` example
simulator, in which scratch storage is used). The last two
arguments are `std::map` objects (in this case both empty), that are used 
to configure properties of the compute service (see details in 
[this section below](@ref wrench-101-customizing-services)).

The second service is a storage service:

~~~~~~~~~~~~~{.cpp}
auto storage_service = simulation.add(new wrench::SimpleStorageService( "WMSHost", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "50000000"}}, {}));
~~~~~~~~~~~~~

The `wrench::SimpleStorageService` class implements a  simulation of a
remotely-accessible storage service on which files can be stored, copied,
deleted, read, and written. In this particular case, the storage service
is  started on host `WMSHost`. It uses storage mounted at `/`.  The
last two arguments, as for the compute service, are used to configure
particular properties of the service. In this case, the service is
configured to use a 50-MB buffer size to pipeline network and disk accesses
(see details in [this section below](@ref wrench-101-customizing-services)).

The third service is a file registry service:

~~~~~~~~~~~~~{.cpp}
auto file_registry_service = new wrench::FileRegistryService("WMSHost"); 
simulation.add(file_registry_service); 
~~~~~~~~~~~~~

The `wrench::FileRegistryService`  class implements a simulation of a
key-values pair  service that stores for each file (the key)  the locations
where  the file is  available for read/write access (the values). This service
can be used by a WMS to find out where  workflow files
are located (and is often required - see Step #4 hereafter). 

## Step 4: Create at least one workflow #     {#wrench-101-simulator-1000ft-step-4}

Every WRENCH simulator simulates the execution of a workflow, and thus
must create an instance of the `wrench::Workflow` class. This class has
member functions to manually create tasks and files and add them to the workflow.
For instance, the bare-metal-chain simulator does this as follows:

~~~~~~~~~~~~~{.cpp}
wrench::Workflow workflow;

/* Add workflow tasks */
for (int i=0; i < num_tasks; i++) {
  /* Create a task: 10GFlop, 1 to 10 cores, 10MB memory footprint */
  auto task = workflow.addTask("task_" + std::to_string(i), 10000000000.0, 1, 10, 10000000);
}

/* Add workflow files */
for (int i=0; i < num_tasks+1; i++) {
  /* Create a 100MB file */
  workflow.addFile("file_" + std::to_string(i), 100000000);
}

/* Set input/output files for each task */
for (int i=0; i < num_tasks; i++) {
  auto task = workflow.getTaskByID("task_" + std::to_string(i));
  task->addInputFile(workflow.getFileByID("file_" + std::to_string(i)));
  task->addOutputFile(workflow.getFileByID("file_" + std::to_string(i + 1)));
}
~~~~~~~~~~~~~

The above creates a "chain" workflow (hence the name of the simulator), in which the
output from one task is input to the next task. The number of tasks is obtained 
from a command-line argument. In the above code, each task has 100% parallel efficiency
(e.g., will run 10 times faster when running on 10 cores than when running on 1 core). It is
possible to customize the parallel efficiency behavior of a task, as demonstrated
in `examples/basic-examples/bare-metal-multicore-tasks` for an example
simulator in which tasks with different parallel efficiency models are created and executed. 

The `wrench::Workflow` class also provides member functions to import workflows from
workflow description files in standard [JSON format](https://github.com/workflowhub/workflow-schema) and 
[DAX format](http://workflowarchive.org).

The input files to the workflow must be available (at some storage service) before
the simulated workflow execution begins.  These are the files that are input 
to some tasks, but not output from any task. They must be "staged" on some
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
`std::vector` of time-stamped task completion events.  
The second line of code iterates through this vector and prints task 
names and task completion dates (in seconds). The classes that 
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
in `examples/basic-examples/bare-metal-bag-of-task` and
`examples/basic-examples/cloud-bag-of-task`. These simulators produce a JSON file
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
