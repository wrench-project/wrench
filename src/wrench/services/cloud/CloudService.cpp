/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include "wrench/services/cloud/CloudService.h"
#include "wrench/services/compute/MulticoreComputeService.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"

namespace wrench {

    static unsigned long VM_ID = 1;

    /**
     * @brief Constructor
     */
    CloudService::CloudService(std::string &hostname) :
            Service("cloud_service", "cloud_service") {

      this->hostname = hostname;
    }

    /**
     * @brief
     *
     * @param pm_hostname: the name of the physical machine host
     * @param num_cores: the number of cores the service can use (0 means "use as many as there are cores on the host")
     * @param default_storage_service: a storage service
     * @return
     */
    std::unique_ptr<ComputeService> CloudService::createVM(std::string pm_hostname,
                                                           int num_cores,
                                                           StorageService *default_storage_service) {

      std::string vm_name = "vm" + std::to_string(VM_ID++) + "_" + pm_hostname;
      if (simgrid::s4u::Host::by_name_or_null(vm_name) == nullptr) {

        if (num_cores <= 0) {
          num_cores = S4U_Simulation::getNumCores(hostname);
        }

        this->vm_list[vm_name] = new simgrid::s4u::VirtualMachine(vm_name.c_str(),
                                                                  simgrid::s4u::Host::by_name(pm_hostname),
                                                                  num_cores);

        return std::unique_ptr<MulticoreComputeService>(
                new MulticoreComputeService(vm_name, true, true,
                                            default_storage_service,
                                            {{wrench::MulticoreComputeServiceProperty::STOP_DAEMON_MESSAGE_PAYLOAD, "666"}}));
      }
      //TODO: launch exception if vm already created or cannot be created
      return nullptr;
    }

    int CloudService::main() {
      return 0;
    }
}
