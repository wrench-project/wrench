/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/failure_causes/FileNotFound.h>
#include <wrench/failure_causes/StorageServiceNotEnoughSpace.h>

#include <wrench/services/storage/simple/SimpleStorageService.h>
#include <wrench/services/storage/simple/SimpleStorageServiceBufferized.h>
#include <wrench/services/storage/simple/SimpleStorageServiceNonBufferized.h>
#include <wrench/services/ServiceMessage.h>
#include "wrench/services/storage/StorageServiceMessage.h"
#include <wrench/simgrid_S4U_util/S4U_CommPort.h>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/data_file/DataFile.h>
#include <wrench/exceptions/ExecutionException.h>
#include <wrench/simulation/SimulationTimestampTypes.h>
#include <wrench/services/storage/storage_helpers/FileLocation.h>
#include <wrench/services/memory/MemoryManager.h>
#include <wrench/util/UnitParser.h>

namespace sgfs = simgrid::fsmod;


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
            const std::set<std::string> &mount_points,
            WRENCH_PROPERTY_COLLECTION_TYPE property_list,
            WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list,
            const std::string &suffix) : StorageService(hostname, "simple_storage" + suffix) {

        this->setProperties(this->default_property_values, std::move(property_list));
        this->setMessagePayloads(this->default_messagepayload_values, std::move(messagepayload_list));

        //        this->StorageServiceMessagePayload_FILE_READ_REQUEST_MESSAGE_PAYLOAD = this->getMessagePayloadValue(StorageServiceMessagePayload::FILE_READ_REQUEST_MESSAGE_PAYLOAD);
        //        this->StorageServiceMessagePayload_FILE_READ_ANSWER_MESSAGE_PAYLOAD = this->getMessagePayloadValue(StorageServiceMessagePayload::FILE_READ_ANSWER_MESSAGE_PAYLOAD);

        this->validateProperties();

        if (mount_points.empty()) {
            throw std::invalid_argument("SimpleStorageService::SimpleStorageService(): A storage service must have at least one mount point");
        }

        // TODO: Can we pass infinity as the second parameter?
        this->file_system = sgfs::FileSystem::create(this->getName() + "_fs", 1024*1024*1024);
        for (const auto &mp: mount_points) {
            // Find the disk
            auto disk = S4U_Simulation::hostHasMountPoint(this->hostname, mp);
            if (disk == nullptr) {
                throw std::invalid_argument("SimpleStorageService::SimpleStorageService(): There is no disk at host " + this->hostname + " mounted at " + mp);
            }
            auto disk_capacity = S4U_Simulation::getDiskCapacity(this->hostname, mp);
            sgfs::Partition::CachingScheme caching_scheme;
            std::string caching_behavior_property = this->getPropertyValueAsString(wrench::StorageServiceProperty::CACHING_BEHAVIOR);
            if (caching_behavior_property == "NONE") {
                caching_scheme = sgfs::Partition::CachingScheme::NONE;
            } else if (caching_behavior_property == "FIFO") {
                caching_scheme = sgfs::Partition::CachingScheme::FIFO;
            } else if (caching_behavior_property == "LRU") {
                caching_scheme = sgfs::Partition::CachingScheme::LRU;
            } else {
                throw std::invalid_argument("SimpleStorageService::SimpleStorageService(): Invalid caching behavior " + caching_behavior_property);
            }
            this->file_system->mount_partition(mp, sgfs::OneDiskStorage::create(this->getName()+"_fspart_"+mp, disk), (sg_size_t)disk_capacity, caching_scheme);
        }

        this->num_concurrent_connections = this->getPropertyValueAsUnsignedLong(
                SimpleStorageServiceProperty::MAX_NUM_CONCURRENT_DATA_CONNECTIONS);
    }


    /**
     * @brief Process a file deletion request
     * @param location: the file location
     * @param answer_commport: the commport to which the notification should be sent
     * @return false if the daemon should terminate
     */
    bool SimpleStorageService::processFileDeleteRequest(
            const std::shared_ptr<FileLocation> &location,
            S4U_CommPort *answer_commport) {
        std::shared_ptr<FailureCause> failure_cause = nullptr;

        std::string mount_point;
        std::string path_at_mount_point;

        if (not this->file_system->file_exists(location->getPath())) {
            if (not this->isScratch()) {
                failure_cause = std::shared_ptr<FailureCause>(new FileNotFound(location));
            } // otherwise, we don't care, perhaps it was taken care of elsewhere...
        } else {
            this->file_system->unlink_file(location->getPath());
        }

        answer_commport->dputMessage(
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
     * @param answer_commport: the commport to which the notification should be sent
     * @return false if the daemon should terminate
     */
    bool SimpleStorageService::processFileLookupRequest(
            const std::shared_ptr<FileLocation> &location,
            S4U_CommPort *answer_commport) {

        bool file_found = this->file_system->file_exists(location->getPath());

        answer_commport->dputMessage(
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
        auto partitions = this->file_system->get_partitions();
        for (auto const &p: partitions) {
            capacity += (double)p->get_size();
        }
        return capacity;
    }

    /**
     * @brief Return current total free space from all mount point (in zero simulation time)
     *        for IO tracing purpose.
     * @return total free space in bytes
    */
    double SimpleStorageService::getTotalFreeSpaceZeroTime() {
        double free_space = 0;
        auto partitions = this->file_system->get_partitions();
        for (auto const &p: partitions) {
            free_space += (double)p->get_free_space();
        }
        return free_space;
    }

    /**
     * @brief Return current total number of files (in zero simulation time)
     *        for IO tracing purposes
     * @return total number of files
     */
    double SimpleStorageService::getTotalFilesZeroTime() {
        double num_files = 0;
        auto partitions = this->file_system->get_partitions();
        for (auto const &p: partitions) {
            num_files += (double)p->get_num_files();
        }
        return num_files;
    }

    /**
     * @brief Determine whether the storage service has multiple mount points
     * @return true if multiple mount points, false otherwise
     */
    bool SimpleStorageService::hasMultipleMountPoints() {
        return (this->file_system->get_partitions().size() > 1);
    }

    /**
     * @brief Get the base root path. Note that if this service has multiple mount
     *        points, this method will throw an exception
     * @return
     */
    std::string SimpleStorageService::getBaseRootPath() {
        auto partitions = this->file_system->get_partitions();
        if (partitions.size() > 1) {
            throw std::runtime_error("SimpleStorageService::getBaseRootPath(): storage service has multiple mount points, and thus no single getRootPath");
        } else {
            return (*partitions.begin())->get_name() + "/";
        }
    }


    //    /**
    //     * @brief Get the mount point (will throw is more than one)
    //     * @return the (sole) mount point of the service
    //     */
    //    std::string SimpleStorageService::getMountPoint() {
    //        if (this->hasMultipleMountPoints()) {
    //            throw std::invalid_argument(
    //                    "StorageService::getMountPoint(): The storage service has more than one mount point");
    //        }
    //        return wrench::FileLocation::sanitizePath(this->file_systems.begin()->first);
    //    }

    /**
     * @brief Get the set of mount points
     * @return the set of mount points
     */
    std::set<std::string> SimpleStorageService::getMountPoints() {
        std::set<std::string> to_return;
        for (auto const &fs: this->file_system->get_partitions()) {
            to_return.insert(fs->get_name());
        }
        return to_return;
    }

    /**
    * @brief Checked whether the storage service has a particular mount point
    * @param mp: a mount point
    *
    * @return true whether the service has that mount point
    */
    bool SimpleStorageService::hasMountPoint(const std::string &mp) {
        return (this->file_system->partition_by_name_or_null(mp) != nullptr);
    }

    /**
     * @brief Process a free space request
     * @param answer_commport: the commport to which the notification should be sent
     * @param path: the path at which free space is requested
     * @return false if the daemon should terminate
     */
    bool SimpleStorageService::processFreeSpaceRequest(S4U_CommPort *answer_commport, const std::string &path) {
        double free_space = 0;

        auto sanitized_path = FileLocation::sanitizePath(path);
        for (auto const &p: this->file_system->get_partitions()) {
            free_space += p->get_free_space();
        }

        answer_commport->dputMessage(
                new StorageServiceFreeSpaceAnswerMessage(
                        free_space,
                        this->getMessagePayloadValue(
                                SimpleStorageServiceMessagePayload::FREE_SPACE_ANSWER_MESSAGE_PAYLOAD)));
        return true;
    }

    /**
     * @brief Process a stop daemon request
     * @param ack_commport: the commport to which the ack should be sent
     * @return false if the daemon should terminate
     */
    bool SimpleStorageService::processStopDaemonRequest(S4U_CommPort *ack_commport) {
        try {
            ack_commport->putMessage(
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
     * @return the file's last write date, or -1 if the file is not found or if the path is invalid
     *
     */
    double SimpleStorageService::getFileLastWriteDate(const std::shared_ptr<DataFile> &file, const std::string &path) {
        if (!file) {
            throw std::invalid_argument("SimpleStorageService::getFileLastWriteDate(): Invalid nullptr argument");
        }

        try {
            auto fd = this->file_system->open(path + "/" + file->getID(), "r");
            double date = fd->stat()->last_modification_date;
            this->file_system->close(fd);
            return date;
        } catch (simgrid::Exception &e) {
            return -1.0;
        }
    }

    /**
     * @brief Determines whether the storage service has the file. This doesn't simulate anything and is merely a zero-simulated-time data structure lookup.
     * If you want to simulate the overhead of querying the StorageService, instead use lookupFile().
     * @param location: a location
     * @return true if the file is present, false otherwise
     */
    bool SimpleStorageService::hasFile(const std::shared_ptr<FileLocation> &location) {
        return this->file_system->file_exists(location->getPath() + "/" + location->getFile()->getID());
    }

    /**
     * @brief Remove a directory and all files at the storage service (in zero simulated time)
     * @param path a path
     */
    void SimpleStorageService::removeDirectory(const std::string &path) {
        if (not this->file_system->directory_exists(path)) {
            return;
        } else {
            this->file_system->unlink_directory(path);
        }
    }

    /**
     * @brief Remove a file at the storage service (in zero simulated time)
     * @param location: a location
     */
    void SimpleStorageService::removeFile(const std::shared_ptr<FileLocation> &location) {
        std::string full_path = location->getPath() + "/" + location->getFile()->getID();
        if (not this->file_system->file_exists(full_path)) {
            return;
        } else {
            this->file_system->unlink_file(full_path);
        }
    }


//    /**
//     * @brief Helper method to split a path into mountpoint:path_at_mount_point
//     * @param path: a path string
//     * @param mount_point: the mountpoint
//     * @param path_at_mount_point: the path at the mount point
//     * @return true on success, false on failure (i.e., mount point not found)
//     */
//    bool SimpleStorageService::splitPath(const std::string &path, std::string &mount_point, std::string &path_at_mount_point) {
//        auto sanitized_path = FileLocation::sanitizePath(path);
//        for (auto const &fs: this->file_systems) {
//            auto mp = fs.first;
//            if (FileLocation::properPathPrefix(mp, sanitized_path)) {
//                mount_point = mp;
//                path_at_mount_point = sanitized_path.erase(0, mp.length());
//                return true;
//            }
//        }
//        return false;
//    }

//    /**
//     * @brief Decrement the number of operations for a location
//     * @param location: a location
//     */
//    void SimpleStorageService::decrementNumRunningOperationsForLocation(const std::shared_ptr<FileLocation> &location) {
//        std::string mount_point;
//        std::string path_at_mount_point;
//
//        this->splitPath(location->getPath(), mount_point, path_at_mount_point);
//        this->file_systems[mount_point]->decrementNumRunningTransactionsForFileInDirectory(location->getFile(), path_at_mount_point);
//    }
//
//    /**
//     * @brief increment the number of operations for a location
//     * @param location: a location
//     */
//    void SimpleStorageService::incrementNumRunningOperationsForLocation(const std::shared_ptr<FileLocation> &location) {
//        std::string mount_point;
//        std::string path_at_mount_point;
//
//        this->splitPath(location->getPath(), mount_point, path_at_mount_point);
//        this->file_systems[mount_point]->incrementNumRunningTransactionsForFileInDirectory(location->getFile(), path_at_mount_point);
//    }

    /**
     * @brief Create a file at the storage service (in zero simulated time)
     * @param location: a location
     */
    void SimpleStorageService::createFile(const std::shared_ptr<FileLocation> &location) {
        std::string full_path = location->getPath() + "/" + location->getFile()->getID();

        try {
            this->file_system->create_file(full_path, location->getFile()->getSize());
        } catch (sgfs::FileAlreadyExistsException &e) {
            return; // nothing to do
        } catch (sgfs::NotEnoughSpaceException &e) {
            throw ExecutionException(std::make_shared<StorageServiceNotEnoughSpace>(
                    location->getFile(), location->getStorageService()));
        }
    }


    /**
     * @brief Get a file's last write date at a the storage service (in zero simulated time)
     * @param location:  a location
     * @return a date in seconds, or -1 if the file is not found
     */
    double SimpleStorageService::getFileLastWriteDate(const std::shared_ptr<FileLocation> &location) {
        if (location == nullptr) {
            throw std::invalid_argument("SimpleStorageService::getFileLastWriteDate(): Invalid nullptr argument");
        }

        try {
            std::string full_path = location->getPath() + "/" + location->getFile()->getID();
            auto fd = this->file_system->open(full_path, "r");
            double date = fd->stat()->last_modification_date;
            this->file_system->close(fd);
            return date;
        } catch (simgrid::Exception &e) {
            return -1.0;
        }
    }

    /**
     * @brief Gets the disk that stores a path
     * @param path: a path
     * @return a disk, or nullptr if path is invalid
     */
    simgrid::s4u::Disk *SimpleStorageService::getDiskForPathOrNull(const string &path) {
        auto partition = this->file_system->get_partition_for_path_or_null(path);
        if (!partition) {
            return nullptr;
        }
        return S4U_Simulation::hostHasMountPoint(this->hostname, partition->get_name());
    }


}// namespace wrench
