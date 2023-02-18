/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/failure_causes/InvalidDirectoryPath.h>
#include <wrench/failure_causes/FileNotFound.h>
#include <wrench/failure_causes/StorageServiceNotEnoughSpace.h>

#include <wrench/services/storage/simple/SimpleStorageService.h>
#include <wrench/services/storage/simple/SimpleStorageServiceBufferized.h>
#include <wrench/services/storage/simple/SimpleStorageServiceNonBufferized.h>
#include <wrench/services/ServiceMessage.h>
#include "wrench/services/storage/StorageServiceMessage.h"
#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/data_file/DataFile.h>
#include <wrench/exceptions/ExecutionException.h>
#include <wrench/simulation/SimulationTimestampTypes.h>
#include <wrench/services/storage/storage_helpers/FileLocation.h>
#include <wrench/services/memory/MemoryManager.h>
#include <wrench/util/UnitParser.h>

WRENCH_LOG_CATEGORY(wrench_core_simple_storage_service,
                    "Log category for Simple Storage Service");

namespace wrench {

    /**
     * @brief Factory method to create SimpleStorageService instances
     *
     * @param hostname: the name of the host on which to start the service
     * @param mount_points: the set of mount points
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     * @return a pointer to a simple storage service
     */
    SimpleStorageService *SimpleStorageService::createSimpleStorageService(const std::string &hostname,
                                                                           std::set<std::string> mount_points,
                                                                           WRENCH_PROPERTY_COLLECTION_TYPE property_list,
                                                                           WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list) {

        bool bufferized = false;// By default, non-bufferized
                                //        bool bufferized = true; // By default, bufferized

        if (property_list.find(wrench::SimpleStorageServiceProperty::BUFFER_SIZE) != property_list.end()) {
            double buffer_size = UnitParser::parse_size(property_list[wrench::SimpleStorageServiceProperty::BUFFER_SIZE]);
            bufferized = buffer_size >= 1.0;// more than one byte means bufferized
        } else {
            property_list[wrench::SimpleStorageServiceProperty::BUFFER_SIZE] = "0B";// enforce a zero buffersize
        }

        if (Simulation::isLinkShutdownSimulationEnabled() and (not bufferized)) {
            throw std::runtime_error("SimpleStorageService::createSimpleStorageService(): Cannot use non-bufferized (i.e., buffer size == 0) "
                                     "storage services and also simulate link shutdowns. This feature is not implemented yet.");
        }

        if (bufferized) {
            return (SimpleStorageService *) (new SimpleStorageServiceBufferized(hostname, mount_points, property_list, messagepayload_list));
        } else {
            return (SimpleStorageService *) (new SimpleStorageServiceNonBufferized(hostname, mount_points, property_list, messagepayload_list));
        }
    }




    /**
    * @brief Generate a unique number
    *
    * @return a unique number
    */
    unsigned long SimpleStorageService::getNewUniqueNumber() {
        static unsigned long sequence_number = 0;
        return (sequence_number++);
    }

    /**
     * @brief Destructor
     */
    SimpleStorageService::~SimpleStorageService() {
        this->default_property_values.clear();
    }

    /**
     * @brief Private constructor
     *
     * @param hostname: the name of the host on which to start the service
     * @param mount_points: the set of mount points
     * @param property_list: the property list
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     * @param suffix: the suffix (for the service name)
     *
     * @throw std::invalid_argument
     */
    SimpleStorageService::SimpleStorageService(
            const std::string &hostname,
            const std::set<std::string>& mount_points,
            WRENCH_PROPERTY_COLLECTION_TYPE property_list,
            WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list,
            const std::string &suffix) : StorageService(hostname, "simple_storage" + suffix) {

        this->setProperties(this->default_property_values, std::move(property_list));
        this->setMessagePayloads(this->default_messagepayload_values, std::move(messagepayload_list));
        this->validateProperties();

        if (mount_points.empty()) {
            throw std::invalid_argument("SimpleStorageService::SimpleStorageService(): A storage service must have at least one mount point");
        }
        try {
            for (const auto &mp: mount_points) {
                this->file_systems[mp] = LogicalFileSystem::createLogicalFileSystem(
                        this->getHostname(), this, mp, this->getPropertyValueAsString(wrench::StorageServiceProperty::CACHING_BEHAVIOR));
            }
        } catch (std::invalid_argument &e) {
            throw;
        }

        this->num_concurrent_connections = this->getPropertyValueAsUnsignedLong(
                SimpleStorageServiceProperty::MAX_NUM_CONCURRENT_DATA_CONNECTIONS);
    }


