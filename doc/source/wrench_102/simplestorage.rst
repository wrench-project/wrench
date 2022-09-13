.. _guide-102-simplestorage:

Interacting with a simple storage service
=========================================

The following operations are supported by an instance of
:cpp:class:`wrench::SimpleStorageService`:

-  Synchronously check that a file exists
-  Synchronously read a file (rarely used by an execution controller but
   included for completeness)
-  Synchronously write a file (rarely used by an execution controller
   but included for completeness)
-  Synchronously delete a file
-  Synchronously copy a file from one storage service to another
-  Asynchronously copy a file from one storage service to another

The first 4 interactions above are done by calling member functions of
the :cpp:class:`wrench::StorageService` class. The last two are done via a Data
Movement Manager, i.e., by calling member functions of the
:cpp:class:`wrench::DataMovementManager` class. Some of these member functions
take an optional :cpp:class:`wrench::FileRegistryService` argument, in which case
they will also update entries in a file registry service (e.g., removing
an entry when a file is deleted).

Several interactions with a simple storage service are done simple by calling
**static methods** of the :cpp:class:`wrench::StorageService` class. These make
it possible to lookup, delete, read, and write files. For instance:

.. code:: cpp

   std::shared_ptr<wrench::SimpleStorageService> storage_service;
   // Get the file registry service
   std::shared_ptr<wrench::FileRegistryService> file_registry;

   std::shared_ptr<wrench::DataFile> some_file;

   [...]

   // Check whether the storage service holds the file at path /data/ and delete it if so
   auto file_location = wrench::FileLocation::LOCATION(storage_service, "/data/");
   if (wrench::StorageService::lookupFile(some_file, file_location) {
     std::cerr << "File found!" << std::endl;
     wrench::StorageService::deleteFile(some_file, file_location, file_registry);
   }

Note that the file registry service is passed to the
:cpp:class:`wrench::StorageService::deleteFile()` method since the file deletion
should cause the file registry to remove one of its entries.

Reading and writing files is something an execution controller typically
does not do directly (instead, jobs created by the controller contain
actions/tasks  that read and write files as
they execute). But, if for some reason an execution controller needs to
spend time doing file I/O, it is easily done:

.. code:: cpp

   // Read some file from the "/" path at some storage service. 
   // This does not change the simulation state besides simulating a time overhead during which the execution controller is busy
   wrench::StorageService::readFile(some_file, wrench::FileLocation::LOCATION(storage_service, "/");

   // Write some file to the "/stuff/" path at some storage service. 
   // This simulates a time overhead after which the storage service will host the file. It
   // is a good idea to then add an entry to the file registry service
   wrench::StorageService::writeFile(some_file, wrench::FileLocation::LOCATION(storage_service, "/stuff/");

An operation commonly performed by an execution controller is copying
files between storage services (e.g., to enforce some data locality).
This is typically done by :ref:`specifying file copy operations as part of
standard jobs <wrench-102-controller-services-compute-job>`.
But it can also be done manually by the execution controller via the
data movement manager’s methods
:cpp:class:`wrench::DataMovementManager::doSynchronousFileCopy()` and
:cpp:class:`wrench::DataMovementManager::initiateAsynchronousFileCopy()`. Here is
an example in which a file is copied between storage services:

.. code:: cpp

   // Create a data movement manager
   auto data_movement_manager = this->createDataMovementManager();

   // Synchronously copy some_file from storage_service1 to storage_service2
   // While this is taking place, the execution controller is busy
   data_movement_manager->doSynchronousFileCopy(some_file, wrench::FileLocation::LOCATION(storage_service1), wrench::FileLocation::LOCATION(storage_service2));

   // Asynchronously copy some_file from storage_service2 to storage_service3
   data_movement_manager->initiateAsynchronousFileCopy(some_file, wrench::FileLocation::LOCATION(storage_service2), wrench::FileLocation::LOCATION(storage_service3));

   [...]

   // Wait for and process the next event (may be a file copy completion or failure)
   this->waitForAndProcessNextEvent();

See the execution controller implementation in
``examples/workflow_api/basic-examples/bare-metal-data-movement/DataMovementWMS.cpp``
for a more complete example.
