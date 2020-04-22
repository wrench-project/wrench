SimpleStorageService                        {#guide-101-simplestorage}
============


[TOC]

# Overview #            {#guide-simplestorage-overview}

A Simple storage service is the simplest possible abstraction for a 
service that can store and provide workflow files. It has a certain storage
capacity, and provides write, read, and delete operations on files. Writes
and Reads can be done synchronously or asynchronously. In addition, higher-level
semantics such as copying a file directly from a storage service to another
are provided. 

# Creating a Simple storage service #        {#guide-simplestorage-creating}

In WRENCH, a Simple storage service represents a storage service
(`wrench::StorageService`), which is defined by the `wrench::SimpleStorage`
class. An instantiation of a Simple storage service requires the following
parameters:

- The name of a host on which to start the service (this is the entry point to the service);
- A list of mount points (corresponding to disks attached to the host)
- Maps (`std::map`) of configurable properties (`wrench::BatchComputeServiceProperty`) and configurable message
  payloads (`wrench::BatchComputeServiceMessagePayload`).
  
The example below shows how to create an instance of a Simple storage service
that runs on host "Gateway", has access to 64TiB of storage. 
Furthermore, the number of maximum concurrent data connections supported by
the service is configured to be 8:

~~~~~~~~~~~~~{.cpp}
auto storage_service = simulation->add(
          new wrench::SimpleStorageService("Gateway", 
                                  {"/data/", "/home/"},
                                       {{wrench::SimpleStorageProperty::MAX_NUM_CONCURRENT_DATA_CONNECTIONS, "8"}}
                                      );
~~~~~~~~~~~~~


