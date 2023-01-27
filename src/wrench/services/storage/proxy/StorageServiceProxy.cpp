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
namespace wrench{

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
    int StorageServiceProxy::main(){
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
    std::map<std::string, double> StorageServiceProxy::getTotalSpace(){
        if(remote){
            return remote->getTotalSpace();
        }
        throw runtime_error("Proxy with no default location does not support getTotalSpace()");
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
        }else if (auto msg = dynamic_cast<StorageServiceFileLookupRequestMessage *>(message.get())) {
            auto target=remote;

            if(auto location=std::dynamic_pointer_cast<ProxyLocation>(msg->location)){
                target=location->target;
            }
            //cerr<<"FILE LOOKUP!!!! "<<remote<<" "<<target<<endl;
            if(StorageService::lookupFile(FileLocation::LOCATION(cache, msg->location->getFile()))) {//forward request to cache
                //cerr<<"File cached"<<endl;
                S4U_Mailbox::putMessage(msg->answer_mailbox,new StorageServiceFileLookupAnswerMessage(msg->location->getFile(),true,StorageServiceMessagePayload::FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD));

            }else if(target){
                pending[msg->location->getFile()].push_back(std::move(message));
                //message=std::move(pending[msg->location->getFile()][0]);
                S4U_Mailbox::putMessage(
                        target->mailbox,
                        new StorageServiceFileLookupRequestMessage(
                                mailbox,
                                FileLocation::LOCATION(target,msg->location->getMountPoint(),msg->location->getFile()),
                                target->getMessagePayloadValue(
                                        StorageServiceMessagePayload::FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD)));
            }else{
                S4U_Mailbox::putMessage(msg->answer_mailbox,new StorageServiceFileLookupAnswerMessage(msg->location->getFile(),false,StorageServiceMessagePayload::FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD));
            }
        }else if (auto msg = dynamic_cast<StorageServiceFileReadRequestMessage *>(message.get())) {
            auto target=remote;
            if(auto location=std::dynamic_pointer_cast<ProxyLocation>(msg->location)){
                target=location->target;
            }

            if(StorageService::lookupFile(FileLocation::LOCATION(cache, msg->location->getFile()))) {//check cache
                WRENCH_INFO("Forwarding to cache reply mailbox %s",msg->answer_mailbox->get_name().c_str());
                S4U_Mailbox::putMessage(
                        cache->mailbox,
                        new StorageServiceFileReadRequestMessage(msg->answer_mailbox,msg->requesting_host,msg->mailbox_to_receive_the_file_content,FileLocation::LOCATION(cache,msg->location->getMountPoint(),msg->location->getFile()),msg->num_bytes_to_read,0)
                );

            }else if(target){
                pending[msg->location->getFile()].push_back(std::move(message));
                StorageService::initiateFileCopy(mailbox, FileLocation::LOCATION(target,msg->location->getFile()),FileLocation::LOCATION(cache,msg->location->getFile()));
            }else{
                S4U_Mailbox::putMessage(msg->answer_mailbox,new StorageServiceFileReadAnswerMessage(msg->location,false,std::make_shared<FileNotFound>(msg->location),0,StorageServiceMessagePayload::FILE_READ_ANSWER_MESSAGE_PAYLOAD));
            }
        } else if (auto msg = dynamic_cast<StorageServiceFileDeleteRequestMessage *>(message.get())) {
            auto target=remote;
            if(auto location=std::dynamic_pointer_cast<ProxyLocation>(msg->location)){
                target=location->target;
            }
            bool deleted=false;
            if(cache){
                try{
                    cache->deleteFile(msg->location->getFile());
                    deleted=true;
                }catch(ExecutionException& e){
                    //silently ignore
                }
            }
            if(target) {
                try{
                    target->deleteFile(msg->location->getFile());
                    deleted=true;
                }catch(ExecutionException& e){
                    //silently ignore
                }
            }
            if(deleted){
                S4U_Mailbox::putMessage(msg->answer_mailbox, new StorageServiceFileDeleteAnswerMessage(msg->location->getFile(),target,true,nullptr,StorageServiceMessagePayload::FILE_DELETE_ANSWER_MESSAGE_PAYLOAD));
            }else{
                S4U_Mailbox::putMessage(msg->answer_mailbox, new StorageServiceFileDeleteAnswerMessage(msg->location->getFile(),target,false,std::make_shared<FileNotFound>(msg->location),StorageServiceMessagePayload::FILE_DELETE_ANSWER_MESSAGE_PAYLOAD));
            }
        } else if (auto msg = dynamic_cast<StorageServiceFileWriteRequestMessage *>(message.get())) {
            auto target=remote;
            if(auto location=std::dynamic_pointer_cast<ProxyLocation>(msg->location)){
                //WRENCH_INFO("Proxy Location");
                target=location->target;
            }
            //cerr<<"writing"<<target<<" "<<remote<<endl;
            if(cache){//check cache
                if(target) {//check that someone is waiting for this message
                    pending[msg->location->getFile()].push_back(std::move(message));
                    WRENCH_INFO("Adding pending write");
                    cerr<<target<<endl;
                }
                S4U_Mailbox::putMessage(cache->mailbox,new StorageServiceFileWriteRequestMessage(mailbox,msg->requesting_host,FileLocation::LOCATION(cache,msg->location->getFile()),msg->buffer_size,0));
            }else{
                S4U_Mailbox::putMessage(msg->answer_mailbox,new StorageServiceFileWriteAnswerMessage(msg->location,false,std::make_shared<HostError>(hostname),0,StorageServiceMessagePayload::FILE_WRITE_ANSWER_MESSAGE_PAYLOAD));
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
        }else if (auto msg = dynamic_cast<StorageServiceFileCopyAnswerMessage *>(message.get())) {//Our remote read request has finished

            std::vector<unique_ptr<SimulationMessage>>& messages=pending[msg->src->getFile()];
            for(unsigned int i=0;i<messages.size();i++){
                if(auto tmpMsg= dynamic_cast<StorageServiceFileReadRequestMessage *>(messages[i].get())){
                    if(msg->success){
                        S4U_Mailbox::putMessage(mailbox,tmpMsg);//now that the data is cached, resend the message
                        std::swap(messages[i],messages.back());
                        messages.back().release();
                        messages.pop_back();
                        i--;
                    }else{
                        S4U_Mailbox::putMessage(tmpMsg->answer_mailbox,new StorageServiceFileReadAnswerMessage(tmpMsg->location,false,msg->failure_cause,0,StorageServiceMessagePayload::FILE_READ_ANSWER_MESSAGE_PAYLOAD));
                        std::swap(messages[i],messages.back());
                        messages.pop_back();
                        i--;

                    }
                }

            }

        }else if (auto msg = dynamic_cast<StorageServiceFileLookupAnswerMessage *>(message.get())) {//Our remote lookup has finished
            std::vector<unique_ptr<SimulationMessage>>& messages=pending[msg->file];
            for(unsigned int i=0;i<messages.size();i++){
                if(auto tmpMsg= dynamic_cast<StorageServiceFileLookupRequestMessage *>(messages[i].get())){
                    S4U_Mailbox::putMessage(tmpMsg->answer_mailbox,new StorageServiceFileLookupAnswerMessage(*msg));
                    std::swap(messages[i],messages.back());
                    messages.pop_back();
                    i--;
                }

            }
        }else if (auto msg = dynamic_cast<StorageServiceFileWriteAnswerMessage *>(message.get())) {//forward through
            //WRENCH_INFO("Got write answer");
            std::vector<unique_ptr<SimulationMessage>>& messages=pending[msg->location->getFile()];
            for(unsigned int i=0;i<messages.size();i++){
                if(auto tmpMsg= dynamic_cast<StorageServiceFileWriteRequestMessage *>(messages[i].get())){
                    //WRENCH_INFO("Forwarding");
                    S4U_Mailbox::putMessage(tmpMsg->answer_mailbox,new StorageServiceFileWriteAnswerMessage(*msg));
                    if(!msg->success){//the the file write failed, there will be no ack message
                        //WRENCH_INFO("write failed");
                        std::swap(messages[i],messages.back());
                        messages.pop_back();
                        i--;
                    }
                }

            }
        //}else if (auto msg = dynamic_cast<StorageServiceFileReadAnswerMessage *>(message.get())) {//we never actually do this

        }else if (auto msg = dynamic_cast<StorageServiceAckMessage *>(message.get())) {//our file write has finished
            std::vector<unique_ptr<SimulationMessage>>& messages=pending[msg->location->getFile()];
            for(unsigned int i=0;i<messages.size();i++){
                if(auto tmpMsg= dynamic_cast<StorageServiceFileWriteRequestMessage *>(messages[i].get())){

                    S4U_Mailbox::putMessage(tmpMsg->answer_mailbox, new StorageServiceAckMessage(*msg));//forward ACK
                    auto target=remote;
                    if(auto location=std::dynamic_pointer_cast<ProxyLocation>(tmpMsg->location)){
                        target=location->target;
                    }
                    //WRENCH_INFO("initiating file copy");
                    initiateFileCopy(recv_mailbox,FileLocation::LOCATION(cache,msg->location->getFile()),FileLocation::LOCATION(target,msg->location->getFile()));
                    S4U_Mailbox::igetMessage(recv_mailbox);//there will be a message on this mailbox, but we dont actually care
                    //TODO this is horrible hacky and might leak memory like a sieve


                    std::swap(messages[i],messages.back());
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
    std::map<std::string, double> StorageServiceProxy::getFreeSpace(){
        if(remote){
            return remote->getFreeSpace();
        }
        throw runtime_error("Proxy with no default location does not support getFreeSpace()");

    }
    /**
     * @brief Get the mount point of the remote server (will throw is more than one).  If there isnt a default, returns DEV_NUL
     * @return the (sole) mount point of the service
     */
    std::string StorageServiceProxy::getMountPoint(){
        if(remote){
            return remote->getMountPoint();
        }
        if(cache){
            return cache->getMountPoint();
        }
        return LogicalFileSystem::DEV_NULL;
    }
    /**
     * @brief Get the set of mount points of the remote server, if not returns {}
     * @return the set of mount points
     */
    std::set<std::string> StorageServiceProxy::getMountPoints(){
        if(remote){
            return remote->getMountPoints();
        }
        if(cache){
            return cache->getMountPoints();
        }
        return {};
    }
    /**
     * @brief Checked whether the remote storage service has multiple mount points
     * @return true whether the service has multiple mount points
     */
    bool StorageServiceProxy::hasMultipleMountPoints(){
        if(remote){
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
    bool StorageServiceProxy::hasMountPoint(const std::string &mp){
        if(remote){
            return remote->hasMountPoint(mp);
        }
        if(cache){
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
        if(remote){
            return remote->getFileLastWriteDate(location);
        }
        if(cache){
            return cache->getFileLastWriteDate(location);
        }
        return -1;
    }


    /**
     * @brief StorageServiceProxy.createFile() is ambiguous where the file should go.  You should call createFile on the remote service where you wish to create the file.
If you want it to start cached, you should also call StorageServiceProxy.getCache().createFile
     *
     **/
    void StorageServiceProxy::createFile(const std::shared_ptr<FileLocation> &location){
        throw std::runtime_error("StorageServiceProxy.createFile() is ambiguous where the file should go.  You should call createFile on the remote service where you wish to create the file.  \nIf you want it to start cached, you should also call StorageServiceProxy.getCache().createFile");
    }
    /**
     * @brief StorageServiceProxy.createFile() is ambiguous where the file should go.  You should call createFile on the remote service where you wish to create the file.
If you want it to start cached, you should also call StorageServiceProxy.getCache().createFile
     *
     **/
    void StorageServiceProxy::createFile(const std::shared_ptr<DataFile> &file, const std::string &path){
        throw std::runtime_error("StorageServiceProxy.createFile() is ambiguous where the file should go.  You should call createFile on the remote service where you wish to create the file.  \nIf you want it to start cached, you should also call StorageServiceProxy.getCache().createFile");
    }
    /**
     * @brief StorageServiceProxy.createFile() is ambiguous where the file should go.  You should call createFile on the remote service where you wish to create the file.
If you want it to start cached, you should also call StorageServiceProxy.getCache().createFile
     *
     **/
    void StorageServiceProxy::createFile(const std::shared_ptr<DataFile> &file){
        throw std::runtime_error("StorageServiceProxy.createFile() is ambiguous where the file should go.  You should call createFile on the remote service where you wish to create the file.  \nIf you want it to start cached, you should also call StorageServiceProxy.getCache().createFile");
    }
    /**
     * Get the load of the cache
     * @return the load of the cache
     */
    double StorageServiceProxy::getLoad() {
        if(cache){
            return cache->getLoad();
        }else if(remote){
            return remote->getLoad();
        }
        throw std::invalid_argument("This Proxy has not cache, no Remote fileserver to target, and no fallback default to use");
    }//cache

    /***********************/
    /** \cond INTERNAL    **/
    /***********************/
    /**
     * @brief Constructor
     * @param hostname The host to run on
     * @param cache A Storage server to use as a cache.  Ideal on the same host
     * @param defaultRemote A remote file server to use as a default target.  Defaults to nullptr
     * @param properties The wrench property overrides
     * @param messagePayload The wrench message payload overrides
     */
    StorageServiceProxy::StorageServiceProxy(const std::string &hostname,
                                             const std::shared_ptr<StorageService>& cache,
                                             const std::shared_ptr<StorageService>& defaultRemote,
                                             WRENCH_PROPERTY_COLLECTION_TYPE properties,WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagePayload
                                             ):
                                                 StorageService(hostname,
                                                                "storage_proxy"
                                                                ),
                                                 cache(cache),
                                                 remote(defaultRemote) {

        this->setProperties(this->default_property_values, std::move(property_list));
        this->setMessagePayloads(this->default_messagepayload_values, std::move(messagepayload_list));
        this->setProperty(StorageServiceProperty::BUFFER_SIZE,cache->getPropertyValueAsString(StorageServiceProperty::BUFFER_SIZE));//the internal cache has the same buffer properties as this service.
        if(cache and cache->hasMultipleMountPoints()){
            throw std::invalid_argument("StorageServiceProxy::StorageServiceProxy() A storage service proxy's cache can not have multiple mountpoints");
        }
        if(remote and remote->hasMultipleMountPoints()){
            throw std::invalid_argument("StorageServiceProxy::StorageServiceProxy() A storage service proxy's default remote can not have multiple mountpoints");
        }

    }
    /**
     *  @brief
     * @param targetServer
     * @param file
     * @param file_registry_service
     */
    void StorageServiceProxy::deleteFile(const std::shared_ptr<StorageService>& targetServer, const std::shared_ptr<DataFile> &file, const std::shared_ptr<FileRegistryService> &file_registry_service){
        StorageService::deleteFile(ProxyLocation::LOCATION(targetServer,std::static_pointer_cast<StorageService>(shared_from_this()), file),file_registry_service);
    }
    bool StorageServiceProxy::lookupFile(const std::shared_ptr<StorageService>& targetServer,const std::shared_ptr<DataFile> &file){
        return StorageService::lookupFile(ProxyLocation::LOCATION(targetServer,std::static_pointer_cast<StorageService>(shared_from_this()), file));
    }
    void StorageServiceProxy::readFile(const std::shared_ptr<DataFile> &file){
        StorageService::readFile(ProxyLocation::LOCATION(remote,std::static_pointer_cast<StorageService>(shared_from_this()), file));
    }
    void StorageServiceProxy::readFile(const std::shared_ptr<StorageService>& targetServer,const std::shared_ptr<DataFile> &file){
        StorageService::readFile(ProxyLocation::LOCATION(targetServer,std::static_pointer_cast<StorageService>(shared_from_this()), file));
    }
    void StorageServiceProxy::readFile(const std::shared_ptr<StorageService>& targetServer,const std::shared_ptr<DataFile> &file, double num_bytes){
        StorageService::readFile(ProxyLocation::LOCATION(targetServer,std::static_pointer_cast<StorageService>(shared_from_this()), file),num_bytes);
    }
    void StorageServiceProxy::readFile(const std::shared_ptr<StorageService>& targetServer,const std::shared_ptr<DataFile> &file, const std::string &path){
        StorageService::readFile(ProxyLocation::LOCATION(targetServer,std::static_pointer_cast<StorageService>(shared_from_this()),path, file));
    }
    void StorageServiceProxy::readFile(const std::shared_ptr<StorageService>& targetServer,const std::shared_ptr<DataFile> &file, const std::string &path, double num_bytes){
        StorageService::readFile(ProxyLocation::LOCATION(targetServer,std::static_pointer_cast<StorageService>(shared_from_this()),path, file),num_bytes);
    }
    void StorageServiceProxy::writeFile(const std::shared_ptr<StorageService>& targetServer,const std::shared_ptr<DataFile> &file, const std::string &path){
        StorageService::writeFile(ProxyLocation::LOCATION(targetServer,std::static_pointer_cast<StorageService>(shared_from_this()),path, file));
    }

    void StorageServiceProxy::writeFile(const std::shared_ptr<StorageService>& targetServer,const std::shared_ptr<DataFile> &file){
        StorageService::writeFile(ProxyLocation::LOCATION(targetServer,std::static_pointer_cast<StorageService>(shared_from_this()), file));
    }

    const std::shared_ptr<StorageService> StorageServiceProxy::getCache(){
        return this->cache;
    }
}