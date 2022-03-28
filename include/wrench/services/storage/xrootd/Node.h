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
#include <stack>
namespace wrench {
    namespace XRootD{
        class XRootD;
        class Node:public Service{//Conceptualy all nodes ARE storage services, HOWEVER, the API is entirly differnt for accessing a file in an XRootD deployment than usuall.
        public:
            Node(const std::string& hostname);
            std::shared_ptr<SimpleStorageService> getStorageServer();
            Node* getChild(int n);
            Node* getParrent();
            int main();
            bool lookupFile(std::shared_ptr<DataFile>file);
            void deleteFile(std::shared_ptr<DataFile>file);//meta delete from sub tree
            void readFile(std::shared_ptr<DataFile>file);
            void readFile(std::shared_ptr<DataFile>file, double num_bytes);
            //void writeFile(std::shared_ptr<DataFile>file);//unclear how this would work, do we write to 1 existing file then let the background clone it?


            //utility
            std::shared_ptr<FileLocation> traverse(std::stack<Node*> nodes,std::shared_ptr<DataFile> file,bool meta=false);//fake a search for the file, adding to the cache as we return

            std::vector<std::shared_ptr<FileLocation>> traverse(std::vector<std::stack<Node*>> nodes,std::shared_ptr<DataFile> file,bool meta=false);//"search" multiple paths that go through this supreviser in parallel
            //meta

            std::stack<Node*> search(Node* other);//returns the path of nodes between here and other IF other is in this subtree.
            bool cached(shared_ptr<DataFile> file);
            std::vector<std::shared_ptr<FileLocation>> getCached(shared_ptr<DataFile> file);
        private:
            std::vector<shared_ptr<FileLocation>> XRootDSearch(shared_ptr<DataFile> file);
            std::vector< std::stack<Node*>> searchAll(std::vector<std::shared_ptr<Node>> potential);
            bool makeSupervisor();
            bool makeFileServer(std::set <std::string> path,WRENCH_PROPERTY_COLLECTION_TYPE property_list,
                                WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list);
            std::shared_ptr<SimpleStorageService> internalStorage=nullptr;
            std::shared_ptr<std::shared_ptr<Node>> children=nullptr;
            int numChildren=-1;
            std::unordered_map< std::shared_ptr<DataFile>, std::vector<std::shared_ptr<FileLocation>>> cache;//probiably change the payload of this to an object containing the file location AND its queue time stamp
            Node* supervisor=nullptr;
            XRootD* metavisor=nullptr;
            friend XRootD;

        };
    }
}
#endif //WRENCH_XROOTD_NODE_H
