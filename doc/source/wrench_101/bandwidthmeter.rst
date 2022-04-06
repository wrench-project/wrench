.. _guide-101-bandwidthmeter:

Creating a bandwidth-meter service
==================================

.. _guide-bandwidthmeter-overview:

Overview
========

A bandwidth-meter service simply measures, at regular intervals, the
bandwidth usage of one or more network links, making measurement traces
available as part of the simulation output. Note that this is something
that’s not easy to do in real-world systems, but yay simulation!

.. _guide-bandwidthmeter-creating:

Creating a bandwidth-meter service
==================================

In WRENCH, a bandwidth-meter service is defined by the
:cpp:class:`wrench::BandwidthMeterService` class, an instantiation of which
requires the following parameters:

-  The name of a host on which to start the service;
-  A map of key-value pairs, where the keys are link names and the
   values are measurement periods in seconds.

The example below creates an instance that runs on host
``MeasurerHost``, and measures the available bandwidth on link ``link1``
every second and the available bandwidth on link ``link2`` every 10
seconds:

.. code:: cpp

   auto np_service = simulation->add(
             new wrench::BandwidthMeterService("MeasurerHost", {{"link1",1.0},{"link2", 10.0}});

One the simulation is completed, bandwidth usage measurement time stamps
can be accessed as follows:

.. code:: cpp

       auto bandwidth_usage = simulation->getOutput().getTrace<wrench::SimulationTimestampLinkUsage>();

See the documentation of :cpp:class:`wrench::SimulationOutput` for more details.
