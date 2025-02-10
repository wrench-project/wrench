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
#include <wrench/services/storage/storage_helpers/FileLocation.h>
// #include <wrench/services/memory/MemoryManager.h>
#include <wrench/util/UnitParser.h>
#include <wrench/failure_causes/InvalidDirectoryPath.h>
#include <wrench/failure_causes/NotAllowed.h>

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
                                                                           const std::set<std::string>& mount_points,
                                                                           WRENCH_PROPERTY_COLLECTION_TYPE property_list,
                                                                           const WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE& messagepayload_list) {

        bool bufferized = false;// By default, non-bufferized
        //        bool bufferized = true; // By default, bufferized

        if (property_list.find(wrench::SimpleStorageServiceProperty::BUFFER_SIZE) != property_list.end()) {
            sg_size_t buffer_size = UnitParser::parse_size(property_list[wrench::SimpleStorageServiceProperty::BUFFER_SIZE]);
            bufferized = buffer_size >= 1;// more than one byte means bufferized
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
    SimpleStorageService::~SimpleStorageService() = default;

    /**
     * @brief Private constructor
     *
     * @param hostname: the name of the host on which to start the service
     * @param mount_points: the set of mount points
     * @param property_list: the property list
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     * @param suffix: the suffix (for the service name)
     *
     */
    SimpleStorageService::SimpleStorageService(
            const std::string &hostname,
            const std::set<std::string> &mount_points,
            const WRENCH_PROPERTY_COLLECTION_TYPE& property_list,
            const WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE& messagepayload_list,
            const std::string &suffix) : StorageService(hostname, "simple_storage" + suffix) {

        this->setProperties(this->default_property_values, property_list);
        this->setMessagePayloads(this->default_messagepayload_values, messagepayload_list);

        //        this->StorageServiceMessagePayload_FILE_READ_REQUEST_MESSAGE_PAYLOAD = this->getMessagePayloadValue(StorageServiceMessagePayload::FILE_READ_REQUEST_MESSAGE_PAYLOAD);
        //        this->StorageServiceMessagePayload_FILE_READ_ANSWER_MESSAGE_PAYLOAD = this->getMessagePayloadValue(StorageServiceMessagePayload::FILE_READ_ANSWER_MESSAGE_PAYLOAD);

        this->validateProperties();

        if (mount_points.empty()) {
            throw std::invalid_argument("SimpleStorageService::SimpleStorageService(): A storage service must have at least one mount point");
        }

        this->file_system = sgfs::FileSystem::create(this->getName() + "_fs", INT_MAX);
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
            auto storage = sgfs::OneDiskStorage::create(this->getName()+"_fspart_"+mp, disk);
            this->file_system->mount_partition(mp, storage, (sg_size_t)disk_capacity, caching_scheme);
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

        if (not this->file_system->file_exists(location->getFilePath())) {
            if (not this->isScratch()) {
                failure_cause = std::make_shared<FileNotFound>(location);
            } // otherwise, we don't care, perhaps it was taken care of elsewhere...
        } else {
            try {
                this->file_system->unlink_file(location->getFilePath());
            } catch (simgrid::Exception &e) {
                std::string error_msg = "Cannot delete a file that's open for reading/writing";
               failure_cause = std::make_shared<NotAllowed>(this->getSharedPtr<SimpleStorageService>(), error_msg);
            }
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

        bool file_found = this->file_system->file_exists(location->getFilePath());

        try {
            // Synchronous so that it won't be overtaken by an MQ message
            answer_commport->putMessage(
                    new StorageServiceFileLookupAnswerMessage(
                            location->getFile(),
                            file_found,
                            this->getMessagePayloadValue(
                                    SimpleStorageServiceMessagePayload::FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD)));
        } catch (ExecutionException &ignore) {} // Oh, well

        return true;
    }

    /**
    * @brief Get the total static capacity of the storage service (in zero simulation time)
    * @return capacity of the storage service in bytes
    */
    sg_size_t SimpleStorageService::getTotalSpace() {
        sg_size_t capacity = 0;
        auto partitions = this->file_system->get_partitions();
        for (auto const &p: partitions) {
            capacity += p->get_size();
        }
        return capacity;
    }

    /**
     * @brief Return current total free space from all mount point (in zero simulation time)
     *        for IO tracing purpose.
     * @return total free space in bytes
    */
    sg_size_t SimpleStorageService::getTotalFreeSpaceZeroTime() {
        sg_size_t free_space = 0;
        auto partitions = this->file_system->get_partitions();
        for (auto const &p: partitions) {
            free_space += p->get_free_space();
        }
        return free_space;
    }

    /**
     * @brief Return current total number of files (in zero simulation time)
     *        for IO tracing purposes
     * @return total number of files
     */
    unsigned long SimpleStorageService::getTotalFilesZeroTime() {
        unsigned long num_files = 0;
        auto partitions = this->file_system->get_partitions();
        for (auto const &p: partitions) {
            num_files += p->get_num_files();
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
     * @brief Get the storage service's mount point
     * @return the  mount points
     */
    std::string SimpleStorageService::getMountPoint() {
        if (this->file_system->get_partitions().size() > 1) {
            throw std::invalid_argument("SimpleStorageService::getMountPoint(): Storage service has multiple mount points");
        } else {
            return this->file_system->get_partitions().at(0)->get_name();
        }
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

        // TODO: Remove the sanitize
        sg_size_t free_space = 0;
        if (not path.empty()) {
            auto sanitized_path = FileLocation::sanitizePath(path);
            try {
                free_space = this->file_system->get_free_space_at_path(path);
            } catch (simgrid::Exception &ignore) {
            }
        } else {
            for (auto const &part: this->file_system->get_partitions()) {
                free_space += part->get_free_space();
            }
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
            fd->close();
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
        return this->file_system->file_exists(location->getFilePath());
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
        std::string full_path = location->getFilePath();
        if (not this->file_system->file_exists(full_path)) {
            return;
        } else {
            this->file_system->unlink_file(full_path);
        }
    }

    /**
     * @brief Create a file at the storage service (in zero simulated time)
     * @param location: a location
     */
    void SimpleStorageService::createFile(const std::shared_ptr<FileLocation> &location) {
        std::string full_path = location->getFilePath();

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
            std::string full_path = location->getFilePath();
            auto fd = this->file_system->open(full_path, "r");
            double date = fd->stat()->last_modification_date;
            fd->close();
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

    /**
     * @brief Helper method to validate a file read request
     * @param location: the location to read
     * @param opened_file: an opened file (if success)
     * @return a FailureCause or nullptr if success
     */
    std::shared_ptr<FailureCause> SimpleStorageService::validateFileReadRequest(const std::shared_ptr<FileLocation> &location,
                                                                                std::shared_ptr<simgrid::fsmod::File> &opened_file) {

        auto partition = this->file_system->get_partition_for_path_or_null(location->getFilePath());
        if ((not partition) or (not this->file_system->directory_exists(location->getDirectoryPath()))) {
            return std::make_shared<InvalidDirectoryPath>(location);
        }
        if (not this->file_system->file_exists(location->getFilePath())) {
            return std::make_shared<FileNotFound>(location);
        }
        opened_file = this->file_system->open(location->getFilePath(), "r");
        return nullptr;
    }

    /**
     * @brief Helper method to validate a file write request
     * @param location: the location to read
     * @param num_bytes_to_write: number of bytes to write
     * @param opened_file: an opened file (if success)
     * @return a FailureCause or nullptr if success
     */
    std::shared_ptr<FailureCause> SimpleStorageService::validateFileWriteRequest(const std::shared_ptr<FileLocation> &location,
                                                                                 sg_size_t num_bytes_to_write,
                                                                                 std::shared_ptr<simgrid::fsmod::File> &opened_file) {

        auto file = location->getFile();

        // Is the partition valid?
        auto partition = this->file_system->get_partition_for_path_or_null(location->getFilePath());
        if (!partition) {
            return std::make_shared<InvalidDirectoryPath>(location);
        }

        bool file_already_there = this->file_system->file_exists(location->getFilePath());
        try {
            if (not file_already_there) { // Open dot file
                if (num_bytes_to_write < location->getFile()->getSize()) {
                    std::string err_msg = "Cannot write fewer number of bytes than the file size if the file isn't already present";
                    return std::make_shared<NotAllowed>(this->getSharedPtr<Service>(), err_msg);
                }
                std::string dot_file_path = location->getADotFilePath();
                this->file_system->create_file(dot_file_path, location->getFile()->getSize());
                opened_file = this->file_system->open(dot_file_path, "r+");
                opened_file->seek(0, SEEK_SET);
            } else { // Open the file
                opened_file = this->file_system->open(location->getFilePath(), "r+");
                opened_file->seek(0, SEEK_SET);
            }
        } catch (simgrid::fsmod::NotEnoughSpaceException &e) {
            return std::make_shared<StorageServiceNotEnoughSpace>(
                file,
                this->getSharedPtr<SimpleStorageService>());
        }
        return nullptr;
    }

    /**
     * @brief Helper method to validate a file copy request
     * @param src_location: the src location
     * @param dst_location: the dst location
     * @param src_opened_file: a src opened file (if success)
     * @param dst_opened_file: a dst opened file (if success)
     * @return A FailureCause or nullptr if success
     */
    std::shared_ptr<FailureCause> SimpleStorageService::validateFileCopyRequest(const std::shared_ptr<FileLocation> &src_location,
                                                                                std::shared_ptr<FileLocation> &dst_location,
                                                                                std::shared_ptr<simgrid::fsmod::File> &src_opened_file,
                                                                                std::shared_ptr<simgrid::fsmod::File> &dst_opened_file) {
        std::shared_ptr<FailureCause> failure_cause = nullptr;
        auto file = src_location->getFile();


        auto src_file_system = src_location->getStorageService()->getFileSystem();
        auto dst_file_system = dst_location->getStorageService()->getFileSystem();

        // Validate source
        auto src_partition = src_file_system->get_partition_for_path_or_null(src_location->getFilePath());

        if ((not src_partition) or (not src_file_system->directory_exists(src_location->getDirectoryPath()))) {
            return std::make_shared<InvalidDirectoryPath>(src_location);
        }
        if (not src_file_system->file_exists(src_location->getFilePath())) {
            return std::make_shared<FileNotFound>(src_location);
        }

        // Validate destination
        auto dst_partition = dst_file_system->get_partition_for_path_or_null(dst_location->getFilePath());
        if (!dst_partition) {
            return std::make_shared<InvalidDirectoryPath>(dst_location);
        }

        // Open destination file (as it may fail)
        try {
            bool dst_file_already_there = dst_file_system->file_exists(dst_location->getFilePath());
            if (not dst_file_already_there) { // Open dot file
                std::string dot_file_path = dst_location->getADotFilePath();
                dst_file_system->create_file(dot_file_path, dst_location->getFile()->getSize());
                dst_opened_file = dst_file_system->open(dot_file_path, "r+");
                dst_opened_file->seek(0, SEEK_SET);
            } else { // Open the file
                dst_opened_file = dst_file_system->open(dst_location->getFilePath(), "r+");
                dst_opened_file->seek(0, SEEK_SET);
            }
        } catch (simgrid::fsmod::NotEnoughSpaceException &e) {
            return std::make_shared<StorageServiceNotEnoughSpace>(file, this->getSharedPtr<SimpleStorageService>());
        }

        // Open source file
        src_opened_file = src_file_system->open(src_location->getFilePath(), "r");

        return nullptr;
    }

}// namespace wrench
