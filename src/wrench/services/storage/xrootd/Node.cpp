/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/storage/xrootd/Node.h>

#include <wrench/services/storage/xrootd/Deployment.h>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/simgrid_S4U_util/S4U_CommPort.h>
#include <wrench/failure_causes/FileNotFound.h>
#include "wrench/services/ServiceMessage.h"
#include "wrench/services/storage/xrootd/XRootDMessage.h"
#include "wrench/services/storage/StorageServiceMessage.h"
#include "wrench/services/storage/xrootd/SearchStack.h"
#include <wrench/services/storage/xrootd/XRootDProperty.h>
#include <wrench/exceptions/ExecutionException.h>
#include <wrench/services/helper_services/alarm/Alarm.h>
#include <wrench/failure_causes/NotAllowed.h>
#include <wrench/util/UnitParser.h>

#include <utility>

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
                    "XRootD Node " + this->getName() + "  starting on host " + this->getHostname();
            WRENCH_INFO("%s",
                        message.c_str());
            reduced = getPropertyValueAsBoolean(Property::REDUCED_SIMULATION);
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
         * @return the child id of the new node
         */
        //should this throw an exception instead?
        std::shared_ptr<Node> Node::addChild(std::shared_ptr<Node> child) {
            child->supervisor = this;
            if (children.size() < 64) {
                children.push_back(child);
                return child;
            }
            throw std::runtime_error("Supervisor running on " + hostname + " already has 64 children");
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
            //S4U_Simulation::compute(flops);
            S4U_Simulation::computeZeroFlop();
            // Wait for a message
            std::unique_ptr<SimulationMessage> message = nullptr;

            S4U_Simulation::compute(this->getPropertyValueAsDouble(Property::MESSAGE_OVERHEAD));
            try {
                message = this->commport->getMessage();
            } catch (ExecutionException &e) {
                WRENCH_INFO(
                        "Got a network error while getting some message... ignoring");
                return true;// oh! well
            }

            WRENCH_DEBUG("Got a [%s] message", message->getName().c_str());
            if (reduced) {//handle the 2 Advanced Messages if reduced simulation is enabled for this node
                if (auto msg = dynamic_cast<AdvancedContinueSearchMessage *>(message.get())) {
                    WRENCH_DEBUG("Advanced Continue Search for %s", msg->file->getID().c_str());

                    S4U_Simulation::compute(this->getPropertyValueAsDouble(Property::CACHE_LOOKUP_OVERHEAD));
                    if (cached(msg->file)) {//caching is even more efficient than advanced search
                        WRENCH_DEBUG("File %s found in cache", msg->file->getID().c_str());

                        auto cached = getCached(msg->file);
                        supervisor->commport->dputMessage(
                                new UpdateCacheMessage(
                                        msg->answer_commport,
                                        msg->original,
                                        msg->node,
                                        msg->file,
                                        cached,
                                        getMessagePayloadValue(MessagePayload::UPDATE_CACHE) +
                                                getMessagePayloadValue(MessagePayload::CACHE_ENTRY) *
                                                        cached.size(),
                                        msg->answered));
                    } else {
                        if (!children.empty()) {

                            map<Node *, vector<stack<Node *>>> splitStacks = splitStack(msg->search_stack);
                            S4U_Simulation::compute(
                                    this->getPropertyValueAsDouble(Property::SEARCH_BROADCAST_OVERHEAD));
                            WRENCH_DEBUG("Advanced Broadcast to %zu hosts", splitStacks.size());

                            for (auto const &entry: splitStacks) {
                                if (entry.first == this) {//this node was the target
                                    if (internalStorage &&//check the storage, it SHOULD be there, but we should check still
                                        internalStorage->hasFile(msg->file)) {
                                        //File in internal storage
                                        cache.add(msg->file, FileLocation::LOCATION(internalStorage, msg->file));
                                        supervisor->commport->dputMessage(
                                                new UpdateCacheMessage(
                                                        msg->answer_commport,
                                                        msg->original,
                                                        msg->node,
                                                        msg->file,
                                                        set<std::shared_ptr<FileLocation>>{FileLocation::LOCATION(internalStorage, msg->file)},
                                                        getMessagePayloadValue(MessagePayload::UPDATE_CACHE) +
                                                                getMessagePayloadValue(MessagePayload::CACHE_ENTRY),
                                                        msg->answered));
                                    }
                                } else {
                                    entry.first->commport->dputMessage(
                                            new AdvancedContinueSearchMessage(
                                                    msg,
                                                    entry.second));
                                }
                            }
                        }

                        if (internalStorage &&//check the storage, it SHOULD be there, but we should check still
                            internalStorage->hasFile(msg->file)) {
                            //File in internal storage
                            cache.add(msg->file, FileLocation::LOCATION(internalStorage, msg->file));
                            supervisor->commport->dputMessage(
                                    new UpdateCacheMessage(
                                            msg->answer_commport,
                                            msg->original,
                                            msg->node,
                                            msg->file,
                                            set<std::shared_ptr<FileLocation>>{FileLocation::LOCATION(internalStorage, msg->file)},
                                            getMessagePayloadValue(MessagePayload::UPDATE_CACHE) +
                                                    getMessagePayloadValue(MessagePayload::CACHE_ENTRY),
                                            msg->answered));
                        }
                    }
                    return true;
                } else if (auto msg = dynamic_cast<AdvancedRippleDelete *>(message.get())) {
                    S4U_Simulation::compute(this->getPropertyValueAsDouble(Property::UPDATE_CACHE_OVERHEAD));
                    if (cached(msg->file)) {//Clean Cache
                        cache.remove(msg->file);
                    }
                    if (internalStorage) {
                        //File in internal storage
                        internalStorage->deleteFile(msg->file);
                    }

                    S4U_Simulation::compute(this->getPropertyValueAsDouble(Property::SEARCH_BROADCAST_OVERHEAD));

                    if (!children.empty()) {

                        map<Node *, vector<stack<Node *>>> splitStacks = splitStack(msg->search_stack);
                        S4U_Simulation::compute(
                                this->getPropertyValueAsDouble(Property::SEARCH_BROADCAST_OVERHEAD));
                        for (auto const &entry: splitStacks) {
                            if (entry.first != this) {
                                entry.first->commport->dputMessage(
                                        new AdvancedRippleDelete(
                                                msg,
                                                entry.second));
                            }
                        }
                    }
                    return true;
                }//both of these must return something, or we will break later in this function
            }
            if (auto msg = dynamic_cast<ServiceStopDaemonMessage *>(message.get())) {//handle all the rest of the messages
                try {
                    msg->ack_commport->dputMessage(
                            new ServiceDaemonStoppedMessage(this->getMessagePayloadValue(
                                    ServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
                } catch (ExecutionException &e) {
                    return false;
                }
                return false;

            } else if (auto msg = dynamic_cast<FileNotFoundAlarm *>(message.get())) {

                //WRENCH_INFO("Got message %p %d %d",msg,*msg->answered,msg->fileReadRequest);
                if (!*msg->answered) {
                    *msg->answered = true;
                    // WRENCH_INFO("%p %p",msg,msg->answered.get());
                    if (msg->fileReadRequest) {

                        msg->answer_commport->dputMessage(
                                new StorageServiceFileReadAnswerMessage(
                                        FileLocation::LOCATION(getSharedPtr<Node>(), msg->file),
                                        false,
                                        std::shared_ptr<FailureCause>(
                                                new FileNotFound(FileLocation::LOCATION(getSharedPtr<Node>(), msg->file))),
                                        nullptr,
                                        0,
                                        1,
                                        getMessagePayloadValue(MessagePayload::FILE_SEARCH_ANSWER_MESSAGE_PAYLOAD)));

                    } else {
                        msg->answer_commport->dputMessage(
                                new StorageServiceFileLookupAnswerMessage(
                                        msg->file,
                                        false,
                                        getMessagePayloadValue(MessagePayload::FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD)));
                    }
                }
            } else if (auto msg = dynamic_cast<StorageServiceFileLookupRequestMessage *>(message.get())) {
                auto file = msg->location->getFile();
                WRENCH_DEBUG("External File Lookup Request for %s", file->getID().c_str());
                S4U_Simulation::compute(this->getPropertyValueAsDouble(Property::CACHE_LOOKUP_OVERHEAD));
                if (cached(file)) {//File Cached
                    WRENCH_DEBUG("File %s found in cache", file->getID().c_str());
                    auto cacheCopies = getCached(file);
                    shared_ptr<FileLocation> best = selectBest(cacheCopies);

                    msg->answer_commport->dputMessage(
                            new StorageServiceFileLookupAnswerMessage(
                                    file,
                                    true,
                                    getMessagePayloadValue(
                                            MessagePayload::FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD)));
                } else {//File Not Cached
                    if (internalStorage && internalStorage->hasFile(file)) {

                        msg->answer_commport->dputMessage(
                                new StorageServiceFileLookupAnswerMessage(
                                        file,
                                        true,
                                        getMessagePayloadValue(
                                                MessagePayload::FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD)));

                    } else {//no internal storage


                        if (!children.empty()) {//recursive search
                            shared_ptr<bool> answered = make_shared<bool>(false);
                            Alarm::createAndStartAlarm(this->simulation_, wrench::S4U_Simulation::getClock() + this->getPropertyValueAsTimeInSecond(Property::FILE_NOT_FOUND_TIMEOUT), this->hostname, this->commport,
                                                       new FileNotFoundAlarm(msg->answer_commport, file, false, answered), "XROOTD_FileNotFoundAlarm");
                            if (reduced) {
                                WRENCH_DEBUG("Starting advanced lookup for %s", file->getID().c_str());

                                auto targets = metavisor->getFileNodes(file);
                                auto search_stack = constructFileSearchTree(targets);
                                map<Node *, vector<stack<Node *>>> splitStacks = splitStack(search_stack);
                                WRENCH_DEBUG("Searching %zu subtrees for %s", search_stack.size(), file->getID().c_str());
                                S4U_Simulation::compute(this->getPropertyValueAsDouble(Property::SEARCH_BROADCAST_OVERHEAD));
                                for (const auto& entry: splitStacks) {
                                    if (entry.first == this) {//this node was the target
                                        //we shouldn't have to worry about this, it should have been handled earlier.
                                        // But just in case, I don't want a rogue search going who knows where
                                    } else {
                                        entry.first->commport->dputMessage(
                                                new AdvancedContinueSearchMessage(
                                                        msg->answer_commport,
                                                        nullptr,
                                                        file,
                                                        this,
                                                        getMessagePayloadValue(
                                                                MessagePayload::CONTINUE_SEARCH),
                                                        answered,
                                                        metavisor->defaultTimeToLive,
                                                        entry.second));
                                    }
                                }
                            } else {//shotgun continued search message to all children
                                WRENCH_DEBUG("Starting basic lookup for %s", file->getID().c_str());


                                for (const auto& child: children) {
                                    child->commport->dputMessage(
                                            new ContinueSearchMessage(
                                                    msg->answer_commport,
                                                    nullptr,
                                                    file,
                                                    this,
                                                    getMessagePayloadValue(
                                                            MessagePayload::CONTINUE_SEARCH),
                                                    answered,
                                                    metavisor->defaultTimeToLive));
                                }
                            }
                        } else {
                            msg->answer_commport->dputMessage(
                                    new StorageServiceFileLookupAnswerMessage(
                                            file,
                                            false,
                                            getMessagePayloadValue(
                                                    MessagePayload::FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD)));
                        }
                    }
                }
                return true;

            } else if (auto msg = dynamic_cast<StorageServiceFileReadRequestMessage *>(message.get())) {

                auto file = msg->location->getFile();

                WRENCH_DEBUG("External File Read Request for %s", file->getID().c_str());
                S4U_Simulation::compute(this->getPropertyValueAsDouble(Property::CACHE_LOOKUP_OVERHEAD));
                if (cached(file)) {//File Cached
                    WRENCH_DEBUG("File %s found in cache", file->getID().c_str());
                    auto cacheCopies = getCached(file);
                    auto best = selectBest(cacheCopies);

                    best->getStorageService()->commport->dputMessage(
                            new StorageServiceFileReadRequestMessage(
                                    msg->answer_commport,
                                    simgrid::s4u::this_actor::get_host(),
                                    best,
                                    msg->num_bytes_to_read,
                                    getMessagePayloadValue(
                                            MessagePayload::FILE_SEARCH_ANSWER_MESSAGE_PAYLOAD)));
                } else {//File Not Cached
                    if (internalStorage &&
                        internalStorage->hasFile(file)) {

                        WRENCH_DEBUG("File %s found in internal Storage", file->getID().c_str());
                        //File in internal storage
                        cache.add(file, FileLocation::LOCATION(internalStorage, file));
                        internalStorage->commport->dputMessage(
                                new StorageServiceFileReadRequestMessage(
                                        msg->answer_commport,
                                        simgrid::s4u::this_actor::get_host(),
                                        FileLocation::LOCATION(internalStorage, file),
                                        msg->num_bytes_to_read,
                                        getMessagePayloadValue(
                                                MessagePayload::FILE_SEARCH_ANSWER_MESSAGE_PAYLOAD))

                        );
                    } else {//File not in internal storage or cache
                        //S4U_Simulation::compute(
                        //        this->getPropertyValueAsDouble(Property::SEARCH_BROADCAST_OVERHEAD));
                        //extra compute call

                        if (!children.empty()) {//recursive search
                            shared_ptr<bool> answered = make_shared<bool>(false);
                            Alarm::createAndStartAlarm(this->simulation_, wrench::S4U_Simulation::getClock() + this->getPropertyValueAsTimeInSecond(Property::FILE_NOT_FOUND_TIMEOUT), this->hostname, this->commport,
                                                       new FileNotFoundAlarm(msg->answer_commport, file, true, answered), "XROOTD_FileNotFoundAlarm");
                            if (reduced) {
                                WRENCH_DEBUG("Starting advanced search for %s", file->getID().c_str());

                                auto targets = metavisor->getFileNodes(file);
                                auto search_stack = constructFileSearchTree(targets);
                                map<Node *, vector<stack<Node *>>> splitStacks = splitStack(search_stack);
                                WRENCH_DEBUG("Searching %zu subtrees for %s", search_stack.size(), file->getID().c_str());
                                S4U_Simulation::compute(this->getPropertyValueAsDouble(Property::SEARCH_BROADCAST_OVERHEAD));
                                for (auto const &entry: splitStacks) {
                                    if (entry.first == this) {//this node was the target
                                        //we shouldn't have to worry about this, it should have been handled earlier.
                                        // But just in case, I don't want a rogue search going who knows where
                                    } else {
                                        entry.first->commport->dputMessage(
                                                new AdvancedContinueSearchMessage(
                                                        msg->answer_commport,
                                                        make_shared<StorageServiceFileReadRequestMessage>(msg),
                                                        file,
                                                        this,
                                                        getMessagePayloadValue(
                                                                MessagePayload::CONTINUE_SEARCH),
                                                        answered,
                                                        metavisor->defaultTimeToLive,
                                                        entry.second));
                                    }
                                }
                            } else {//shotgun continued search message to all children
                                WRENCH_DEBUG("Starting basic search for %s", file->getID().c_str());
                                for (const auto& child: children) {
                                    child->commport->dputMessage(
                                            new ContinueSearchMessage(
                                                    msg->answer_commport,
                                                    make_shared<StorageServiceFileReadRequestMessage>(
                                                            msg),
                                                    file,
                                                    this,
                                                    getMessagePayloadValue(
                                                            MessagePayload::CONTINUE_SEARCH),
                                                    answered,
                                                    metavisor->defaultTimeToLive));
                                }
                            }
                        } else {// you asked a leaf directly, and it didn't have the file
                            msg->answer_commport->putMessage(new StorageServiceFileReadAnswerMessage(
                                    FileLocation::LOCATION(internalStorage, file),
                                    false,
                                    std::shared_ptr<FailureCause>(
                                            new FileNotFound(
                                                    FileLocation::LOCATION(internalStorage, file))),
                                    nullptr,
                                    0,
                                    1,
                                    getMessagePayloadValue(
                                            MessagePayload::FILE_SEARCH_ANSWER_MESSAGE_PAYLOAD)));
                        }
                    }
                }
                return true;

            } else if (auto msg = dynamic_cast<ContinueSearchMessage *>(message.get())) {
                WRENCH_DEBUG("Continuing Basic Search for %s", msg->file->getID().c_str());
                S4U_Simulation::compute(this->getPropertyValueAsDouble(Property::CACHE_LOOKUP_OVERHEAD));
                if (cached(msg->file)) {//File Cached
                    WRENCH_DEBUG("Found %s in cache", msg->file->getID().c_str());
                    auto cached = getCached(msg->file);
                    supervisor->commport->dputMessage(new UpdateCacheMessage(
                            msg->answer_commport,
                            msg->original,
                            msg->node,
                            msg->file,
                            cached,
                            getMessagePayloadValue(MessagePayload::UPDATE_CACHE) +
                                    getMessagePayloadValue(MessagePayload::CACHE_ENTRY) *
                                            cached.size(),
                            msg->answered));
                } else {//File Not Cached
                    if (internalStorage &&
                        internalStorage->hasFile(msg->file)) {
                        WRENCH_DEBUG("Found %s in internal storage", msg->file->getID().c_str());
                        //File in internal storage
                        cache.add(msg->file, FileLocation::LOCATION(internalStorage, msg->file));
                        supervisor->commport->dputMessage(
                                new UpdateCacheMessage(
                                        msg->answer_commport,
                                        msg->original,
                                        msg->node,
                                        msg->file,
                                        set<std::shared_ptr<FileLocation>>{FileLocation::LOCATION(internalStorage, msg->file)},
                                        getMessagePayloadValue(MessagePayload::UPDATE_CACHE) +
                                                getMessagePayloadValue(MessagePayload::CACHE_ENTRY),
                                        msg->answered));
                    } else {//File not in internal storage or cache
                        if (!children.empty() &&
                            msg->timeToLive > 0) {//shotgun continued search message to all children
                            WRENCH_DEBUG(" Basic Search broadcast for %s", msg->file->getID().c_str());

                            S4U_Simulation::compute(
                                    this->getPropertyValueAsDouble(Property::SEARCH_BROADCAST_OVERHEAD));
                            for (const auto& child: children) {
                                child->commport->dputMessage(
                                        new ContinueSearchMessage(msg));
                            }
                        } else {
                            //this is a leaf that just didn't have the file.  XRootD protocol is to silently fail in this case.  Do not respond
                        }
                    }
                }
                return true;
            } else if (auto msg = dynamic_cast<UpdateCacheMessage *>(message.get())) {
                WRENCH_DEBUG("Updating Cache after finding %s", msg->file->getID().c_str());
                S4U_Simulation::compute(this->getPropertyValueAsDouble(Property::UPDATE_CACHE_OVERHEAD));
                cache.add(msg->file, msg->locations);
                if (this != msg->node && supervisor) {
                    WRENCH_DEBUG("Forward update to super");
                    supervisor->commport->dputMessage(new UpdateCacheMessage(msg));
                } else {
                    WRENCH_DEBUG("Update has reached top of subtree");
                    if (!*msg->answered) {
                        *msg->answered = true;
                        //WRENCH_INFO("%p %p",msg,msg->answered.get());
                        //WRENCH_DEBUG("Sending File %s found to request", msg->file->getID().c_str());

                        auto cacheCopies = getCached(msg->file);
                        if (msg->original) {//this was a file read
                            shared_ptr<FileLocation> best = selectBest(cacheCopies);
                            //msg->original->location=best;
                            best->getStorageService()->commport->dputMessage(
                                    new StorageServiceFileReadRequestMessage(
                                            msg->answer_commport,
                                            simgrid::s4u::this_actor::get_host(),
                                            best,
                                            msg->original->num_bytes_to_read,
                                            getMessagePayloadValue(
                                                    MessagePayload::FILE_READ_REQUEST_MESSAGE_PAYLOAD)));
                        } else {//this was a file lookup
                            msg->answer_commport->dputMessage(
                                    new StorageServiceFileLookupAnswerMessage(
                                            msg->file,
                                            true,//xrootd silently fails if the file doesn't exist, so if we have gotten here, the file does for sure exist
                                            getMessagePayloadValue(
                                                    MessagePayload::FILE_SEARCH_ANSWER_MESSAGE_PAYLOAD)));
                        }
                    }
                }

                return true;

            } else if (auto msg = dynamic_cast<StorageServiceFileDeleteRequestMessage *>(message.get())) {
                if (reduced) {//TODO use optimised search

                    shared_ptr<bool> answered = make_shared<bool>(false);
                    auto targets = metavisor->getFileNodes(msg->location->getFile());
                    auto search_stack = constructFileSearchTree(targets);
                    map<Node *, vector<stack<Node *>>> splitStacks = splitStack(search_stack);
                    S4U_Simulation::compute(this->getPropertyValueAsDouble(Property::SEARCH_BROADCAST_OVERHEAD));
                    for (const auto& entry: splitStacks) {
                        if (entry.first == this) {//this node was the target
                            //we shouldn't have to worry about this, it should have been handled earlier.
                            // But just in case, I don't want a rogue search going who knows where
                        } else {
                            entry.first->commport->dputMessage(
                                    new AdvancedRippleDelete(
                                            msg,
                                            metavisor->defaultTimeToLive,
                                            entry.second));
                        }
                    }
                    msg->answer_commport->dputMessage(
                            new StorageServiceFileDeleteAnswerMessage(
                                    msg->location->getFile(),
                                    getSharedPtr<Node>(),
                                    true,
                                    nullptr,
                                    getMessagePayloadValue(StorageServiceMessagePayload::FILE_DELETE_ANSWER_MESSAGE_PAYLOAD))

                    );
                } else {
                    this->commport->dputMessage(new RippleDelete(msg, metavisor->defaultTimeToLive));


                    msg->answer_commport->dputMessage(
                            new StorageServiceFileDeleteAnswerMessage(
                                    msg->location->getFile(),
                                    this->getSharedPtr<Node>(),
                                    true,
                                    nullptr,
                                    getMessagePayloadValue(
                                            MessagePayload::FILE_DELETE_ANSWER_MESSAGE_PAYLOAD)));
                }

                metavisor->deleteFile(msg->location->getFile());
                return true;
            } else if (auto msg = dynamic_cast<RippleDelete *>(message.get())) {
                S4U_Simulation::compute(this->getPropertyValueAsDouble(Property::UPDATE_CACHE_OVERHEAD));
                if (cached(msg->file)) {//Clean Cache
                    cache.remove(msg->file);
                }
                if (internalStorage) {
                    //File in internal storage
                    try {
                        internalStorage->deleteFile(msg->file);
                    } catch (ExecutionException &e) {
                        //we don't actually care if this fails, that just means the file
                        // we tried to delete wasn't there already.  Big whoop.
                    }
                }
                S4U_Simulation::compute(this->getPropertyValueAsDouble(Property::SEARCH_BROADCAST_OVERHEAD));
                if (!children.empty() && msg->timeToLive > 0) {//shotgun remove search message to all children
                    shared_ptr<bool> answered = make_shared<bool>(false);

                    for (const auto& child: children) {
                        child->commport->dputMessage(new RippleDelete(msg));
                    }
                }
            } else if (auto msg = dynamic_cast<StorageServiceFileWriteRequestMessage *>(message.get())) {
                auto file = msg->location->getFile();
                if (not internalStorage) {
                    // Reply this is not allowed
                    std::string error_message = "Cannot write file at non-storage XRootD node";
                    auto location = FileLocation::LOCATION(this->getSharedPtr<Node>(), file);
                    msg->answer_commport->dputMessage(
                            new StorageServiceFileWriteAnswerMessage(
                                    location,
                                    false,
                                    std::shared_ptr<FailureCause>(
                                            new NotAllowed(getSharedPtr<Node>(), error_message)),
                                    {},
                                    0,
                                    getMessagePayloadValue(MessagePayload::FILE_WRITE_ANSWER_MESSAGE_PAYLOAD)));

                } else {
                    // Forward the message
                    msg->location = FileLocation::LOCATION(internalStorage, file);
                    //                    msg->buffer_size = internalStorage->getPropertyValueAsSizeInByte(
                    //                            SimpleStorageServiceProperty::BUFFER_SIZE);
                    internalStorage->commport->dputMessage(message.release());
                }

            } else if (auto msg = dynamic_cast<StorageServiceFileCopyRequestMessage *>(message.get())) {
                auto file = msg->src->getFile();
                if (not internalStorage) {
                    // Reply this is not allowed
                    if (msg->answer_commport) {
                        std::string error_message = "Cannot copy file to/from non-storage XRooD node";
                        auto location = FileLocation::LOCATION(getSharedPtr<Node>(), file);
                        msg->answer_commport->dputMessage(
                                new StorageServiceFileCopyAnswerMessage(
                                        msg->src,
                                        msg->dst,
                                        false,
                                        std::shared_ptr<FailureCause>(
                                                new NotAllowed(getSharedPtr<Node>(), error_message)),
                                        getMessagePayloadValue(MessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD)));
                    }

                } else {
                    // Forward the message
                    msg->dst = FileLocation::LOCATION(internalStorage, file);
                    internalStorage->commport->dputMessage(message.release());
                }

            } else if (auto msg = dynamic_cast<StorageServiceMessage *>(message.get())) {//we got a message targeted at a normal storage server
                if (not internalStorage) {
                    // TODO: deal with the Copy message, which then needs a reply with a NotAllowed failure cause,
                    // TODO: just like for FileWriteRequest above
                    throw std::runtime_error("Non-Storage XRooD node received a message it cannot process - internal error");
                } else {
                    // Forwarding the message as-is to the internal Storage
                    internalStorage->commport->dputMessage(message.release());
                }
            } else {
                throw std::runtime_error(
                        "Node:processNextMessage(): Unexpected [" + message->getName() + "] message");
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

            std::shared_ptr<FileLocation> best = *locations.begin();
            double bestLoad = best->getStorageService()->getLoad();
            for (const auto& location: locations) {
                if (location->getStorageService()->getLoad() < bestLoad) {
                    best = location;
                    bestLoad = best->getStorageService()->getLoad();
                }
            }
            return best;
        }
        /**
        * @brief A meta tree traversal operation to get the nth child of this node
        * @param n: The index of the child to receive.  Nodes are in order added
        * @return the Child Nodes shared pointer, or nullptr if this node is a leaf
        */
        std::shared_ptr<Node> Node::getChild(unsigned int n) {
            if (n >= 0 && n < children.size()) {
                return children[n];
            } else {
                return nullptr;
            }
        }
        /**
        * @brief A Meta tree traversal to get the parent of this node
        * @return pointer supervisor.  Will be nullptr if root
        */
        Node *Node::getParent() {
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
            return cache.isCached(std::move(file));
        }
        /**
        * @brief Get all cached locations of the file.
        * @param file: The file to check the cache for
        * @return A set of valid cached files.  Empty set if none are cached
        */
        std::set<std::shared_ptr<FileLocation>> Node::getCached(const shared_ptr<DataFile>& file) {
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
        *
        * @param deployment: the XRootD deployment this node belongs to
        * @param hostname: the name of the host on which the service and its storage service should run
        * @param property_list: A property list
        * @param messagepayload_list: A Message Payload list
        *
        */
        Node::Node(Deployment *deployment, const std::string &hostname, const WRENCH_PROPERTY_COLLECTION_TYPE& property_list,
                   const WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE& messagepayload_list) : StorageService(hostname, "XRootDNode") {

            // Create /dev/null mountpoint so that Locations can be created
            // TODO: Removed this due to using simgrid:fsmod.... big deal?
//            this->file_system->mount_dev_null_partition("/dev/null");
//            this->file_systems[LogicalFileSystem::DEV_NULL] =
//                    LogicalFileSystem::createLogicalFileSystem(hostname, this, LogicalFileSystem::DEV_NULL, "NONE");

            this->setProperties(this->default_property_values, property_list);
            setMessagePayloads(default_messagepayload_values, messagepayload_list);
            cache.maxCacheTime = getPropertyValueAsTimeInSecond(Property::CACHE_MAX_LIFETIME);
            this->deployment = deployment;
            //            this->buffer_size = DBL_MAX;// Not used, but letting it be zero will raise unwanted exception since
            //            // clients "think" that they're talking to a real storage service
        }

        /**
        * @brief make this node a file server
        * @param path: the path the filesystem gets.
        * @param property_list: The property list to use for the underlying storage server
        * @param messagepayload_list: The message payload list to use for the underlying storage server
        *
        * @return true if the Node was made a storage server.  false if it was already a storage server
        */
        bool Node::makeFileServer(std::set<std::string> path, WRENCH_PROPERTY_COLLECTION_TYPE property_list,
                                  WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE messagepayload_list) {
            if (internalStorage != nullptr) {
                return false;
            }
            //            internalStorage = make_shared<SimpleStorageService>(hostname, path, property_list, messagepayload_list);
            internalStorage = std::shared_ptr<SimpleStorageService>(SimpleStorageService::createSimpleStorageService(hostname, std::move(path), std::move(property_list), std::move(messagepayload_list)));
            return true;
        }

        /**
        * @brief Gets the underlying storage server
        * @return A pointer to the simple storage server for this file server
        */
        std::shared_ptr<SimpleStorageService> Node::getStorageServer() {
            return internalStorage;
        }

        /**
        * @brief Split a search stack into subtrees
        * @param search_stack: The search stack to split
        * @return A map where each key is the next node to broadcast too, and the value is the search stack to send
        */
        map<Node *, vector<stack<Node *>>> Node::splitStack(const vector<stack<Node *>>& search_stack) {
            map<Node *, vector<stack<Node *>>> splitStacks;
            for (stack<Node *> aStack: search_stack) {
                if (!aStack.empty()) {
                    Node *top = aStack.top();
                    aStack.pop();
                    splitStacks[top].push_back(aStack);
                } else {
                    splitStacks[this].push_back(stack<Node *>());//indicates THIS node was an endpoint
                }
            }
            return splitStacks;//I sure hope RVO takes care of this copy
        }


        /**
     * @brief Get a file's last write date at a location (in zero simulated time)
     *
     * @param location: the file location
     *
     * @return the file's last write date, or -1 if the file is not found
     *
     */
        double Node::getFileLastWriteDate(const std::shared_ptr<FileLocation> &location) {
            if (location == nullptr) {
                throw std::invalid_argument("Node::getFileLastWriteDate(): Invalid nullptr argument");
            }
            if (internalStorage) {
                return internalStorage->getFileLastWriteDate(location);
            } else {
                throw std::runtime_error("Node::getFileLastWriteDate() called on non storage Node " + hostname);
            }
        }

        /**
         * @brief Get the load of the underlying storage service
         * @return the load on the service
         */
        double Node::getLoad() {
            if (internalStorage) {
                return internalStorage->getLoad();
            }
            return 0;
        }

        /**
         * @brief Remove a directory and all its content at the Node (in zero simulated time)
         * @param path: a path
         */
        void Node::removeDirectory(const std::string &path) {
            throw std::runtime_error("Node::removeDirectory(): Not implemented yet (ever?)");
        }


        /**
        * @brief construct the path to all targets IF they are in the subtree
        * @param targets: All the nodes to search for
        *
        * @returns the path to each target in the subtree.
        */
        vector<stack<Node *>> Node::constructFileSearchTree(const vector<shared_ptr<Node>> &targets) {
            vector<stack<Node *>> ret;
            for (const auto& target: targets) {
                if (target.get() == this) {
                    ret.push_back(stack<Node *>());//I don't think this should ever happen, but it might
                } else {
                    stack<Node *> tmp = constructSearchStack(target.get());
                    if (!tmp.empty()) {
                        ret.push_back(tmp);
                    }
                }
            }
            return ret;
        }
        /**
        * @brief construct the path to a targets IF it is in the subtree
        * @param target: The node to search for
        *
        * @returns the path to the target if it is in the subtree, empty stack otherwise.
        */
        stack<Node *> Node::constructSearchStack(Node *target) {
            stack<Node *> ret;
            Node *next = target;
            while (next != nullptr && next != this) {
                ret.push(next);
                next = next->supervisor;
            }
            if (next != this) {//failed to find this in parent tree
                ret = stack<Node *>();
            } else {
                WRENCH_DEBUG("Found file server %s in subtree at depth %zu", target->getName().c_str(), ret.size());
            }
            return ret;
        }

        /**
        * @brief create a new file in the federation on this node.  Use instead of wrench::Simulation::createFile when adding files to XRootD
        * @param location: a file location, must be the same object as the function is invoked on
        *
        */
        void Node::createFile(const std::shared_ptr<FileLocation> &location) {
            if (internalStorage == nullptr) {
                throw std::runtime_error("Node::createFile() called on non storage Node " + hostname);
            }

            internalStorage->createFile(location);
            metavisor->files[location->getFile()].push_back(this->getSharedPtr<Node>());
        }

        /**
        * @brief remove a new file in the federation on this node.
        * @param location: a file location, must be the same object as the function is invoked on
        *
        */
        void Node::removeFile(const std::shared_ptr<FileLocation> &location) {
            if (internalStorage == nullptr) {
                throw std::runtime_error("Node::removeFile() called on non storage Node " + hostname);
            }

            internalStorage->removeFile(location);
            auto it = std::find(metavisor->files[location->getFile()].begin(), metavisor->files[location->getFile()].end(), this->getSharedPtr<Node>());
            if (it != metavisor->files[location->getFile()].end()) {
                metavisor->files[location->getFile()].erase(it);
            }
        }


        /**
        * @brief write a file on this node.
	    * @param answer_commport: a commport on which to send the answer message
        * @param location: a location
	    * @param num_bytes_to_write: A number of bytes to write
	    * @param wait_for_answer: true if this method should wait for the answer, false otherwise
        *
        */
        void Node::writeFile(S4U_CommPort *answer_commport,
                             const std::shared_ptr<FileLocation> &location,
                             sg_size_t num_bytes_to_write,
                             bool wait_for_answer) {
            if (internalStorage == nullptr) {
                std::string error_message = "Cannot write file at non-storage XRootD node";
                throw ExecutionException(
                        std::shared_ptr<FailureCause>(
                                new NotAllowed(getSharedPtr<Node>(), error_message)));
            }
            // Replace the location with
            // TODO: THIS LOCATION REWRITE WAS DONE TO FIX SOMETHING BUT HENRI HAS NO
            // TODO: IDEA HOW COME IT'S EVER WORKED BEFORE SINCE THE FTT INSIDE THE INTERNAL STORAGE
            // TODO: WILL SAY "THIS IS NOT ME, MY PARENT IS THE INTERNAL STORAGE, NOT THE NODE"
            auto new_location = FileLocation::LOCATION(internalStorage, location->getDirectoryPath(), location->getFile());
            internalStorage->writeFile(answer_commport, new_location, num_bytes_to_write, wait_for_answer);
            //            internalStorage->writeFile(answer_commport, location, wait_for_answer);
            metavisor->files[location->getFile()].push_back(this->getSharedPtr<Node>());
        }

        /**
         * @brief Adds a child, which will be a supervisor, to a node
         * @param hostname: the name of the host on which the child will run
         * @return The child
         */
        std::shared_ptr<Node> Node::addChildSupervisor(const std::string &hostname) {
            return this->addChild(this->deployment->createSupervisor(hostname));
        }

        /**
         * @brief Adds a child, which will be a storage server, to a node
         * @param hostname: the name of the host on which the child will run
         * @param mount_point: the mount point at that host
         * @param storage_property_list: the storage server's property list
         * @param storage_messagepayload_list: the storage server's message payload list
         * @param node_property_list: the XRootD node's property list
         * @param node_messagepayload_list: the XRootD node's message payload list
         * @return The child
         */
        std::shared_ptr<Node> Node::addChildStorageServer(const std::string &hostname, const std::string &mount_point,
                                                          WRENCH_PROPERTY_COLLECTION_TYPE storage_property_list,
                                                          WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE storage_messagepayload_list,
                                                          WRENCH_PROPERTY_COLLECTION_TYPE node_property_list,
                                                          WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE node_messagepayload_list) {
            if (storage_property_list.find(wrench::SimpleStorageServiceProperty::BUFFER_SIZE) != storage_property_list.end()) {
                if (UnitParser::parse_size(storage_property_list[wrench::SimpleStorageServiceProperty::BUFFER_SIZE]) < 1) {
                    throw std::invalid_argument("Node::addChildStorageServer(): XRootD current does not support 0 buffer_size");
                }
            }
            return this->addChild(this->deployment->createStorageServer(hostname, mount_point,
                                                                        storage_property_list, std::move(storage_messagepayload_list),
                                                                        std::move(node_property_list), std::move(node_messagepayload_list)));
        }

        /**
         * @brief Determines whether the storage service has the file. This doesn't simulate anything and is merely a zero-simulated-time data structure lookup.
         * If you want to simulate the overhead of querying the StorageService, instead use lookupFile().
         * @param location: a location
         * @return true if the file is present, false otherwise
         */
        bool Node::hasFile(const shared_ptr<FileLocation> &location) {
            if (internalStorage)
                return internalStorage->hasFile(location);
            //return false;//no internal storage here, so I don't have any files.  But I am pretending to have some, so it's reasonable to ask.
            //alternatively
            return !constructFileSearchTree(metavisor->getFileNodes(location->getFile())).empty();//meta-search the subtree for the file.  If it's in the subtree we can find a route to it, so we have it
        }

        /**
         * @brief Get the storage service's total space (in zero simulated time)
         * @return a capacity in bytes
         */
        sg_size_t Node::getTotalSpace() {
            if (internalStorage) {
                return internalStorage->getTotalSpace();
            } else {
                return 0;
            }
        }

        /**
         * @brief Determine whether the storage service is bufferized
         * @return true if bufferized, false otherwise
         */
        bool Node::isBufferized() const {
            if (internalStorage) {
                return internalStorage->isBufferized();
            } else {
                return false;// TODO: IS THIS A GOOD IDEA? MAY MESS UP COPYING???
                throw std::runtime_error("Node::isBufferized() called on non storage Node " + hostname);
            }
        }

        /**
         * @brief Determine the storage service's buffer size
         * @return a size in bytes
         */
        sg_size_t Node::getBufferSize() const {
            if (internalStorage) {
                return internalStorage->getBufferSize();
            } else {
                throw std::runtime_error("Node::getBufferSize() called on non storage Node " + hostname);
            }
        }

        /**
         * @brief Reserve space at the storage service
         * @param location: a location
         * @return true if success, false otherwise
         */
        bool Node::reserveSpace(std::shared_ptr<FileLocation> &location) {
            if (internalStorage) {
                return internalStorage->reserveSpace(location);
            } else {
                throw std::runtime_error("Node::reserveSpace() called on non storage Node " + hostname);
            }
        }

        /**
         * @brief Unreserve space at the storage service
         * @param location: a location
         */
        void Node::unreserveSpace(std::shared_ptr<FileLocation> &location) {
            if (internalStorage) {
                internalStorage->unreserveSpace(location);
            } else {
                throw std::runtime_error("Node::unreserveSpace() called on non storage Node " + hostname);
            }
        }


    }// namespace XRootD
}// namespace wrench
