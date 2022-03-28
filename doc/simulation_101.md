This page provides a gentle introduction to the simulation of parallel and
distributed executions, as enabled by WRENCH. This content is intended for
users who have never implemented (or even thought of implementing) a
simulator.

[TOC]

# Simulation Overview      {#simulation-overview}

A simulator is a software artifact that mimics the behavior of some system
of interest.  In the context of the WRENCH project, the systems of
interests are parallel and distributed platforms on which run various
software runtime systems by which some application workload is to be
executed.  For instance, the platform could be a homogeneous cluster with
some network attached storage, the software runtime systems could be a batch
scheduler and a file server that controls access to the network attached
storage, and the application workload could be a scientific workflow. The
system could be much more complex, with different kinds of compute and
storage services running on hardware resources connected over a wide-area
network, including virtual machine instances provided by cloud
infrastructures. Regardless, the underlying concepts are the same, and are
outlined hereafter.

# Simulated Platform    {#simulated-platform}

A simulated platform consists of a set of computers, or, **hosts**.  These
hosts can have various characteristics (e.g., number of cores, clock rate).
Each host can have one or more **disks** attached to it, on which data can
be stored and accessed.  The hosts are interconnected with each other over
a network (otherwise this wouldn't be parallel and distributed computing).
The network is a set of network **links**, each with some latency and
bandwidth specification. Two hosts are connected via a network path, which
is simply a sequence of links.

The above concepts allow us to describe a platform to be simulated, that
can resemble real-world, either current or upcoming, platforms. Many more
details and features of the platform can be described, but the above
concepts gives us enough of a basis for everything that follows. Platform
description in WRENCH is based on the platform description capabilities in
[SimGrid](https://simgrid.org), on which WRENCH builds: a platform can be
described in an XML file or programmatically (see more details are given on the
[WRENCH 101 page](@ref wrench-101)).

# Simulated Processes   {#simulated-processes}

The execution of processes (i.e., running programs) can be simulated on the
hosts of the simulated platform. These processes can execute arbitrary
(C++) code and also place calls to WRENCH to simulate usage of the platform
resources (i.e., now I am computing, now I am sending data to the network,
now I am reading data from disk, now I am creating a new process, etc.).
As a result, the speed of the execution of these processes is limited by
the characteristics of the hardware resources in the platform, and their
usage by other processes. Process executions proceed through simulated time
until the end of the simulation, e.g., when the application workload of
interest has completed. At that point, the simulator can, for instance,
print the simulated time, which is the simulated execution time of the
application workload.

At this point, you may be thinking: "Are you telling me that I need to
implement a bunch of simulated processes that do things and talk to each
other?  My system is complicated and don't even know all the processes I'd
need to simulate!  There is no way I can do this!".  **And you'd be
right.** It is true that any parallel and distributed system of interest
is, at its most basic level, just a set of processes that compute,
read/write data, and send/receive messages.  But it is a lot of work to
implement a simulator of a complex system at such a low level. This is
where WRENCH comes in.

# Simulated Services    {#simulated-services}

WRENCH comes with a large set of already-implemented-for-you
**services**. A service is a set of one or more running simulated processes
that simulate a software runtime system that is commonly useful and used
for parallel and distributed computing. The two main kinds of services
are *compute services* and *storage services*. There are other services,
which are detailed on the [WRENCH 101 page](@ref wrench-101).

A compute service is a runtime system to which you can say "run this
computation" and it replies either "ok, I'll run it" or "I can". If it can
run it, then later on it will tell you either "It's done" or "It's failed".
And that's it. Underneath, this entails all kinds of processes that
compute, communicate with each other, and/or start other processes. But
this is all abstracted away in the notion of a "service" that exposes a
simple, high-level, easy-to-understand API.  For instance, in our example
earlier we mentioned a batch scheduler. In the HPC (High Performance
Computing) world, this is popular runtime system that controls access to
and the execution of jobs on a set of compute nodes on some fast local
network, i.e., a cluster. In the real-world, a batch scheduler consists of
many running processes (a.k.a. daemons) running on the cluster, implements
sophisticated algorithms to decide which job should run next, makes sure
jobs don't run on the same cores, etc. WRENCH provides an
already-implemented compute service called a `wrench::BatchComputeService`
that does all this for you, under the cover.

A storage service is a runtime system to which you can say "here is some
data I want you to store", "I want to read some bytes from that data I
stored before", "Do you have this data?", etc.  Here again, a storage
service in the real world consists of several processes (e.g., to handle
bounded numbers of concurrent reads/writes from different clients) and can
use non-trivial algorithms (e.g., for overlapping network communication and
disk accesses). Here again, WRENCH comes with an already-implemented
storage service called `wrench::SimpleStorageService` that does all this
for you and computes with a straightforward, high-level API.

Each service in WRENCH comes with configurable *properties*, that are
well-documented and can be used to specify particular features and/or
behaviors. Each service also comes with *configurable message payloads*,
which specify the size in bytes of the control messages that underlying
processes exchange with each other to implement the service's
functionality.  In the real-world, the processes the comprise a service
exchange various messages, and in WRENCH you get to specify the size of all
these messages (the larger the sizes, the longer the simulated
communication times). See [Service Customization](@ref wrench-101-customizing-services) on the [WRENCH 101 page](@ref wrench-101).


# Simulated Controller          {#simulated-controller}

As you recall, the goal of a WRENCH simulator is to simulate the execution
of some application workload. And so far, we haven't said much about this
workload or about how one goes about simulating its execution. So let's...

An application workload is executed using the services deployed on the
platform. To do so, you need to implement one process called an **execution
controller**. This process invokes the services to execute the application
workload, whatever that workload is. Say for instance, the your application
workload consists in performing some amount of computation based on input
storage in some file.  Then the controller will ask a compute service to
start a job to performance the computation, while reading the input from
some storage service that stores the input file. Whenever the compute
service replies that the computation has finished, then the execution
controller's work is done.

The execution controller is the core of the simulator, as it is when you
can implement whatever algorithm/strategy you wish to simulate for
executing the application workload.  We won't say more here, even though
at this point the execution controller likely seems a bit abstract.
This is why the simulation controller is the main topic of
the [WRENCH 102 page](@ref wrench-102)!


# What's next

At this point, you should be able to jump into [WRENCH 101](@ref wrench-101)!

