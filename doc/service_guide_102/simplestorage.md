SimpleStorage                        {#guide-simplestorage}
============

@WRENCHUserDoc <div class="doc-type">User Documentation</div><div class="doc-link">Other: <a href="../developer/wrench-101.html">Developer</a> - <a href="../internal/wrench-101.html">Internal</a></div> @endWRENCHDoc
@WRENCHDeveloperDoc  <div class="doc-type">Developer Documentation</div><div class="doc-link">Other: <a href="../user/wrench-101.html">User</a> - <a href="../internal/wrench-101.html">Internal</a></div> @endWRENCHDoc
@WRENCHInternalDoc  <div class="doc-type">Internal Documentation</div><div class="doc-link">Other: <a href="../user/wrench-101.html">User</a> -  <a href="../developer/wrench-101.html">Developer</a></div> @endWRENCHDoc

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


@WRENCHNotUserDoc

# Using a simple storage service #        {#guide-simplestorage-using}

One can interact directly with a simple storage service to check whether
the service hosts a particular file or to delete a file:

~~~~~~~~~~~~~{.cpp}
wrench::WorkflowFile *some_file;

[...]

if (wrench::StorageService::lookupFile(some_file, wrench::FileLocation::LOCATION(storage_service, "/data/")) {
  std::cerr << "File is there! Let's delete it...\n";
  storage_service->deleteFile(some_file);
}
~~~~~~~~~~~~~

While there are few other direct interactions are possible 
(see the documentation of the `wrench::StorageService` and `wrench::SimpleStorageService` classes),
many interactions are via a data movement manager (an instance of the `wrench::DataMovementManager` class). 
 This is a helper process that makes it possible interact asynchronously and transparently with storage services.
For instance, here is an example of a synchronous file copy:

~~~~~~~~~~~~~{.cpp}
wrench::WorkflowFile *some_file;
wrench::StorageService *src, *dst;

[...]

// Create a data movement manager
auto data_movement_manager = this->createDataMovementManager();

data_movement_manager->doSynchronousFileCopy(some_file, 
                                             wrench::FileLocation::LOCATION(src), 
                                             wrench::FileLocation::LOCATION(dst));
~~~~~~~~~~~~~

The above call blocks until the file copy has completed. An asynchronous file copy would work as follows:

~~~~~~~~~~~~~{.cpp}
wrench::WorkflowFile *some_file;
wrench::StorageService *src, *dst;

[...]

// Create a job manager
auto job_manager = this->createJobManager();

job_manager->initiateAsynchronousFileCopy(some_file, 
                                          wrench::FileLocation::LOCATION(src), 
                                          wrench::FileLocation::LOCATION(dst));

[...]

// Wait for a workflow execution event
auto event = this->getWorkflow()->waitForNextExecutionEvent();

if (event->type == wrench::WorkflowExecutionEvent::FILE_COPY_COMPLETION) {
  std::cerr << "Success!\n";
} else if (event->type == wrench::WorkflowExecutionEvent::FILE_COPY_COMPLETION) {
  std::cerr << "Failure!\n";
}
~~~~~~~~~~~~~

See the documentation of the `wrench::DataMovementManager` class for all available API methods.


@endWRENCHDoc

