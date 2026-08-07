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

#include <utility>
#include <vector>
#include <map>
#include <queue>
#include <set>
#include <memory>
#include <string>
#include <wrench/services/compute/serverless/Invocation.h>
#include <wrench/services/storage/StorageService.h>
#include <wrench/data_file/DataFile.h>

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
        ServerlessComputeNode(std::string  h, unsigned int c) :
            hostname(std::move(h)), total_cores(c), available_cores(c) {}

        std::string hostname;
        unsigned long total_cores;
        unsigned int available_cores;

        std::shared_ptr<SimpleStorageService> disk;
        std::shared_ptr<SimpleStorageService> memory;

        std::set<std::shared_ptr<DataFile>> images_being_copied;
        std::set<std::shared_ptr<DataFile>> images_being_loaded;

//        std::unordered_map<ContainerId, std::shared_ptr<ServerlessContainer>> containers;
    };

    /***********************/
    /** \endcond           */
    /***********************/

} // namespace wrench

#endif // WRENCH_SERVERLESSCOMPUTENODE_H
