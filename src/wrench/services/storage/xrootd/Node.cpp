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

            std::shared_ptr<SimpleStorageService> Node::getStorageServer(){}
            Node* Node::getChild(int n){}
            Node* Node::getParrent(){}

            bool Node::lookupFile(std::shared_ptr<DataFile>file){}
            void Node::deleteFile(std::shared_ptr<DataFile>file){}//meta opperation
            void Node::readFile(std::shared_ptr<DataFile>file){}
            void Node::readFile(std::shared_ptr<DataFile>file, double num_bytes){}
            //void writeFile(std::shared_ptr<DataFile>file);//unclear how this would work, do we write to 1 existing file then let the background clone it?


            //utility
            void Node::traverse(std::stack<Node*> nodes,std::shared_ptr<DataFile> file ,std::shared_ptr<FileLocation> location){}//fake a search for the file, adding to the cache as we return

            void Node::traverse(std::vector<std::stack<Node*>> nodes,std::shared_ptr<DataFile> file ,std::shared_ptr<FileLocation> location){}//"search" multiple paths that go through this supreviser in parallel
            //meta

            std::stack<Node*> Node::search(Node* other){}//returns the path of nodes between here and other IF other is in this subtree.
    }
}