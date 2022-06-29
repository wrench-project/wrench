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
#include <wrench/services/storage/xrootd/XRootDProperty.h>
#include <wrench/exceptions/ExecutionException.h>
WRENCH_LOG_CATEGORY(wrench_core_xrootd_data_server,
                    "Log category for XRootD");
namespace wrench {
    namespace XRootD {
        /**
      * @brief Main method of the daemon
      *
      * @return 0 on termination
      */
        int Node::main() {
            //TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_CYAN);
            // Start file storage server
            //if(internalStorage){
            //    internalStorage->start(internalStorage,true,true);
            //}
            std::string message =
                    "XRootD Node " + this->getName() + "  starting on host " + this->getHostname();WRENCH_INFO("%s",
                                                                                                               message.c_str());
            reduced= getPropertyValueAsBoolean(Property::REDUCED_SIMULATION);
            /** Main loop **/
            while (this->processNextMessage()) {

            }

            WRENCH_INFO("XRootD Node %s on host %s cleanly terminating!",
                        this->getName().c_str(),
                        S4U_Simulation::getHostName().c_str());

            return 0;
        }

        /**
         * @brief Adds a child node to an XRootD supervisor
         * @param child: the new child to add
         * @return the child id of the new node, or -1 if the supervisor already has 64
         */
        int Node::addChild(std::shared_ptr<Node> child) {
            child->supervisor = this;
            if (children.size() < 64) {
                children.push_back(child);
                return children.size() - 1;
            }
            return -1;
        }
//            /**
//             * @brief Get mount point of underlying storage service
//             * @return the (sole) mount point of the service if there is one, and null otherwise
//             */
//            std::string Node::getMountPoint(){
//                if(internalStorage){
//                    return internalStorage->getMountPoint();
//                }
//                return "";
//            }
//            /**
//             * @brief Get the set of mount points of the underlying storage service
//             * @return the set of mount points of the service if there is one, and empty set otherwise
//             */
//            std::set<std::string> Node::getMountPoints() {
//                if(internalStorage){
//                    return internalStorage->getMountPoints();
//                }
//                return std::set<std::string>();
//            }
//            /**
//             * @brief Check whether the underlying storage service has multiple mount points
//             * @return whether the service has multiple mount points, if there is no storage service, returns false
//             */
//            bool Node::hasMultipleMountPoints(){
//                if(internalStorage){
//                    return internalStorage->hasMultipleMountPoints();
//                }
//                return false;
//            }
//            /**
//            * @brief Check whether the underlying storage service has a particular mount point
//            * @param mp: a mount point
//            *
//            * @return whether the underlying service has that mount point, if there is no storage service, returns false
//            */
//            bool Node::hasMountPoint(const std::string &mp){
//                if(internalStorage){
//                    return internalStorage->hasMultipleMountPoints();
//                }
//                return false;
//
//            }
        /**
         * @brief Process a received control message
         *
         * @return false if the daemon should terminate
         */

