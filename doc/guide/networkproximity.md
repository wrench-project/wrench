Network Proximity                        {#guide-networkproximity}
==========

@WRENCHUserDoc <div class="doc-type">User Documentation</div><div class="doc-link">Other: <a href="../developer/guide-networkproximity.html">Developer</a> - <a href="../internal/guide-networkproximity.html">Internal</a></div> @endWRENCHDoc
@WRENCHDeveloperDoc  <div class="doc-type">Developer Documentation</div><div class="doc-link">Other: <a href="../user/guide-networkproximity.html">User</a> - <a href="../internal/guide-networkproximity.html">Internal</a></div> @endWRENCHDoc
@WRENCHInternalDoc  <div class="doc-type">Internal Documentation</div><div class="doc-link">Other: <a href="../user/guide-networkproximity.html">User</a> -  <a href="../developer/guide-networkproximity.html">Developer</a></div> @endWRENCHDoc

[TOC]

# Overview #            {#guide-networkproximity-overview}

A network proximity service answers queries regarding the network proximity
between hosts. The service accomplishes this by periodically performing network transfer experiments
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


@WRENCHNotUserDoc

# Querying a network proximity service #        {#guide-networkproximity-using}

Querying a network proximity service is straightforward. For instance, to
obtain a measure of the network distance between hosts "Host1" and "Host3", one
simply does:

~~~~~~~~~~~~~{.cpp}
double distance = np_service->query(std::make_pair("Host1","Host2"));
~~~~~~~~~~~~~

This distance corresponds to half the round-trip-time, in seconds, between the
two hosts.
If the service is configured to use the Vivaldi coordinate-based system, as in our
example above, this distance is actually derived from network coordinates, as computed
by the Vivaldi algorithm. In this case one can actually ask for these coordinates for any given host:

~~~~~~~~~~~~~{.cpp}
std::pair<double,double> coords = np_service->getCoordinates("Host1");
~~~~~~~~~~~~~

See the documentation of `wrench::NetworkProximityService` for more API methods.


@endWRENCHDoc
