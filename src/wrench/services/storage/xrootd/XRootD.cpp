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

        std::shared_ptr<Node> XRootD::createStorageServer(const std::string& hostname,std::set <std::string> path,WRENCH_PROPERTY_COLLECTION_TYPE property_list,WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list){
            std::shared_ptr<Node> ret=createNode(hostname);
            ret->makeSupervisor();
            supervisors.push_back(ret);
            ret->makeFileServer(path,property_list,messagepayload_list);
            dataservers.push_back(ret);
            return ret;
        }
        std::shared_ptr<Node> XRootD::createSupervisor(const std::string& hostname){
            std::shared_ptr<Node> ret=createNode(hostname);
            ret->makeSupervisor();
            supervisors.push_back(ret);
            return ret;
        }
        std::shared_ptr<Node> XRootD::createStorageSupervisor(const std::string& hostname,std::set <std::string> path,WRENCH_PROPERTY_COLLECTION_TYPE property_list,WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list){
            auto ret=createNode(hostname);
            ret->makeFileServer(path,property_list,messagepayload_list);
            dataservers.push_back(ret);
            return ret;
        }

        std::shared_ptr<Node> XRootD::createNode(const std::string& hostname){
            std::shared_ptr<Node> ret= make_shared<Node>(hostname);
            ret->metavisor=this;
            nodes.push_back(ret);
            return ret;
        }
        std::vector<std::shared_ptr<Node>> XRootD::getFileNodes(std::shared_ptr<DataFile> file){
            return files[file];

        }

    }
}