        bool Node::processNextMessage() {
            //TODO: add cpu overhead to... everything
            //S4U_Simulation::compute(flops);
            S4U_Simulation::computeZeroFlop();
            // Wait for a message
            std::unique_ptr<SimulationMessage> message = nullptr;

            S4U_Simulation::compute(this->getPropertyValueAsDouble(Property::MESSAGE_OVERHEAD));
            try {
                message = S4U_Mailbox::getMessage(this->mailbox);
            } catch (std::shared_ptr<NetworkError> &cause) { WRENCH_INFO(
                        "Got a network error while getting some message... ignoring");
                return true;// oh well
            }

            WRENCH_DEBUG("Got a [%s] message", message->getName().c_str());
            if(reduced){//handle the 2 Advancecd Messages if reduced simulation is enabled for this node
                if (auto msg = dynamic_cast<AdvancedContinueSearchMessage *>(message.get())) {
                    S4U_Simulation::compute(this->getPropertyValueAsDouble(Property::CACHE_LOOKUP_OVERHEAD));
                    if(cached(msg->file)){//caching is even more efficient than advanced search
                        auto cached = getCached(msg->file);
                        try {
                            S4U_Mailbox::putMessage(supervisor->mailbox,
                                                    new UpdateCacheMessage(
                                                            msg->answer_mailbox,
                                                            msg->original,
                                                            msg->node,
                                                            msg->file,
                                                            cached,
                                                            getMessagePayloadValue(MessagePayload::UPDATE_CACHE) +
                                                            getMessagePayloadValue(MessagePayload::CACHE_ENTRY) *
                                                            cached.size(),
                                                            msg->answered
                                                    )
                            );
                        } catch (std::shared_ptr<NetworkError> &cause) {
                            throw ExecutionException(cause);
                        }
                    }else {
                        if(children.size()>0) {

                            map<Node *, vector<stack<Node *>>> splitStacks = splitStack(msg->searchStack);
                            S4U_Simulation::compute(
                                    this->getPropertyValueAsDouble(Property::SEARCH_BROADCAST_OVERHEAD));
                                for (auto entry: splitStacks) {
                                    if (entry.first == this) {//this node was the target
                                        if (internalStorage &&//check the storage, it SHOULD be there, but we should check still
                                            StorageService::lookupFile(msg->file, FileLocation::LOCATION(internalStorage))) {
                                            //File in internal storage
                                            cache.add(msg->file, FileLocation::LOCATION(internalStorage));
                                            try {
                                                S4U_Mailbox::putMessage(supervisor->mailbox,
                                                                        new UpdateCacheMessage(
                                                                                msg->answer_mailbox,
                                                                                msg->original,
                                                                                msg->node,
                                                                                msg->file,
                                                                                set<std::shared_ptr<FileLocation
                                                                                >>{FileLocation::LOCATION(internalStorage)},
                                                                                getMessagePayloadValue(MessagePayload::UPDATE_CACHE) +
                                                                                getMessagePayloadValue(MessagePayload::CACHE_ENTRY),
                                                                                msg->answered
                                                                        )
                                                );
                                            } catch (std::shared_ptr<NetworkError> &cause) {
                                                throw ExecutionException(cause);
                                            }
                                        }
                                    } else {
                                        try {
                                            S4U_Mailbox::putMessage(entry.first->mailbox,
                                                                    new AdvancedContinueSearchMessage(
                                                                            msg,
                                                                            entry.second
                                                                    )
                                            );
                                        } catch (std::shared_ptr<NetworkError> &cause) {
                                            throw ExecutionException(cause);
                                        }

                                    }

                                }
                        }
                        if (internalStorage &&//check the storage, it SHOULD be there, but we should check still
                                    StorageService::lookupFile(msg->file, FileLocation::LOCATION(internalStorage))) {
                            //File in internal storage
                            cache.add(msg->file, FileLocation::LOCATION(internalStorage));
                            try {
                                S4U_Mailbox::putMessage(supervisor->mailbox,
                                                        new UpdateCacheMessage(
                                                                msg->answer_mailbox,
                                                                msg->original,
                                                                msg->node,
                                                                msg->file,
                                                                set<std::shared_ptr<FileLocation
                                                                >>{FileLocation::LOCATION(internalStorage)},
                                                                getMessagePayloadValue(MessagePayload::UPDATE_CACHE) +
                                                                getMessagePayloadValue(MessagePayload::CACHE_ENTRY),
                                                                msg->answered
                                                        )
                                );
                            } catch (std::shared_ptr<NetworkError> &cause) {
                                throw ExecutionException(cause);
                            }

                        }

                    }
                    return true;
                }else if (auto msg = dynamic_cast<AdvancedRippleDelete *>(message.get())) {
                    S4U_Simulation::compute(this->getPropertyValueAsDouble(Property::UPDATE_CACHE_OVERHEAD));
                    if (cached(msg->file)) {//Clean Cache
                        cache.remove(msg->file);
                    }
                    if (internalStorage) {
                        //File in internal storage
                        StorageService::deleteFile(msg->file, FileLocation::LOCATION(internalStorage));

                    }

                    S4U_Simulation::compute(this->getPropertyValueAsDouble(Property::SEARCH_BROADCAST_OVERHEAD));

                    if(children.size()>0) {

                        map<Node *, vector<stack<Node *>>> splitStacks = splitStack(msg->searchStack);
                        S4U_Simulation::compute(
                                this->getPropertyValueAsDouble(Property::SEARCH_BROADCAST_OVERHEAD));
                        for (auto entry: splitStacks) {
                            if (entry.first != this) {
                                try {
                                    S4U_Mailbox::putMessage(entry.first->mailbox,
                                                            new AdvancedRippleDelete(
                                                                    msg,
                                                                    entry.second
                                                            )
                                    );
                                } catch (std::shared_ptr<NetworkError> &cause) {
                                    throw ExecutionException(cause);
                                }

                            }

                        }
                    }
                    return true;
                }//both of these must return something, or we will break later in this function
            }
            if (auto msg = dynamic_cast<ServiceStopDaemonMessage *>(message.get())) {//handle all the rest of the messages
                try {
                    S4U_Mailbox::putMessage(msg->ack_mailbox,
                                            new ServiceDaemonStoppedMessage(this->getMessagePayloadValue(
                                                    SimpleStorageServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
                } catch (std::shared_ptr<NetworkError> &cause) {
                    return false;
                }
                return false;

            } else if (auto msg = dynamic_cast<StorageServiceFileReadRequestMessage *>(message.get())) {

                S4U_Simulation::compute(this->getPropertyValueAsDouble(Property::CACHE_LOOKUP_OVERHEAD));
                if (cached(msg->file)) {//File Cached
                    auto cacheCopies = getCached(msg->file);
                    shared_ptr<FileLocation> best = selectBest(cacheCopies);

                    try {
                        S4U_Mailbox::putMessage(best->getStorageService()->mailbox,
                                                new StorageServiceFileReadRequestMessage(
                                                        msg->answer_mailbox,
                                                        msg->mailbox_to_receive_the_file_content,
                                                        msg->file,
                                                        best,
                                                        msg->num_bytes_to_read,
                                                        getMessagePayloadValue(
                                                                MessagePayload::FILE_SEARCH_ANSWER_MESSAGE_PAYLOAD))
                        );
                    } catch (std::shared_ptr<NetworkError> &cause) {
                        throw ExecutionException(cause);
                    }
                } else {//File Not Cached
                    if (internalStorage &&
                            StorageService::lookupFile(msg->file, FileLocation::LOCATION(internalStorage))) {
                        //File in internal storage
                        cache.add(msg->file, FileLocation::LOCATION(internalStorage));
                        try {
                            S4U_Mailbox::putMessage(internalStorage->mailbox,
                                                    new StorageServiceFileReadRequestMessage(
                                                            msg->answer_mailbox,
                                                            msg->mailbox_to_receive_the_file_content,
                                                            msg->file,
                                                            FileLocation::LOCATION(internalStorage),
                                                            msg->num_bytes_to_read,
                                                            getMessagePayloadValue(
                                                                    MessagePayload::FILE_SEARCH_ANSWER_MESSAGE_PAYLOAD))

                            );
                        } catch (std::shared_ptr<NetworkError> &cause) {
                            throw ExecutionException(cause);
                        }
                    } else {//File not in internal storage or cache
                        S4U_Simulation::compute(
                                this->getPropertyValueAsDouble(Property::SEARCH_BROADCAST_OVERHEAD));

                        if (children.size() > 0) {//recursive search
                            if(reduced){//TODO use optimised search


                            }else {//shotgun continued search message to all children

                                shared_ptr<bool> answered = make_shared<bool>(false);

                                for (auto child: children) {
                                    S4U_Mailbox::putMessage(child->mailbox,
                                                            new ContinueSearchMessage(
                                                                    msg->answer_mailbox,
                                                                    make_shared<StorageServiceFileReadRequestMessage>(
                                                                            msg),
                                                                    msg->file,
                                                                    this,
                                                                    getMessagePayloadValue(
                                                                            MessagePayload::CONTINUE_SEARCH),
                                                                    answered,
                                                                    metavisor->defaultTimeToLive
                                                            )
                                    );
                                }
                            }
                        } else {//you asked a leaf directly and it didnt have the file
                            try {
                                S4U_Mailbox::putMessage(msg->answer_mailbox,
                                                        new StorageServiceFileReadAnswerMessage(
                                                                msg->file,
                                                                FileLocation::LOCATION(internalStorage),
                                                                false,
                                                                std::shared_ptr<FailureCause>(
                                                                        new FileNotFound(
                                                                                msg->file,
                                                                                FileLocation::LOCATION(internalStorage)
                                                                        )
                                                                ),
                                                                0,
                                                                getMessagePayloadValue(
                                                                        MessagePayload::FILE_SEARCH_ANSWER_MESSAGE_PAYLOAD))
                                );
                            } catch (std::shared_ptr<NetworkError> &cause) {
                                throw ExecutionException(cause);
                            }
                        }
                    }
                }
                return true;

            } else if (auto msg = dynamic_cast<ContinueSearchMessage *>(message.get())) {
                S4U_Simulation::compute(this->getPropertyValueAsDouble(Property::CACHE_LOOKUP_OVERHEAD));
                if (cached(msg->file)) {//File Cached
                    auto cached = getCached(msg->file);
                    try {
                        S4U_Mailbox::putMessage(supervisor->mailbox,
                                                new UpdateCacheMessage(
                                                        msg->answer_mailbox,
                                                        msg->original,
                                                        msg->node,
                                                        msg->file,
                                                        cached,
                                                        getMessagePayloadValue(MessagePayload::UPDATE_CACHE) +
                                                        getMessagePayloadValue(MessagePayload::CACHE_ENTRY) *
                                                        cached.size(),
                                                        msg->answered
                                                )
                        );
                    } catch (std::shared_ptr<NetworkError> &cause) {
                        throw ExecutionException(cause);
                    }
                } else {//File Not Cached
                    if (internalStorage &&
                        StorageService::lookupFile(msg->file, FileLocation::LOCATION(internalStorage))) {
                        //File in internal storage
                        cache.add(msg->file, FileLocation::LOCATION(internalStorage));
                        try {
                            S4U_Mailbox::putMessage(supervisor->mailbox,
                                                    new UpdateCacheMessage(
                                                            msg->answer_mailbox,
                                                            msg->original,
                                                            msg->node,
                                                            msg->file,
                                                            set<std::shared_ptr<FileLocation
                                                            >>{FileLocation::LOCATION(internalStorage)},
                                                            getMessagePayloadValue(MessagePayload::UPDATE_CACHE) +
                                                            getMessagePayloadValue(MessagePayload::CACHE_ENTRY),
                                                            msg->answered
                                                    )
                            );
                        } catch (std::shared_ptr<NetworkError> &cause) {
                            throw ExecutionException(cause);
                        }
                    } else {//File not in internal storage or cache

                        S4U_Simulation::compute(
                                this->getPropertyValueAsDouble(Property::SEARCH_BROADCAST_OVERHEAD));
                        if (children.size() > 0 &&
                            msg->timeToLive > 0) {//shotgun continued search message to all chldren

                            for (auto child: children) {
                                S4U_Mailbox::putMessage(child->mailbox,
                                                        new ContinueSearchMessage(msg)
                                );
                            }
                        } else {
                            //this is a leaf that just didnt have the file.  XRootD protocal is to silently fail in this case.  Do not respond
                        }
                    }
                }
                return true;
            } else if (auto msg = dynamic_cast<UpdateCacheMessage *>(message.get())) {
                S4U_Simulation::compute(this->getPropertyValueAsDouble(Property::UPDATE_CACHE_OVERHEAD));
                cache.add(msg->file, msg->locations);
                if (this != msg->node && supervisor) {
                    S4U_Mailbox::putMessage(supervisor->mailbox, new UpdateCacheMessage(msg));
                } else {
                    if (!*msg->answered) {
                        *msg->answered = true;

                        auto cacheCopies = getCached(msg->file);
                        if (msg->original) {//this was a file read
                            shared_ptr<FileLocation> best = selectBest(cacheCopies);
                            //msg->original->location=best;
                            try {

                                S4U_Mailbox::putMessage(best->getStorageService()->mailbox,
                                                        new StorageServiceFileReadRequestMessage(
                                                                msg->answer_mailbox,
                                                                msg->original->mailbox_to_receive_the_file_content,
                                                                msg->file,
                                                                best,
                                                                msg->original->num_bytes_to_read,
                                                                getMessagePayloadValue(
                                                                        MessagePayload::FILE_READ_REQUEST_MESSAGE_PAYLOAD))
                                );
                            } catch (std::shared_ptr<NetworkError> &cause) {
                                throw ExecutionException(cause);
                            }
                        } else {//this was a file lookup
                            try {
                                S4U_Mailbox::putMessage(msg->answer_mailbox,
                                                        new StorageServiceFileLookupAnswerMessage(
                                                                msg->file,
                                                                true,//xrootd silently fails if the file doesnt exist, so if we have gotten here, the file does for sure exist
                                                                getMessagePayloadValue(
                                                                        MessagePayload::FILE_SEARCH_ANSWER_MESSAGE_PAYLOAD))
                                );
                            } catch (std::shared_ptr<NetworkError> &cause) {
                                throw ExecutionException(cause);
                            }
                        }
                    }
                }

                return true;

            } else if (auto msg = dynamic_cast<StorageServiceFileDeleteRequestMessage *>(message.get())) {
                if(reduced) {//TODO use optimised search
                    metavisor->deleteFile(msg->file);
                }else {
                    metavisor->deleteFile(msg->file);
                    S4U_Mailbox::putMessage(this->mailbox, new RippleDelete(msg, metavisor->defaultTimeToLive));


                    S4U_Mailbox::putMessage(msg->answer_mailbox,
                                            new StorageServiceFileDeleteAnswerMessage(
                                                    msg->file,
                                                    this->getSharedPtr<Node>(),
                                                    true,
                                                    nullptr,
                                                    getMessagePayloadValue(
                                                            MessagePayload::FILE_DELETE_ANSWER_MESSAGE_PAYLOAD)));
                }
            } else if (auto msg = dynamic_cast<RippleDelete *>(message.get())) {
                S4U_Simulation::compute(this->getPropertyValueAsDouble(Property::UPDATE_CACHE_OVERHEAD));
                if (cached(msg->file)) {//Clean Cache
                    cache.remove(msg->file);
                }
                if (internalStorage) {
                    //File in internal storage
                    StorageService::deleteFile(msg->file, FileLocation::LOCATION(internalStorage));
                }
                S4U_Simulation::compute(this->getPropertyValueAsDouble(Property::SEARCH_BROADCAST_OVERHEAD));
                if (children.size() > 0 && msg->timeToLive > 0) {//shotgun remove search message to all chldren
                    shared_ptr<bool> answered = make_shared<bool>(false);

                    for (auto child: children) {
                        S4U_Mailbox::putMessage(child->mailbox, new RippleDelete(msg));
                    }

                }
            } else if (auto msg = dynamic_cast<StorageServiceMessage *>(message.get())) {//we got a message targeted at a normal storage server
                if (internalStorage) {//if there is an internal storage server, assume the message is misstargeted and forward
                    S4U_Mailbox::putMessage(internalStorage->mailbox, message.release());
                } else { WRENCH_WARN("XRootD manager %s received an unhandled vanilla StorageService message %s",
                                     hostname.c_str(), msg->getName().c_str());
                }
            } else {
                throw std::runtime_error(
                        "SimpleStorageService::processNextMessage(): Unexpected [" + message->getName() + "] message");
            }
            return true;
        }
        /**
        * @brief Select the best file server to read from based on current load
        * @param locations: All locations to consider for file read
        *
        * @return the "best" file server from within locations to use
        */
        std::shared_ptr<FileLocation> Node::selectBest(std::set<std::shared_ptr<FileLocation>> locations) {

            std::shared_ptr<FileLocation> best=*locations.begin();
            double bestLoad=best->getStorageService()->getLoad();
            for(auto location : locations){
                if(location->getStorageService()->getLoad()<bestLoad){
                    best=location;
                    bestLoad=best->getStorageService()->getLoad();
                }
            }
            return best;
        }
        /**
        * @brief A meta tree traversal operation to get the nth child of this node
        * @param n: The index of the child to receive.  Nodes are in order added
        * @return the Child Nodes shared pointer, or nullptr if this node is a leaf
        */
        std::shared_ptr<Node> Node::getChild(unsigned int n){
            if(n>=0&&n<children.size()){
                return children[n];
            }else{
                return nullptr;
            }
        }
        /**
        * @brief A Meta tree traversal to get the parent of this node
        * @return pointer supervisor.  Will be nullptr if root
        */
        Node* Node::getParent(){
            return supervisor;
        }

//        /**
//        * @brief A meta operation to determine if a file exists on this node
//        * @param file: the file to check for
//        * @return a shared pointer the file location if the file was found.  Nullptr if the file was not found or if this is not a storage server
//        */
//        std::shared_ptr<FileLocation> Node::hasFile(shared_ptr<DataFile> file){
//            if(internalStorage==nullptr or file==nullptr){
//                return nullptr;
//            }
//
//            return FileLocation::LOCATION(internalStorage);
//
//        }
        /**
        * @brief Check the cache for a file
        * @param file: The file to check the cache for
        * @return true if the file is cached, false otherwise
        */
        bool Node::cached(shared_ptr<DataFile> file) {
            return cache.isCached(file);
        }
        /**
        * @brief Get all cached locations of the file.
        * @param file: The file to check the cache for
        * @return A set of valid cached files.  Empty set if none are cached
        */
        std::set<std::shared_ptr<FileLocation>> Node::getCached(shared_ptr<DataFile> file) {
            return cache[file];
        }





        /**
        * @brief Makes this node a supervisor.  Since there is nothing special about a supervisor, this cant fail and does nothing
        * @return true
        */
        bool Node::makeSupervisor() {//this function does nothing anymore?
            return true;

        }
        /**
        * @brief Constructor, should not be used directly except by XRootD createNode
        * @param hostname: the name of the host on which the service and its storage service should run
        * @param property_list: A property list
        * @param messagepayload_list: A Message Payload list
        *
        * @return a shared pointer to the newly created Node
        */
        Node::Node(const std::string& hostname,WRENCH_PROPERTY_COLLECTION_TYPE property_list, WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list):StorageService(hostname,"XRootD"){
            this->setProperties(this->default_property_values, property_list);
            setMessagePayloads(default_messagepayload_values,messagepayload_list);
            cache.maxCacheTime= getPropertyValueAsDouble(Property::CACHE_MAX_LIFETIME);
        }
/**
        * @brief make this node a file server
        * @param path: the path the filesystem gets.
        * @param property_list: The property list to use for the underlying storage server
        * @param messagepayload_list: The message payload list to use for the underlying storage server
        *
        * @return true if the Node was made a storage server.  false if it was already a storage server
        */
        bool Node::makeFileServer(std::set <std::string> path,WRENCH_PROPERTY_COLLECTION_TYPE property_list,
                                  WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list){
            if(internalStorage!=nullptr){
                return false;
            }
            internalStorage=make_shared<SimpleStorageService>(hostname,path,property_list,messagepayload_list);
            return true;
        }
        /**
        * @brief Gets the underlying storage server
        * @return A pointer to the simple storage server for this file server
        */
        std::shared_ptr<SimpleStorageService> Node::getStorageServer(){
            return internalStorage;
        }
        /**
        * @brief Split a search stack into subtrees
        * @searchStack: The search stack to split
        * @return A map where each key is the next node to broadcast too, and the value is the search stack to send
        */
        map<Node*,vector<stack<Node*>>> Node::splitStack(vector<stack<Node*>> searchStack){
            map<Node*,vector<stack<Node*>>> splitStacks;
            for(stack<Node *> aStack: searchStack){
                if(!aStack.empty()) {
                    Node *top = aStack.top();
                    aStack.pop();
                    splitStacks[top].push_back(aStack);
                }else{
                    splitStacks[this].push_back(stack<Node*>());//indicates THIS node was an endpoint

                }
            }
            return splitStacks;//I sure hope RVO takes care of this copy
        }
        /*
        bool Node::lookupFile(std::shared_ptr<DataFile>file){
            if ((file == nullptr) ) {
                throw std::invalid_argument("XrootD::Node::lookupFile(): Invalid arguments");
            }
            assertServiceIsUp();
            auto answer_mailbox = S4U_Daemon::getRunningActorRecvMailbox();
            try {
                S4U_Mailbox::putMessage(mailbox,
                                        new FileSearchRequestMessage(
                                                answer_mailbox,
                                                file,
                                                getMessagePayloadValue(MessagePayload::FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD))
                );
            } catch (std::shared_ptr<NetworkError> &cause) {
                throw ExecutionException(cause);
            }

            // Wait for a reply
            std::unique_ptr<SimulationMessage> message = nullptr;

            try {
                message = S4U_Mailbox::getMessage(answer_mailbox, network_timeout);
            } catch (std::shared_ptr<NetworkError> &cause) {
                return false;
            }

            if (auto msg = dynamic_cast<FileSearchAnswerMessage *>(message.get())) {
                // If it's not a success, throw an exception
                return msg->success;
            } else {
                throw std::runtime_error("StorageService::readFile(): Received an unexpected [" +
                                         message->getName() + "] message!");
            }
        }
        void Node::deleteFile(std::shared_ptr<DataFile>file) { //nonmeta delete from sub tree
            S4U_Mailbox::putMessage(mailbox,
                                    new FileDeleteRequestMessage(
                                            file,
                                            getMessagePayloadValue(MessagePayload::FILE_DELETE_REQUEST_MESSAGE_PAYLOAD),
                                            metavisor->defaultTimeToLive
                                            )
            );
        }

        void Node::readFile(std::shared_ptr<DataFile>file){
            readFile(file,file->getSize());
        }
        void Node::readFile(std::shared_ptr<DataFile>file, double num_bytes){
            if ((file == nullptr)  or (num_bytes < 0.0)) {

                throw std::invalid_argument("XrootD::Node::readFile(): Invalid arguments");
            }
            assertServiceIsUp();
            auto answer_mailbox = S4U_Daemon::getRunningActorRecvMailbox();
            try {
                S4U_Mailbox::putMessage(mailbox,
                                        new FileSearchRequestMessage(
                                                answer_mailbox,
                                                file,
                                                getMessagePayloadValue(MessagePayload::FILE_READ_REQUEST_MESSAGE_PAYLOAD))
                                        );
            } catch (std::shared_ptr<NetworkError> &cause) {
                throw ExecutionException(cause);
            }

            // Wait for a reply
            std::unique_ptr<SimulationMessage> message = nullptr;

            try {
                message = S4U_Mailbox::getMessage(answer_mailbox, network_timeout);
            } catch (std::shared_ptr<NetworkError> &cause) {
                throw ExecutionException(cause);
            }

            if (auto msg = dynamic_cast<FileSearchAnswerMessage *>(message.get())) {
                // If it's not a success, throw an exception
                if (not msg->success) {
                    std::shared_ptr<FailureCause> &cause = msg->failure_cause;
                    throw ExecutionException(cause);
                }
                StorageService::readFile(msg->file,msg->location);
            } else {
                throw std::runtime_error("StorageService::readFile(): Received an unexpected [" +
                                         message->getName() + "] message!");
            }
        }
        */
    /**
     * @brief Get the load of the underlying storage service
     * @return the load on the service
     */
        double Node::getLoad() {
            if(internalStorage){
                return internalStorage->getLoad();
            }
            return 0;

        }
        /**
        * @brief create a new file to the file server
        * @param file: A shared pointer to the file to add
        *
        * @throw std::invalid_argument
        */
        void Node::createFile(const std::shared_ptr<DataFile> &file) {
            metavisor->createFile(file,this->getSharedPtr<Node>());
        }
    }
}