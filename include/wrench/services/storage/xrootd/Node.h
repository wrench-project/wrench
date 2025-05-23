/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_XROOTD_NODE_H
#define WRENCH_XROOTD_NODE_H
#include "wrench/services/storage/simple/SimpleStorageService.h"
#include "wrench/services/storage/xrootd/XRootDMessagePayload.h"
#include <stack>
#include "wrench/services/storage/xrootd/Cache.h"

#include <wrench/services/storage/xrootd/XRootDProperty.h>

//todo overload mountpoint functions
namespace wrench {
    namespace XRootD {
        class Deployment;
        class SearchStack;
        /**
         * @brief An XRootD node, this can be either a supervisor or a storage server.
         * All nodes are classified as storage services even though not all have physical storage
         * Unless a node is also has an internal storage service, some normal storage service messages will error out.
         * Only File Read, locate, and delete are supported at this time, anything else requires talking directly to a specific file server with physical storage.
         * Nodes not directly be created, instead an XRootD Metavisor should create them
         */
        class Node : public StorageService {
        public:
            std::shared_ptr<Node> addChildSupervisor(const std::string &hostname);
            std::shared_ptr<Node> addChildStorageServer(const std::string &hostname, const std::string &mount_point,
                                                        WRENCH_PROPERTY_COLLECTION_TYPE storage_property_list = {}, WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE storage_messagepayload_list = {},
                                                        WRENCH_PROPERTY_COLLECTION_TYPE node_property_list = {}, WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE node_messagepayload_list = {});

            std::shared_ptr<Node> getChild(unsigned int n);
            Node *getParent() const;

            /***********************/
            /** \cond DEVELOPER    */
            /***********************/

            using StorageService::createFile;
            using StorageService::deleteFile;
            using StorageService::hasFile;
            using StorageService::lookupFile;
            using StorageService::readFile;
            using StorageService::removeFile;
            using StorageService::writeFile;

            void createFile(const std::shared_ptr<FileLocation> &location) override;
            void removeFile(const std::shared_ptr<FileLocation> &location) override;

            virtual void writeFile(S4U_CommPort *answer_commport,
                                   const std::shared_ptr<FileLocation> &location,
                                   sg_size_t num_bytes_to_write,
                                   bool wait_for_answer) override;

            virtual double getFileLastWriteDate(const std::shared_ptr<FileLocation> &location) override;

            virtual sg_size_t getTotalSpace() override;
            virtual bool isBufferized() const override;
            virtual sg_size_t getBufferSize() const override;
            virtual bool reserveSpace(std::shared_ptr<FileLocation> &location) override;
            virtual void unreserveSpace(std::shared_ptr<FileLocation> &location) override;


            /***********************/
            /** \endcond           */
            /***********************/

        private:
            WRENCH_PROPERTY_COLLECTION_TYPE default_property_values = {
                    {Property::MESSAGE_OVERHEAD, "1"},
                    {Property::CACHE_LOOKUP_OVERHEAD, "1"},
                    {Property::SEARCH_BROADCAST_OVERHEAD, "1"},
                    {Property::UPDATE_CACHE_OVERHEAD, "1"},
                    {Property::CACHE_MAX_LIFETIME, "infinity"},
                    {StorageServiceProperty::BUFFER_SIZE, "1000000"},
                    {Property::REDUCED_SIMULATION, "false"},
                    {Property::FILE_NOT_FOUND_TIMEOUT, "30"}};

            WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE default_messagepayload_values = {
                    {MessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                    {MessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                    {MessagePayload::FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                    {MessagePayload::FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                    {MessagePayload::UPDATE_CACHE, S4U_CommPort::default_control_message_size},
                    {MessagePayload::CONTINUE_SEARCH, S4U_CommPort::default_control_message_size},
                    {MessagePayload::FILE_READ_ANSWER_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                    {MessagePayload::FILE_READ_REQUEST_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                    {MessagePayload::FILE_SEARCH_ANSWER_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                    {MessagePayload::CACHE_ENTRY, S4U_CommPort::default_control_message_size},
                    {MessagePayload::FILE_DELETE_REQUEST_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                    {MessagePayload::FILE_DELETE_ANSWER_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                    {MessagePayload::FILE_WRITE_ANSWER_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                    {MessagePayload::FILE_WRITE_REQUEST_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                    {MessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                    {MessagePayload::FILE_COPY_REQUEST_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size}};

        public:
            /***********************/
            /** \cond DEVELOPER    */
            /***********************/

            bool hasFile(const std::shared_ptr<FileLocation> &location) override;

            double getLoad() override;

            void removeDirectory(const std::string &path) override;

            /***********************/
            /** \endcond           */
            /***********************/


            /***********************/
            /** \cond INTERNAL     */
            /***********************/

            std::shared_ptr<SimpleStorageService> getStorageServer();

            bool cached(shared_ptr<DataFile> file);
            std::set<std::shared_ptr<FileLocation>> getCached(const shared_ptr<DataFile>& file);


            int main() override;
            bool processNextMessage();
            Node(Deployment *deployment, const std::string &hostname, const WRENCH_PROPERTY_COLLECTION_TYPE& property_list, const WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE& messagepayload_list);

            /**
            * @brief Return the storage service's default mountpoint, if any.
            * If none, throws an std::runtime_error exception
            */
            virtual std::string getMountPoint() override {
                if (this->internalStorage) {
                    return this->internalStorage->getMountPoint();
                } else {
                    return "/";
                }
            }

            /**
             * @brief Return the storage service's mountpoints. If none, throws
             * an std::runtime_error exception
             */
            virtual std::set<std::string> getMountPoints() override {
                if (this->internalStorage) {
                    return this->internalStorage->getMountPoints();
                } else {
                    std::set<std::string> mps = {"/"};
                    return mps;
                }
            }

            /** @brief Retrieve the simple storage service's file system object **/
            virtual std::shared_ptr<simgrid::fsmod::FileSystem> getFileSystem() override {
                if (this->internalStorage) {
                    return this->internalStorage->getFileSystem();
                } else {
                    throw std::runtime_error("Node::getFileSystem(): No file system");
                }
            }

        private:
            Deployment *deployment;

            std::shared_ptr<Node> addChild(std::shared_ptr<Node> child);

            /** @brief Fictitious file system */
            std::shared_ptr<simgrid::fsmod::FileSystem> file_system;

            map<Node *, vector<stack<Node *>>> splitStack(const vector<stack<Node *>>& search_stack);
            virtual std::shared_ptr<FileLocation> selectBest(std::set<std::shared_ptr<FileLocation>> locations);
            vector<stack<Node *>> constructFileSearchTree(const vector<shared_ptr<Node>> &targets);
            stack<Node *> constructSearchStack(Node *target) const;
            //std::shared_ptr<FileLocation> hasFile(shared_ptr<DataFile> file);

            bool makeSupervisor();
            bool makeFileServer(std::set<std::string> path, WRENCH_PROPERTY_COLLECTION_TYPE property_list,
                                WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE messagepayload_list);

            /** @brief A pointer to the internal file storage, IF it exists */
            std::shared_ptr<SimpleStorageService> internalStorage = nullptr;
            /** @brief A vector of all children If the node has any */
            std::vector<std::shared_ptr<Node>> children;
            /** @brief The internal cache of files within the subtree */
            Cache cache;
            /** @brief The supervisor of this node.  All continue search and ripple delete messages should come from this, and all update cache messages go to this server */
            Node *supervisor = nullptr;
            /** @brief The Meta supervisor for this entire XRootD data federation */
            Deployment *metavisor = nullptr;
            /** @brief Whether this node is running a reduced simulation.  Initilized from the properties in main */
            bool reduced;
            friend Deployment;
            friend SearchStack;
            /***********************/
            /** \endcond           */
            /***********************/
        };
    }// namespace XRootD
}// namespace wrench
#endif//WRENCH_XROOTD_NODE_H