    /**
     * @brief Process a file deletion request
     * @param location: the file location
     * @param answer_mailbox: the mailbox to which the notification should be sent
     * @return false if the daemon should terminate
     */
    bool SimpleStorageService::processFileDeleteRequest(
            const std::shared_ptr<FileLocation> &location,
            simgrid::s4u::Mailbox *answer_mailbox) {
        std::shared_ptr<FailureCause> failure_cause = nullptr;

        std::string mount_point;
        std::string path_at_mount_point;

        if (not this->splitPath(location->getPath(), mount_point, path_at_mount_point)) {
            failure_cause = std::shared_ptr<FailureCause>(
                    new FileNotFound(location));
        } else {

            auto fs = this->file_systems[mount_point].get();
            auto file = location->getFile();

            if ((not fs->doesDirectoryExist(path_at_mount_point)) or
                (not fs->isFileInDirectory(file, path_at_mount_point))) {
                // If this is scratch, we don't care, perhaps it was taken care of elsewhere...
                if (not this->isScratch()) {
                    failure_cause = std::shared_ptr<FailureCause>(
                            new FileNotFound(location));
                }
            } else {
                fs->removeFileFromDirectory(file, path_at_mount_point);
            }
        }

        S4U_Mailbox::dputMessage(
                answer_mailbox,
                new StorageServiceFileDeleteAnswerMessage(
                        location->getFile(),
                        this->getSharedPtr<SimpleStorageService>(),
                        (failure_cause == nullptr),
                        failure_cause,
                        this->getMessagePayloadValue(
                                SimpleStorageServiceMessagePayload::FILE_DELETE_ANSWER_MESSAGE_PAYLOAD)));
        return true;
    }


    /**
     * @brief Process a file lookup request
     * @param location: the file location
     * @param answer_mailbox: the mailbox to which the notification should be sent
     * @return false if the daemon should terminate
     */
    bool SimpleStorageService::processFileLookupRequest(
            const std::shared_ptr<FileLocation> &location,
            simgrid::s4u::Mailbox *answer_mailbox) {

        bool file_found;

        std::string mount_point;
        std::string path_at_mount_point;

        if (not this->splitPath(location->getPath(), mount_point, path_at_mount_point)) {
            file_found = false;
        } else {
            auto fs = this->file_systems[mount_point].get();
            auto file = location->getFile();
            file_found = fs->isFileInDirectory(file, path_at_mount_point);
        }

        S4U_Mailbox::dputMessage(
                answer_mailbox,
                new StorageServiceFileLookupAnswerMessage(
                        location->getFile(),
                        file_found,
                        this->getMessagePayloadValue(
                                SimpleStorageServiceMessagePayload::FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD)));
        return true;
    }

    /**
    * @brief Get the total static capacity of the storage service (in zero simulation time)
    * @return capacity of the storage service in bytes
    */
    double SimpleStorageService::getTotalSpace() {
        double capacity = 0.0;
        for (auto const &fs: this->file_systems) {
            capacity += fs.second->getTotalCapacity();
        }
        return capacity;
    }

    std::string SimpleStorageService::getBaseRootPath(){
            if (this->file_systems.size() == 1) {
                return this->file_systems.begin()->first;
            } else {
                return "/";
            }
    }


/**
 * @brief Get the mount point (will throw is more than one)
 * @return the (sole) mount point of the service
 */
    std::string SimpleStorageService::getMountPoint() {
        if (this->hasMultipleMountPoints()) {
            throw std::invalid_argument(
                    "StorageService::getMountPoint(): The storage service has more than one mount point");
        }
        return wrench::FileLocation::sanitizePath(this->file_systems.begin()->first);
    }

    /**
 * @brief Get the set of mount points
 * @return the set of mount points
 */
    std::set<std::string> SimpleStorageService::getMountPoints() {
        std::set<std::string> to_return;
        for (auto const &fs: this->file_systems) {
            to_return.insert(fs.first);
        }
        return to_return;
    }

    /**
 * @brief Checked whether the storage service has multiple mount points
 * @return true whether the service has multiple mount points
 */
    bool SimpleStorageService::hasMultipleMountPoints() {
        return (this->file_systems.size() > 1);
    }

    /**
* @brief Checked whether the storage service has a particular mount point
* @param mp: a mount point
*
* @return true whether the service has that mount point
*/
    bool SimpleStorageService::hasMountPoint(const std::string &mp) {
        return (this->file_systems.find(mp) != this->file_systems.end());
    }


    /**
     * @brief Process a free space request
     * @param answer_mailbox: the mailbox to which the notification should be sent
     * @return false if the daemon should terminate
     */
    bool SimpleStorageService::processFreeSpaceRequest(simgrid::s4u::Mailbox *answer_mailbox) {
        double free_space = 0;

        for (auto const &mp: this->file_systems) {
            free_space += mp.second->getFreeSpace();
        }

        S4U_Mailbox::dputMessage(
                answer_mailbox,
                new StorageServiceFreeSpaceAnswerMessage(
                        free_space,
                        this->getMessagePayloadValue(
                                SimpleStorageServiceMessagePayload::FREE_SPACE_ANSWER_MESSAGE_PAYLOAD)));
        return true;
    }

