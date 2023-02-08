/**
* Copyright (c) 2017-2021. The WRENCH Team.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/
#include "wrench/services/storage/proxy/StorageServiceProxy.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
#include "wrench/simulation/SimulationMessage.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/exceptions/ExecutionException.h"
#include "wrench/services/storage/StorageServiceMessage.h"
#include "wrench/services/storage/StorageServiceMessagePayload.h"
#include "wrench/failure_causes/FileNotFound.h"
#include "wrench/failure_causes/HostError.h"
#include "wrench/services/storage/StorageServiceProperty.h"
#include "wrench/services/storage/proxy/StorageServiceProxyProperty.h"
#include "wrench/services/storage/proxy/StorageServiceProxyProperty.h"

WRENCH_LOG_CATEGORY(wrench_core_proxy_file_server,
                    "Log category for ProxyFileServers");
namespace wrench {

    //handle FileDeleteRequest
    //cache and forward
    //handle FileReadRequestMessage
    //Cache then forward
    //handle FileWriteRequestMessage
    //Cache and Forward
    //handle FileLookupRequestMessage
    //Cache then Forward
    //handle StorageServiceFreeSpaceRequestMessage
    //unclear
    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int StorageServiceProxy::main() {
        //TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_CYAN);
        // Start file storage server
        //if(internalStorage){
        //    internalStorage->start(internalStorage,true,true);
        //}
        std::string message =
                "Proxy Server " + this->getName() + "  starting on host " + this->getHostname();
        WRENCH_INFO("%s",
                    message.c_str());

        /** Main loop **/
        while (this->processNextMessage()) {
        }

        WRENCH_INFO("Proxy Server Node %s on host %s cleanly terminating!",
                    this->getName().c_str(),
                    S4U_Simulation::getHostName().c_str());

        return 0;
    }
    /**
     * @brief Get the total static capacity of the remote storage service (in zero simulation time) if there is no remote, this function is invalid
     * @return capacity of the storage service (double) for each mount point, in a map
     */
    std::map<std::string, double> StorageServiceProxy::getTotalSpace() {
        if (remote) {
            return remote->getTotalSpace();
        }
        throw runtime_error("Proxy with no default location does not support getTotalSpace()");
    }
    bool StorageServiceProxy::hasFile(const std::shared_ptr<DataFile> &file, const std::string &path) {
        if (cache) {
            return cache->hasFile(file, path);
        }
        if (remote) {
            return remote->hasFile(file, path);
        }
        return false;
    }
    /**
     * @brief Process a received control message
     *
     * @return false if the daemon should terminate
     */
    bool StorageServiceProxy::processNextMessage() {
        //S4U_Simulation::compute(flops);
        S4U_Simulation::computeZeroFlop();
        // Wait for a message
        std::unique_ptr<SimulationMessage> message = nullptr;


        S4U_Simulation::compute(this->getPropertyValueAsDouble(StorageServiceProxyProperty::MESSAGE_OVERHEAD));
        try {
            message = S4U_Mailbox::getMessage(this->mailbox);
        } catch (ExecutionException &e) {
            WRENCH_INFO(
                    "Got a network error while getting some message... ignoring");
            return true;// oh well
        }

        WRENCH_DEBUG("Got a [%s] message", message->getName().c_str());

        if (auto msg = dynamic_cast<ServiceStopDaemonMessage *>(message.get())) {//handle all the rest of the messages
            try {
                S4U_Mailbox::dputMessage(msg->ack_mailbox,
                                         new ServiceDaemonStoppedMessage(this->getMessagePayloadValue(
                                                 ServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
            } catch (ExecutionException &e) {
                return false;
            }
            return false;
        } else if (auto msg = dynamic_cast<StorageServiceFileLookupRequestMessage *>(message.get())) {
            auto target = remote;

            if (auto location = std::dynamic_pointer_cast<ProxyLocation>(msg->location)) {
                target = location->target;
            }
            //cerr<<"FILE LOOKUP!!!! "<<remote<<" "<<target<<endl;
            if (cache->hasFile(msg->location->getFile(), msg->location->getFullAbsolutePath())) {//forward request to cache
                //cerr<<"File cached"<<endl;
                S4U_Mailbox::putMessage(msg->answer_mailbox, new StorageServiceFileLookupAnswerMessage(msg->location->getFile(), true, StorageServiceMessagePayload::FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD));

            } else if (target) {
                pending[msg->location->getFile()].push_back(std::move(message));
                //message=std::move(pending[msg->location->getFile()][0]);
                S4U_Mailbox::putMessage(
                        target->mailbox,
                        new StorageServiceFileLookupRequestMessage(
                                mailbox,
                                FileLocation::LOCATION(target, msg->location->getMountPoint(), msg->location->getFile()),
                                target->getMessagePayloadValue(
                                        StorageServiceMessagePayload::FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD)));
            } else {
                S4U_Mailbox::putMessage(msg->answer_mailbox, new StorageServiceFileLookupAnswerMessage(msg->location->getFile(), false, StorageServiceMessagePayload::FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD));
            }
        } else if ((this->*readMethod)(message)) {
            //no other handling required
        } else if (auto msg = dynamic_cast<StorageServiceFileDeleteRequestMessage *>(message.get())) {
            auto target = remote;
            if (auto location = std::dynamic_pointer_cast<ProxyLocation>(msg->location)) {
                target = location->target;
            }
            bool deleted = false;
            if (cache) {
                try {
                    cache->deleteFile(msg->location->getFile());
                    deleted = true;
                } catch (ExecutionException &e) {
                    //silently ignore
                }
            }
            if (target) {
                try {
                    target->deleteFile(msg->location->getFile());
                    deleted = true;
                } catch (ExecutionException &e) {
                    //silently ignore
                }
            }
            if (deleted) {
                S4U_Mailbox::putMessage(msg->answer_mailbox, new StorageServiceFileDeleteAnswerMessage(msg->location->getFile(), target, true, nullptr, StorageServiceMessagePayload::FILE_DELETE_ANSWER_MESSAGE_PAYLOAD));
            } else {
                S4U_Mailbox::putMessage(msg->answer_mailbox, new StorageServiceFileDeleteAnswerMessage(msg->location->getFile(), target, false, std::make_shared<FileNotFound>(msg->location), StorageServiceMessagePayload::FILE_DELETE_ANSWER_MESSAGE_PAYLOAD));
            }
        } else if (auto msg = dynamic_cast<StorageServiceFileWriteRequestMessage *>(message.get())) {
            auto target = remote;
            if (auto location = std::dynamic_pointer_cast<ProxyLocation>(msg->location)) {
                //WRENCH_INFO("Proxy Location");
                target = location->target;
            }
            //cerr<<"writing"<<target<<" "<<remote<<endl;
            if (cache) {     //check cache
                if (target) {//check that someone is waiting for this message
                    pending[msg->location->getFile()].push_back(std::move(message));
                    WRENCH_INFO("Adding pending write");
                }
                S4U_Mailbox::putMessage(cache->mailbox, new StorageServiceFileWriteRequestMessage(mailbox, msg->requesting_host, FileLocation::LOCATION(cache, msg->location->getFile()), msg->buffer_size, 0));
            } else {
                S4U_Mailbox::putMessage(msg->answer_mailbox, new StorageServiceFileWriteAnswerMessage(msg->location, false, std::make_shared<HostError>(hostname), 0, StorageServiceMessagePayload::FILE_WRITE_ANSWER_MESSAGE_PAYLOAD));
            }

        } else if (auto msg = dynamic_cast<StorageServiceFileCopyRequestMessage *>(message.get())) {
            throw std::runtime_error("Copy currently not supported for Storage Service Proxy");
            //            bool isDest
            //            auto target=remote;
            //            if(auto location=std::dynamic_pointer_cast<ProxyLocation>(msg->location)){
            //                target=location->target;
            //            }


            //        } else if (auto msg = dynamic_cast<StorageServiceMessage *>(message.get())) {//we got a message targeted at a normal storage server
            //            auto target=remote;
            //            if(auto location=std::dynamic_pointer_cast<ProxyLocation>(msg->location)){
            //                target=location->target;
            //            }
            //            if(not target){
            //                throw std::runtime_error("Can not forward File Lookup Request Message to null target");
            //            }
            //            if (target) {
            //                try{
            //                    S4U_Mailbox::dputMessage(cache->mailbox, message.release());
            //                    return true;
            //                }catch(...){}
            //
            //            }
            //            if(remote){
            //                S4U_Mailbox::dputMessage(remote->mailbox, message.release());
            //                return true;
            //            }
            //            throw std::runtime_error( "StorageServiceProxy:processNextMessage(): Unexpected [" + message->getName() + "] message that either could not be forwared");
        } else if (auto msg = dynamic_cast<StorageServiceFileLookupAnswerMessage *>(message.get())) {//Our remote lookup has finished
            std::vector<unique_ptr<SimulationMessage>> &messages = pending[msg->file];
            for (unsigned int i = 0; i < messages.size(); i++) {
                if (auto tmpMsg = dynamic_cast<StorageServiceFileLookupRequestMessage *>(messages[i].get())) {
                    S4U_Mailbox::putMessage(tmpMsg->answer_mailbox, new StorageServiceFileLookupAnswerMessage(*msg));
                    std::swap(messages[i], messages.back());
                    messages.pop_back();
                    i--;
                }
            }
        } else if (auto msg = dynamic_cast<StorageServiceFileWriteAnswerMessage *>(message.get())) {//forward through
            //WRENCH_INFO("Got write answer");
            std::vector<unique_ptr<SimulationMessage>> &messages = pending[msg->location->getFile()];
            for (unsigned int i = 0; i < messages.size(); i++) {
                if (auto tmpMsg = dynamic_cast<StorageServiceFileWriteRequestMessage *>(messages[i].get())) {
                    //WRENCH_INFO("Forwarding");
                    S4U_Mailbox::putMessage(tmpMsg->answer_mailbox, new StorageServiceFileWriteAnswerMessage(*msg));
                    if (!msg->success) {//the the file write failed, there will be no ack message
                        //WRENCH_INFO("write failed");
                        std::swap(messages[i], messages.back());
                        messages.pop_back();
                        i--;
                    }
                }
            }


        } else if (auto msg = dynamic_cast<StorageServiceAckMessage *>(message.get())) {//our file write has finished
            std::vector<unique_ptr<SimulationMessage>> &messages = pending[msg->location->getFile()];
            for (unsigned int i = 0; i < messages.size(); i++) {
                if (auto tmpMsg = dynamic_cast<StorageServiceFileWriteRequestMessage *>(messages[i].get())) {

                    S4U_Mailbox::putMessage(tmpMsg->answer_mailbox, new StorageServiceAckMessage(*msg));//forward ACK
                    auto target = remote;
                    if (auto location = std::dynamic_pointer_cast<ProxyLocation>(tmpMsg->location)) {
                        target = location->target;
                    }
                    //WRENCH_INFO("initiating file copy");
                    // Initiate File Copy but not wanting to receive an answer, hence the NULL_MAILBOX
                    initiateFileCopy(S4U_Mailbox::NULL_MAILBOX, FileLocation::LOCATION(cache, msg->location->getFile()), FileLocation::LOCATION(target, msg->location->getFile()));

                    std::swap(messages[i], messages.back());
                    messages.pop_back();
                    i--;
                }
            }
        } else {
            throw std::runtime_error(
                    "StorageServiceProxy:processNextMessage(): Unexpected [" + message->getName() + "] message");
        }
        return true;
    }
    /**
     * @brief Synchronously asks the remote storage service for its capacity at all its
     *        mount points.  invalid if there is no default location
     * @return The free space in bytes of each mount point, as a map
     *
     */
    std::map<std::string, double> StorageServiceProxy::getFreeSpace() {
        if (remote) {
            return remote->getFreeSpace();
        }
        throw runtime_error("Proxy with no default location does not support getFreeSpace()");
    }
    /**
     * @brief Get the mount point of the remote server (will throw is more than one).  If there isnt a default, returns DEV_NUL
     * @return the (sole) mount point of the service
     */
    std::string StorageServiceProxy::getMountPoint() {
        if (remote) {
            return remote->getMountPoint();
        }
        if (cache) {
            return cache->getMountPoint();
        }
        return LogicalFileSystem::DEV_NULL;
    }
    /**
     * @brief Get the set of mount points of the remote server, if not returns {}
     * @return the set of mount points
     */
    std::set<std::string> StorageServiceProxy::getMountPoints() {
        if (remote) {
            return remote->getMountPoints();
        }
        if (cache) {
            return cache->getMountPoints();
        }
        return {};
    }
    /**
     * @brief Checked whether the remote storage service has multiple mount points
     * @return true whether the service has multiple mount points
     */
    bool StorageServiceProxy::hasMultipleMountPoints() {
        if (remote) {
            return remote->hasMultipleMountPoints();
        }

        return false;
    }
    /**
    * @brief Checked whether the remote storage service has a particular mount point
    * @param mp: a mount point
    *
    * @return true whether the service has that mount point
    */
    bool StorageServiceProxy::hasMountPoint(const std::string &mp) {
        if (remote) {
            return remote->hasMountPoint(mp);
        }
        if (cache) {
            return cache->hasMountPoint(mp);
        }
        return false;
    }


    /**
         * @brief Forward to remote server
         * @param location: the file location
         * @return a (simulated) date in seconds
         */
    double StorageServiceProxy::getFileLastWriteDate(const std::shared_ptr<FileLocation> &location) {
        if (remote) {
            return remote->getFileLastWriteDate(location);
        }
        if (cache) {
            return cache->getFileLastWriteDate(location);
        }
        return -1;
    }


    /**
     * @brief StorageServiceProxy.createFile() is ambiguous where the file should go.  You should call createFile on the remote service where you wish to create the file.
If you want it to start cached, you should also call StorageServiceProxy.getCache().createFile
     * @param location: the file location
     *
     **/
    void StorageServiceProxy::createFile(const std::shared_ptr<FileLocation> &location) {
        throw std::runtime_error("StorageServiceProxy.createFile() is ambiguous where the file should go.  You should call createFile on the remote service where you wish to create the file.  \nIf you want it to start cached, you should also call StorageServiceProxy.getCache().createFile");
    }
    /**
     * @brief StorageServiceProxy.createFile() is ambiguous where the file should go.  You should call createFile on the remote service where you wish to create the file.
If you want it to start cached, you should also call StorageServiceProxy.getCache().createFile
     * @param file: the file to create
     * @param path: the file path
     *
     **/
    void StorageServiceProxy::createFile(const std::shared_ptr<DataFile> &file, const std::string &path) {
        throw std::runtime_error("StorageServiceProxy.createFile() is ambiguous where the file should go.  You should call createFile on the remote service where you wish to create the file.  \nIf you want it to start cached, you should also call StorageServiceProxy.getCache().createFile");
    }
    /**
     * @brief StorageServiceProxy.createFile() is ambiguous where the file should go.  You should call createFile on the remote service where you wish to create the file.
If you want it to start cached, you should also call StorageServiceProxy.getCache().createFile
     * @param file: the file to create
     *
     **/
    void StorageServiceProxy::createFile(const std::shared_ptr<DataFile> &file) {
        throw std::runtime_error("StorageServiceProxy.createFile() is ambiguous where the file should go.  You should call createFile on the remote service where you wish to create the file.  \nIf you want it to start cached, you should also call StorageServiceProxy.getCache().createFile");
    }
    /**
     * Get the load of the cache
     * @return the load of the cache
     */
    double StorageServiceProxy::getLoad() {
        if (cache) {
            return cache->getLoad();
        } else if (remote) {
            return remote->getLoad();
        }
        throw std::invalid_argument("This Proxy has not cache, no Remote fileserver to target, and no fallback default to use");
    }//cache

    /**
     * @brief Constructor
     * @param hostname The host to run on
     * @param cache A Storage server to use as a cache.  Ideal on the same host
     * @param default_remote A remote file server to use as a default target.  Defaults to nullptr
     * @param properties The wrench property overrides
     * @param message_payloads The wrench message payload overrides
     */
    StorageServiceProxy::StorageServiceProxy(const std::string &hostname,
                                             const std::shared_ptr<StorageService> &cache,
                                             const std::shared_ptr<StorageService> &default_remote,
                                             WRENCH_PROPERTY_COLLECTION_TYPE properties, WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE message_payloads) : StorageService(hostname,
                                                                                                                                                                  "storage_proxy"),
                                                                                                                                                   cache(cache),
                                                                                                                                                   remote(default_remote) {

        this->setProperties(this->default_property_values, std::move(properties));
        this->setMessagePayloads(this->default_messagepayload_values, std::move(message_payloads));
        this->setProperty(StorageServiceProperty::BUFFER_SIZE, cache->getPropertyValueAsString(StorageServiceProperty::BUFFER_SIZE));//the internal cache has the same buffer properties as this service.
        if (cache and cache->hasMultipleMountPoints()) {
            throw std::invalid_argument("StorageServiceProxy::StorageServiceProxy() A storage service proxy's cache can not have multiple mountpoints");
        }
        if (remote and remote->hasMultipleMountPoints()) {
            throw std::invalid_argument("StorageServiceProxy::StorageServiceProxy() A storage service proxy's default remote can not have multiple mountpoints");
        }
        string readProperty = getPropertyValueAsString(StorageServiceProxyProperty::UNCACHED_READ_METHOD);
        WRENCH_DEBUG("%s", readProperty.c_str());
        if (readProperty == "CopyThenRead") {
            readMethod = &StorageServiceProxy::copyThenRead;
        } else if (readProperty == "MagicRead") {
            readMethod = &StorageServiceProxy::magicRead;
        } else if (readProperty == "ReadThrough") {
            readMethod = &StorageServiceProxy::readThrough;
        } else {
            throw invalid_argument("Unknown value " + readProperty + " for StorageServiceProxyProperty::UNCACHED_READ_METHOD");
        }
    }
    /**
     * @brief Delete a file
     * @param targetServer
     * @param file
     * @param file_registry_service
     */
    void StorageServiceProxy::deleteFile(const std::shared_ptr<StorageService> &targetServer, const std::shared_ptr<DataFile> &file, const std::shared_ptr<FileRegistryService> &file_registry_service) {
        StorageService::deleteFile(ProxyLocation::LOCATION(targetServer, std::static_pointer_cast<StorageService>(shared_from_this()), file), file_registry_service);
    }

    /**
     * @brief Lookup a file
     * @param targetServer: the target server
     * @param file: the file
     *
     * @return true if the file is present, false otherwise
     */
    bool StorageServiceProxy::lookupFile(const std::shared_ptr<StorageService> &targetServer, const std::shared_ptr<DataFile> &file) {
        return StorageService::lookupFile(ProxyLocation::LOCATION(targetServer, std::static_pointer_cast<StorageService>(shared_from_this()), file));
    }

    /**
     * @brief Read a file
     * @param file: the file
     */
    void StorageServiceProxy::readFile(const std::shared_ptr<DataFile> &file) {
        StorageService::readFile(ProxyLocation::LOCATION(remote, std::static_pointer_cast<StorageService>(shared_from_this()), file));
    }

    /**
     * @brief Read a file
     * @param targetServer: the target server
     * @param file: the file
     */
    void StorageServiceProxy::readFile(const std::shared_ptr<StorageService> &targetServer, const std::shared_ptr<DataFile> &file) {
        StorageService::readFile(ProxyLocation::LOCATION(targetServer, std::static_pointer_cast<StorageService>(shared_from_this()), file));
    }

    /**
     * @brief Read a file
     * @param targetServer: the target server
     * @param file: the file
     * @param num_bytes: the number of bytes to read
     */
    void StorageServiceProxy::readFile(const std::shared_ptr<StorageService> &targetServer, const std::shared_ptr<DataFile> &file, double num_bytes) {
        StorageService::readFile(ProxyLocation::LOCATION(targetServer, std::static_pointer_cast<StorageService>(shared_from_this()), file), num_bytes);
    }

    /**
     * @brief Read a file
     * @param targetServer: the target server
     * @param file: the file
     * @param path: the file path
     */
    void StorageServiceProxy::readFile(const std::shared_ptr<StorageService> &targetServer, const std::shared_ptr<DataFile> &file, const std::string &path) {
        StorageService::readFile(ProxyLocation::LOCATION(targetServer, std::static_pointer_cast<StorageService>(shared_from_this()), path, file));
    }

    /**
     * @brief Read a file
     * @param targetServer: the target server
     * @param file: the file
     * @param path: the file path
     * @param num_bytes: the number of bytes to read
     */
    void StorageServiceProxy::readFile(const std::shared_ptr<StorageService> &targetServer, const std::shared_ptr<DataFile> &file, const std::string &path, double num_bytes) {
        StorageService::readFile(ProxyLocation::LOCATION(targetServer, std::static_pointer_cast<StorageService>(shared_from_this()), path, file), num_bytes);
    }

    /**
     * @brief Write a file
     * @param targetServer: the target server
     * @param file: the file
     * @param path: the file path
     */
    void StorageServiceProxy::writeFile(const std::shared_ptr<StorageService> &targetServer, const std::shared_ptr<DataFile> &file, const std::string &path) {
        StorageService::writeFile(ProxyLocation::LOCATION(targetServer, std::static_pointer_cast<StorageService>(shared_from_this()), path, file));
    }

    /**
     * @brief Write a file
     * @param targetServer: the target server
     * @param file: the file
     */
    void StorageServiceProxy::writeFile(const std::shared_ptr<StorageService> &targetServer, const std::shared_ptr<DataFile> &file) {
        StorageService::writeFile(ProxyLocation::LOCATION(targetServer, std::static_pointer_cast<StorageService>(shared_from_this()), file));
    }

    /**
     * @brief Get the proxy's associated cache
     *
     * @return the cache storage service
     */
    const std::shared_ptr<StorageService> StorageServiceProxy::getCache() {
        return this->cache;
    }

    /**
     * @brief Check the cache and ongoing reads for the requested file.  If it is, this function handles the rest of reading it.
     * @param msg the message that is being processed
     * @param message the raw unique_ptr for the message.  Assumed to be the same as msg, passed to avoid second cast.
     * @return True if the file is cached, false otherwise
     */
    bool StorageServiceProxy::commonReadFile(StorageServiceFileReadRequestMessage *msg, unique_ptr<SimulationMessage> &message) {
        if (cache->hasFile(msg->location->getFile(), msg->location->getFullAbsolutePath())) {//check cache
            WRENCH_INFO("Forwarding to cache reply mailbox %s", msg->answer_mailbox->get_name().c_str());
            S4U_Mailbox::putMessage(
                    cache->mailbox,
                    new StorageServiceFileReadRequestMessage(
                            msg->answer_mailbox,
                            msg->requesting_host,//msg->mailbox_to_receive_the_file_content,
                            FileLocation::LOCATION(cache, msg->location->getMountPoint(),
                                                   msg->location->getFile()),
                            msg->num_bytes_to_read, 0));
            return true;

        } else {
            std::vector<unique_ptr<SimulationMessage>> &messages = pending[msg->location->getFile()];
            for (unsigned int i = 0; i < messages.size(); i++) {
                if (auto tmpMsg = dynamic_cast<StorageServiceFileWriteRequestMessage *>(messages[i].get())) {
                    //there is A writeRequest for this file in progress, ignore
                    pending[msg->location->getFile()].push_back(std::move(message));
                    return true;
                }
            }
            //the message was not cached, and is not in progress of being read
        }
        return false;
    }

    /**
     * @brief function for CopyThenRead method. this function will handle everything to do with StorageServerReadRequestMessage.  Also handles StorageServerFileCopyAnswer.  The only behavioral difference is in uncached files. This copies the file requested to the cache, and forwards the ongoing reads to the cache.  This is the default, and gives the most accurate time-to-cache for a file, and the most accurate network congestion, but overestimates how long the file will take to arive at the end.
     * @param message the message that is being processed
     * @return True if the message was processed by this function.  False otherwise
     */
    bool StorageServiceProxy::copyThenRead(unique_ptr<SimulationMessage> &message) {
        WRENCH_DEBUG("Trying copyThenRead(message)");
        if (auto msg = dynamic_cast<StorageServiceFileReadRequestMessage *>(message.get())) {
            if (commonReadFile(msg, message)) {
                return true;
            }
            auto target = remote;
            if (auto location = std::dynamic_pointer_cast<ProxyLocation>(msg->location)) {
                target = location->target;
            }
            if (target) {

                pending[msg->location->getFile()].push_back(std::move(message));
                //TODO add magicRead and readThrough
                //current is copyThenRead
                //Magic read: copy to internal->instantly report to client when its done (read answer buffersize 0, then emediatly ack)
                //Readthrough: read from target to client emediatly, then instantly create on cache.  REQUIRES EXTANT NETWORK PATH
                //cached behavior for all 3 is the same.
                //concurrent first read behavior:
                //copyThenRead: all block until copy finished, then all read
                //magicRead:    all block until copy finsished, then all magic read
                //readthrough:  all block until first read is finished, then all others read
                //do not speed excessive time on readThrough
                StorageService::initiateFileCopy(mailbox, FileLocation::LOCATION(target, msg->location->getFile()), FileLocation::LOCATION(cache, msg->location->getFile()));
            } else {
                S4U_Mailbox::putMessage(msg->answer_mailbox, new StorageServiceFileReadAnswerMessage(msg->location, false, std::make_shared<FileNotFound>(msg->location), nullptr, 0, StorageServiceMessagePayload::FILE_READ_ANSWER_MESSAGE_PAYLOAD));
            }
            return true;
        } else if (auto msg = dynamic_cast<StorageServiceFileCopyAnswerMessage *>(message.get())) {//Our remote read request has finished
            std::vector<unique_ptr<SimulationMessage>> &messages = pending[msg->src->getFile()];
            for (unsigned int i = 0; i < messages.size(); i++) {
                if (auto tmpMsg = dynamic_cast<StorageServiceFileReadRequestMessage *>(messages[i].get())) {
                    if (msg->success) {
                        tmpMsg->payload = 0;                     //this message has already been sent, this is a fake resend
                        S4U_Mailbox::putMessage(mailbox, tmpMsg);//now that the data is cached, resend the message
                        std::swap(messages[i], messages.back());
                        messages.back().release();
                        messages.pop_back();
                        i--;
                    } else {
                        S4U_Mailbox::putMessage(tmpMsg->answer_mailbox, new StorageServiceFileReadAnswerMessage(tmpMsg->location, false, msg->failure_cause, nullptr, 0, StorageServiceMessagePayload::FILE_READ_ANSWER_MESSAGE_PAYLOAD));
                        std::swap(messages[i], messages.back());
                        messages.pop_back();
                        i--;
                    }
                }
                return true;
            }
        }
        return false;
    }
    /**
    * @brief function for MagicRead method. this function will handle everything to do with StorageServerReadRequestMessage.  Also handles StorageServerFileCopyAnswer.  The only behavioral difference is in uncached files.  This copies the file to the cache, and then instantly "magically" transfers the file to anyone waiting on it. This gives the most accurate time-to-cache, and a reasonably accurate arrival time by assuming the bottleneck is the bandwidth from cache to remote, not the internal network.  This does sacrifice some internal network congestion.
    * @param message the message that is being processed
    * @return True if the message was processed by this function.  False otherwise
    */
    bool StorageServiceProxy::magicRead(unique_ptr<SimulationMessage> &message) {
        WRENCH_DEBUG("Trying magicRead(message)");
        if (auto msg = dynamic_cast<StorageServiceFileReadRequestMessage *>(message.get())) {
            //Same FileRead as ReadThenCopy, only CopyAnswer should be different
            if (commonReadFile(msg, message)) {
                return true;
            }
            auto target = remote;
            if (auto location = std::dynamic_pointer_cast<ProxyLocation>(msg->location)) {
                target = location->target;
            }
            if (target) {

                pending[msg->location->getFile()].push_back(std::move(message));
                StorageService::initiateFileCopy(mailbox, FileLocation::LOCATION(target, msg->location->getFile()), FileLocation::LOCATION(cache, msg->location->getFile()));
            } else {
                S4U_Mailbox::putMessage(msg->answer_mailbox, new StorageServiceFileReadAnswerMessage(msg->location, false, std::make_shared<FileNotFound>(msg->location), nullptr, 0, StorageServiceMessagePayload::FILE_READ_ANSWER_MESSAGE_PAYLOAD));
            }
            return true;
        } else if (auto msg = dynamic_cast<StorageServiceFileCopyAnswerMessage *>(message.get())) {//Our remote read request has finished
            std::vector<unique_ptr<SimulationMessage>> &messages = pending[msg->src->getFile()];
            for (unsigned int i = 0; i < messages.size(); i++) {
                if (auto tmpMsg = dynamic_cast<StorageServiceFileReadRequestMessage *>(messages[i].get())) {
                    if (msg->success) {
                        S4U_Mailbox::putMessage(tmpMsg->answer_mailbox, new StorageServiceFileReadAnswerMessage(tmpMsg->location, true, nullptr, nullptr, 0, StorageServiceMessagePayload::FILE_READ_ANSWER_MESSAGE_PAYLOAD));//magic read, send buffersize 0 and we are assumed to be nonbufferized
                        S4U_Mailbox::putMessage(tmpMsg->answer_mailbox, new StorageServiceAckMessage(tmpMsg->location));                                                                                                      //emediatly send the expected ack
                        std::swap(messages[i], messages.back());
                        messages.pop_back();
                        i--;
                    } else {
                        S4U_Mailbox::putMessage(tmpMsg->answer_mailbox, new StorageServiceFileReadAnswerMessage(tmpMsg->location, false, msg->failure_cause, nullptr, 0, StorageServiceMessagePayload::FILE_READ_ANSWER_MESSAGE_PAYLOAD));
                        std::swap(messages[i], messages.back());
                        messages.pop_back();
                        i--;
                    }
                }
                return true;
            }
        }
        return false;
    }
    /**
     * @brief function for CopyThenRead ReadThrough method. this function will handle everything to do with StorageServerReadRequestMessage.  Also handles StorageServiceAnswerMessage and some StorageService Ack messages. The only behavioral difference is in uncached files.  This reads the file directly to the client with the proxy acting as a mediary.  Once the write finishes, the file is instantly created on the cache.  Assuming the network is configured properly, this gives the best network congestion and time-to-arival estimate, but at the cost of time-to-cache, which it over estimates.  Concurrent reads will wait until the file is cached.
     * @param message the message that is being processed
     * @return True if the message was processed by this function.  False otherwise
     */
    bool StorageServiceProxy::readThrough(unique_ptr<SimulationMessage> &message) {
        WRENCH_DEBUG("Trying readThrough(message)");
        if (auto msg = dynamic_cast<StorageServiceFileReadRequestMessage *>(message.get())) {

            if (commonReadFile(msg, message)) {
                return true;
            }
            auto target = remote;
            if (auto location = std::dynamic_pointer_cast<ProxyLocation>(msg->location)) {
                target = location->target;
            }
            if (target) {

                pending[msg->location->getFile()].push_back(std::move(message));
                //pending[msg->location->getFile()].push_back(std::move(message));
                //Readthrough: read from target to client emediatly, then instantly create on cache.  REQUIRES EXTANT NETWORK PATH
                //readthrough:  all block until first read is finished, then all others read
                //do not speed excessive time on readThrough
                auto forward = new StorageServiceFileReadRequestMessage(msg);
                forward->answer_mailbox = mailbox;                                                                                 //setup intercept mailbox
                forward->location = FileLocation::LOCATION(target, msg->location->getFullAbsolutePath(), msg->location->getFile());//hyjack locaiton to be on target
                S4U_Mailbox::putMessage(target->mailbox, forward);                                                                 //send to target
            } else {
                S4U_Mailbox::putMessage(msg->answer_mailbox, new StorageServiceFileReadAnswerMessage(msg->location, false, std::make_shared<FileNotFound>(msg->location), nullptr, 0, StorageServiceMessagePayload::FILE_READ_ANSWER_MESSAGE_PAYLOAD));
            }
            return true;
        } else if (auto msg = dynamic_cast<StorageServiceFileReadAnswerMessage *>(message.get())) {//Our readthrough is in progress
            auto &messages = pending[msg->location->getFile()];

            for (unsigned int i = 0; i < messages.size(); i++) {
                if (auto tmpMsg = dynamic_cast<StorageServiceFileReadRequestMessage *>(messages[i].get())) {
                    if (msg->success) {
                        cache->createFile(msg->location->getFile());
                        msg->location = tmpMsg->location;                                  //fix up the location
                        S4U_Mailbox::putMessage(tmpMsg->answer_mailbox, message.release());//forward success message to first waiting read host
                        return true;
                    } else {
                        //remote read has failed, notify all waiting
                        S4U_Mailbox::putMessage(tmpMsg->answer_mailbox, new StorageServiceFileReadAnswerMessage(tmpMsg->location, false, msg->failure_cause, nullptr, 0, StorageServiceMessagePayload::FILE_READ_ANSWER_MESSAGE_PAYLOAD));
                        std::swap(messages[i], messages.back());
                        messages.pop_back();
                        i--;
                    }
                }
                return true;
            }
        } else if (auto msg = dynamic_cast<StorageServiceAckMessage *>(message.get())) {//Our readthrough has finished
            if (msg->location->getStorageService() == shared_from_this() or msg->location->getStorageService() == cache) {
                //this is not a proxied Ack, this is actually directed to us
                return false;
            }
            std::vector<unique_ptr<SimulationMessage>> &messages = pending[msg->location->getFile()];
            bool first = true;
            for (unsigned int i = 0; i < messages.size(); i++) {
                if (auto tmpMsg = dynamic_cast<StorageServiceFileReadRequestMessage *>(messages[i].get())) {
                    if (first) {//this is the fileread we have been faking
                        S4U_Mailbox::putMessage(tmpMsg->answer_mailbox, new StorageServiceAckMessage(*msg));
                        std::swap(messages[i], messages.back());
                        messages.back().release();
                        messages.pop_back();
                        i--;
                        first = false;
                    } else {                                     //these are the pending reads
                        tmpMsg->payload = 0;                     //this message has already been sent, this is a fake resend
                        S4U_Mailbox::putMessage(mailbox, tmpMsg);//these should now be cached, and should just drop down to the cache automatically
                        std::swap(messages[i], messages.back());
                        messages.back().release();
                        messages.pop_back();
                        i--;
                    }
                }
                return true;
            }
        }
        return false;
    }
}// namespace wrench
