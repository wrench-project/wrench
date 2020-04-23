Interacting with a storage service  {#guide-102-simplestorage}
============

Several interactions with a storage service are done simple by calling
**static methods** of the `wrench::StorageService` class. These make it possible
to lookup, delete, read, and write files. For instance:

~~~~~~~~~~~~~{.cpp}
// Get the first simple storage service (assuming there's at least one)
auto storage_service = *(this->getAvailableComputeServices<wrench::SimpleStorageService>().begin());
// Get the file registry service
auto file_registry = this->getAvailableFileRegistryService();

wrench::WorkflowFile *some_file = ...;

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

Reading and writing files is something a WMS typically does not do directly (instead,
workflow tasks read and write files as they are executing). But, if for some reason
a WMS needs to spend time doing file I/O, it is easy done:

~~~~~~~~~~~~~{.cpp}
// Read some file from the "/" path at some storage service. 
// This does nothing but simulate a time overhead during which the WMS is busy
wrench::StorageService::readFile(some_file, wrench::FileLocation::LOCATION(storage_service, "/");

// Write some file from to "/" path at some storage service. 
// This simulates a time overhead after which the storage service will host the file. It
// is a good idea to then add an entry to the file registry service
wrench::StorageService::readFile(some_file, wrench::FileLocation::LOCATION(storage_service, "/");

~~~~~~~~~~~~~


An operation commonly  performed by WMS is copying files between storage services (e.g., to 
enforce some data locality).  This is often done by [specifying file copy operations
as part of standard jobs](@ref wrench-102-jobs), but can also be done manually by the WMS.  
This is done via the data movement manager's methods 
`wrench::DataMovementManager::doSynchronousFileCopy()` and 
`wrench::DataMovementManager::initiateAsynchronousFileCopy()`.  Here is an example
in  which a file is copied between storage services:

~~~~~~~~~~~~~{.cpp}
// Create a data movement manager
auto data_movement_manager = this->createDataMovementManager();

// Synchronously copy some_file from storage_service1 to storage_service2
// While this is taking place, the WMS is busy
data_movement_manager->doSynchronousFileCopy(some_file, 
                                             wrench::FileLocation::LOCATION(storage_service1), 
                                             wrench::FileLocation::LOCATION(storage_service2));

// Asynchronously copy some_file from storage_service2 to storage_service3
data_movement_manager->initiateAsynchronousFileCopy(some_file, 
                                             wrench::FileLocation::LOCATION(storage_service2), 
                                             wrench::FileLocation::LOCATION(storage_service3));

[...]

// Wait for and process the next event  (may be a file copy completion or failure)
this->waitForAndProcessNextEvent();
~~~~~~~~~~~~~

See the WMS implementation in `examples/basic-examples/bare-metal-data-movement/DataMovementWMS.cpp` for a more complete example.


