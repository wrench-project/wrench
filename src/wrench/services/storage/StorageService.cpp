/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/storage/StorageServiceProperty.h>
#include <services/storage/storage_helpers/LogicalFileSystem.h>
#include "wrench/exceptions/WorkflowExecutionException.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/services/storage/StorageService.h"
#include "wrench/services/compute/cloud/CloudComputeService.h"
#include "wrench/services/compute/virtualized_cluster/VirtualizedClusterComputeService.h"
#include "services/storage/StorageServiceMessage.h"
#include "wrench/services/storage/StorageServiceMessagePayload.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/simulation/Simulation.h"
#include "wrench/simgrid_S4U_util/S4U_PendingCommunication.h"

WRENCH_LOG_NEW_DEFAULT_CATEGORY(storage_service, "Log category for Storage Service");

#define GB (1000.0 * 1000.0 * 1000.0)

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param hostname: the name of the host on which the service should run
     * @param mount_points: the mount points of each disk usable by the service
     * @param service_name: the name of the storage service
     * @param mailbox_name_prefix: the mailbox name prefix
     *
     * @throw std::invalid_argument
     */
    StorageService::StorageService(const std::string &hostname,
                                   const std::set<std::string> mount_points,
                                   const std::string &service_name,
                                   const std::string &mailbox_name_prefix) :
            Service(hostname, service_name, mailbox_name_prefix) {

        if (mount_points.empty()) {
            throw std::invalid_argument("StorageService::StorageService(): At least one mount point must be provided");
        }

        try {
            for (auto mp : mount_points) {
                this->file_systems[mp] = std::unique_ptr<LogicalFileSystem>(new LogicalFileSystem(hostname, mp));
            }
        } catch (std::invalid_argument &e) {
            throw;
        }

        this->state = StorageService::UP;
    }

    /**
     * @brief Store a file at a particular mount point BEFORE the simulation is launched
     *
     * @param file: a file
     * @param location: a file location
     *
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    void StorageService::stageFile(WorkflowFile *file, std::shared_ptr<FileLocation> location) {

        location->getStorageService()->stageFile(file, location->getMountPoint(), location->getDirectory());
    }

    /**
     * @brief Stop the service
     */
    void StorageService::stop() {

        // Call the super class's method
        Service::stop();
    }

    /***************************************************************/
    /****** EVERYTHING BELOW ARE INTERACTIONS WITH THE DAEMON ******/
    /***************************************************************/

    /**
     * @brief Synchronously asks the storage service for its capacity at all its
     *        mount points
     * @return The free space in bytes of each mount point, as a map
     *
     * @throw WorkflowExecutionException
     *
     * @throw std::runtime_error
     *
     */
    std::map<std::string, double> StorageService::getFreeSpace() {

        assertServiceIsUp();

        // Send a message to the daemon
        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("how_much_free_space");
        try {
            S4U_Mailbox::putMessage(this->mailbox_name, new StorageServiceFreeSpaceRequestMessage(
                    answer_mailbox,
                    this->getMessagePayloadValue(
                            StorageServiceMessagePayload::FREE_SPACE_REQUEST_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        // Wait for a reply
        std::shared_ptr<SimulationMessage> message = nullptr;
        try {
            message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        if (auto msg = std::dynamic_pointer_cast<StorageServiceFreeSpaceAnswerMessage>(message)) {
            return msg->free_space;
        } else {
            throw std::runtime_error("StorageService::howMuchFreeSpace(): Unexpected [" + msg->getName() + "] message");
        }
    }


    /**
     * @brief Synchronously asks the storage service whether it holds a file
     *
     * @param file: the file
     *
     * @return true or false
     *
     * @throw WorkflowExecutionException
     * @throw std::invalid_arguments
     */
    bool StorageService::lookupFile(WorkflowFile *file, std::shared_ptr<FileLocation> location) {

        if ((file == nullptr) or (location == nullptr)) {
            throw std::invalid_argument("StorageService::lookupFile(): Invalid arguments");
        }

        auto storage_service = location->getStorageService();

        assertServiceIsUp(storage_service);

        // Send a message to the daemon
        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("lookup_file");
        try {
            S4U_Mailbox::putMessage(location->getStorageService()->mailbox_name,
                                    new StorageServiceFileLookupRequestMessage(
                                            answer_mailbox,
                                            file,
                                            location,
                                            storage_service->getMessagePayloadValue(
                                                    StorageServiceMessagePayload::FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        // Wait for a reply
        std::shared_ptr<SimulationMessage> message;
        try {
            message = S4U_Mailbox::getMessage(answer_mailbox, storage_service->network_timeout);
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        if (auto msg = std::dynamic_pointer_cast<StorageServiceFileLookupAnswerMessage>(message)) {
            return msg->file_is_available;
        } else {
            throw std::runtime_error("StorageService::lookupFile(): Unexpected [" + msg->getName() + "] message");
        }
    }


    /**
     * @brief Synchronously read a file from the storage service
     *
     * @param file: the file
     * @param location: the location to read the file from
     *
     * @throw WorkflowExecutionException
     * @throw std::invalid_arguments
     */
    void StorageService::readFile(WorkflowFile *file, std::shared_ptr<FileLocation> location) {

        if ((file == nullptr) or (location == nullptr)) {
            throw std::invalid_argument("StorageService::readFile(): Invalid arguments");
        }

        auto storage_service = location->getStorageService();

        assertServiceIsUp(storage_service);

        // Send a message to the daemon
        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("read_file");
        try {
            S4U_Mailbox::putMessage(storage_service->mailbox_name,
                                    new StorageServiceFileReadRequestMessage(
                                            answer_mailbox,
                                            answer_mailbox,
                                            file,
                                            location,
                                            storage_service->buffer_size,
                                            storage_service->getMessagePayloadValue(
                                                    StorageServiceMessagePayload::FILE_READ_REQUEST_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        // Wait for a reply
        std::shared_ptr<SimulationMessage> message = nullptr;

        try {
            message = S4U_Mailbox::getMessage(answer_mailbox, storage_service->network_timeout);
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        if (auto msg = std::dynamic_pointer_cast<StorageServiceFileReadAnswerMessage>(message)) {
            // If it's not a success, throw an exception
            if (not msg->success) {
                std::shared_ptr<FailureCause> &cause = msg->failure_cause;
                throw WorkflowExecutionException(cause);
            }

            if (storage_service->buffer_size == 0) {

                throw std::runtime_error("StorageService::writeFile(): Zero buffer size not implemented yet");

            } else {

                // Otherwise, retrieve the file chunks until the last one is received
                while (true) {
                    std::shared_ptr<SimulationMessage> file_content_message = nullptr;
                    try {
                        file_content_message = S4U_Mailbox::getMessage(answer_mailbox);
                    } catch (std::shared_ptr<NetworkError> &cause) {
                        throw WorkflowExecutionException(cause);
                    }

                    if (auto file_content_chunk_msg = std::dynamic_pointer_cast<StorageServiceFileContentChunkMessage>(
                            file_content_message)) {
                        if (file_content_chunk_msg->last_chunk) {
                            break;
                        }
                    } else {
                        throw std::runtime_error("StorageService::readFile(): Received an unexpected [" +
                                                 file_content_message->getName() + "] message!");
                    }
                }
            }

        } else {
            throw std::runtime_error("StorageService::readFile(): Received an unexpected [" +
                                     message->getName() + "] message!");
        }
    }

    /**
     * @brief Synchronously write a file to the storage service
     *
     * @param file: the file
     * @param location: the location to write it to
     *
     * @throw WorkflowExecutionException
     */
    void StorageService::writeFile(WorkflowFile *file, std::shared_ptr<FileLocation> location) {

        if ((file == nullptr) or (location == nullptr)) {
            throw std::invalid_argument("StorageService::writeFile(): Invalid arguments");
        }

        auto storage_service = location->getStorageService();

        assertServiceIsUp(storage_service);


        // Send a  message to the daemon
        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("write_file");
        try {
            S4U_Mailbox::putMessage(storage_service->mailbox_name,
                                    new StorageServiceFileWriteRequestMessage(
                                            answer_mailbox,
                                            file,
                                            location,
                                            storage_service->buffer_size,
                                            storage_service->getMessagePayloadValue(
                                                    StorageServiceMessagePayload::FILE_WRITE_REQUEST_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        // Wait for a reply
        std::shared_ptr<SimulationMessage> message;

        try {
            message = S4U_Mailbox::getMessage(answer_mailbox, storage_service->network_timeout);
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        if (auto msg = std::dynamic_pointer_cast<StorageServiceFileWriteAnswerMessage>(message)) {
            // If not a success, throw an exception
            if (not msg->success) {
                throw WorkflowExecutionException(msg->failure_cause);
            }

            if (storage_service->buffer_size == 0) {
                throw std::runtime_error("StorageService::writeFile(): Zero buffer size not implemented yet");
            } else {
                try {
                    double remaining = file->getSize();
                    while (remaining > storage_service->buffer_size) {
                        S4U_Mailbox::putMessage(msg->data_write_mailbox_name, new StorageServiceFileContentChunkMessage(
                                file, storage_service->buffer_size, false));
                        remaining -= storage_service->buffer_size;
                    }
                    S4U_Mailbox::putMessage(msg->data_write_mailbox_name, new StorageServiceFileContentChunkMessage(
                            file, remaining, true));

                } catch (std::shared_ptr<NetworkError> &cause) {
                    throw WorkflowExecutionException(cause);
                }
            }

        } else {
            throw std::runtime_error("StorageService::writeFile(): Received an unexpected [" +
                                     message->getName() + "] message!");
        }
    }

    /**
     * @brief Synchronously and sequentially read a set of files from storage services
     *
     * @param files: the set of files to read
     * @param locations: a map of files to locations
     *
     * @throw std::runtime_error
     * @throw WorkflowExecutionException
     */
    void StorageService::readFiles(std::map<WorkflowFile *, std::shared_ptr<FileLocation>> locations) {
        StorageService::writeOrReadFiles(READ, std::move(locations));
    }

    /**
     * @brief Synchronously and sequentially upload a set of files from storage services
     *
     * @param files: the set of files to write
     * @param locations: a map of files to locations
     *
     * @throw std::runtime_error
     * @throw WorkflowExecutionException
     */
    void StorageService::writeFiles(std::map<WorkflowFile *, std::shared_ptr<FileLocation>> locations) {
        StorageService::writeOrReadFiles(WRITE, std::move(locations));
    }

    /**
     * @brief Synchronously and sequentially write/read a set of files to/from storage services
     *
     * @param action: FileOperation::READ (download) or FileOperation::WRITE
     * @param locations: a map of files to locations
     *
     * @throw std::runtime_error
     * @throw WorkflowExecutionException
     */
    void StorageService::writeOrReadFiles(FileOperation action,
                                          std::map<WorkflowFile *, std::shared_ptr<FileLocation>> locations) {

        for (auto const &f : locations) {
            if ((f.first == nullptr) or (f.second == nullptr)) {
                throw std::invalid_argument("StorageService::writeOrReadFiles(): invalid argument");
            }
        }

        // Create a temporary sorted list of files so that the order in which files are read/written is deterministic!
        std::map<std::string, WorkflowFile *> sorted_files;
        for (auto const &f : locations) {
            sorted_files[f.first->getID()] = f.first;
        }

        for (auto const &f : sorted_files) {
            auto file = f.second;
            auto location = locations[file];

            if (action == READ) {
                WRENCH_INFO("Reading file %s from location %s",
                            file->getID().c_str(),
                            location->toString().c_str());
                StorageService::readFile(file, location);
                WRENCH_INFO("File %s read", file->getID().c_str());
            } else {
                WRENCH_INFO("Writing file %s to location %s",
                            file->getID().c_str(),
                            location->toString().c_str());
                StorageService::writeFile(file, location);
                WRENCH_INFO("File %s written", file->getID().c_str());
            }
        }
    }


    /**
     * @brief Synchronously delete a file at a location
     *
     * @param file: the file
     * @param location: the file's location
     * @param file_registry_service: a file registry service that should be updated once the
     *         file deletion has (successfully) completed (none if nullptr)
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     * @throw std::invalid_argument
     */
    void StorageService::deleteFile(WorkflowFile *file, std::shared_ptr<FileLocation> location,
                                    std::shared_ptr<FileRegistryService> file_registry_service) {

        if ((file == nullptr) or (location == nullptr)) {
            throw std::invalid_argument("StorageService::deleteFile(): Invalid arguments");
        }

        auto storage_service = location->getStorageService();

        assertServiceIsUp(storage_service);

        bool unregister = (file_registry_service != nullptr);
        // Send a message to the daemon
        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("delete_file");
        try {
            S4U_Mailbox::putMessage(storage_service->mailbox_name,
                                    new StorageServiceFileDeleteRequestMessage(
                                            answer_mailbox,
                                            file,
                                            location,
                                            storage_service->getMessagePayloadValue(
                                                    StorageServiceMessagePayload::FILE_DELETE_REQUEST_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        // Wait for a reply
        std::shared_ptr<SimulationMessage> message = nullptr;

        try {
            message = S4U_Mailbox::getMessage(answer_mailbox, storage_service->network_timeout);
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        if (auto msg = std::dynamic_pointer_cast<StorageServiceFileDeleteAnswerMessage>(message)) {
            // On failure, throw an exception
            if (!msg->success) {
                throw WorkflowExecutionException(std::move(msg->failure_cause));
            }
            WRENCH_INFO("Deleted file %s at location %s",
                        file->getID().c_str(), location->toString().c_str());

            if (unregister) {
                file_registry_service->removeEntry(file, location);
            }

        } else {
            throw std::runtime_error("StorageService::deleteFile(): Unexpected [" + message->getName() + "] message");
        }
    }


    /**
     * @brief Synchronously ask the storage service to read a file from another storage service
     *
     * @param file: the file to copy
     * @param src: the storage service from which to read the file
     *
     * @throw WorkflowExecutionException
     * @throw std::invalid_argument
     */
    void StorageService::copyFile(WorkflowFile *file,
                                  std::shared_ptr<FileLocation> src_location,
                                  std::shared_ptr<FileLocation> dst_location) {

        if ((file == nullptr) || (src_location == nullptr) || (dst_location == nullptr)) {
            throw std::invalid_argument("StorageService::copyFile(): Invalid arguments");
        }

        if ((src_location->getStorageService() == dst_location->getStorageService()) and
            (src_location->getMountPoint() == dst_location->getMountPoint()) and
            (src_location->getDirectory() == dst_location->getDirectory())) {
            throw std::invalid_argument(
                    "StorageService::copyFile(): Cannot redundantly copy a file onto itself");
        }

        assertServiceIsUp(src_location->getStorageService());
        assertServiceIsUp(dst_location->getStorageService());

        // Send a message to the daemon of the dst service
        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("copy_file");
        auto start_timestamp = new SimulationTimestampFileCopyStart(file, src_location, dst_location);
        this->simulation->getOutput().addTimestamp<SimulationTimestampFileCopyStart>(start_timestamp);

        try {
            S4U_Mailbox::putMessage(this->mailbox_name, new StorageServiceFileCopyRequestMessage(
                    answer_mailbox,
                    file,
                    src,
                    src_mountpoint,
                    this->getSharedPtr<StorageService>(),
                    dst_mountpoint,
                    nullptr,
                    start_timestamp,
                    this->getMessagePayloadValue(StorageServiceMessagePayload::FILE_COPY_REQUEST_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        // Wait for a reply
        std::shared_ptr<SimulationMessage> message = nullptr;

        try {
            message = S4U_Mailbox::getMessage(answer_mailbox);
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        if (auto msg = std::dynamic_pointer_cast<StorageServiceFileCopyAnswerMessage>(message)) {
            if (msg->failure_cause) {
                throw WorkflowExecutionException(std::move(msg->failure_cause));
            }
        } else {
            throw std::runtime_error("StorageService::copyFile(): Unexpected [" + message->getName() + "] message");
        }
    }

    /**
     * @brief Asynchronously ask the storage service to read a file from another storage service
     *
     * @param answer_mailbox: the mailbox to which a notification message will be sent
     * @param file: the file
     * @param src: the storage service from which to read the file
     * @param src_mountpoint: the source mount point
     * @param dst_mountpoint'': the destination mount point
     *
     * @throw WorkflowExecutionException
     * @throw std::invalid_argument
     *
     */
    void StorageService::initiateFileCopy(std::string answer_mailbox, WorkflowFile *file,
                                          std::shared_ptr<StorageService> src,
                                          std::string src_mountpoint, std::string dst_mountpoint) {

        if ((file == nullptr) || (src == nullptr)) {
            throw std::invalid_argument("StorageService::initiateFileCopy(): Invalid arguments");
        }

        // Empty mount point means "/"
        if (src_mountpoint.empty()) {
            src_mountpoint = "/";
        }
        if (dst_mountpoint.empty()) {
            dst_mountpoint = "/";
        }

        if ((src.get() == this) && (src_mountpoint == dst_mountpoint)) {
            throw std::invalid_argument(
                    "StorageService::copyFile(): Cannot redundantly copy a file to the its own partition");
        }

        assertServiceIsUp();

        auto start_timestamp = new SimulationTimestampFileCopyStart(file, src, src_mountpoint,
                                                                    this->getSharedPtr<StorageService>(),
                                                                    dst_mountpoint);
        this->simulation->getOutput().addTimestamp<SimulationTimestampFileCopyStart>(start_timestamp);

        // Send a message to the daemon
        try {
            S4U_Mailbox::putMessage(this->mailbox_name, new StorageServiceFileCopyRequestMessage(
                    answer_mailbox,
                    file,
                    src,
                    src_mountpoint,
                    this->getSharedPtr<StorageService>(),
                    dst_mountpoint,
                    nullptr,
                    start_timestamp,
                    this->getMessagePayloadValue(StorageServiceMessagePayload::FILE_COPY_REQUEST_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }
    }


    /**
     * @brief Get the total static capacity of the storage service (in zero simulation time)
     * @return capacity of the storage service (double) for each mount point, in a map
     */
    std::map<std::string, double> StorageService::getTotalSpace() {
        std::map<std::string, double> to_return;
        for (auto const &fs : this->file_systems) {
            to_return[fs.first] = fs.second->;
        }
        return to_return;
    }

    /**
     * @brief Get the mount point (will throw is more than one)
     * @return the (sole) mount point of the service
     */
    std::string StorageService::getMountPoint() {
        if (this->hasMultipleMountPoints()) {
            throw std::invalid_argument("StorageService::getMountPoint(): The storage service has more than one mount point");
        }
        return this->file_systems.begin()->first;
    }


    /**
     * @brief Get the set of mount points
     * @return the set of mount points
     */
    std::set<std::string> StorageService::getMountPoints() {
        std::set<std::string> to_return;
        for (auto const &fs : this->file_systems) {
            to_return.insert(fs.first);
        }
        return to_return;
    }

    /**
     * @brief Checked whether the storage service has multiple mount points
     * @return true whether the service has multiple mount points
     */
    bool StorageService::hasMultipleMountPoints() {
        return (this->file_systems.size() > 1);
    }

    /**
    * @brief Checked whether the storage service has a particular mount point
    * @return true whether the service has that mount point
    */
    bool StorageService::hasMountPoint(std::string mp) {
        return (this->file_systems.find(mp) != this->file_systems.end());
    }

    /**
     * @brief Download a file to a local destination mount point
     * @param file: the file to download
     * @param dst_mountpoint: the destination local mount point
     * @param buffer_size: buffer size to use (0 means use "ideal fluid model")
     */
    void StorageService::downloadFile(WorkflowFile *file, std::string dst_mountpoint, unsigned long buffer_size) {
        if (this->getMountPoints().size() > 1) {
            throw std::invalid_argument(
                    "StorageService::downloadFile(): This storage service has more than one mount point; you should "
                    "specify which mount point should be used (i.e., call the version of this method that "
                    "takes a mount point argument");
        }

        this->downloadFile(file, *(this->getMountPoints().begin()), dst_mountpoint, buffer_size);
    }


};
