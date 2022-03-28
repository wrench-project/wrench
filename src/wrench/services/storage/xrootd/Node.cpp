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
//todo add specific handling for boing a file server
//todo mae children a vector
            std::shared_ptr<SimpleStorageService> Node::getStorageServer(){}
            Node* Node::getChild(int n){
                if(children!=nullptr&&n>0&&n<numChildren){
                    return children[n].get();
                }else{
                    return nullptr;
                }
            }
            Node* Node::getParrent(){
                return supervisor;
            }

            bool Node::lookupFile(std::shared_ptr<DataFile>file){
                //seperate handling if not supervisor
                return XRootDSearch(file).empty();

            }
            bool Node::cached(shared_ptr<DataFile> file) {
                return cache.find(file)==cache.end();//once timestamps are implimented also check the file is still in cache  it (remove if not)
            }
            std::vector<std::shared_ptr<FileLocation>> Node::getCached(shared_ptr<DataFile> file) {
                return cache[file];//once timestamps are implimented also check the file is still in cache it (remove if not) and unwrap, and refresh timestamp
            }
            void Node::deleteFile(std::shared_ptr<DataFile>file){
                //seperate handling if not supervisor
                auto allNodes=metavisor->getFileNodes(file);
                auto allSub=searchAll(allNodes);
                auto allLocation =traverse(allSub,file,true);
                for(auto location:allLocation){
                    StorageService::deleteFile(file, location);
                }
            }//meta delete from sub tree
            void Node::readFile(std::shared_ptr<DataFile>file){
                //seperate handling if not supervisor
                auto locations=XRootDSearch(file);
                //determine best location
                shared_ptr<FileLocation> best;
                StorageService::readFile(file,best);
            }
            void Node::readFile(std::shared_ptr<DataFile>file, double num_bytes){
                auto locations=XRootDSearch(file);
                //determine best location
                shared_ptr<FileLocation> best;
                StorageService::readFile(file,best,num_bytes);
            }
            //void writeFile(std::shared_ptr<DataFile>file);//unclear how this would work, do we write to 1 existing file then let the background clone it?
            //utility
            std::vector<shared_ptr<FileLocation>> Node::XRootDSearch(std::shared_ptr<DataFile> file){
                //something to make service think, unsure how to do that
                std::vector<shared_ptr<FileLocation>> subLocations;
                if(!cached(file)){
                    subLocations= getCached(file);
                }else{
                    auto potential=metavisor->getFileNodes(file);
                    auto searchPath =searchAll(potential);
                    subLocations=traverse(searchPath,file);
                }
                return subLocations;
            }
            std::shared_ptr<FileLocation> Node::traverse(std::stack<Node*> nodes,std::shared_ptr<DataFile> file,bool meta){

                    Node* node;
                    std::stack<Node*> reverse;
                    while(!nodes.empty()){
                        node=nodes.top();
                        if(!meta){
                            //some message thing here I dont know
                        }
                        reverse.push(node);
                        nodes.pop();
                    }
                    shared_ptr<FileLocation> location= FileLocation::LOCATION(node->internalStorage);
                    if(!meta){
                        while(!nodes.empty()){
                            node=nodes.top();
                            //some message thing here I dont know
                            //add to cache
                            nodes.pop();
                        }
                    }
            }//fake a search for the file, adding to the cache as we return

            std::vector<std::shared_ptr<FileLocation>> Node::traverse(std::vector<std::stack<Node*>> nodes,std::shared_ptr<DataFile> file,bool meta){


            }//"search" multiple paths that go through this supreviser in parallel
            //meta
           std::vector< std::stack<Node*>> Node::searchAll(std::vector<std::shared_ptr<Node>> potential){

                std::vector< std::stack<Node*>> ret(potential.size());
                for(auto host:potential){
                    auto path=search(host.get());
                    if(!path.empty()){
                        ret.push_back(path);
                    }
                }
                return ret;
            }
            std::stack<Node*> Node::search(Node* other){
                std::stack<Node*> ret;
                Node* current=other;
                do{
                    ret.push(current);
                    if(current==this){
                        return ret;
                    }
                    current=current->supervisor;
                    if(ret.size()>metavisor->nodes.size()){//crude cycle detection, but should prevent infinite unexplainable loop on file read
                       throw std::runtime_error("Cycle detected in XRootD file server.  This version of wrench does not support cycles within XRootD.");
                    }
                }while( current!=nullptr);

                return  std::stack<Node*>();//if we get to here, the node is not in our subtree

            }//returns the path of nodes between here and other IF other is in this subtree.
            bool Node::makeSupervisor() {
                if(children!=nullptr){
                    return false;
                }
                children= make_unique<std::shared_ptr<Node>[]>(64);//make_shared on an array was not added until c++ 20 [insert confused jacki chan], but this works for not
                numChildren=0;
                return true;

            }
            Node::Node(const std::string& hostname):Service(hostname,"XRootD"){
                //no other construction needed

            }

            bool Node::makeFileServer(std::set <std::string> path,WRENCH_PROPERTY_COLLECTION_TYPE property_list,
                                      WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list){
                if(internalStorage!=nullptr){
                    return false;
                }
                internalStorage=make_shared<SimpleStorageService>(hostname,path,property_list,messagepayload_list);
                return true;
            }

    }
}