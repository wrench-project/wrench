/**
* Copyright (c) 2017-2020. The WRENCH Team.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/

#ifndef WRENCH_STORAGESERVICEPROXY_H
#define WRENCH_STORAGESERVICEPROXY_H
#include "wrench/services/storage/StorageService.h"
#include "wrench/services/storage/StorageServiceMessage.h"
#include "wrench/services/storage/StorageServiceMessagePayload.h"
#include "StorageServiceProxyProperty.h"

namespace wrench {
    /***********************/
    /** \cond DEVELOPER   **/
    /***********************/

    class StorageServiceProxy : public StorageService {

    public:

        using StorageService::deleteFile;
        using StorageService::lookupFile;
        using StorageService::writeFile;
        using StorageService::readFile;



        std::map<std::string, double> getTotalSpace();
        std::map<std::string, double> getFreeSpace();

        std::string getMountPoint();//simple forward
        std::set<std::string> getMountPoints();//simple forward
        bool hasMultipleMountPoints();//simple forward
        bool hasMountPoint(const std::string &mp);//simple forward

        /**
	 * @brief Get the last write date of a file
	 * @param location: the file location
	 * @return a (simulated) date in seconds
	 */
        double getFileLastWriteDate(const std::shared_ptr<FileLocation> &location);//forward

        virtual void deleteFile(const std::shared_ptr<StorageService>& targetServer, const std::shared_ptr<DataFile> &file, const std::shared_ptr<FileRegistryService> &file_registry_service = nullptr);

        virtual bool lookupFile(const std::shared_ptr<StorageService>& targetServer, const std::shared_ptr<DataFile> &file);

        void readFile(const std::shared_ptr<DataFile> &file);
        virtual void readFile(const std::shared_ptr<StorageService>& targetServer,const std::shared_ptr<DataFile> &file);
        virtual void readFile(const std::shared_ptr<StorageService>& targetServer,const std::shared_ptr<DataFile> &file, double num_bytes);
        virtual void readFile(const std::shared_ptr<StorageService>& targetServer,const std::shared_ptr<DataFile> &file, const std::string &path);
        virtual void readFile(const std::shared_ptr<StorageService>& targetServer,const std::shared_ptr<DataFile> &file, const std::string &path, double num_bytes);

        virtual void writeFile(const std::shared_ptr<StorageService>& targetServer,const std::shared_ptr<DataFile> &file, const std::string &path);
        virtual void writeFile(const std::shared_ptr<StorageService>& targetServer,const std::shared_ptr<DataFile> &file);

        void createFile(const std::shared_ptr<FileLocation> &location);//forward
        void createFile(const std::shared_ptr<DataFile> &file, const std::string &path);//forward
        void createFile(const std::shared_ptr<DataFile> &file);//forward

        const std::shared_ptr<StorageService> getCache();


        double getLoad() ;//cache
                          /**
         * @brief Factory to create a StorageServiceProxy that can a default destination to forward requests too, and will cache requests as they are made
         * @param hostname: hostname
         * @param cache: The StorageService to use as a Cache
         * @param remote: The StorageService to use as a remote file source
         * @param properties: Properties for the fileServiceProxy
         * @param messagePayload: Message Payloads for the fileServiceProxy
         * @return the StorageServiceProxy created
         */
        static std::shared_ptr<StorageServiceProxy> createRedirectProxy(
                const std::string &hostname,
                const std::shared_ptr<StorageService> &cache,
                const std::shared_ptr<StorageService> &remote = nullptr, WRENCH_PROPERTY_COLLECTION_TYPE properties = {}, WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagePayload = {}) {
            return std::make_shared<StorageServiceProxy>(hostname,cache,remote,properties,messagePayload);
        }

        bool hasFile(const std::shared_ptr<DataFile> &file, const std::string& path);
        /***********************/
        /** \cond INTERNAL    **/
        /***********************/

        int main();
        bool processNextMessage();
        StorageServiceProxy(const std::string &hostname, const std::shared_ptr<StorageService>& cache=nullptr,const std::shared_ptr<StorageService>& defaultRemote=nullptr,WRENCH_PROPERTY_COLLECTION_TYPE properties={},WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagePayload={});

        /**
         * @brief Factory to create a StorageServiceProxy that does not cache reads, and does not have a default destination to forward too
         * @param hostname: hostname
         * @param cache: The StorageService to use as a Cache
         * @param remote: The StorageService to use as a remote file source
         * @param properties: Properties for the fileServiceProxy
         * @param messagePayload: Message Payloads for the fileServiceProxy
         * @return the StorageServiceProxy created
         */
        static std::shared_ptr<StorageServiceProxy> createCachelessRedirectProxy(const std::string &hostname,WRENCH_PROPERTY_COLLECTION_TYPE properties={},WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagePayload={}){

            throw std::runtime_error("Cacheless proxies are not currently supported");
            return std::make_shared<StorageServiceProxy>(hostname,nullptr,nullptr,properties,messagePayload);
        }
        /**
         * @brief Factory to create a StorageServiceProxy that does not cache reads, and only forwards requests to another service
         * @param hostname: hostname
         * @param cache: The StorageService to use as a Cache
         * @param remote: The StorageService to use as a remote file source
         * @param properties: Properties for the fileServiceProxy
         * @param messagePayload: Message Payloads for the fileServiceProxy
         * @return the StorageServiceProxy created
         */
        static std::shared_ptr<StorageServiceProxy> createCachelessProxy(const std::string &hostname, const std::shared_ptr<StorageService>& defaultRemote,WRENCH_PROPERTY_COLLECTION_TYPE properties={},WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagePayload={}){

            throw std::runtime_error("Cacheless proxies are not currently supported");
            return std::make_shared<StorageServiceProxy>(hostname,nullptr,defaultRemote,properties,messagePayload);
        }
    protected:
        std::map<std::shared_ptr<DataFile>,std::vector<unique_ptr<SimulationMessage>>> pending;
        std::shared_ptr<StorageService> cache;
        std::shared_ptr<StorageService> remote;

        bool (StorageServiceProxy::*readMethod)(unique_ptr<SimulationMessage>&);
        bool commonReadFile(StorageServiceFileReadRequestMessage* msg,unique_ptr<SimulationMessage>& message);
        bool copyThenRead(unique_ptr<SimulationMessage>& message);
        bool magicRead(unique_ptr<SimulationMessage>& message);
        bool readThrough(unique_ptr<SimulationMessage>& message);
    private:

        /** @brief Default property values */
        WRENCH_PROPERTY_COLLECTION_TYPE default_property_values = {
                {StorageServiceProperty::BUFFER_SIZE, "10000000"},// 10 MEGA BYTE
                {StorageServiceProperty::CACHING_BEHAVIOR, "NONE"},
                {StorageServiceProxyProperty::UNCACHED_READ_METHOD, "CopyThenRead"},
                {StorageServiceProxyProperty::MESSAGE_OVERHEAD,"0"}
        };


        WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE default_messagepayload_values = {
                {ServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD, 1024},
                {ServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD, 1024},
                {StorageServiceMessagePayload::FREE_SPACE_REQUEST_MESSAGE_PAYLOAD, 1024},
                {StorageServiceMessagePayload::FREE_SPACE_ANSWER_MESSAGE_PAYLOAD, 1024},
                {StorageServiceMessagePayload::FILE_DELETE_REQUEST_MESSAGE_PAYLOAD, 1024},
                {StorageServiceMessagePayload::FILE_DELETE_ANSWER_MESSAGE_PAYLOAD, 1024},
                {StorageServiceMessagePayload::FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD, 1024},
                {StorageServiceMessagePayload::FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD, 1024},
                {StorageServiceMessagePayload::FILE_COPY_REQUEST_MESSAGE_PAYLOAD, 1024},
                {StorageServiceMessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD, 1024},
                {StorageServiceMessagePayload::FILE_WRITE_REQUEST_MESSAGE_PAYLOAD, 1024},
                {StorageServiceMessagePayload::FILE_WRITE_ANSWER_MESSAGE_PAYLOAD, 1024},
                {StorageServiceMessagePayload::FILE_READ_REQUEST_MESSAGE_PAYLOAD, 1024},
                {StorageServiceMessagePayload::FILE_READ_ANSWER_MESSAGE_PAYLOAD, 1024},

        };
    };
    /***********************/
    /** \endcond          **/
    /***********************/

    class ProxyLocation : public FileLocation{
        public:
            const std::shared_ptr<StorageService> target;
            static std::shared_ptr<ProxyLocation> LOCATION(
                    const std::shared_ptr<StorageService> &target,
                    const std::shared_ptr<FileLocation> &other) {
                return std::shared_ptr<ProxyLocation>(new ProxyLocation(target, other));
            }
            static std::shared_ptr<ProxyLocation> LOCATION(
                    const std::shared_ptr<StorageService> &target,
                    const std::shared_ptr<StorageService> &ss,
                    const std::shared_ptr<DataFile> &file) {
                return std::shared_ptr<ProxyLocation>(new ProxyLocation(target, FileLocation::LOCATION(ss, file)));
            }

            static std::shared_ptr<ProxyLocation> LOCATION(
                    const std::shared_ptr<StorageService> &target,
                    const std::shared_ptr<StorageService> &ss,
                   std::shared_ptr<StorageService> server_ss,
                   const std::shared_ptr<DataFile> &file) {
                return std::shared_ptr<ProxyLocation>(new ProxyLocation(target, FileLocation::LOCATION(ss, server_ss, file)));
            }
        static std::shared_ptr<ProxyLocation> LOCATION(
                    const std::shared_ptr<StorageService> &target,
                    const std::shared_ptr<StorageService> &ss,
                    std::string absolute_path,
                    const std::shared_ptr<DataFile> &file) {
                return std::shared_ptr<ProxyLocation>(new ProxyLocation(target, FileLocation::LOCATION(ss, ss->getMountPoint()+"/"+absolute_path, file)));
            }
        private:
            /***********************/
            /** \cond INTERNAL    **/
            /***********************/
            //TODO unique instance factory
            ProxyLocation(const std::shared_ptr<StorageService>& target, const std::shared_ptr<FileLocation>& other):FileLocation(*other),target(target){
                if (target == nullptr) {
                    throw std::invalid_argument("ProxyLocation::LOCATION(): Cannot pass nullptr target");
                }
            }
            /***********************/
            /** \endcond           */
            /***********************/
    };
    /***********************/
    /** \endcond           */
    /***********************/

}// namespace wrench



#endif//WRENCH_STORAGESERVICEPROXY_H