    /**
     * @brief Process a stop daemon request
     * @param ack_mailbox: the mailbox to which the ack should be sent
     * @return false if the daemon should terminate
     */
    bool SimpleStorageService::processStopDaemonRequest(simgrid::s4u::Mailbox *ack_mailbox) {
        try {
            S4U_Mailbox::putMessage(ack_mailbox,
                                    new ServiceDaemonStoppedMessage(this->getMessagePayloadValue(
                                            SimpleStorageServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
        } catch (ExecutionException &ignore) {}
        return false;
    }

    /**
     * @brief Helper method to validate property values
     * throw std::invalid_argument
     */
    void SimpleStorageService::validateProperties() {
        this->getPropertyValueAsUnsignedLong(SimpleStorageServiceProperty::MAX_NUM_CONCURRENT_DATA_CONNECTIONS);
        this->getPropertyValueAsSizeInByte(SimpleStorageServiceProperty::BUFFER_SIZE);
    }


    /**
     * @brief Get a file's last write date at a location (in zero simulated time)
     *
     * @param file: the file
     * @param path: the path
     *
     * @return the file's last write date, or -1 if the file is not found
     *
     */
    double SimpleStorageService::getFileLastWriteDate(const std::shared_ptr<DataFile> &file, const std::string &path) {
        if (!file) {
            throw std::invalid_argument("SimpleStorageService::getFileLastWriteDate(): Invalid nullptr argument");
        }

        std::string mount_point;
        std::string path_at_mount_point;
        this->splitPath(path, mount_point, path_at_mount_point);

        auto fs = this->file_systems[mount_point].get();
        return fs->getFileLastWriteDate(file, path_at_mount_point);
    }

    /**
     * @brief Determines whether the storage service has the file. This doesn't simulate anything and is merely a zero-simulated-time data structure lookup.
     * If you want to simulate the overhead of querying the StorageService, instead use lookupFile().
     * @param file a file
     * @param path the path
     * @return true if the file is present, false otherwise
     */
    bool SimpleStorageService::hasFile(const std::shared_ptr<FileLocation> &location) {
        std::string mount_point;
        std::string path_at_mount_point;
        if (not this->splitPath(location->getPath(), mount_point, path_at_mount_point)) {
            return false;
        }
        auto fs = this->file_systems[mount_point].get();
        return fs->isFileInDirectory(location->getFile(), path_at_mount_point);
    }


    bool SimpleStorageService::splitPath(const std::string &path, std::string &mount_point, std::string &path_at_mount_point) {
        auto sanitized_path = FileLocation::sanitizePath(path);
        for (auto const &fs : this->file_systems) {
            auto mp = fs.first;
            if (FileLocation::properPathPrefix(mp, sanitized_path)) {
                mount_point = mp;
                path_at_mount_point = sanitized_path.erase(0, mp.length());
                return true;
            }
        }
        return false;


    }

    void SimpleStorageService::decrementNumRunningOperationsForLocation(const std::shared_ptr<FileLocation> &location) {
        std::string mount_point;
        std::string path_at_mount_point;

        this->splitPath(location->getPath(), mount_point, path_at_mount_point);
        this->file_systems[mount_point]->decrementNumRunningTransactionsForFileInDirectory(location->getFile(), path_at_mount_point);
    }

    void SimpleStorageService::incrementNumRunningOperationsForLocation(const std::shared_ptr<FileLocation> &location) {
        std::string mount_point;
        std::string path_at_mount_point;

        this->splitPath(location->getPath(), mount_point, path_at_mount_point);
        this->file_systems[mount_point]->incrementNumRunningTransactionsForFileInDirectory(location->getFile(), path_at_mount_point);
    }

    void SimpleStorageService::createFile(const std::shared_ptr<FileLocation> &location)  {
        std::string mount_point, path_at_mount_point;
        this->splitPath(location->getPath(), mount_point, path_at_mount_point);
        this->file_systems[mount_point]->unreserveSpace(location->getFile(), path_at_mount_point);
        bool enough_space = (this->file_systems[mount_point]->getFreeSpace() >= location->getFile()->getSize());
        if (!enough_space) {
            throw ExecutionException(std::make_shared<StorageServiceNotEnoughSpace>(
                    location->getFile(), location->getStorageService()));
        }
        this->file_systems[mount_point]->storeFileInDirectory(location->getFile(), path_at_mount_point);
    }


    double SimpleStorageService::getFileLastWriteDate(const std::shared_ptr<FileLocation> &location) {
        std::string mount_point, path_at_mount_point;
        if (not this->splitPath(location->getPath(), mount_point, path_at_mount_point)) {
            return -1.0;
        }
        return this->file_systems[mount_point]->getFileLastWriteDate(location->getFile(), path_at_mount_point);
    }


}// namespace wrench
