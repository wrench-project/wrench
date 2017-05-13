/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include "StorageService.h"
#include "simulation/Simulation.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(storage_service, "Log category for Storage Service");


namespace wrench {

    /**
		 * @brief Sets the simulation the compute service belongs to
		 * @param simulation: a pointer to a Simulation object
		 */
    void StorageService::setSimulation(Simulation *simulation) {
      this->simulation = simulation;
    }

    /**
     * @brief Stop the compute service - must be called by the stop()
     *        method of derived classes
     */
    void StorageService::stop() {
      // Notify the simulation that the service is terminated, if that
      // service was registered with the simulation
      if (this->simulation) {
        this->simulation->mark_storage_service_as_terminated(this);
      }

      // Call the super class's method
      Service::stop();
    }

    /**
		 * @brief Set the state of the storage service to DOWN
		 */
    void StorageService::setStateToDown() {
      this->state = StorageService::DOWN;
    }

    /**
   * @brief Constructor
   *
   * @param service_name: the name of the storage service
   */
    StorageService::StorageService(std::string service_name, std::string mailbox_name_prefix, double capacity) :
          Service(service_name, mailbox_name_prefix) {
      this->capacity  = capacity;
      this->simulation = nullptr; // will be filled in via Simulation::add()
      this->state = StorageService::UP;
    }

    /**
     * @brief Internal method to add a file to the storage in a StorageService
     *
     * @param file: a raw pointer to a WorkflowFile object
     *
     * @throw std::runtime_error
     */
    void StorageService::addFileToStorage(WorkflowFile *file) {

      if (file->getSize() > this->getFreeSpace()) {
        throw std::runtime_error("File exceeds free space capacity on storage service");
      }
      this->stored_files.insert(file);
      this->occupied_space += file->getSize();
      XBT_INFO("Stored file %s (storage usage: %.2lf%%)", file->getId().c_str(), 100.0 * this->occupied_space / this->capacity);
    }

    /**
     * @brief Internal method to delete a file from the storage  in a StorageService
     *
     * @param file: a raw pointer to a WorkflowFile object
     *
     * @throw std::runtime_error
     */
    void StorageService::removeFileFromStorage(WorkflowFile *file) {

      if (this->stored_files.find(file) == this->stored_files.end()) {
        throw std::runtime_error("Attempting to remove a file that's not on the storage service");
      }
      this->stored_files.erase(file);
      this->occupied_space -= file->getSize();
      XBT_INFO("Deleted file %s (storage usage: %.2lf%%)", file->getId().c_str(), 100.0 * this->occupied_space / this->capacity);
    }


    /**
    * @brief Retrieve the storage capacity of the storage service
    * @return the capacity in bytes
    */
    double StorageService::getCapacity() {
      return this->capacity;
    }

    /**
     * @brief Retrieve the free storage space on the storage service
     * @return the free space in bytes
     */
    double StorageService::getFreeSpace() {
      return this->capacity - this->occupied_space;
    }

};