/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <wrench/services/storage/xrootd/Node.h>
#include <wrench/services/storage/xrootd/Deployment.h>
namespace wrench {
    namespace XRootD {

        /**
        * @brief Create the XRootD Node that will be the root supervisor
        * @param hostname: the name of the host on which the service should run
        * @param node_property_list: The property list to use for the new Node, defaluts to {}
        * @param node_messagepayload_list: The message payload list to use for the new Node, defaults to {}
        *
        * @return a shared pointer to the newly created Node
        */
        std::shared_ptr<Node> Deployment::createRootSupervisor(const std::string &hostname,
                                                               WRENCH_PROPERTY_COLLECTION_TYPE node_property_list,
                                                               WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE node_messagepayload_list) {
            if (this->root_supervisor) {
                throw std::runtime_error("Deployment::createRootSupervisor(): A Root supervisor has already been created for this XRootD deployment");
            }
            std::shared_ptr<Node> ret = createNode(hostname, node_property_list, node_messagepayload_list);
            ret->makeSupervisor();
            supervisors.push_back(ret);
            this->root_supervisor = ret;
            return ret;
        }

        /**
         * @brief Get the deployment's root supervisor
         * @return the root supervisor
         */
        std::shared_ptr<Node> Deployment::getRootSupervisor() {
            return this->root_supervisor;
        }

        /**
        * @brief Create the an XRootD Node that will be a supervisor
        *
        * @param hostname: the name of the host on which the service should run
        * @param node_property_list: The property list to use for the new Node, defaluts to {}
        * @param node_messagepayload_list: The message payload list to use for the new Node, defaults to {}
        *
        * @return a shared pointer to the newly created Node
        */
        std::shared_ptr<Node> Deployment::createSupervisor(const std::string &hostname,
                                                           WRENCH_PROPERTY_COLLECTION_TYPE node_property_list,
                                                           WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE node_messagepayload_list) {
            std::shared_ptr<Node> ret = createNode(hostname, node_property_list, node_messagepayload_list);
            ret->makeSupervisor();
            supervisors.push_back(ret);
            return ret;
        }

        /**
        * @brief Create a new XRootD Node that is a leaf storage server
        * @param hostname: the name of the host on which the service and its storage service should run
        * @param mount_point: the single mountpoint of the disk on which files will be stored
        * @param storage_property_list: The property list to use for the underlying storage server
        * @param storage_messagepayload_list: The message payload list to use for the underlying storage server
        * @param node_property_list: The property list to use for the new Node, defaluts to {}
        * @param node_messagepayload_list: The message payload list to use for the new Node, defaults to {}
        *
        * @return a shared pointer to the newly created Node
        */
        std::shared_ptr<Node> Deployment::createStorageServer(
                const std::string &hostname,
                const std::string &mount_point,
                WRENCH_PROPERTY_COLLECTION_TYPE storage_property_list,
                WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE storage_messagepayload_list,
                WRENCH_PROPERTY_COLLECTION_TYPE node_property_list,
                WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE node_messagepayload_list) {
            std::shared_ptr<Node> ret = createNode(hostname, node_property_list, node_messagepayload_list);
            ret->makeFileServer({mount_point}, storage_property_list, storage_messagepayload_list);
            simulation->add(ret->internalStorage);
            dataservers.push_back(ret);
            return ret;
        }
        /**
        * @brief Create a new blank XRootD Node.  The newly created Node is neither a supervisor nor a storage server.  This function should not be called directly.
        * @param hostname: the name of the host on which the service and its storage service should run
        * @param property_list_override: The property list to use for the new Node
        * @param messagepayload_list_override: The message payload list to use for the new Node
        * @return a shared pointer to the newly created Node
        */
        std::shared_ptr<Node> Deployment::createNode(const std::string &hostname, WRENCH_PROPERTY_COLLECTION_TYPE property_list_override, WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list_override) {
            WRENCH_PROPERTY_COLLECTION_TYPE properties = property_values;
            WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE payloads = messagepayload_values;
            for (auto property: property_list_override) {//override XRootD default properties with suplied properties for this node
                properties[property.first] = property.second;
            }
            for (auto property: messagepayload_list_override) {//override XRootD default message payload with suplied properties for this node
                payloads[property.first] = property.second;
            }
            std::shared_ptr<Node> ret = make_shared<Node>(this, hostname, properties, payloads);
            ret->metavisor = this;
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
        std::vector<std::shared_ptr<Node>> Deployment::getFileNodes(std::shared_ptr<DataFile> file) {
            return files[file];
        }
        /**
        * @brief get the size of the XRootD federation
        *
        * @return the number of nodes in the federation
        */
        unsigned int Deployment::size() {
            return nodes.size();
        }


        //        /**
        //        * @brief create a new file in the federation.  Use instead of wrench::Simulation::createFile when adding files to XRootD
        //        * @param file: A shared pointer to a file
        //        * @param location: A shared pointer to the Node to put the file on.  The Node MUST be a storage server
        //        *
        //        * @throw std::invalid_argument
        //        */
        //         void Deployment::createFile(const std::shared_ptr<DataFile> &file, const std::shared_ptr<Node> &location) {
        //            if(file==nullptr){
        //                throw std::invalid_argument("XRootD::createFile(): The file can not be null");
        //            }else if(location==nullptr){
        //                throw std::invalid_argument("XRootD::createFile(): The location can not be null");
        //            }else if(location->internalStorage==nullptr) {
        //                throw std::invalid_argument("XRootD::createFile(): The location must be a storage service");
        //            }
        //            location->createFile(file);
        //        }

        /**
        * @brief remove a specific file from the registry.  DOES NOT REMOVE FILE FROM SERVERS
        * @param file: A shared pointer to the file to remove
        *
        * @throw std::invalid_argument
        */
        void Deployment::deleteFile(const std::shared_ptr<DataFile> &file) {
            files.erase(file);
        }
        /**
        * @brief remove a specific file location from the registry.  DOES NOT REMOVE FILE FROM SERVER
        * @param file: A shared pointer to the file the location is for
        * @param location: The location to remove
        *
        * @throw std::invalid_argument
        */
        void Deployment::removeFileLocation(const std::shared_ptr<DataFile> &file, const std::shared_ptr<Node> &location) {
            if (file == nullptr) {
                throw std::invalid_argument("XRootD::createFile(): The file can not be null");
            }
            std::remove(files[file].begin(), files[file].end(), location);
        }

    }// namespace XRootD
}// namespace wrench
