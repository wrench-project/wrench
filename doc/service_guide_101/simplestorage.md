Creating a simple storage service          {#guide-101-simplestorage}
============


[TOC]

# Overview #            {#guide-simplestorage-overview}

A Simple storage service is the simplest possible abstraction for a 
service that can store and provide access to workflow files. It has a certain storage
capacity, and provides write, read, and delete operations on files. 
In addition, higher-level semantics such as copying a file directly from a
storage service to another are provided.

# Creating a Simple storage service #        {#guide-simplestorage-creating}

In WRENCH, a Simple storage service represents a storage service
(`wrench::StorageService`), which is defined by the `wrench::SimpleStorage`
class. An instantiation of a Simple storage service requires the following
parameters:

- The name of a host on which to start the service;
- A list of mount points (corresponding to disks attached to the host); and
- Maps (`std::map`) of configurable properties (`wrench::BatchComputeServiceProperty`) and configurable message
  payloads (`wrench::BatchComputeServiceMessagePayload`).
  
The example below creates an instance of a Simple storage service
that runs on host `BigDisk`, has access to the disks mounted at paths
`/data/` and `/home/` at host `BigDisk`. 
Furthermore, the number of maximum concurrent data connections supported by
the service is configured to be 8, and
the message sent to the service to find out its free space is configured to be 1KiB:

~~~~~~~~~~~~~{.cpp}
auto storage_service = simulation->add(
          new wrench::SimpleStorageService("BigDisk", 
                                  {"/data/", "/home/"},
                                       {{wrench::SimpleStorageProperty::MAX_NUM_CONCURRENT_DATA_CONNECTIONS, "8"}},
                                       {{wrench::SimpleStorageProperty::FREE_SPACE_REQUEST_MESSAGE_PAYLOAD, "1024"}
                                      );
~~~~~~~~~~~~~

See the documentation of `wrench::SimpleStorageServiceProperty` and
`wrench::SimpleStorageServiceMessagePayload` for all possible configuration
options.

Also see the simulators in the `examples/basic-examples/*` directories, which all  use simple storage services.

