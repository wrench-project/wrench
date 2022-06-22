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
    namespace XRootD{
        class XRootD;
        class SearchStack;
        /**
         * @brief An XRootD node, this can be either a supervisor or a storage server.
         * All nodes are classified as storage services even though not all have physical storage
         * Unless a node is also has an internal storage service, some normal storage service messages will error out.
         * Only File Read, locate, and delete are supported at this time, anything else requires talking directly to a specific file server with physical storage.
         */
        class Node:public StorageService{
        /***********************/
        /** \cond DEVELOPER    */
        /***********************/
        private:
            WRENCH_PROPERTY_COLLECTION_TYPE default_property_values = {
                    {Property::MESSAGE_OVERHEAD,"1"},
                    {Property::CACHE_LOOKUP_OVERHEAD,"1"},
                    {Property::SEARCH_BROADCAST_OVERHEAD,"1"},
                    {Property::UPDATE_CACHE_OVERHEAD,"1"}
            };

            WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE default_messagepayload_values = {
                    {MessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD, 1024},
                    {MessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD, 1024},
                    {MessagePayload::FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD, 1024},
                    {MessagePayload::UPDATE_CACHE, 1024},
                    {MessagePayload::CONTINUE_SEARCH, 1024},
                    {MessagePayload::FILE_READ_REQUEST_MESSAGE_PAYLOAD, 1024},
                    {MessagePayload::FILE_SEARCH_ANSWER_MESSAGE_PAYLOAD, 1024},
                    {MessagePayload::CACHE_ENTRY,1024},
                    {MessagePayload::FILE_DELETE_REQUEST_MESSAGE_PAYLOAD,1024},
                    {MessagePayload::FILE_DELETE_ANSWER_MESSAGE_PAYLOAD,1024}
            };
        public:
            //XRootD* getMetavisor();

            std::shared_ptr<SimpleStorageService> getStorageServer();
            std::shared_ptr<Node> getChild(unsigned int n);
            Node* getParent();
            int main();
            bool processNextMessage();
            //bool lookupFile(std::shared_ptr<DataFile>file);
            //void deleteFile(std::shared_ptr<DataFile>file);//meta delete from sub tree
            //void readFile(std::shared_ptr<DataFile>file);
            //void readFile(std::shared_ptr<DataFile>file, double num_bytes);

           // std::string getMountPoint() override;
            //std::set<std::string> getMountPoints() override;
            //bool hasMultipleMountPoints() override;
            //bool hasMountPoint(const std::string &mp) override;

            bool cached(shared_ptr<DataFile> file);
            std::set<std::shared_ptr<FileLocation>> getCached(shared_ptr<DataFile> file);
            int addChild(std::shared_ptr<Node> child);
            Node(const std::string& hostname);
            double getLoad() override;
            void createFile(const std::shared_ptr<DataFile> &file);
            /***********************/
            /** \cond INTERNAL     */
            /***********************/

        private:
            static std::shared_ptr<FileLocation> selectBest(std::set<std::shared_ptr<FileLocation>> locations);
            std::shared_ptr<FileLocation> hasFile(shared_ptr<DataFile> file);

            bool makeSupervisor();
            bool makeFileServer(std::set <std::string> path,WRENCH_PROPERTY_COLLECTION_TYPE property_list,
                                WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list);

            /** @brief A pointer to the internal file storage, IF it exists */
            std::shared_ptr<SimpleStorageService> internalStorage=nullptr;
            /** @brief A vector of all children If the node has any */
            std::vector<std::shared_ptr<Node>> children;
            /** @brief The internal cache of files within the subtree */
            Cache cache;
            /** @brief The supervisor of this node.  All continue search and ripple delete messages should come from this, and all update cache messages go to this server */
            Node* supervisor=nullptr;
            /** @brief The Meta supervisor for this entire XRootD data federation */
            XRootD* metavisor=nullptr;
            friend XRootD;
            friend SearchStack;
            /***********************/
            /** \endcond           */
            /***********************/
        };
    }
}
#endif //WRENCH_XROOTD_NODE_H
