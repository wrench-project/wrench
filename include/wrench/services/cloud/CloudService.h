/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef WRENCH_CLOUDSERVICE_H
#define WRENCH_CLOUDSERVICE_H

#include <simgrid/s4u/VirtualMachine.hpp>

#include "wrench/services/Service.h"
#include "wrench/services/compute/ComputeService.h"

namespace wrench {

    class CloudService : public Service {

        /**
         * @brief A Cloud Service
         */

    public:
        explicit CloudService(std::string &hostname);

        std::unique_ptr<ComputeService> createVM(std::string pm_hostname,
                                                 int num_cores,
                                                 StorageService *default_storage_service);


    private:
        int main();

        std::map<std::string, std::string> default_property_values = {};

        std::map<std::string, simgrid::s4u::VirtualMachine *> vm_list;
    };

}

#endif //WRENCH_CLOUDSERVICE_H
