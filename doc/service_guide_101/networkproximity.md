NetworkProximityService                        {#guide-101-networkproximity}
==========


[TOC]

# Overview #            {#guide-networkproximity-overview}

A network proximity service answers queries regarding the network proximity
between hosts. The service accomplishes this by periodically performing round-trip network transfer experiments
between hosts, keeping a record of observed network transfer times, and computing network distances.


# Creating a network proximity service #        {#guide-networkproximity-creating}

In WRENCH, a network proximity service is defined by the
`wrench::NetworkProximityService` class, an instantiation of which
requires the following parameters:

- The name of a host on which to start the service (this is the entry point to the service);
- A set of hosts names in a vector (`std::vector`), which define which hosts are monitored by the service;
- Maps (`std::map`) of configurable properties (`wrench::NetworkProximityServiceProperty`) and configurable message payloads (`wrench::NetworkProximityServiceMessagePayload`).
  
The example below shows how to create an instance 
that runs on host "Networkcentral", and can answer network distance queries about 
hosts "Host1", "Host2", "Host3", and "Host4".  The service's properties are
customized to specify that the service performs network transfer experiments on average every 60 seconds, and that the Vivaldi algorithm is used to compute network coordinates.

~~~~~~~~~~~~~{.cpp}
auto np_service = simulation->add(
          new wrench::NetworkProximityService("Networkcentral", 
                                       {"Host1", "Host2", "Host3", "Host4"},
                                       {{wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_MEASUREMENT_PERIOD, "60"},
                                        {wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_SERVICE_TYPE, "VIVALDI"}},
                                       {});
~~~~~~~~~~~~~

## Network proximity service properties             {#guide-networkproximity-creating-properties}


The properties that can be configured for a network proximity service include:

- `wrench::NetworkProximityServiceProperty::LOOKUP_OVERHEAD`
- `wrench::NetworkProximityServiceProperty::NETWORK_DAEMON_COMMUNICATION_COVERAGE`
- `wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_MEASUREMENT_PERIOD`
- `wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_MEASUREMENT_PERIOD_MAX_NOISE`
- `wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_MESSAGE_SIZE`
- `wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_PEER_LOOKUP_SEED`
- `wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_SERVICE_TYPE`


