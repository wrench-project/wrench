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
        /**
        * @brief Create a new XRootD Node that is both a storage server and a supervisor
        * @param hostname: the name of the host on which the service and its storage service should run
        * @param storage_property_list: The property list to use for the Node AND the underlying storage server
        * @param storage_messagepayload_list: The message payload list to use for the Node AND the underlying storage server
        *
        * @return a shared pointer to the newly created Node
        */
        std::shared_ptr<Node> XRootD::createStorageSupervisor(const std::string& hostname,WRENCH_PROPERTY_COLLECTION_TYPE storage_property_list,WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE storage_messagepayload_list){
            std::shared_ptr<Node> ret=createNode(hostname);
            ret->makeSupervisor();
            supervisors.push_back(ret);
            ret->makeFileServer({"/"},storage_property_list,storage_messagepayload_list);
            simulation->add(ret->internalStorage);
            dataservers.push_back(ret);
            return ret;
        }
        /**
        * @brief Create a new XRootD Node that is a supervisor
        * @param hostname: the name of the host on which the service should run
        * @param property_list: The property list to use for the supervisor
        * @param messagepayload_list: The message payload list to use for supervisor
        *
        * @return a shared pointer to the newly created Node
        */
        std::shared_ptr<Node> XRootD::createSupervisor(const std::string& hostname){
            std::shared_ptr<Node> ret=createNode(hostname);
            ret->makeSupervisor();
            supervisors.push_back(ret);
            return ret;
        }
        /**
        * @brief Create a new XRootD Node that is a leaf storage server
        * @param hostname: the name of the host on which the service and its storage service should run
        * @param storage_property_list: The property list to use for the underlying storage server
        * @param storage_messagepayload_list: The message payload list to use for the underlying storage server
        *
        * @return a shared pointer to the newly created Node
        */
        std::shared_ptr<Node> XRootD::createStorageServer(const std::string& hostname,WRENCH_PROPERTY_COLLECTION_TYPE storage_property_list,WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE storage_messagepayload_list){
            auto ret=createNode(hostname);
            ret->makeFileServer({"/"},storage_property_list,storage_messagepayload_list);
            simulation->add(ret->internalStorage);
            dataservers.push_back(ret);
            return ret;
        }
        /**
        * @brief Create a new blank XRootD Node.  The newly created Node is neither a supervisor nor a storage server.  This function should not be called directly.
        * @param hostname: the name of the host on which the service and its storage service should run
        *
        * @return a shared pointer to the newly created Node
        */
        std::shared_ptr<Node> XRootD::createNode(const std::string& hostname){
            std::shared_ptr<Node> ret= make_shared<Node>(hostname);
            ret->metavisor=this;
            nodes.push_back(ret);
            simulation->add(ret);
            return ret;
        }
        /**
        * @brief Meta operation to get all File servers in the federation that have the file specified, intended for advanced file search simulation optimization.
        * @param file: A shared pointer to the file to search for
        *
        * @return vector of Node pointers that are all File Servers with the file
        */
        std::vector<std::shared_ptr<Node>> XRootD::getFileNodes(std::shared_ptr<DataFile> file){
            return files[file];

        }
        /**
        * @brief get the size of the XRootD federation
        *
        * @return the number of nodes in the federation
        */
        unsigned int XRootD::size(){
            return nodes.size();

        }
        /**
        * @brief create a new file to the federation.  Use instead of wrench::Simulation::createFile when adding files to XRootD
        * @param file: A shared pointer to the file to add
        * @param location: A shared pointer to the Node to put the file on.  The Node MUST be a storage server
        *
        * @throw std::invalid_argument
        */
        void XRootD::createFile(const std::shared_ptr<DataFile> &file, const std::shared_ptr<Node> &location){
            if(file==nullptr){
                throw std::invalid_argument("XRootD::createFile(): The file can not be null");
            }else if(location==nullptr){
                throw std::invalid_argument("XRootD::createFile(): The location can not be null");
            }else if(location->internalStorage==nullptr) {
                throw std::invalid_argument("XRootD::createFile(): The location must be a storage service");
            }
            files[file].push_back(location);
            wrench::Simulation::createFile(file, location->getStorageServer());

        }
    }
}
