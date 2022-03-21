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
//#include "wrench/services/storage/StorageService.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include "wrench/data_file/DataFile.h"
namespace wrench {
    namespace XRootD{
        class StorageServer;
        class Supervisor;
        class Node;

        class XRootD{
        public:
            std::shared_ptr<Node> createStorageServer(const std::string& hostname,const std::string& mount_points);
            std::shared_ptr<Node> createSupervisor(const std::string& hostname);
            std::shared_ptr<Node> createStorageSupervisor(const std::string& hostname,const std::string& mount_points);

        private:
            std::vector<std::shared_ptr<Node>> nodes;
            std::vector<std::shared_ptr<StorageServer>> dataservers;
            std::vector<std::shared_ptr<Supervisor>> supervisors;
            std::unordered_map<std::shared_ptr<DataFile> ,std::vector<std::shared_ptr<Node>>> files;
        };
    }
}
#endif //WRENCH_XROOTD_H
