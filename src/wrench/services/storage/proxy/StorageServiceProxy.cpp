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
#include "wrench/simgrid_S4U_util/S4U_CommPort.h"
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
        WRENCH_DEBUG("%s",
                     message.c_str());

        /** Main loop **/
        while (this->processNextMessage()) {
        }

        WRENCH_DEBUG("Proxy Server Node %s on host %s cleanly terminating!",
                     this->getName().c_str(),
                     S4U_Simulation::getHostName().c_str());

        return 0;
    }


    bool StorageServiceProxy::hasFile(const std::shared_ptr<FileLocation> &location) {
        if (cache) {
            return cache->hasFile(location);
        }
        if (remote) {
            return remote->hasFile(location);
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
        std::unique_ptr<ServiceMessage> message = nullptr;


        S4U_Simulation::compute(this->getPropertyValueAsDouble(StorageServiceProxyProperty::MESSAGE_OVERHEAD));
        try {
            message = this->commport->getMessage<ServiceMessage>();
        } catch (ExecutionException &e) {
            WRENCH_DEBUG(
                    "Got a network error while getting some message... ignoring");
            return true;// oh well
        }

        WRENCH_DEBUG("Got a [%s] message", message->getName().c_str());

        if (auto msg = dynamic_cast<ServiceStopDaemonMessage *>(message.get())) {//handle all the rest of the messages
            try {
                msg->ack_commport->dputMessage(
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
            if (cache->hasFile(msg->location)) {//forward request to cache
                //cerr<<"File cached"<<endl;
                msg->answer_commport->dputMessage(new StorageServiceFileLookupAnswerMessage(msg->location->getFile(), true, StorageServiceMessagePayload::FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD));

            } else if (target) {
                pending[msg->location->getFile()].push_back(std::move(message));
                //message=std::move(pending[msg->location->getFile()][0]);
                target->commport->dputMessage(
                        new StorageServiceFileLookupRequestMessage(
                                commport,
                                FileLocation::LOCATION(target, msg->location->getPath(), msg->location->getFile()),
                                target->getMessagePayloadValue(
                                        StorageServiceMessagePayload::FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD)));
            } else {
                msg->answer_commport->dputMessage(new StorageServiceFileLookupAnswerMessage(msg->location->getFile(), false, StorageServiceMessagePayload::FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD));
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
                msg->answer_commport->dputMessage(new StorageServiceFileDeleteAnswerMessage(msg->location->getFile(), target, true, nullptr, StorageServiceMessagePayload::FILE_DELETE_ANSWER_MESSAGE_PAYLOAD));
            } else {
                msg->answer_commport->dputMessage(new StorageServiceFileDeleteAnswerMessage(msg->location->getFile(), target, false, std::make_shared<FileNotFound>(msg->location), StorageServiceMessagePayload::FILE_DELETE_ANSWER_MESSAGE_PAYLOAD));
            }
        } else if (auto msg = dynamic_cast<StorageServiceFileWriteRequestMessage *>(message.get())) {
            auto target = remote;
            if (auto location = std::dynamic_pointer_cast<ProxyLocation>(msg->location)) {
                //WRENCH_DEBUG("Proxy Location");
                target = location->target;
            }
            //cerr<<"writing"<<target<<" "<<remote<<endl;
            if (cache) {     //check cache
                if (target) {//check that someone is waiting for this message
                    pending[msg->location->getFile()].push_back(std::move(message));
                    WRENCH_DEBUG("Adding pending write");
                }
                cache->commport->putMessage(new StorageServiceFileWriteRequestMessage(commport, msg->requesting_host,
                                                                                                  FileLocation::LOCATION(cache, msg->location->getFile()), msg->location->getFile()->getSize(), 0));
            } else {
                msg->answer_commport->putMessage(new StorageServiceFileWriteAnswerMessage(msg->location, false, nullptr, {}, 0, StorageServiceMessagePayload::FILE_WRITE_ANSWER_MESSAGE_PAYLOAD));
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
            //                    S4U_CommPort::dputMessage(cache->commport, message.release());
            //                    return true;
            //                }catch(...){}
            //
            //            }
            //            if(remote){
            //                S4U_CommPort::dputMessage(remote->commport, message.release());
            //                return true;
            //            }
            //            throw std::runtime_error( "StorageServiceProxy:processNextMessage(): Unexpected [" + message->getName() + "] message that either could not be forwared");
        } else if (auto msg = dynamic_cast<StorageServiceFileLookupAnswerMessage *>(message.get())) {//Our remote lookup has finished
            std::vector<unique_ptr<ServiceMessage>> &messages = pending[msg->file];
            for (unsigned int i = 0; i < messages.size(); i++) {
                if (auto tmpMsg = dynamic_cast<StorageServiceFileLookupRequestMessage *>(messages[i].get())) {
                    tmpMsg->answer_commport->dputMessage(new StorageServiceFileLookupAnswerMessage(*msg));
                    std::swap(messages[i], messages.back());
                    messages.pop_back();
                    i--;
                }
            }
        } else if (auto msg = dynamic_cast<StorageServiceFileWriteAnswerMessage *>(message.get())) {//forward through
            //WRENCH_DEBUG("Got write answer");
            std::vector<unique_ptr<ServiceMessage>> &messages = pending[msg->location->getFile()];
            for (unsigned int i = 0; i < messages.size(); i++) {
                if (auto tmpMsg = dynamic_cast<StorageServiceFileWriteRequestMessage *>(messages[i].get())) {
                    //WRENCH_DEBUG("Forwarding");
                    tmpMsg->answer_commport->dputMessage(new StorageServiceFileWriteAnswerMessage(*msg));
                    if (!msg->success) {//the the file write failed, there will be no ack message
                        //WRENCH_DEBUG("write failed");
                        std::swap(messages[i], messages.back());
                        messages.pop_back();
                        i--;
                    }
                }
            }


        } else if (auto msg = dynamic_cast<StorageServiceAckMessage *>(message.get())) {//our file write has finished
            std::vector<unique_ptr<ServiceMessage>> &messages = pending[msg->location->getFile()];
            for (unsigned int i = 0; i < messages.size(); i++) {
                if (auto tmpMsg = dynamic_cast<StorageServiceFileWriteRequestMessage *>(messages[i].get())) {

                    tmpMsg->answer_commport->dputMessage(new StorageServiceAckMessage(*msg));//forward ACK
                    auto target = remote;
                    if (auto location = std::dynamic_pointer_cast<ProxyLocation>(tmpMsg->location)) {
                        target = location->target;
                    }
                    //WRENCH_DEBUG("initiating file copy");
                    // Initiate File Copy but not wanting to receive an answer, hence the NULL_COMMPORT
                    initiateFileCopy(S4U_CommPort::NULL_COMMPORT, FileLocation::LOCATION(cache, msg->location->getFile()), FileLocation::LOCATION(target, msg->location->getFile()));

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
     * @brief Synchronously asks the proxy service for its capacity (which returns
     *        the remote)
     *
     * @param path: the path
     * @return The free space in bytes of each mount point, as a map
     *
     */
    double StorageServiceProxy::getTotalFreeSpaceAtPath(const std::string &path) {
        if (remote) {
            return remote->getTotalFreeSpaceAtPath(path);
        }
        throw runtime_error("Proxy with no default location does not support getFreeSpace()");
    }

    /**
     * @brief Synchronously asks the proxy service for its total capacity (which returns
     *        the remote)
     * @return The free space in bytes of each mount point, as a map
     *
     */
    double StorageServiceProxy::getTotalSpace() {
        if (remote) {
            return remote->getTotalSpace();
        }
        throw runtime_error("Proxy with no default location does not support getTotalSpace()");
    }


    //    /**
    //     * @brief Get the mount point of the remote server (will throw is more than one).  If there isnt a default, returns DEV_NUL
    //     * @return the (sole) mount point of the service
    //     */
    //    std::string StorageServiceProxy::getMountPoint() {
    //        if (remote) {
    //            return remote->getMountPoint();
    //        }
    //        if (cache) {
    //            return cache->getMountPoint();
    //        }
    //        return LogicalFileSystem::DEV_NULL;
    //    }

    /**
     * @brief Returns true if the cache is bufferized, false otherwise
     * @return true or false
     */
    bool StorageServiceProxy::isBufferized() const {
        if (cache) {
            return cache->isBufferized();
        } else {
            return false;// Not sure that makes sense
        }
    }

    /**
     * @brief Remove a directory and all its content at the storage service (in zero simulated time)
     * @param path: a path
     */
    void StorageServiceProxy::removeDirectory(const std::string &path) {
        throw std::runtime_error("StorageServiceProxy::removeDirectory(): not implemented yet");
    }

    /**
     * @brief Get the buffer size of the cache (which could be 0 or >0), or 0 if there is no cache
     * @return a size in bytes
     */
    double StorageServiceProxy::getBufferSize() const {
        if (cache) {
            return cache->getBufferSize();
        } else {
            return 0;// Not sure that makes sense
        }
    }


    //    /**
    //     * @brief Get the set of mount points of the remote server, if not returns {}
    //     * @return the set of mount points
    //     */
    //    std::set<std::string> StorageServiceProxy::getMountPoints() {
    //        if (remote) {
    //            return remote->getMountPoints();
    //        }
    //        if (cache) {
    //            return cache->getMountPoints();
    //        }
    //        return {};
    //    }
    //    /**
    //     * @brief Checked whether the remote storage service has multiple mount points
    //     * @return true whether the service has multiple mount points
    //     */
    //    bool StorageServiceProxy::hasMultipleMountPoints() {
    //        if (remote) {
    //            return remote->hasMultipleMountPoints();
    //        }
    //
    //        return false;
    //    }
    //    /**
    //    * @brief Checked whether the remote storage service has a particular mount point
    //    * @param mp: a mount point
    //    *
    //    * @return true whether the service has that mount point
    //    */
    //    bool StorageServiceProxy::hasMountPoint(const std::string &mp) {
    //        if (remote) {
    //            return remote->hasMountPoint(mp);
    //        }
    //        if (cache) {
    //            return cache->hasMountPoint(mp);
    //        }
    //        return false;
    //    }


    /**
     * @brief Forward to remote server
     * @param location: the file location
     * @return a (simulated) date in seconds
     */
    double StorageServiceProxy::getFileLastWriteDate(const std::shared_ptr<FileLocation> &location) {
        if (location == nullptr) {
            throw std::invalid_argument("StorageServiceProxy::getFileLastWriteDate(): Invalid nullptr argument");
        }
        if (remote) {
            return remote->getFileLastWriteDate(location);
        }
        if (cache) {
            return cache->getFileLastWriteDate(location);
        }
        return -1;
    }


    /**
     * @brief StorageServiceProxy.createFile() is ambiguous where the file should go.  You should call createFile
     * on the remote service where you wish to create the file. If you want it to start cached, you should also call
     * StorageServiceProxy.getCache().createFile
     * @param location: the file location
     *
     **/
    void StorageServiceProxy::createFile(const std::shared_ptr<FileLocation> &location) {
        throw std::runtime_error("StorageServiceProxy.createFile(): is ambiguous where the file should be removed. You should call createFile() on the remote service where you wish to create the file.  \nIf you want it to start cached, you should also call StorageServiceProxy.getCache().createFile()");
    }

    /**
     * @brief StorageServiceProxy.removeFile() is ambiguous where the file should go.  You should call removeFile
     * on the remote service where you wish to remove the file. If you want it to start cached, you should also call
     * StorageServiceProxy.getCache().removeFile
     * @param location: the file location
     *
     **/
    void StorageServiceProxy::removeFile(const std::shared_ptr<FileLocation> &location) {
        throw std::runtime_error("StorageServiceProxy.removeFile(): is ambiguous where the file should be removed. You should call removeFile() on the remote service where you wish to remove the file.");
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


        if (properties.find(StorageServiceProperty::BUFFER_SIZE) != properties.end()) {
            throw std::invalid_argument("StorageServiceProxy::StorageServiceProxy(): You cannot pass a buffer size property to a StorageServiceProxy");
        }
        this->setProperties(this->default_property_values, std::move(properties));
        this->setMessagePayloads(this->default_messagepayload_values, std::move(message_payloads));
        this->setProperty(StorageServiceProperty::BUFFER_SIZE, cache->getPropertyValueAsString(StorageServiceProperty::BUFFER_SIZE));//the internal cache has the same buffer properties as this service.

        //        if (cache and cache->hasMultipleMountPoints()) {
        //            throw std::invalid_argument("StorageServiceProxy::StorageServiceProxy(): A storage service proxy's cache can not have multiple mountpoints");
        //        }
        //        if (remote and remote->hasMultipleMountPoints()) {
        //            throw std::invalid_argument("StorageServiceProxy::StorageServiceProxy(): A storage service proxy's default remote can not have multiple mountpoints");
        //        }
        //        if (cache and default_remote) {
        //            if ((cache->isBufferized() and not default_remote->isBufferized()) or
        //                (not cache->isBufferized() and default_remote->isBufferized())) {
        //                throw std::invalid_argument("StorageServiceProxy::StorageServiceProxy(): The cache and the default_remote storage services must has the same bufferization mode");
        //        }

        string readProperty = getPropertyValueAsString(StorageServiceProxyProperty::UNCACHED_READ_METHOD);
        //        WRENCH_DEBUG("%s", readProperty.c_str());
        if (readProperty == "CopyThenRead") {
            readMethod = &StorageServiceProxy::copyThenRead;
        } else if (readProperty == "MagicRead") {
            readMethod = &StorageServiceProxy::magicRead;
        } else if (readProperty == "ReadThrough") {
            readMethod = &StorageServiceProxy::readThrough;
        } else {
            throw invalid_argument("Unknown value " + readProperty + " for StorageServiceProxyProperty::UNCACHED_READ_METHOD");
        }
        this->network_timeout = -1.0;//turn off network time out.  A proxy will wait to respond to a second file request until it has downloaded the file completely.  For large files this can easily excede any reasonable timeout.  So we dissable it completely
    }

    /**
     * @brief Delete a file
     * @param targetServer: the target server
     * @param file: the file 
     */
    void StorageServiceProxy::deleteFile(const std::shared_ptr<StorageService> &targetServer, const std::shared_ptr<DataFile> &file) {
        this->deleteFile(ProxyLocation::LOCATION(targetServer, std::static_pointer_cast<StorageService>(shared_from_this()), file));
    }

    /**
     * @brief Lookup a file
     * @param targetServer: the target server
     * @param file: the file
     *
     * @return true if the file is present, false otherwise
     */
    bool StorageServiceProxy::lookupFile(const std::shared_ptr<StorageService> &targetServer, const std::shared_ptr<DataFile> &file) {
        return this->lookupFile(ProxyLocation::LOCATION(targetServer, this->getSharedPtr<StorageService>(), file));
    }

    //    /**
    //     * @brief Read a file
    //     * @param location: a location
    //     * @param num_bytes: a number of bytes to read
    //     */
    //    void StorageServiceProxy::readFile(const std::shared_ptr<FileLocation> &location, double num_bytes) {
    //        this->readFile(ProxyLocation::LOCATION(remote, this->getSharedPtr<StorageService>(), location->getFile()));
    //    }

    /**
     * @brief Read a file
     * @param targetServer: the target server
     * @param file: the file
     */
    void StorageServiceProxy::readFile(const std::shared_ptr<StorageService> &targetServer, const std::shared_ptr<DataFile> &file) {
        this->readFile(ProxyLocation::LOCATION(targetServer, this->getSharedPtr<StorageService>(), file));
    }

    /**
     * @brief Read a file
     * @param targetServer: the target server
     * @param file: the file
     * @param num_bytes: the number of bytes to read
     */
    void StorageServiceProxy::readFile(const std::shared_ptr<StorageService> &targetServer, const std::shared_ptr<DataFile> &file, double num_bytes) {
        this->readFile(ProxyLocation::LOCATION(targetServer, this->getSharedPtr<StorageService>(), file), num_bytes);
    }

    /**
 * @brief Read a file
 * @param targetServer: the target server
 * @param file: the file
 * @param path: the file path
 */
    void StorageServiceProxy::readFile(const std::shared_ptr<StorageService> &targetServer, const std::shared_ptr<DataFile> &file, const std::string &path) {
        this->readFile(ProxyLocation::LOCATION(targetServer, this->getSharedPtr<StorageService>(), path, file));
    }

    /**
 * @brief Read a file
 * @param targetServer: the target server
 * @param file: the file
 * @param path: the file path
 * @param num_bytes: the number of bytes to read
 */
    void StorageServiceProxy::readFile(const std::shared_ptr<StorageService> &targetServer, const std::shared_ptr<DataFile> &file, const std::string &path, double num_bytes) {
        this->readFile(ProxyLocation::LOCATION(targetServer, std::static_pointer_cast<StorageService>(shared_from_this()), path, file), num_bytes);
    }

    /**
 * @brief Write a file
 * @param targetServer: the target server
 * @param file: the file
 * @param path: the file path
 */
    void StorageServiceProxy::writeFile(const std::shared_ptr<StorageService> &targetServer, const std::shared_ptr<DataFile> &file, const std::string &path) {
        this->writeFile(ProxyLocation::LOCATION(targetServer, std::static_pointer_cast<StorageService>(shared_from_this()), path, file));
    }

    /**
 * @brief Write a file
 * @param targetServer: the target server
 * @param file: the file
 */
    void StorageServiceProxy::writeFile(const std::shared_ptr<StorageService> &targetServer, const std::shared_ptr<DataFile> &file) {
        this->writeFile(ProxyLocation::LOCATION(targetServer, std::static_pointer_cast<StorageService>(shared_from_this()), file));
    }

    /**
 * @brief Get the proxy's associated cache
 *
 * @return the cache storage service
 */
    std::shared_ptr<StorageService> StorageServiceProxy::getCache() {
        return this->cache;
    }

    /**
     * @brief Check the cache and ongoing reads for the requested file.  If it is, this function handles the rest of reading it.
     * @param msg the message that is being processed
     * @param message the raw unique_ptr for the message.  Assumed to be the same as msg, passed to avoid second cast.
     * @return True if the file is cached, false otherwise
     */
    bool StorageServiceProxy::commonReadFile(StorageServiceFileReadRequestMessage *msg, unique_ptr<ServiceMessage> &message) {
        if (cache->hasFile(msg->location->getFile(), msg->location->getPath())) {//check cache
            WRENCH_INFO("Forwarding to cache reply commport %s", msg->answer_commport->get_name().c_str());
            cache->commport->putMessage(
                    new StorageServiceFileReadRequestMessage(
                            msg->answer_commport,
                            msg->requesting_host,//msg->commport_to_receive_the_file_content,
                            FileLocation::LOCATION(cache, msg->location->getPath(), msg->location->getFile()),
                            msg->num_bytes_to_read, 0));
            return true;

        } else {
            //im not really sure what this was suppose to be for.  Preventing read from a file being written I think, but Im not 100% sure
            //std::vector<unique_ptr<ServiceMessage>> &messages = pending[msg->location->getFile()];
            // for (unsigned int i = 0; i < messages.size(); i++) {
            //    if (auto tmpMsg = dynamic_cast<StorageServiceFileWriteRequestMessage *>(messages[i].get())) {
            //        //there is A writeRequest for this file in progress, ignore
            //        pending[msg->location->getFile()].push_back(std::move(message));
            //        return true;
            //    }
            //}
            //the message was not cached, and is not in progress of being read
        }
        return false;
    }
    /**
     * @brief Detect if a remote file read is already in process for this file.  If it is, return true
     * @param file the file to find
     * @return True if the message was processed.  False otherwise
     */
    bool StorageServiceProxy::rejectDuplicateRead(const std::shared_ptr<DataFile> &file) {
        WRENCH_DEBUG("Looking for Duplicate Read");
        bool second = false;
        std::vector<unique_ptr<ServiceMessage>> &messages = pending[file];

        for (unsigned int i = 0; i < messages.size(); i++) {
            if (auto tmpMsg = dynamic_cast<StorageServiceFileReadRequestMessage *>(messages[i].get())) {
                if (second) {
                    WRENCH_DEBUG("Duplicate remote read detected, Queuing");
                    return true;
                }
                second = true;
            }
        }
        return false;
    }
    /**
     * @brief function for CopyThenRead method. this function will handle everything to do with StorageServerReadRequestMessage.  Also handles StorageServerFileCopyAnswer.  The only behavioral difference is in uncached files. This copies the file requested to the cache, and forwards the ongoing reads to the cache.  This is the default, and gives the most accurate time-to-cache for a file, and the most accurate network congestion, but overestimates how long the file will take to arive at the end.
     * @param message the message that is being processed
     * @return True if the message was processed by this function.  False otherwise
     */
    bool StorageServiceProxy::copyThenRead(unique_ptr<ServiceMessage> &message) {
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
                //handle duplicate requests
                if (rejectDuplicateRead(msg->location->getFile())) {
                    return true;
                }

                StorageService::initiateFileCopy(commport, FileLocation::LOCATION(target, msg->location->getFile()), FileLocation::LOCATION(cache, msg->location->getFile()));
            } else {
                msg->answer_commport->putMessage(new StorageServiceFileReadAnswerMessage(msg->location, false, std::make_shared<FileNotFound>(msg->location), nullptr, 0, 1, StorageServiceMessagePayload::FILE_READ_ANSWER_MESSAGE_PAYLOAD));
            }
            return true;
        } else if (auto msg = dynamic_cast<StorageServiceFileCopyAnswerMessage *>(message.get())) {//Our remote read request has finished
            std::vector<unique_ptr<ServiceMessage>> &messages = pending[msg->src->getFile()];

            for (unsigned int i = 0; i < messages.size(); i++) {

                if (auto tmpMsg = dynamic_cast<StorageServiceFileReadRequestMessage *>(messages[i].get())) {

                    if (msg->success) {
                        tmpMsg->payload = 0;                      //this message has already been sent, this is a fake resend
                        commport->dputMessage(tmpMsg);//now that the data is cached, resend the message
                        std::swap(messages[i], messages.back());
                        messages.back().release();
                        messages.pop_back();
                        i--;
                    } else {
                        tmpMsg->answer_commport->putMessage(new StorageServiceFileReadAnswerMessage(tmpMsg->location, false, msg->failure_cause, nullptr, 0, 1, StorageServiceMessagePayload::FILE_READ_ANSWER_MESSAGE_PAYLOAD));
                        std::swap(messages[i], messages.back());

                        messages.pop_back();
                        i--;
                    }
                }
            }
            return true;
        }
        return false;
    }
    /**
    * @brief function for MagicRead method. this function will handle everything to do with StorageServerReadRequestMessage.  Also handles StorageServerFileCopyAnswer.  The only behavioral difference is in uncached files.  This copies the file to the cache, and then instantly "magically" transfers the file to anyone waiting on it. This gives the most accurate time-to-cache, and a reasonably accurate arrival time by assuming the bottleneck is the bandwidth from cache to remote, not the internal network.  This does sacrifice some internal network congestion.
    * @param message the message that is being processed
    * @return True if the message was processed by this function.  False otherwise
    */
    bool StorageServiceProxy::magicRead(unique_ptr<ServiceMessage> &message) {
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
                //handle duplicate requests
                if (rejectDuplicateRead(msg->location->getFile())) {
                    return true;
                }
                StorageService::initiateFileCopy(commport, FileLocation::LOCATION(target, msg->location->getFile()), FileLocation::LOCATION(cache, msg->location->getFile()));
            } else {
                msg->answer_commport->putMessage(new StorageServiceFileReadAnswerMessage(msg->location, false, std::make_shared<FileNotFound>(msg->location), nullptr, 0, 1, StorageServiceMessagePayload::FILE_READ_ANSWER_MESSAGE_PAYLOAD));
            }
            return true;
        } else if (auto msg = dynamic_cast<StorageServiceFileCopyAnswerMessage *>(message.get())) {//Our remote read request has finished
            std::vector<unique_ptr<ServiceMessage>> &messages = pending[msg->src->getFile()];
            for (unsigned int i = 0; i < messages.size(); i++) {
                if (auto tmpMsg = dynamic_cast<StorageServiceFileReadRequestMessage *>(messages[i].get())) {
                    if (msg->success) {
                        tmpMsg->answer_commport->putMessage(new StorageServiceFileReadAnswerMessage(tmpMsg->location, true, nullptr, nullptr, 0, 1, StorageServiceMessagePayload::FILE_READ_ANSWER_MESSAGE_PAYLOAD));//magic read, send buffersize 0 and we are assumed to be nonbufferized
                        tmpMsg->answer_commport->putMessage(new StorageServiceAckMessage(tmpMsg->location));                                                                                                         //emediatly send the expected ack
                        std::swap(messages[i], messages.back());
                        messages.pop_back();
                        i--;
                    } else {
                        tmpMsg->answer_commport->putMessage(new StorageServiceFileReadAnswerMessage(tmpMsg->location, false, msg->failure_cause, nullptr, 0, 1, StorageServiceMessagePayload::FILE_READ_ANSWER_MESSAGE_PAYLOAD));
                        std::swap(messages[i], messages.back());
                        messages.pop_back();
                        i--;
                    }
                }
            }
            return true;
        }
        return false;
    }
    /**
     * @brief function for CopyThenRead ReadThrough method. this function will handle everything to do with StorageServerReadRequestMessage.  Also handles StorageServiceAnswerMessage and some StorageService Ack messages. The only behavioral difference is in uncached files.  This reads the file directly to the client with the proxy acting as a mediary.  Once the write finishes, the file is instantly created on the cache.  Assuming the network is configured properly, this gives the best network congestion and time-to-arival estimate, but at the cost of time-to-cache, which it over estimates.  Concurrent reads will wait until the file is cached.
     * @param message the message that is being processed
     * @return True if the message was processed by this function.  False otherwise
     */
    bool StorageServiceProxy::readThrough(unique_ptr<ServiceMessage> &message) {
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
                //handle duplicate requests
                if (rejectDuplicateRead(msg->location->getFile())) {
                    return true;
                }
                //pending[msg->location->getFile()].push_back(std::move(message));
                //Readthrough: read from target to client emediatly, then instantly create on cache.  REQUIRES EXTANT NETWORK PATH
                //readthrough:  all block until first read is finished, then all others read
                //do not spend excessive time on readThrough
                auto forward = new StorageServiceFileReadRequestMessage(msg);
                forward->answer_commport = commport;                                                                     //setup intercept commport
                forward->location = FileLocation::LOCATION(target, msg->location->getPath(), msg->location->getFile());//hyjack locaiton to be on target
                target->commport->dputMessage(forward);                                                    //send to target
            } else {
                msg->answer_commport->putMessage(new StorageServiceFileReadAnswerMessage(msg->location, false, std::make_shared<FileNotFound>(msg->location), nullptr, 0, 1, StorageServiceMessagePayload::FILE_READ_ANSWER_MESSAGE_PAYLOAD));
            }
            return true;
        } else if (auto msg = dynamic_cast<StorageServiceFileReadAnswerMessage *>(message.get())) {//Our readthrough is in progress
            auto &messages = pending[msg->location->getFile()];

            for (unsigned int i = 0; i < messages.size(); i++) {
                if (auto tmpMsg = dynamic_cast<StorageServiceFileReadRequestMessage *>(messages[i].get())) {
                    if (msg->success) {

                        msg->location = tmpMsg->location;                                   //fix up the location
                        tmpMsg->answer_commport->dputMessage(message.release());//forward success message to first waiting read host
                        return true;
                    } else {
                        //remote read has failed, notify all waiting
                        tmpMsg->answer_commport->putMessage(new StorageServiceFileReadAnswerMessage(tmpMsg->location, false, msg->failure_cause, nullptr, 0, 1, StorageServiceMessagePayload::FILE_READ_ANSWER_MESSAGE_PAYLOAD));
                        std::swap(messages[i], messages.back());
                        messages.pop_back();
                        i--;
                    }
                }
            }
            return true;
        } else if (auto msg = dynamic_cast<StorageServiceAckMessage *>(message.get())) {//Our readthrough has finished

            if (msg->location->getStorageService() == shared_from_this() or msg->location->getStorageService() == cache) {
                //this is not a proxied Ack, this is actually directed to us
                return false;
            }
            std::vector<unique_ptr<ServiceMessage>> &messages = pending[msg->location->getFile()];
            bool first = true;
            for (unsigned int i = 0; i < messages.size(); i++) {
                if (auto tmpMsg = dynamic_cast<StorageServiceFileReadRequestMessage *>(messages[i].get())) {
                    if (first) {//this is the fileread we have been faking
                        cache->createFile(msg->location->getFile());
                        tmpMsg->answer_commport->dputMessage(new StorageServiceAckMessage(*msg));
                        std::swap(messages[i], messages.back());
                        messages.back().release();
                        messages.pop_back();
                        i--;
                        first = false;
                    } else {                                      //these are the pending reads
                        tmpMsg->payload = 0;                      //this message has already been sent, this is a fake resend
                        commport->dputMessage(tmpMsg);//these should now be cached, and should just drop down to the cache automatically
                        std::swap(messages[i], messages.back());
                        messages.back().release();
                        messages.pop_back();
                        i--;
                    }
                }
            }
            return true;
        }
        return false;
    }

}// namespace wrench
