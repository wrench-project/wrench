.. _guide-101-energymeter:

Creating an energy-meter service
================================

.. _guide-energymeter-overview:

Overview
========

An energy-meter service simply measures, at regular intervals, the
energy consumed by one or more hosts, making measurement traces
available as part of the simulation output.

.. _guide-energymeter-creating:

Creating an energy-meter service
================================

In WRENCH, an energy-meter service is defined by the
:cpp:class:`wrench::EnergyMeterService` class, an instantiation of which requires
the following parameters:

-  The name of a host on which to start the service;
-  A map of key-value pairs, where the keys are hostnames and the values
   are measurement periods in seconds.

The example below creates an instance that runs on host
``MeasurerHost``, and measures the energy consumed on host ``Host1``
every second and the energy consumed on host ``Host2`` every 10 seconds:

.. code:: cpp

   auto np_service = simulation->add(
             new wrench::EnergyMeterService("MeasurerHost", {{"Host1",1.0},{"Host2", 10.0}});

One the simulation is completed, energy measurement time stamps can be
accessed as follows:

.. code:: cpp

       auto energy_consumption_timestamps = simulation->getOutput().getTrace<wrench::SimulationTimestampEnergyConsumption>();

See the documentation of :cpp:class:`wrench::SimulationOutput` for more details.
