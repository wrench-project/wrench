Creating a file registry service                        {#guide-101-fileregistry}
==========

[TOC]

# Overview #            {#guide-fileregistry-overview}

A file registry service is a simple store of key-values pairs where keys are files (i.e., `wrench::WorkflowFile`) and values are file locations
(i.e., `wrench::FileLocation`).  It is used to keep track of the location of file copies. In real-world deployments, this service
is often called a "replica catalog". 

# Creating a file registry service #        {#guide-fileregistry-creating}

In WRENCH, a file registry service is defined by the
`wrench::FileRegistryService` class, an instantiation of which
requires the following parameters:

- The name of a host on which to start the service; and
- Maps (`std::map`) of configurable properties (`wrench::NetworkProximityServiceProperty`) and configurable message payloads (`wrench::NetworkProximityServiceMessagePayload`).

The example below creates an instance that runs on host
`ReplicaCatalog`. Furthermore, the service is configured so that looking up
an entry takes 100 flops of computation, and so that the message sent to
the service to lookup an entry is 1KiB:

~~~~~~~~~~~~~{.cpp}
auto fr_service = simulation->add(
          new wrench::FileRegistryService("ReplicaCatalog",
                                       {{wrench::FileRegistryServiceProperty::LOOKUP_COMPUTE_COST, "0.1"}},
                                       {{wrench::BareMetalComputeServiceMessagePayload::FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD, 1024}});
~~~~~~~~~~~~~

See the documentation of `wrench::FileRegistryServiceProperty` and
`wrench::FileRegistryServiceMessagePayload` for all possible configuration
options.


