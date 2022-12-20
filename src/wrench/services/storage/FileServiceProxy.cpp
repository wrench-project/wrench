/**
* Copyright (c) 2017-2021. The WRENCH Team.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/
#include "wrench/services/storage/FileServiceProxy.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
#include "wrench/simulation/SimulationMessage.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/exceptions/ExecutionException.h"
#include "wrench/services/storage/StorageServiceMessage.h"

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
    int FileServiceProxy::main(){
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
    bool FileServiceProxy::processNextMessage() {
        //S4U_Simulation::compute(flops);
        S4U_Simulation::computeZeroFlop();
        // Wait for a message
        std::unique_ptr<SimulationMessage> message = nullptr;


        S4U_Simulation::compute(this->getPropertyValueAsDouble(Property::MESSAGE_OVERHEAD));
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
            if(target or StorageService::lookupFile(FileLocation::LOCATION(cache, msg->location->getFile()))) {//forward request to cache

            }else if(remote){//forward request to remote

            }
        }else if (auto msg = dynamic_cast<StorageServiceFileReadRequestMessage *>(message.get())) {
            auto target=remote;
            if(auto location=std::dynamic_pointer_cast<ProxyLocation>(msg->location)){
                target=location->target;
            }
           if(target or StorageService::lookupFile(FileLocation::LOCATION(cache, msg->location->getFile()))) {//forward request to cache

           }else if(remote){//forward request to remote

           }
        } else if (auto msg = dynamic_cast<StorageServiceFileDeleteRequestMessage *>(message.get())) {
            auto target=remote;
            if(auto location=std::dynamic_pointer_cast<ProxyLocation>(msg->location)){
                target=location->target;
            }
            if(target) {//forward request to cache

            }
            if(remote){//forward request to remote

            }
        } else if (auto msg = dynamic_cast<StorageServiceFileWriteRequestMessage *>(message.get())) {
            auto target=remote;
            if(auto location=std::dynamic_pointer_cast<ProxyLocation>(msg->location)){
                target=location->target;
            }

            if(target) {//forward request to cache

            }
            if(remote){//forward request to remote

            }

        } else if (auto msg = dynamic_cast<StorageServiceFileCopyRequestMessage *>(message.get())) {
            auto target=remote;
            if(auto location=std::dynamic_pointer_cast<ProxyLocation>(msg->location)){
                target=location->target;
            }
            if(not target){
                throw std::runtime_error("Can not forward File Lookup Request Message to null target");
            }
            if(target or StorageService::lookupFile(FileLocation::LOCATION(cache, msg->location->getFile()))) {//forward request to cache

            }else if(remote){//forward request to remote

            }
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
//            throw std::runtime_error( "FileServiceProxy:processNextMessage(): Unexpected [" + message->getName() + "] message that either could not be forwared");
        } else {
            throw std::runtime_error(
                    "FileServiceProxy:processNextMessage(): Unexpected [" + message->getName() + "] message");
        }
        return true;
    }
    //std::map<std::string, double> getFreeSpace();
    //TODO handle StorageServiceFreeSpaceRequestMessage and
    //std::map<std::string, double> getTotalSpace();
    //TODO HENRI I dont know the best way to forward this function
    std::string FileServiceProxy::getMountPoint(){
        if(remote){
            return remote->getMountPoint();
        }
        throw std::invalid_argument("No Remote Fileserver supplied and no fallback default to use");
    }
    std::set<std::string> FileServiceProxy::getMountPoints(){
        if(remote){
            return remote->getMountPoints();
        }
        throw std::invalid_argument("No Remote Fileserver supplied and no fallback default to use");
    }
    bool FileServiceProxy::hasMultipleMountPoints(){
        if(remote){
            return remote->hasMultipleMountPoints();
        }
        throw std::invalid_argument("No Remote Fileserver supplied and no fallback default to use");
    }
    bool FileServiceProxy::hasMountPoint(const std::string &mp){
        if(remote){
            return remote->hasMountPoint(mp);
        }
        throw std::invalid_argument("No Remote Fileserver supplied and no fallback default to use");
    }


    /**
         * @brief Forward to remote server
         * @param location: the file location
         * @return a (simulated) date in seconds
         */
    double FileServiceProxy::getFileLastWriteDate(const std::shared_ptr<FileLocation> &location) {
        if(remote){
            return remote->getFileLastWriteDate(location);
        }
        throw std::invalid_argument("No Remote Fileserver supplied and no fallback default to use");
    }



    void FileServiceProxy::createFile(const std::shared_ptr<FileLocation> &location){
        if(remote){
            return remote->createFile(location);
        }
        throw std::invalid_argument("No Remote Fileserver supplied and no fallback default to use");
    }
    void FileServiceProxy::createFile(const std::shared_ptr<DataFile> &file, const std::string &path){
        if(remote){
            return remote->createFile(file,path);
        }
        throw std::invalid_argument("No Remote Fileserver supplied and no fallback default to use");

    }
    void FileServiceProxy::createFile(const std::shared_ptr<DataFile> &file){
        if(remote){
            return remote->createFile(file);
        }
        throw std::invalid_argument("No Remote Fileserver supplied and no fallback default to use");
    }



    double FileServiceProxy::getLoad() {
        if(cache){
            return cache->getLoad();
        }else if(remote){
            return remote->getLoad();
        }
        throw std::invalid_argument("This Proxy has not cache, no Remote fileserver to target, and no fallback default to use");
    }//cache

    /***********************/
    /** \cond INTERNAL    **/


    FileServiceProxy::FileServiceProxy(const std::string &hostname, const std::shared_ptr<StorageService>& cache,const std::shared_ptr<StorageService>& defaultRemote,WRENCH_PROPERTY_COLLECTION_TYPE properties,WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagePayload): StorageService(hostname,  "storage_proxy" ),cache(cache),remote(remote) {

        this->setProperties(this->default_property_values, std::move(property_list));
        this->setMessagePayloads(this->default_messagepayload_values, std::move(messagepayload_list));
    }
    void FileServiceProxy::deleteFile(const std::shared_ptr<StorageService>& targetServer, const std::shared_ptr<DataFile> &file, const std::shared_ptr<FileRegistryService> &file_registry_service){
        StorageService::deleteFile(ProxyLocation::LOCATION(targetServer,std::static_pointer_cast<StorageService>(shared_from_this()), file),file_registry_service);
    }
    void FileServiceProxy::readFile(const std::shared_ptr<StorageService>& targetServer,const std::shared_ptr<DataFile> &file){
        StorageService::readFile(ProxyLocation::LOCATION(targetServer,std::static_pointer_cast<StorageService>(shared_from_this()), file));
    }
    void FileServiceProxy::readFile(const std::shared_ptr<StorageService>& targetServer,const std::shared_ptr<DataFile> &file, double num_bytes){
        StorageService::readFile(ProxyLocation::LOCATION(targetServer,std::static_pointer_cast<StorageService>(shared_from_this()), file),num_bytes);
    }
    void FileServiceProxy::readFile(const std::shared_ptr<StorageService>& targetServer,const std::shared_ptr<DataFile> &file, const std::string &path){
        StorageService::readFile(ProxyLocation::LOCATION(targetServer,std::static_pointer_cast<StorageService>(shared_from_this()),path, file));
    }
    void FileServiceProxy::readFile(const std::shared_ptr<StorageService>& targetServer,const std::shared_ptr<DataFile> &file, const std::string &path, double num_bytes){
        StorageService::readFile(ProxyLocation::LOCATION(targetServer,std::static_pointer_cast<StorageService>(shared_from_this()),path, file),num_bytes);
    }
    void FileServiceProxy::writeFile(const std::shared_ptr<StorageService>& targetServer,const std::shared_ptr<DataFile> &file, const std::string &path){
        StorageService::writeFile(ProxyLocation::LOCATION(targetServer,std::static_pointer_cast<StorageService>(shared_from_this()),path, file));
    }
    void FileServiceProxy::writeFile(const std::shared_ptr<StorageService>& targetServer,const std::shared_ptr<DataFile> &file){
        StorageService::writeFile(ProxyLocation::LOCATION(targetServer,std::static_pointer_cast<StorageService>(shared_from_this()), file));
    }
}