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

    /**
     * @brief Implementation of a storage service proxy
     */
    class StorageServiceProxy : public StorageService {

    public:
        using StorageService::deleteFile;
        using StorageService::lookupFile;
        using StorageService::readFile;
        using StorageService::writeFile;


        std::map<std::string, double> getTotalSpace();
        std::map<std::string, double> getFreeSpace();

        std::string getMountPoint();              //simple forward
        std::set<std::string> getMountPoints();   //simple forward
        bool hasMultipleMountPoints();            //simple forward
        bool hasMountPoint(const std::string &mp);//simple forward

        /**
	 * @brief Get the last write date of a file
	 * @param location: the file location
	 * @return a (simulated) date in seconds
	 */
        double getFileLastWriteDate(const std::shared_ptr<FileLocation> &location);//forward

        virtual void deleteFile(const std::shared_ptr<StorageService> &targetServer, const std::shared_ptr<DataFile> &file, const std::shared_ptr<FileRegistryService> &file_registry_service = nullptr);

        virtual bool lookupFile(const std::shared_ptr<StorageService> &targetServer, const std::shared_ptr<DataFile> &file);

        void readFile(const std::shared_ptr<DataFile> &file);
        virtual void readFile(const std::shared_ptr<StorageService> &targetServer, const std::shared_ptr<DataFile> &file);
        virtual void readFile(const std::shared_ptr<StorageService> &targetServer, const std::shared_ptr<DataFile> &file, double num_bytes);
        virtual void readFile(const std::shared_ptr<StorageService> &targetServer, const std::shared_ptr<DataFile> &file, const std::string &path);
        virtual void readFile(const std::shared_ptr<StorageService> &targetServer, const std::shared_ptr<DataFile> &file, const std::string &path, double num_bytes);

        virtual void writeFile(const std::shared_ptr<StorageService> &targetServer, const std::shared_ptr<DataFile> &file, const std::string &path);
        virtual void writeFile(const std::shared_ptr<StorageService> &targetServer, const std::shared_ptr<DataFile> &file);

        void createFile(const std::shared_ptr<FileLocation> &location);                 //forward
        void createFile(const std::shared_ptr<DataFile> &file, const std::string &path);//forward
        void createFile(const std::shared_ptr<DataFile> &file);                         //forward

        const std::shared_ptr<StorageService> getCache();


        double getLoad();//cache
                         /**
         * @brief Factory to create a StorageServiceProxy that can a default destination to forward requests too, and will cache requests as they are made
         * @param hostname: hostname
         * @param cache: The StorageService to use as a Cache
         * @param remote: The StorageService to use as a remote file source
         * @param properties: Properties for the fileServiceProxy
         * @param message_payloads: Message Payloads for the fileServiceProxy
         * @return the StorageServiceProxy created
         */
        static std::shared_ptr<StorageServiceProxy> createRedirectProxy(
                const std::string &hostname,
                const std::shared_ptr<StorageService> &cache,
                const std::shared_ptr<StorageService> &remote = nullptr, WRENCH_PROPERTY_COLLECTION_TYPE properties = {}, WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE message_payloads = {}) {
            return std::make_shared<StorageServiceProxy>(hostname, cache, remote, properties, message_payloads);
        }

        bool hasFile(const std::shared_ptr<DataFile> &file, const std::string &path);

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/

        int main();
        bool processNextMessage();
        StorageServiceProxy(const std::string &hostname, const std::shared_ptr<StorageService> &cache = nullptr, const std::shared_ptr<StorageService> &default_remote = nullptr, WRENCH_PROPERTY_COLLECTION_TYPE properties = {}, WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE message_payloads = {});

        /**
         * @brief Factory to create a StorageServiceProxy that does not cache reads, and does not have a default destination to forward too
         * @param hostname: hostname
         * @param properties: Properties for the fileServiceProxy
         * @param message_payloads: Message Payloads for the fileServiceProxy
         * @return the StorageServiceProxy created
         */
        static std::shared_ptr<StorageServiceProxy> createCachelessRedirectProxy(const std::string &hostname, WRENCH_PROPERTY_COLLECTION_TYPE properties = {}, WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE message_payloads = {}) {

            throw std::runtime_error("Cacheless proxies are not currently supported");
            return std::make_shared<StorageServiceProxy>(hostname, nullptr, nullptr, properties, message_payloads);
        }

        /**
         * @brief Factory to create a StorageServiceProxy that does not cache reads, and only forwards requests to another service
         * @param hostname: hostname
         * @param default_remote: The StorageService to use as a remote file source
         * @param properties: Properties for the fileServiceProxy
         * @param message_payloads: Message Payloads for the fileServiceProxy
         * @return the StorageServiceProxy created
         */
        static std::shared_ptr<StorageServiceProxy> createCachelessProxy(const std::string &hostname, const std::shared_ptr<StorageService> &default_remote, WRENCH_PROPERTY_COLLECTION_TYPE properties = {}, WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE message_payloads = {}) {

            throw std::runtime_error("Cacheless proxies are not currently supported");
            return std::make_shared<StorageServiceProxy>(hostname, nullptr, default_remote, properties, message_payloads);
        }

    protected:
        std::map<std::shared_ptr<DataFile>, std::vector<unique_ptr<SimulationMessage>>> pending;
        std::shared_ptr<StorageService> cache;
        std::shared_ptr<StorageService> remote;
        /** @brief the function to actually call when handling a file read */
        bool (StorageServiceProxy::*readMethod)(unique_ptr<SimulationMessage> &);
        bool commonReadFile(StorageServiceFileReadRequestMessage *msg, unique_ptr<SimulationMessage> &message);
        bool copyThenRead(unique_ptr<SimulationMessage> &message);
        bool magicRead(unique_ptr<SimulationMessage> &message);
        bool readThrough(unique_ptr<SimulationMessage> &message);

    private:
        /** @brief Default property values */
        WRENCH_PROPERTY_COLLECTION_TYPE default_property_values = {
                {StorageServiceProperty::BUFFER_SIZE, "10000000"},// 10 MEGA BYTE
                {StorageServiceProperty::CACHING_BEHAVIOR, "NONE"},
                {StorageServiceProxyProperty::UNCACHED_READ_METHOD, "CopyThenRead"},
                {StorageServiceProxyProperty::MESSAGE_OVERHEAD, "0"}};


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

    /**
     * @brief A specialized FileLocation for use by the Proxy
     */
    class ProxyLocation : public FileLocation {
    public:
        const std::shared_ptr<StorageService> target;

        /**
	     * @brief Location specifier for a proxy
	     * @param target: a (remote) storage service to access, which overrides the default remote
	     *                service (if any) of the proxy
	     * @param other: a file location whose storage service should be the proxy
	     */
        static std::shared_ptr<ProxyLocation> LOCATION(
                const std::shared_ptr<StorageService> &target,
                const std::shared_ptr<FileLocation> &other) {
            return std::shared_ptr<ProxyLocation>(new ProxyLocation(target, other));
        }

        /**
	     * @brief Location specifier for a proxy
	     * @param target: a (remote) storage service to access, which overrides the default remote
             *                service (if any) of the proxy
	     * @param ss: The proxy
	     * @param file: The file
	     */
        static std::shared_ptr<ProxyLocation> LOCATION(
                const std::shared_ptr<StorageService> &target,
                const std::shared_ptr<StorageService> &ss,
                const std::shared_ptr<DataFile> &file) {
            return std::shared_ptr<ProxyLocation>(new ProxyLocation(target, FileLocation::LOCATION(ss, file)));
        }

        /**
	     * @brief Location specifier for a proxy
	     * @param target: a (remote) storage service to access, which overrides the default remote
             *                service (if any) of the proxy
	     * @param ss: The proxy
	     * @param server_ss: The linux page cache server
	     * @param file: The file
	     */
        static std::shared_ptr<ProxyLocation> LOCATION(
                const std::shared_ptr<StorageService> &target,
                const std::shared_ptr<StorageService> &ss,
                std::shared_ptr<StorageService> server_ss,
                const std::shared_ptr<DataFile> &file) {
            return std::shared_ptr<ProxyLocation>(new ProxyLocation(target, FileLocation::LOCATION(ss, server_ss, file)));
        }

        /**
	     * @brief Location specifier for a proxy
	     * @param target: a (remote) storage service to access, which overrides the default remote
             *                service (if any) of the proxy
	     * @param ss: The proxy
	     * @param absolute_path: The absolute path
	     * @param file: The file
	     */
        static std::shared_ptr<ProxyLocation> LOCATION(
                const std::shared_ptr<StorageService> &target,
                const std::shared_ptr<StorageService> &ss,
                const std::string &absolute_path,
                const std::shared_ptr<DataFile> &file) {
            return std::shared_ptr<ProxyLocation>(new ProxyLocation(target, FileLocation::LOCATION(ss, ss->getMountPoint() + "/" + absolute_path, file)));
        }

    private:
        /***********************/
        /** \cond INTERNAL    **/
        /***********************/
        //TODO unique instance factory
        ProxyLocation(const std::shared_ptr<StorageService> &target, const std::shared_ptr<FileLocation> &other) : FileLocation(*other), target(target) {
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
