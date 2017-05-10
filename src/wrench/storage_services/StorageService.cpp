/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include "StorageService.h"
#include "../simulation/Simulation.h"

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
      this->state = StorageService::DOWN;
      // Notify the simulation that the service is terminated, if that
      // service was registered with the simulation
      if (this->simulation) {
        this->simulation->mark_storage_service_as_terminated(this);
      }
    }

    /**
     * @brief Get the name of the compute service
     * @return the compute service name
     */
    std::string StorageService::getName() {
      return this->service_name;
    }

    /**
    * @brief Find out whether the compute service is UP
    * @return true if the compute service is UP, false otherwise
    */
    bool StorageService::isUp() {
      return (this->state == StorageService::UP);
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
    StorageService::StorageService(std::string service_name) {
      this->service_name = service_name;
      this->simulation = nullptr; // will be filled in via Simulation::add()
      this->state = StorageService::UP;
    }
};