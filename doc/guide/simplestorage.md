SimpleStorage                        {#guide-simplestorage}
============

@WRENCHUserDoc <div class="doc-type">User Documentation</div><div class="doc-link">Other: <a href="../developer/wrench-101.html">Developer</a> - <a href="../internal/wrench-101.html">Internal</a></div> @endWRENCHDoc
@WRENCHDeveloperDoc  <div class="doc-type">Developer Documentation</div><div class="doc-link">Other: <a href="../user/wrench-101.html">User</a> - <a href="../internal/wrench-101.html">Internal</a></div> @endWRENCHDoc
@WRENCHInternalDoc  <div class="doc-type">Internal Documentation</div><div class="doc-link">Other: <a href="../user/wrench-101.html">User</a> -  <a href="../developer/wrench-101.html">Developer</a></div> @endWRENCHDoc

[TOC]

# Overview #            {#guide-batch-overview}

A Simple storage service is the simplest possible abstration for a 
service that can store and provide workflow files. It has a certain storage
capacity, and provides write, read, and delete operations on files. Writes
and Reads can be done synchronously or asynchronously. In addition, higher-level
semantices such as copying a file directly from a storage service to another
are provided. 

# Creating a Simple storage service #        {#guide-simplestorage-creating}

In WRENCH, a Simple storage service represents a storage service
(`wrench::StorageService`), which is defined by the `wrench::SimpleStorage`
class. An instantiation of a Simple storage service requires the following
parameters:

- A hostname on which to start the service (this is the entry point to the service);
- A capacity in bytes;
- Maps (`std::map`) of configurable properties (`wrench::BatchServiceProperty`) and configurable message 
  payloads (`wrench::BatchServiceMessagePayload`).
  
The example below shows how to create an instance of a Simple storage service
that runs on host "Gateway", has access to 64TiB of storage. 
Furthermore, the number of maximum concurrent data connections supported by
the service is configured to be 8:

~~~~~~~~~~~~~{.cpp}
auto storage_service = simulation->add(
          new wrench::SimpleStorageService("Gateway", 
                                  pow(2,46),
                                       {{wrench::SimpleStorageProperty::MAX_NUM_CONCURRENT_DATA_CONNECTIONS, "8"}}
                                      );
~~~~~~~~~~~~~

