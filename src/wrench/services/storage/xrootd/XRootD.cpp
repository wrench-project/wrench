/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <wrench/services/storage/xrootd/Node.h>
#include <wrench/services/storage/xrootd/XRootD.h>
namespace wrench {
    namespace XRootD{

        std::shared_ptr<Node> XRootD::createStorageServer(const std::string& hostname,const std::string& mount_points){}
        std::shared_ptr<Node> XRootD::createSupervisor(const std::string& hostname){}
        std::shared_ptr<Node> XRootD::createStorageSupervisor(const std::string& hostname,const std::string& mount_points){}


    }
}
