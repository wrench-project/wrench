.. _simulation-101:

Simulation 101
**************

This page provides a gentle introduction to the simulation of parallel
and distributed executions, as enabled by WRENCH. This content is
intended for users who have never implemented (or even thought of
implementing) a simulator.

Simulation Overview
===================

A simulator is a software artifact that mimics the behavior of some
system of interest. In the context of the WRENCH project, the systems of
interest are parallel and distributed platforms on which various
software runtime systems are deployed by which some application workload
is to be executed. For instance, the platform could be a homogeneous
cluster with some network attached storage, the software runtime systems
could be a batch scheduler and a file server that controls access to the
network attached storage, and the application workload could be a
scientific workflow. The system could be much more complex, with
different kinds of runtime systems running on hardware or virtualized
resources connected over a wide-area network.

Simulated Platform
==================

A simulated platform consists of a set of computers, or, **hosts**.
These hosts can have various characteristics (e.g., number of cores,
clock rate). Each host can have one or more **disks** attached to it, on
which data can be stored and accessed. The hosts are interconnected with
each other over a network (otherwise this would not be parallel and
distributed computing). The network is a set of network **links**, each
with some latency and bandwidth specification. Two hosts are connected
via a network path, which is simply a sequence of links through which
messages between the two hosts are routed.

The above concepts allow us to describe a simulated platform that can
resemble real-world, either current or upcoming, platforms. Many more
details and features of the platform can be described, but the above
concepts gives us enough of a basis for everything that follows.
Platform description in WRENCH is based on the platform description
capabilities in :ref:`SimGrid <https://simgrid.org>`: a platform can be
described in an XML file or programmatically (see more details on the
:ref:`WRENCH 101 <wrench-101-header>` page).

Simulated Processes
===================

The execution of processes (i.e., running programs) can be simulated on
the hosts of the simulated platform. These processes can execute
arbitrary (C++) code and also place calls to WRENCH to simulate usage of
the platform resources (i.e., now I am computing, now I am sending data
to the network, now I am reading data from disk, now I am creating a new
process, etc.). As a result, the speed of the execution of these
processes is limited by the characteristics of the hardware resources in
the platform, and their usage by other processes. Process executions
proceed through simulated time until the end of the simulation, e.g.,
when the application workload of interest has completed. At that point,
the simulator can, for instance, print the simulated time.

At this point, you may be thinking: "Are you telling me that I need to
implement a bunch of simulated processes that do things and talk to each
other? My system is complicated and do not even know all the processes
I would need to simulate! There is no way I can do this!". **And you would be
right.** It is true that any parallel and distributed system of interest
is, at its most basic level, just a set of processes that compute,
read/write data, and send/receive messages. But it is a lot of work to
implement a simulator of a complex system at such a low level. This is
where WRENCH comes in.

Simulated Services
==================

WRENCH comes with a large set of already-implemented **services**. A
service is a set of one or more running simulated processes that
simulate a software runtime system that is commonly useful and used for
parallel and distributed computing. The two main kinds of services are
*compute services* and *storage services*, but there are others (all
detailed on the :ref:`WRENCH 101 <wrench-101-header>` page).

A compute service is a runtime system to which you can say "run this
computation" and it replies either "ok, I will run it" or "I cannot". If it
can run it, then later on it will tell you either "It is done" or "It is
failed". And that is it. Underneath, this entails all kinds of processes
that compute, communicate with each other, and start other processes.
This complexity is all abstracted away by the service, which exposes a
simple, high-level, easy-to-understand API. For instance, in our example
earlier we mentioned a batch scheduler. For HPC (High Performance
Computing), this is popular runtime system that manages the execution of
jobs on a set of compute nodes on some fast local network, i.e., a
cluster. In the real-world, a batch scheduler consists of many running
processes (a.k.a. daemons) running on the cluster, implements
sophisticated algorithms to decide which job should run next, makes sure
jobs do not run on the same cores, etc. WRENCH provides an
already-implemented compute service called a
:cpp:class:`wrench::BatchComputeService` that does all this for you,
under the cover. 

For example, the well-known batch scheduler 
:ref:`Slurm <https://www.schedmd.com/>` uses several daemons to schedule
and manage jobs(e.g., the process **slurmd** runs on each compute node
and one **slurmctld** daemon controls everything). In this example, an
instance of :cpp:class:`wrench::BatchComputeService` could represent
one Slurm cluster with one **slurmctld** process and
multiple **slurmd** processes.

A storage service is a runtime system to which you can say "here is some
data I want you to store", "I want to read some bytes from that data I
stored before", "Do you have this data?", etc. A storage service in the
real world consists of several processes (e.g., to handle bounded
numbers of concurrent reads/writes from different clients) and can use
non-trivial algorithms (e.g., for overlapping network communication and
disk accesses). Here again, WRENCH comes with an already-implemented
storage service called :cpp:class:`wrench::SimpleStorageService` that does all
this for you and comes with a straightforward, high-level API. 
Note that a storage service does not provide by default capabilities
traditionally offered by parallel file systems such as 
:ref:`Lustre <https://www.lustre.org/>` 
(i.e., no stripping among storage nodes, no dedicated metadata servers). 
If you want to model such storage back-end, you can do it by extending
the :cpp:class:`wrench::SimpleStorageService`.

Each service in WRENCH comes with configurable *properties*, that are
well-documented and can be used to specify particular features and/or
behaviors (e.g., a specific scheduling algorithm for a 
given :cpp:class:`wrench::BatchComputeService`).
Each service also comes with *configurable message payloads*,
which specify the size in bytes of the control messages that underlying
processes exchange with each other to implement the service's
functionality. In the real-world, the processes that comprise a service
exchange various messages, and in WRENCH you get to specify the size of
all these messages (the larger the sizes the longer the simulated
communication times). See more about :ref:`Service
Customization <wrench-101-customizing-services>` on the :ref:`WRENCH
101 <wrench-101-header>` page.

When the simulator is done, the **calibration** phase begins. 
The **calibration** step is crucial to ensure that your simulator
accurately approximate the performance of the application you study 
on the target platform. Basically, calibrating a simulator implies 
that you fine-tune the simulator to approximate the real performance 
of the target application when running on the modeled platform. 
*Payloads* and *properties* play a central role in this calibration 
step as they control the weight of many important actions (for example, 
how much overhead when reading a file from a storage service?).

Simulated Controller
====================

As you recall, the goal of a WRENCH simulator is to simulate the
execution of some application workload. And so far, we have not said much
about this workload or about how one goes about simulating its
execution. So let's...

An application workload is executed using the services deployed on the
platform. To do so, you need to implement one process called an
**execution controller**. This process invokes the services to execute
the application workload, whatever that workload is. Say, for instance,
that your application workload consists in performing some amount of
computation based on data in some input file. The controller should ask
a compute service to start a job to perform the computation, while
reading the input from some storage service that stores the input file.
Whenever the compute service replies that the computation has finished,
then the execution controller's work is done.

The execution controller is the core of the simulator, as it is where
you implement whatever algorithm/strategy you wish to simulate for
executing the application workload. At this point the execution
controller likely seems a bit abstract. But we would not say more about it
until you get to the :ref:`WRENCH 102 <wrench-102-header>` page, which is
exclusively about the controller.

What's next
===========

At this point, you should be able to jump into :ref:`wrench-101-header`!
