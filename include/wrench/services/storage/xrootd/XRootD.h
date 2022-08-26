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
#include "wrench/simulation/Simulation.h"
#include "wrench/services/storage/xrootd/Node.h"

namespace wrench {
    /**
     * @brief A Meta manager for an XRootD data Federation.  This tracks all nodes and files within the system.
     */
    namespace XRootD{
        //class StorageServer;
        //class Supervisor;
//        class Node;

        class XRootD{
        public:

            /**
             * @brief Create an XRootD manager
             * @param simulation: the simulation that all nodes run in.  Nodes are automatically added to this simulation as created.
             * @param property_values: The property values that should be used to overwrite the defaults of all Nodes (defaults to none) (unless otherwise specified)
             * @param messagepayload_values: The message paylaod values that should be used to overwrite the defaults of all Nodes (defaults to none) (unless otherwise specified)
             */
            XRootD(std::shared_ptr<Simulation>  simulation,WRENCH_PROPERTY_COLLECTION_TYPE property_values={},WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_values={}):property_values(property_values),messagepayload_values(messagepayload_values),simulation(simulation){}
            std::shared_ptr<Node> createStorageServer(const std::string& hostname, const std::string& mount_point, WRENCH_PROPERTY_COLLECTION_TYPE storage_property_list,WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE storage_messagepayload_list,WRENCH_PROPERTY_COLLECTION_TYPE node_property_list={},WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE node_messagepayload_list={});
            std::shared_ptr<Node> createSupervisor(const std::string& hostname,WRENCH_PROPERTY_COLLECTION_TYPE node_property_list={},WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE node_messagepayload_list={});
            std::shared_ptr<Node> createStorageSupervisor(const std::string& hostname,WRENCH_PROPERTY_COLLECTION_TYPE storage_property_list, WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE storage_messagepayload_list,WRENCH_PROPERTY_COLLECTION_TYPE node_property_list={},WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE node_messagepayload_list={});
            ~XRootD(){}
            /***********************/
            /** \cond DEVELOPER    */
            /***********************/
            /** @brief The max number of hops a search message can take.  Used to prevent infinite loops in a poorly constructed XRootD tree. */
            int defaultTimeToLive=1024;//how long trivial search message can wander for;
            
            virtual void createFile(const std::shared_ptr<DataFile> &file, const std::shared_ptr<Node> &location);
            void deleteFile(const std::shared_ptr<DataFile> &file);
            void removeFileLocation(const std::shared_ptr<DataFile> &file, const std::shared_ptr<Node> &location);
            unsigned int size();
        private:

            /** @brief The property values that should be used to overwrite the defaults of all Nodes (unless otherwise specified) */
            WRENCH_PROPERTY_COLLECTION_TYPE property_values = {};

            /** @brief The message paylaod values that should be used to overwrite the defaults of all Nodes (unless otherwise specified) */
            WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_values = {};
            /***********************/
            /** \cond INTERNAL     */
            /***********************/
            friend Node;
            std::vector<std::shared_ptr<Node>> getFileNodes(std::shared_ptr<DataFile> file);
            std::shared_ptr<Node> createNode(const std::string& hostname,WRENCH_PROPERTY_COLLECTION_TYPE property_list_override,WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list_override);
            /** @brief All nodes that are connected to this XRootD data Federation */
            std::vector<std::shared_ptr<Node>> nodes;
            /** @brief All nodes in the XRootD Federation that have and internal file server */
            std::vector<std::shared_ptr<Node>> dataservers;
            /** @brief All nodes in the XRootD Federation that have child nodes */
            std::vector<std::shared_ptr<Node>> supervisors;
            /** @brief All files within the data federation regardless of which server */
            std::unordered_map<std::shared_ptr<DataFile> ,std::vector<std::shared_ptr<Node>>> files;
            /** @brief The simulation that this XRootD federation is connected too */
            std::shared_ptr<Simulation>  simulation;
            /***********************/
            /** \endcond           */
            /***********************/
        };
    }
}
#endif //WRENCH_XROOTD_H
