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
namespace wrench {
    namespace XRootD{
        class XRootD;
        class SearchStack;
        class Node:public StorageService{
        private:
            WRENCH_PROPERTY_COLLECTION_TYPE default_property_values = {
            };

            WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE default_messagepayload_values = {
                    {MessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD, 1024},
                    {MessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD, 1024},
                    {MessagePayload::FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD, 1024},
                    {MessagePayload::UPDATE_CACHE, 1024},
                    {MessagePayload::CONTINUE_SEARCH, 1024},
                    {MessagePayload::FILE_READ_REQUEST_MESSAGE_PAYLOAD, 1024},
                    {MessagePayload::FILE_SEARCH_ANSWER_MESSAGE_PAYLOAD, 1024}
            };
        public:
            //XRootD* getMetavisor();

            std::shared_ptr<SimpleStorageService> getStorageServer();
            Node* getChild(unsigned int n);
            Node* getParent();
            int main();
            bool processNextMessage();
            bool lookupFile(std::shared_ptr<DataFile>file);
            void deleteFile(std::shared_ptr<DataFile>file);//meta delete from sub tree
            void readFile(std::shared_ptr<DataFile>file);
            void readFile(std::shared_ptr<DataFile>file, double num_bytes);

            bool cached(shared_ptr<DataFile> file);
            std::set<std::shared_ptr<FileLocation>> getCached(shared_ptr<DataFile> file);
            int addChild(std::shared_ptr<Node> child);
            Node(const std::string& hostname);
        private:
            std::shared_ptr<FileLocation> hasFile(shared_ptr<DataFile> file);

            bool makeSupervisor();
            bool makeFileServer(std::set <std::string> path,WRENCH_PROPERTY_COLLECTION_TYPE property_list,
                                WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list);
            std::shared_ptr<SimpleStorageService> internalStorage=nullptr;
            std::vector<std::shared_ptr<Node>> children;
            Cache cache;
            Node* supervisor=nullptr;
            XRootD* metavisor=nullptr;
            friend XRootD;
            friend SearchStack;
        };
    }
}
#endif //WRENCH_XROOTD_NODE_H
