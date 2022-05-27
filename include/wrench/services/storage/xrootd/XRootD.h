/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_XROOTD_H
#define WRENCH_XROOTD_H
#include "wrench/services/Service.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include "wrench/data_file/DataFile.h"
#include <set>
namespace wrench {
    namespace XRootD{
        //class StorageServer;
        //class Supervisor;
        class Node;

        class XRootD{
        public:
            int defaultTimeToLive=1024;//how long trivial search message can wander for;
            std::shared_ptr<Node> createStorageServer(const std::string& hostname,WRENCH_PROPERTY_COLLECTION_TYPE property_list,WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list);
            std::shared_ptr<Node> createSupervisor(const std::string& hostname);
            std::shared_ptr<Node> createStorageSupervisor(const std::string& hostname,WRENCH_PROPERTY_COLLECTION_TYPE property_list, WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list);

            unsigned int size();
        private:
            friend Node;
            std::vector<std::shared_ptr<Node>> getFileNodes(std::shared_ptr<DataFile> file);
            std::shared_ptr<Node> createNode(const std::string& hostname);

            std::vector<std::shared_ptr<Node>> nodes;
            std::vector<std::shared_ptr<Node>> dataservers;
            std::vector<std::shared_ptr<Node>> supervisors;
            std::unordered_map<std::shared_ptr<DataFile> ,std::vector<std::shared_ptr<Node>>> files;
        };
    }
}
#endif //WRENCH_XROOTD_H
