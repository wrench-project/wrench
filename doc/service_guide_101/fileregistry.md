FileRegistryService                        {#guide-101-fileregistry}
==========

[TOC]

# Overview #            {#guide-fileregistry-overview}

A file registry service is a simple store of key-value pairs where keys are files (i.e., `wrench::WorkflowFile`) and values are Storage
Services (i.e., `wrench::StorageService`).  It is used to keep track of the location of file copies. In real-world deployments, this service
is often called a "replica catalog". 

# Creating a file registry service #        {#guide-fileregistry-creating}

In WRENCH, a file registry service is defined by the
`wrench::FileRegistryService` class, an instantiation of which
requires the following parameters:

- The name of a host on which to start the service (this is the entry point to the service);
- Maps (`std::map`) of configurable properties (`wrench::NetworkProximityServiceProperty`) and configurable message payloads (`wrench::NetworkProximityServiceMessagePayload`).

The example below shows how to create an instance
that runs on host "ReplicaCatalog".
The service's properties are
customized to specify that it takes 0.1 (simulated) seconds for the service to lookup an entry in its database. 

~~~~~~~~~~~~~{.cpp}
auto fr_service = simulation->add(
          new wrench::FileRegistryService("ReplicaCatalog",
                                       {{wrench::FileRegistryService::LOOKUP_COMPUTE_COST, "0.1"}},
                                       {});
~~~~~~~~~~~~~

## File registry service properties             {#guide-fileregistry-creating-properties}

The properties that can be configured for a file registry service include:

  - `wrench::FileRegistryService::LOOKUP_COMPUTE_COST`

