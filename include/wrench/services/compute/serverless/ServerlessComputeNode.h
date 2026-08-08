/**
 * Copyright (c) 2025. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_SERVERLESSCOMPUTENODE_H
#define WRENCH_SERVERLESSCOMPUTENODE_H

#include <set>
#include <memory>
#include <string>
#include <wrench/data_file/DataFile.h>
#include <wrench/services/compute/serverless/Container.h>

namespace wrench
{

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief A class that stores the state of a serverless compute node
     */
    class ServerlessComputeNode {

    public:
        ServerlessComputeNode(std::string h, unsigned int num_cores, ServerlessComputeService *service);

        std::shared_ptr<Container> spawnContainer(RegisteredFunction *registered_function);
        void makeContainerIdle(const std::shared_ptr<Container>& container);
        void makeContainerBusy(const std::shared_ptr<Container>& container);
        void shutdownContainer(const std::shared_ptr<Container>& container);

        std::string hostname;
        unsigned long total_cores;
        unsigned int available_cores;

        std::shared_ptr<SimpleStorageService> disk;
        std::shared_ptr<SimpleStorageService> memory;

        std::set<std::shared_ptr<DataFile>> images_being_copied;
        std::set<std::shared_ptr<DataFile>> images_being_loaded;

        std::set<std::shared_ptr<Container>> busy_containers;
        std::set<std::shared_ptr<Container>> idle_containers;

        ServerlessComputeService *serverless_compute_service;
    };

    /***********************/
    /** \endcond           */
    /***********************/

} // namespace wrench

#endif // WRENCH_SERVERLESSCOMPUTENODE_H
