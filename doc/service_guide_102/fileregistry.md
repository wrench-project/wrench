File Registry                        {#guide-fileregistry}
==========

@WRENCHUserDoc <div class="doc-type">User Documentation</div><div class="doc-link">Other: <a href="../developer/guide-fileregistry.html">Developer</a> - <a href="../internal/guide-fileregistry.html">Internal</a></div> @endWRENCHDoc
@WRENCHDeveloperDoc  <div class="doc-type">Developer Documentation</div><div class="doc-link">Other: <a href="../user/guide-fileregistry.html">User</a> - <a href="../internal/guide-fileregistry.html">Internal</a></div> @endWRENCHDoc
@WRENCHInternalDoc  <div class="doc-type">Internal Documentation</div><div class="doc-link">Other: <a href="../user/guide-fileregistry.html">User</a> -  <a href="../developer/guide-fileregistry.html">Developer</a></div> @endWRENCHDoc

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

@WRENCHNotUserDoc

# Using a file registry service #        {#guide-fileregistry-using}


Adding/removing an entry to a file registry service is done as follows:

~~~~~~~~~~~~~{.cpp}
wrench::WorkflowFile *some_file;
wrench::StorageService *some_storage_service;

[...]

fr_service->addEntry(some_file, some_storage_service);
fr_service->removeEntry(some_file, some_storage_service);
~~~~~~~~~~~~~

Retrieving all entries for a given file is done as follows:

~~~~~~~~~~~~~{.cpp}
wrench::WorkflowFile *some_file;

[...]

std::set<wrench::StorageService *> entries;
entries = fr_service->lookupEntry(some_file);
~~~~~~~~~~~~~

If a [network proximity service](@ref guide-networkproximity)
is running, it is possible to retrieve entries for a file
sorted by non-decreasing proximity from some reference host. Returned entries are stored in a (sorted) `std::map`
where the keys are network distances to the reference host. For instance:

~~~~~~~~~~~~~{.cpp}
wrench::WorkflowFile *some_file;
wrench::NetworkProximityService *np_service;

[...]
std::map<double, wrench::StorageService *> entries;
entries = fr_service->lookupEntry(some_file, "ReferenceHost", np_service);
~~~~~~~~~~~~~



@endWRENCHDoc

