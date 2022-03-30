Interacting with a storage service  {#guide-102-simplestorage}
============

Several interactions with a storage service are done simple by calling
**static methods** of the `wrench::StorageService` class. These make it possible
to lookup, delete, read, and write files. For instance:

~~~~~~~~~~~~~{.cpp}
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
~~~~~~~~~~~~~

Note that the file registry service is passed to the
`wrench::StorageService::deleteFile()` method since the file deletion
should cause the file registry to remove one of its entries.

Reading and writing files is something an execution controller typically does not do directly (instead,
workflow tasks read and write files as they execute). But, if for some reason
an execution controller needs to spend time doing file I/O, it is easily done:

~~~~~~~~~~~~~{.cpp}
// Read some file from the "/" path at some storage service. 
// This does not change the simulation state besides simulating a time overhead during which the execution controller is busy
wrench::StorageService::readFile(some_file, wrench::FileLocation::LOCATION(storage_service, "/");

// Write some file to the "/stuff/" path at some storage service. 
// This simulates a time overhead after which the storage service will host the file. It
// is a good idea to then add an entry to the file registry service
wrench::StorageService::writeFile(some_file, wrench::FileLocation::LOCATION(storage_service, "/stuff/");

~~~~~~~~~~~~~


An operation commonly  performed by an execution controller is copying files between storage services (e.g., to 
enforce some data locality).  This is typically done by [specifying file copy operations as part of standard jobs](@ref wrench-102-controller-services-compute-job). But it can also be done manually by the execution controller via
the data movement manager's methods 
`wrench::DataMovementManager::doSynchronousFileCopy()` and 
`wrench::DataMovementManager::initiateAsynchronousFileCopy()`.  Here is an example
in which a file is copied between storage services:

~~~~~~~~~~~~~{.cpp}
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
~~~~~~~~~~~~~

See the execution controller implementation in `examples/basic-examples/bare-metal-data-movement/DataMovementWMS.cpp` for a more complete example.


