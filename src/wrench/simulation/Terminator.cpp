/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/logging/TerminalOutput.h"
#include "wrench/simulation/Terminator.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(terminator, "Log category for Terminator");

namespace wrench {

    /**
     * @brief Register a list of references to ComputeService
     *
     * @param compute_services: a list of pointers to ComputeService objects
     */
    void Terminator::registerComputeService(std::set<ComputeService *> compute_services) {
      for (auto compute_service : compute_services) {
        this->registerComputeService(compute_service);
      }
    }

    /**
     * @brief Register a reference to a ComputeService
     *
     * @param compute_service: a pointer to a ComputeService
     */
    void Terminator::registerComputeService(ComputeService *compute_service) {
      this->registerService(this->compute_services, compute_service);
    }

    /**
     * @brief Shutdown a list of ComputeService references. If the ComputeService has only one reference it will
     * be stopped, otherwise the counter is decreased.
     *
     * @param compute_services: a list of pointers to ComputeService objects
     *
     * @throw std::runtime_error
     */
    void Terminator::shutdownComputeService(std::set<ComputeService *> compute_services) {
      for (auto compute_service : compute_services) {
        this->shutdownComputeService(compute_service);
      }
    }

    /**
     * @brief Shutdown a ComputeService. If the ComputeService has only one reference it will be stopped,
     * otherwise the counter is decreased
     *
     * @param compute_service: a pointer to a ComputeService to be stopped
     *
     * @throw std::runtime_error
     */
    void Terminator::shutdownComputeService(ComputeService *compute_service) {
      if (this->shutdownService(this->compute_services, compute_service)) {
        WRENCH_INFO("Terminator shut down compute service: %s", compute_service->getName().c_str());
      }
    }

    /**
     * @brief Register a list of references to StorageService
     *
     * @param storage_services: a list of pointers to StorageService objects
     */
    void Terminator::registerStorageService(std::set<StorageService *> storage_services) {
      for (auto storage_service : storage_services) {
        this->registerStorageService(storage_service);
      }
    }

    /**
     * @brief Register a reference to a StorageService
     *
     * @param storage_service: a pointer to a StorageService
     */
    void Terminator::registerStorageService(StorageService *storage_service) {
      this->registerService(this->storage_services, storage_service);
    }

    /**
     * @brief Shutdown a list of StorageService references. If the StorageService has only one reference it will
     * be stopped, otherwise the counter is decreased.
     *
     * @param storage_services: a list of pointers to StorageService objects
     *
     * @throw std::runtime_error
     */
    void Terminator::shutdownStorageService(std::set<StorageService *> storage_services) {
      for (auto storage_service : storage_services) {
        this->shutdownStorageService(storage_service);
      }
    }

    /**
     * @brief Shutdown a StorageService. If the StorageService has only one reference it will be stopped,
     * otherwise the counter is decreased
     *
     * @param storage_service: a pointer to a StorageService to be stopped
     *
     * @throw std::runtime_error
     */
    void Terminator::shutdownStorageService(StorageService *storage_service) {
      if (this->shutdownService(this->storage_services, storage_service)) {
        WRENCH_INFO("Terminator shut down storage service: %s", storage_service->getName().c_str());
      }
    }

    /**
     * @brief Register a reference to a FileRegistryService
     *
     * @param file_registry_service: a pointer to a FileRegistryService
     */
    void Terminator::registerFileRegistryService(FileRegistryService *file_registry_service) {
      this->registerService(this->file_registry_services, file_registry_service);
    }

    /**
     * @brief Shutdown a FileRegistryService. If the FileRegistryService has only one reference it will be stopped,
     * otherwise the counter is decreased
     *
     * @param file_registry_service: a pointer to a FileRegistryService to be stopped
     *
     * @throw std::runtime_error
     */
    void Terminator::shutdownFileRegistryService(FileRegistryService *file_registry_service) {
      if (this->shutdownService(this->file_registry_services, file_registry_service)) {
        WRENCH_INFO("Terminator shut down file registry service: %s", file_registry_service->getName().c_str());
      }
    }

    /**
     * @brief Register a reference to a NetworkProximityService
     *
     * @param network_proximity_service: a pointer to a NetworkProximityService
     */
    void Terminator::registerNetworkProximityService(NetworkProximityService *network_proximity_service) {
      this->registerService(this->network_proximity_services, network_proximity_service);
    }

    /**
     * @brief Shutdown a NetworkProximityService. If the FileRegistryService has only one reference it will be stopped,
     * otherwise the counter is decreased
     *
     * @param network_proximity_service: a pointer to a NetworkProximityService to be stopped
     */
    void Terminator::shutdownNetworkProximityService(NetworkProximityService *network_proximity_service) {
      if (this->shutdownService(this->network_proximity_services, network_proximity_service)) {
        WRENCH_INFO("Terminator shut down network proximity service: %s", network_proximity_service->getName().c_str());
      }
    }

}
