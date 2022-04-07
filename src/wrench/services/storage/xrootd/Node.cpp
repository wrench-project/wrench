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
#include <wrench/logging/TerminalOutput.h>
#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>
#include <wrench/failure_causes/NetworkError.h>
#include "wrench/services/ServiceMessage.h"
#include "wrench/services/storage/xrootd/XRootDMessage.h"
WRENCH_LOG_CATEGORY(wrench_core_xrootd_data_server,
                    "Log category for XRootD");
namespace wrench {
    namespace XRootD{
//todo add specific handling for being a file server
            int Node::main() {
                //TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_CYAN);

                // Start file storage server
                if(internalStorage!= nullptr){
                    internalStorage->start(internalStorage,true,true);

                }

                std::string message = "XRootD Node " + this->getName() + "  starting on host " + this->getHostname();
                WRENCH_INFO("%s", message.c_str());


                /** Main loop **/
                while (this->processNextMessage()) {

                }

                WRENCH_INFO("XRootD Node %s on host %s cleanly terminating!",
                            this->getName().c_str(),
                            S4U_Simulation::getHostName().c_str());

                return 0;
            }

            /**
             * @brief Process a received control message
             *
             * @return false if the daemon should terminate
             */
            bool Node::processNextMessage() {
                S4U_Simulation::computeZeroFlop();

                // Wait for a message
                std::shared_ptr<SimulationMessage> message = nullptr;

                try {
                    message = S4U_Mailbox::getMessage(this->mailbox);
                } catch (std::shared_ptr<NetworkError> &cause) {
                    WRENCH_INFO("Got a network error while getting some message... ignoring");
                    return true;// oh well
                }

                WRENCH_DEBUG("Got a [%s] message", message->getName().c_str());

                if (auto msg = dynamic_cast<ServiceStopDaemonMessage *>(message.get())) {
                    try {
                        S4U_Mailbox::putMessage(msg->ack_mailbox,
                                                new ServiceDaemonStoppedMessage(this->getMessagePayloadValue(
                                                        SimpleStorageServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
                    } catch (std::shared_ptr<NetworkError> &cause) {
                        return false;
                    }
                    return false;

                }  else if (auto msg = dynamic_cast<FileReadRequestMessage *>(message.get())) {
                    auto all= XRootDSearch(msg->file);
                    //search all
                    //send message to all and appropriate stacks
                    return true;
                }  else if (auto msg = dynamic_cast<UpdateCacheMessage *>(message.get())) {
                    //flops calculation
                    //get current time
                    wrench::S4U_Simulation::getClock();
                    cache[msg->file].push_back(msg->location);//does c++ handle new entries fine?  Im not sure, but testing will show it up pretty fast
                    if(this!=msg->cache_to){
                        S4U_Mailbox::putMessage(supervisor->mailbox,msg);
                    }

                    return true;

                }  else if (auto msg = dynamic_cast<ContinueSearchMessage *>(message.get())) {
                    //add queue checking
                    if(msg->path.size()>0){//continue search in "children"
                        auto targets=msg->path.top();
                        msg->path.pop();
                        //filter repeat sends
                        for(auto target:targets){
                            S4U_Mailbox::putMessage(target->mailbox,msg);
                        }
                    }
                    if(internalStorage){//check own file system
                        auto potentialLocaiton=FileLocation::LOCATION(internalStorage);
                        if(StorageService::lookupFile(msg->file,potentialLocaiton)){
                            if(supervisor&&this!=msg->cache_to){
                                S4U_Mailbox::putMessage(supervisor->mailbox,new UpdateCacheMessage(msg->file,msg->cache_to,potentialLocaiton,MessagePayload::FILE_READ_REQUEST_MESSAGE_PAYLOAD));
                            }
                        }
                    }
                    return true;
                } else {
                    throw std::runtime_error(
                            "SimpleStorageService::processNextMessage(): Unexpected [" + message->getName() + "] message");
                }
            }

            std::shared_ptr<SimpleStorageService> Node::getStorageServer(){
                return internalStorage;
            }
            Node* Node::getChild(int n){
                if(n>0&&n<children.size()){
                    return children[n].get();
                }else{
                    return nullptr;
                }
            }
            Node* Node::getParent(){
                return supervisor;
            }


            std::shared_ptr<FileLocation> Node::hasFile(shared_ptr<DataFile> file){
                if(internalStorage==nullptr or file==nullptr){
                    return nullptr;
                }
                return FileLocation::LOCATION(internalStorage);

            }
            /***********************************************************************************
             *                                                                                 *
             *      Functions under this line may need SERIOUS rework to fit simgird model     *
             *                                                                                 *
             ***********************************************************************************/


            std::vector<shared_ptr<FileLocation>> Node::XRootDSearch(std::shared_ptr<DataFile> file){
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

            bool Node::makeSupervisor() {//this function does nothing anymore?
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