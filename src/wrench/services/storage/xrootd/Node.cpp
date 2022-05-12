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
#include <wrench/failure_causes/FileNotFound.h>
#include "wrench/services/ServiceMessage.h"
#include "wrench/services/storage/xrootd/XRootDMessage.h"
#include "wrench/services/storage/StorageServiceMessage.h"
#include "wrench/services/storage/xrootd/SearchStack.h"
WRENCH_LOG_CATEGORY(wrench_core_xrootd_data_server,
                    "Log category for XRootD");
namespace wrench {
    namespace XRootD{
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
                    // If it's in cache then we know, otherwise we must "search"

                    if(cached(msg->file)){
                        auto cached=getCached(msg->file);
                        shared_ptr<FileLocation> best=cached[0];//use load based search, not just 0
                        StorageService::readFile(msg->file,best,msg->answer_mailbox,msg->mailbox_to_receive_the_file_content,msg->num_bytes_to_read);
                    }else{

                    }
                    return true;
                }  else if (auto msg = dynamic_cast<UpdateCacheMessage *>(message.get())) {
                    //flops calculation
                    //get current time
                    wrench::S4U_Simulation::getClock();
                    cache[msg->file].push_back(msg->location);//does c++ handle new entries fine?  Im not sure, but testing will show it up pretty fast
                    if(this!=msg->stack->headNode){
                        S4U_Mailbox::putMessage(supervisor->mailbox,msg);
                    }
                    // If success, then do: StorageService::readFile(file, ....., client_answer_mailbox, client_mailbox_to_receive_the_file_content, num_bytes);

                    return true;

                }  else if (auto msg = dynamic_cast<ContinueSearchMessage *>(message.get())) {

                    return true;
                } else {
                    throw std::runtime_error(
                            "SimpleStorageService::processNextMessage(): Unexpected [" + message->getName() + "] message");
                }
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
            bool Node::cached(shared_ptr<DataFile> file) {
                return cache.find(file)==cache.end();//once timestamps are implimented also check the file is still in cache  it (remove if not)
            }
            std::vector<std::shared_ptr<FileLocation>> Node::getCached(shared_ptr<DataFile> file) {
                return cache[file];//once timestamps are implimented also check the file is still in cache it (remove if not) and unwrap, and refresh timestamp
            }






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
            std::shared_ptr<SimpleStorageService> Node::getStorageServer(){
                return internalStorage;
            }

            bool Node::lookupFile(std::shared_ptr<DataFile>file){

            }
            void Node::deleteFile(std::shared_ptr<DataFile>file) { //meta delete from sub tree

            }
            void Node::readFile(std::shared_ptr<DataFile>file){
                readFile(file,file->getSize());
            }
            void Node::readFile(std::shared_ptr<DataFile>file, double num_bytes){


            }
    }
}