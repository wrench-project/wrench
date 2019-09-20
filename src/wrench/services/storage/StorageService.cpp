/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/storage/StorageServiceProperty.h>
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
            for (auto const &mp : mount_points) {
                this->capacities[mp] = S4U_Simulation::getDiskCapacity(hostname, mp);
                this->stored_files[mp] = {};
                this->occupied_space[mp] = 0.0;
            }
        } catch (std::invalid_argument &e) {
            throw;
        }

        this->state = StorageService::UP;
        this->mount_points = mount_points;
    }

    /**
     * @brief Store a file on the storage service BEFORE the simulation is launched
     *
     * @param file: a file
     *
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    void StorageService::stageFile(WorkflowFile *file) {

        if (file == nullptr) {
            throw std::invalid_argument("StorageService::stageFile(): Invalid arguments");
        }

        if (this->mount_points.size() > 1) {
            throw std::invalid_argument(
                    "StorageService::stageFile(): Storage service has more than one mount point; you should "
                    "specify which mount point should be used (i.e., call the version of this method that takes a mount point argument");
        }

        this->stageFile(file, *(this->mount_points.begin()));
    }

    /**
     * @brief Store a file on the storage service at a particular mount point BEFORE the simulation is launched
     *
     * @param file: a file
     * @param mount_point: a mount point (e.g., "/home")
     *
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    void StorageService::stageFile(WorkflowFile *file, std::string mount_point) {

        if (this->simulation->isRunning()) {
            throw std::runtime_error("StorageService::stageFile(): Can only be called before the simulation is launched");
        }

        if (this->mount_points.find(mount_point) == this->mount_points.end()) {
            throw std::runtime_error("StorageService::stageFile(): Unknown mount point " + mount_point);
        }

        if (file->getSize() > (this->capacities[mount_point] - this->occupied_space[mount_point])) {
            WRENCH_WARN("File exceeds free space capacity on storage service at mount point %s "
                        "(file size: %lf, free space at mount point: %lf)", mount_point.c_str(),
                        file->getSize(), (this->capacities[mount_point] - this->occupied_space[mount_point]));
            throw std::runtime_error(
                    "StorageService::stageFile(): File exceeds free space capacity at mount point on storage service");
        }

        this->stored_files[mount_point].insert(file);
        this->occupied_space[mount_point] += file->getSize();
    }


    /**
     * @brief Remove a file from storage (internal method)
     *
     * @param file: a file
     * @param mount_point: the mount_point in which the file will be deleted
     *
     * @throw std::runtime_error
     */
    void StorageService::removeFileFromStorage(WorkflowFile *file, std::string mount_point) {

        if (file == nullptr) {
            throw std::invalid_argument("StorageService::removeFileFromStorage(): Cannot pass a nullptr file");
        }

        if (this->mount_points.find(mount_point) == this->mount_points.end()) {
            throw std::invalid_argument("StorageService::removeFileFromStorage(): Unknown mount point "  + mount_point);
        }

        std::set<WorkflowFile *> files = this->stored_files[mount_point];
        if (files.find(file) == files.end()) {
            throw std::runtime_error(
                    "StorageService::removeFileFromStorage(): Attempting to remove a file that is not on the storage service");
        }
        this->stored_files[mount_point].erase(file);
        this->occupied_space[mount_point] -= file->getSize();
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
    bool StorageService::lookupFile(WorkflowFile *file) {

        if (file == nullptr) {
            throw std::invalid_argument("StorageService::lookupFile(): Invalid arguments");
        }

        assertServiceIsUp();

        if (this->mount_points.size() > 1) {
            throw std::invalid_argument("StorageService::lookupFile(): Storage service has more than one mount point; "
                                        "you should specify which mount point should be used (i.e., call the version of this "
                                        "method that takes a mount point argument");
        }

        return this->lookupFile(file, *(this->mount_points.begin()));
    }

#if 0

    /**
     * @brief Synchronously asks the storage service whether it holds a file
     *
     * @param file: the file
     * @param job: the job for whom we are doing the look up, the file is stored in a
     *
     * @return true or false
     *
     * @throw WorkflowExecutionException
     * @throw std::invalid_arguments
     */
    bool StorageService::lookupFile(WorkflowFile *file, WorkflowJob *job) {

        if (file == nullptr) {
            throw std::invalid_argument("StorageService::lookupFile(): Invalid arguments");
        }

        assertServiceIsUp();

        std::string dst_partition = "/";
        if (job != nullptr) {
            dst_partition += job->getName();
        }
        return this->lookupFile(file, dst_partition);
    }
#endif


    /**
     * @brief Synchronously asks the storage service whether it holds a file
     *
     * @param file: the file
     * @param dst_mount_point: the mount point at which to perform the lookup (an empty string means "/")
     *
     * @return true or false
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     * @throw std::invalid_arguments
     */
    bool StorageService::lookupFile(WorkflowFile *file, std::string dst_mount_point) {

        if (file == nullptr) {
            throw std::invalid_argument("StorageService::lookupFile(): Cannot pass a nullptr file");
        }

        assertServiceIsUp();

        // Empty partition means "/"
        if (dst_mount_point.empty()) {
            dst_mount_point = "/";
        }

        // Send a message to the daemon
        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("lookup_file");
        try {
            S4U_Mailbox::putMessage(this->mailbox_name, new StorageServiceFileLookupRequestMessage(
                    answer_mailbox,
                    file,
                    dst_mount_point,
                    this->getMessagePayloadValue(
                            StorageServiceMessagePayload::FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        // Wait for a reply
        std::shared_ptr<SimulationMessage> message;
        try {
            message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
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
     *
     * @throw WorkflowExecutionException
     * @throw std::invalid_arguments
     */
    void StorageService::readFile(WorkflowFile *file) {

        if (this->mount_points.size() > 1) {
            throw std::invalid_argument("StorageService::readFile(): Storage service has more than one mount point; you should "
                                        "specify which mount point should be used (i.e., call the version of this method that "
                                        "takes a mount point argument.");
        }
        this->readFile(file, *(this->mount_points.begin()));
    }

    /**
     * @brief Synchronously read a file from the storage service
     *
     * @param file: the file
     * @param job: the job associated to the read of the workflow file
     *
     * @throw WorkflowExecutionException
     * @throw std::invalid_arguments
     */
    void StorageService::readFile(WorkflowFile *file, WorkflowJob *job) {


        if (job == nullptr) {
            throw std::invalid_argument("StorageService::readFile(): cannot pass a nullptr job");
        }
        auto mount_point = *(job->getParentComputeService()->getScratch()->getMountPoints().begin());
        this->readFile(file, mount_point);
    }


    /**
     * @brief Synchronously read a file from the storage service
     *
     * @param file: the file
     * @param src_mountpoint: the mount point from which to read the file (empty means "/")
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     * @throw std::invalid_arguments
     */

    void StorageService::readFile(WorkflowFile *file, std::string src_mountpoint) {

        if (file == nullptr) {
            throw std::invalid_argument("StorageService::readFile(): Invalid arguments");
        }

        assertServiceIsUp();

        // Empty mount point means "/"
        if (src_mountpoint.empty()) {
            src_mountpoint = "/";
        }

        // Send a message to the daemon
        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("read_file");
        try {
            S4U_Mailbox::putMessage(this->mailbox_name,
                                    new StorageServiceFileReadRequestMessage(
                                            answer_mailbox,
                                            answer_mailbox,
                                            file,
                                            src_mountpoint,
                                            this->buffer_size,
                                            this->getMessagePayloadValue(
                                                    StorageServiceMessagePayload::FILE_READ_REQUEST_MESSAGE_PAYLOAD)));
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

        if (auto msg = std::dynamic_pointer_cast<StorageServiceFileReadAnswerMessage>(message)) {
            // If it's not a success, throw an exception
            if (not msg->success) {
                std::shared_ptr<FailureCause> &cause = msg->failure_cause;
                throw WorkflowExecutionException(cause);
            }

            if (this->buffer_size == 0) {

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
     *
     * @throw WorkflowExecutionException
     */
    void StorageService::writeFile(WorkflowFile *file) {

        if (this->mount_points.size() > 1) {
            throw std::invalid_argument(
                    "StorageService::writeFile(): Storage service has more than one mount point; you should "
                    "specify which mount point should be used (i.e., call the version of this method that "
                    "takes a mount point argument");
        }

        this->writeFile(file, *(this->mount_points.begin()));
    }


    /**
     * @brief Synchronously write a file to the storage service
     *
     * @param file: the file
     * @param job: the job associated to the write of the workflow file
     *
     * @throw WorkflowExecutionException
     */
    void StorageService::writeFile(WorkflowFile *file, WorkflowJob *job) {

        if (job == nullptr) {
            throw std::invalid_argument("StorageService::writeFile(): cannot pass a nullptr job");
        }
        auto mount_point = *(job->getParentComputeService()->getScratch()->getMountPoints().begin());
        this->writeFile(file, mount_point);
    }

    /**
     * @brief Synchronously write a file to the storage service
     *
     * @param file: the file
     * @param dst_mountpoint: the mount point in which to write the file
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    void StorageService::writeFile(WorkflowFile *file, std::string dst_mountpoint) {

        if (file == nullptr) {
            throw std::invalid_argument("StorageService::writeFile(): Invalid arguments");
        }

        assertServiceIsUp();

        // Empty mount point means "/"
        if (dst_mountpoint.empty()) {
            dst_mountpoint = "/";
        }


        // Send a  message to the daemon
        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("write_file");
        try {
            S4U_Mailbox::putMessage(this->mailbox_name,
                                    new StorageServiceFileWriteRequestMessage(
                                            answer_mailbox,
                                            file,
                                            dst_mountpoint,
                                            this->buffer_size,
                                            this->getMessagePayloadValue(
                                                    StorageServiceMessagePayload::FILE_WRITE_REQUEST_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        // Wait for a reply
        std::shared_ptr<SimulationMessage> message;

        try {
            message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        if (auto msg = std::dynamic_pointer_cast<StorageServiceFileWriteAnswerMessage>(message)) {
            // If not a success, throw an exception
            if (not msg->success) {
                throw WorkflowExecutionException(msg->failure_cause);
            }

            if (this->buffer_size == 0) {
                throw std::runtime_error("StorageService::writeFile(): Zero buffer size not implemented yet");
            } else {
                try {
                    double remaining = file->getSize();
                    while (remaining > this->buffer_size) {
                        S4U_Mailbox::putMessage(msg->data_write_mailbox_name, new StorageServiceFileContentChunkMessage(
                                file, this->buffer_size, false));
                        remaining -= this->buffer_size;
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
     * @param file_locations: a map of files to storage services
     * @param scratch_space: the storage service to use when files don't appear in the file_locations map (which must be the job's compute service's scratch space)
     * @param files_in_scratch: the set of files that have been written to the the job's compute service's scratch space)
     * @param job: the job which is doing the read of the files
     *
     * @throw std::runtime_error
     * @throw WorkflowExecutionException
     */
    void StorageService::readFiles(std::set<WorkflowFile *> files,
                                   std::map<WorkflowFile *, std::pair<std::shared_ptr<StorageService>, std::string>> file_locations,
                                   std::shared_ptr<StorageService> scratch_space,
                                   std::set<WorkflowFile *> &files_in_scratch,
                                   WorkflowJob *job) {
        try {
            StorageService::writeOrReadFiles(READ, std::move(files), std::move(file_locations), scratch_space,
                                             files_in_scratch, job);
        } catch (std::runtime_error &e) {
            throw;
        } catch (WorkflowExecutionException &e) {
            throw;
        }
    }

    /**
     * @brief Synchronously and sequentially upload a set of files from storage services
     *
     * @param files: the set of files to write
     * @param file_locations: a map of files to storage services
     * @param scratch_space: the storage service to use when files don't appear in the file_locations map (which must be the job's compute service's scratch storage)
     * @param files_in_scratch: the set of files that have been written to the job's compute service's scratch space
     * @param job: the job which is doing the write of the files
     *
     *
     * @throw std::runtime_error
     * @throw WorkflowExecutionException
     */
    void StorageService::writeFiles(std::set<WorkflowFile *> files,
                                    std::map<WorkflowFile *, std::pair<std::shared_ptr<StorageService>, std::string>> file_locations,
                                    std::shared_ptr<StorageService> scratch_space,
                                    std::set<WorkflowFile *> &files_in_scratch,
                                    WorkflowJob *job) {
        try {
            StorageService::writeOrReadFiles(WRITE, std::move(files), std::move(file_locations),
                                             scratch_space,
                                             files_in_scratch, job);
        } catch (std::runtime_error &e) {
            throw;
        } catch (WorkflowExecutionException &e) {
            throw;
        }
    }

    /**
     * @brief Synchronously and sequentially write/read a set of files to/from storage services
     *
     * @param action: FileOperation::READ (download) or FileOperation::WRITE
     * @param files: the set of files to read/write
     * @param file_locations: a map of files to storage services
     * @param scratch_space: the storage service to use when files don't appear in the file_locations map (which must be the job's compute service's scratch space)
     * @param files_in_scratch: the set of files that have been written to the job's compute service's scratch space
     * @param job: the job associated to the write/read of the files
     *
     * @throw std::runtime_error
     * @throw WorkflowExecutionException
     */
    void StorageService::writeOrReadFiles(FileOperation action,
                                          std::set<WorkflowFile *> files,
                                          std::map<WorkflowFile *, std::pair<std::shared_ptr<StorageService>, std::string>> file_locations,
                                          std::shared_ptr<StorageService> scratch_space,
                                          std::set<WorkflowFile *> &files_in_scratch,
                                          WorkflowJob *job) {

        for (auto const &f : files) {
            if (f == nullptr) {
                throw std::invalid_argument("StorageService::writeOrReadFiles(): invalid files argument");
            }
        }

        for (auto const &l : file_locations) {
            if ((l.first == nullptr) || (l.second.first == nullptr)) {
                throw std::invalid_argument("StorageService::writeOrReadFiles(): invalid file location argument");
            }
        }

        // Create a temporary sorted list of files so that the order in which files are read/written is deterministic!
        std::map<std::string, WorkflowFile *> sorted_files;
        for (auto const &f : files) {
            sorted_files.insert(std::make_pair(f->getID(), f));
        }

        for (auto const &f : sorted_files) {

            WorkflowFile *file = f.second;

            // Identify the Storage Service
            std::shared_ptr<StorageService> storage_service = scratch_space;
            std::string mountpoint = *(storage_service->getMountPoints().begin());
            if (file_locations.find(file) != file_locations.end()) {
                storage_service = file_locations[file].first;
                mountpoint = file_locations[file].second;

            }
            if (storage_service == nullptr) {
                throw WorkflowExecutionException(std::shared_ptr<FailureCause>(new NoStorageServiceForFile(file)));
            }
            if (mountpoint == "") {
                mountpoint = "/";
            }

            if (action == READ) {
                try {
                    WRENCH_INFO("Reading file %s from storage service %s", file->getID().c_str(),
                                storage_service->getName().c_str());
                    if (storage_service != scratch_space) {
                        //if the storage service where I am going to read from is not the scratch storage service, then I
                        // don't want to read from job's temp partition, rather I would like to read from / partition of the storage service
                        storage_service->readFile(file, mountpoint);
                    } else {
                        storage_service->readFile(file, job);
                    }
                    WRENCH_INFO("File %s read", file->getID().c_str());
                } catch (std::runtime_error &e) {
                    throw;
                } catch (WorkflowExecutionException &e) {
                    throw;
                }
            } else {
                try {
                    WRENCH_INFO("Writing file %s to storage service %s", file->getID().c_str(),
                                storage_service->getName().c_str());
                    // Write the file
                    if (storage_service == scratch_space) {
                        files_in_scratch.insert(file);
                        storage_service->writeFile(file, job);
                    } else {
                        storage_service->writeFile(file, mountpoint);
                    }
                    WRENCH_INFO("Wrote file %s", file->getID().c_str());
                } catch (std::runtime_error &e) {
                    throw;
                } catch (WorkflowExecutionException &e) {
                    throw;
                }
            }
        }
    }


    /**
     * @brief Synchronously asks the storage service to delete a file copy
     *
     * @param file: the file
     * @param file_registry_service: a file registry service that should be updated once the
     *         file deletion has (successfully) completed (none if nullptr)
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     * @throw std::invalid_argument
     */
    void StorageService::deleteFile(WorkflowFile *file, std::shared_ptr<FileRegistryService> file_registry_service) {

        if (file == nullptr) {
            throw std::invalid_argument("StorageService::deleteFile(): Invalid arguments");
        }

        if (this->mount_points.size() > 1) {
            throw std::invalid_argument(
                    "StorageService::deleteFile(): Storage service has more than one mount point; you should "
                    "specify which mount point should be used (i.e., call the version of this method that "
                    "takes a mount point argument.");
        }

        assertServiceIsUp();

        this->deleteFile(file, *(this->mount_points.begin()), file_registry_service);
    }

    /**
     * @brief Synchronously ask the storage service to delete a file copy
     *
     * @param file: the file
     * @param job: the job associated to deleting this file
     * @param file_registry_service: a file registry service that should be updated once the
     *         file deletion has (successfully) completed (none if nullptr)
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     * @throw std::invalid_argument
     */
    void StorageService::deleteFile(WorkflowFile *file, WorkflowJob *job,
                                    std::shared_ptr<FileRegistryService> file_registry_service) {

        if (file == nullptr) {
            throw std::invalid_argument("StorageService::deleteFile(): Invalid arguments");
        }

        if (job == nullptr) {
            throw std::invalid_argument("StorageService::deleteFile(): cannot pass a nullptr job");
        }

        auto mountpoint = *(job->getParentComputeService()->getScratch()->getMountPoints().begin());
        this->deleteFile(file, mountpoint, file_registry_service);
    }

    /** @brief Synchronously ask the storage service to delete a file copy
    *
    * @param file: the file
    * @param dst_mountpoint: the mount point in which to delete the file
    * @param file_registry_service: a file registry service that should be updated once the
    *         file deletion has (successfully) completed (none if nullptr)
    *
    * @throw WorkflowExecutionException
    * @throw std::runtime_error
    * @throw std::invalid_argument
    */
    void StorageService::deleteFile(WorkflowFile *file, std::string dst_mountpoint,
                                    std::shared_ptr<FileRegistryService> file_registry_service) {

        // Empty mount point means "/"
        if (dst_mountpoint.empty()) {
            dst_mountpoint = "/";
        }

        bool unregister = (file_registry_service != nullptr);
        // Send a message to the daemon
        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("delete_file");
        try {
            S4U_Mailbox::putMessage(this->mailbox_name, new StorageServiceFileDeleteRequestMessage(
                    answer_mailbox,
                    file,
                    dst_mountpoint,
                    this->getMessagePayloadValue(
                            StorageServiceMessagePayload::FILE_DELETE_REQUEST_MESSAGE_PAYLOAD)));
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

        if (auto msg = std::dynamic_pointer_cast<StorageServiceFileDeleteAnswerMessage>(message)) {
            // On failure, throw an exception
            if (!msg->success) {
                throw WorkflowExecutionException(std::move(msg->failure_cause));
            }
            WRENCH_INFO("Deleted file %s on storage service %s", file->getID().c_str(), this->getName().c_str());

            if (unregister) {
                file_registry_service->removeEntry(file,
                                                   this->getSharedPtr<StorageService>());
            }

        } else {
            throw std::runtime_error("StorageService::deleteFile(): Unexpected [" + message->getName() + "] message");
        }
    }

#if 0
    /**
     * @brief Synchronously and sequentially delete a set of files from storage services
     *
     * @param files: the set of files to delete
     * @param file_locations: a map of files to storage services (all must be in the "/" partition of their storage services)
     * @param default_storage_service: the storage service to use when files don't appear in the file_locations map (or nullptr if none)
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    void StorageService::deleteFiles(std::set<WorkflowFile *> files,
                                     std::map<WorkflowFile *, std::shared_ptr<StorageService>> file_locations,
                                     std::shared_ptr<StorageService> default_storage_service) {
        for (auto f : files) {
            // Identify the Storage Service
            std::shared_ptr<StorageService> storage_service = default_storage_service;
            if (file_locations.find(f) != file_locations.end()) {
                storage_service = file_locations[f];
            }
            if (storage_service == nullptr) {
                throw WorkflowExecutionException(std::shared_ptr<FailureCause>(new NoStorageServiceForFile(f)));
            }

            // Remove the file
            try {
                storage_service->deleteFile(f);
            } catch (WorkflowExecutionException &e) {
                throw;
            } catch (std::runtime_error &e) {
                throw;
            }
        }
    }
#endif


    /**
     * @brief Synchronously ask the storage service to read a file from another storage service
     *
     * @param file: the file to copy
     * @param src: the storage service from which to read the file
     *
     * @throw WorkflowExecutionException
     * @throw std::invalid_argument
     */
    void StorageService::copyFile(WorkflowFile *file, std::shared_ptr<StorageService> src) {

        if ((file == nullptr) || (src == nullptr)) {
            throw std::invalid_argument("StorageService::copyFile(): Invalid arguments");
        }

        if (src == this->getSharedPtr<StorageService>()) {
            throw std::invalid_argument(
                    "StorageService::copyFile(file,src): Cannot redundantly copy a file to its own partition");
        }

        assertServiceIsUp();

        if (this->getMountPoints().size() > 1) {
            throw std::invalid_argument(
                    "StorageService::copyFile(): This storage service has more than one mount point; you should "
                    "specify which mount point should be used (i.e., call the version of this method that "
                    "takes a mount point argument");
        }

        if (src->getMountPoints().size() > 1) {
            throw std::invalid_argument(
                    "StorageService::copyFile(): Source storage service has more than one mount point; you should "
                    "specify which mount point should be used (i.e., call the version of this method that "
                    "takes a mount point argument");
        }

        this->copyFile(file, src, *(this->getMountPoints().begin()), *(src->getMountPoints().begin()));
    }

    /**
     * @brief Synchronously ask the storage service to read a file from another storage service
     *
     * @param file: the file to copy
     * @param src: the storage service from which to read the file
     * @param src_job: the source job for which we are copying this file
     * @param dst_job: the dst job for which we are copying this file
     * @throw WorkflowExecutionException
     * @throw std::invalid_argument
     */
    void StorageService::copyFile(WorkflowFile *file, std::shared_ptr<StorageService> src, WorkflowJob *src_job,
                                  WorkflowJob *dst_job) {

        if ((file == nullptr) || (src == nullptr)) {
            throw std::invalid_argument("StorageService::copyFile(): Invalid arguments");
        }

        if (src == this->getSharedPtr<StorageService>() && src_job == nullptr && dst_job == nullptr) {
            throw std::invalid_argument(
                    "StorageService::copyFile(): Cannot redundantly copy a file to the same mount point");
        }

        if (src_job != nullptr && dst_job != nullptr) {
            throw std::invalid_argument(
                    "StorageService::copyFile(): Cannot copy files from one job's scratch space to another job's scratch space");
        }

        assertServiceIsUp();

        if (this->getMountPoints().size() > 1) {
            throw std::invalid_argument(
                    "StorageService::copyFile(): This storage service has more than one mount point; you should "
                    "specify which mount point should be used (i.e., call the version of this method that "
                    "takes a mount point argument");
        }

        if (src->getMountPoints().size() > 1) {
            throw std::invalid_argument(
                    "StorageService::copyFile(): Source storage service has more than one mount point; you should "
                    "specify which mount point should be used (i.e., call the version of this method that "
                    "takes a mount point argument");
        }

        std::string src_partition = *(src->getMountPoints().begin());
        if (src_job != nullptr) {
            src_partition = *(src_job->getParentComputeService()->getScratch()->getMountPoints().begin());
        }

        std::string dst_partition = *(this->getMountPoints().begin());

        if (dst_job != nullptr) {
            dst_partition = *(dst_job->getParentComputeService()->getScratch()->getMountPoints().begin());
        }

        this->copyFile(file, src, src_partition, dst_partition);
    }

    /**
     * @brief Synchronously ask the storage service to read a file from another storage service
     *
     * @param file: the file to copy
     * @param src: the storage service from which to read the file
     * @param src_mountpoint: the mount point from which the file will be read
     * @param dst_mountpoint: the mount point to which the file will be written
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     * @throw std::invalid_argument
     */
    void StorageService::copyFile(WorkflowFile *file, std::shared_ptr<StorageService> src, std::string src_mountpoint,
                                  std::string dst_mountpoint) {


        if (src.get() == this && (src_mountpoint == dst_mountpoint)) {
            throw std::invalid_argument(
                    "StorageService::copyFile(): Cannot redundantly copy a file to its own mount point");
        }

        // Empty mount points means "/"
        if (dst_mountpoint.empty()) {
            dst_mountpoint = "/";
        }
        if (src_mountpoint.empty()) {
            src_mountpoint = "/";
        }

        // Send a message to the daemon
        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("copy_file");
        auto start_timestamp = new SimulationTimestampFileCopyStart(file, src, src_mountpoint,
                                                                    this->getSharedPtr<StorageService>(),
                                                                    dst_mountpoint);
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
        return this->capacities;
    }

    /**
     * @brief Get the set of mount points
     * @return the set of mount points
     */
    std::set<std::string> StorageService::getMountPoints() {
        return this->mount_points;
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

    /**
     * @brief Download a file to a local partition/disk
     * @param file: the file to download
     * @param src_mountpoint: the source mount point
     * @param dst_mountpoint: the local mount point
     * @param downloader_buffer_size: buffer size of the downloader (0 means use "ideal fluid model")
     */
    void StorageService::downloadFile(WorkflowFile *file, std::string src_mountpoint, std::string dst_mountpoint, unsigned long downloader_buffer_size) {

        if (file == nullptr) {
            throw std::invalid_argument("StorageService::downloadFile(): Invalid arguments");
        }

        WRENCH_INFO("Initiating a file read operation for file %s on storage service %s",
                    file->getID().c_str(), this->getName().c_str());

        // Check that the service is up
        assertServiceIsUp();

        // Check that the buffer size is compatible
        if (((downloader_buffer_size == 0) && (this->buffer_size != 0)) or
            ((downloader_buffer_size != 0) && (this->buffer_size == 0))) {
            throw std::invalid_argument("StorageService::downloadFile(): Incompatible buffer size specs (both must be zero, or both must be non-zero");
        }

        // Empty partition means "/"
        if (src_mountpoint.empty()) {
            src_mountpoint = "/";
        }
        if (dst_mountpoint.empty()) {
            dst_mountpoint = "/";
        }

        // Send a message to the daemon
        std::string request_answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("read_file_request");
        std::string mailbox_that_should_receive_file_content = S4U_Mailbox::generateUniqueMailboxName("read_file_chunks");

        try {
            S4U_Mailbox::putMessage(this->mailbox_name,
                                    new StorageServiceFileReadRequestMessage(request_answer_mailbox,
                                                                             mailbox_that_should_receive_file_content,
                                                                             file,
                                                                             src_mountpoint,
                                                                             std::min<unsigned long>(this->buffer_size, downloader_buffer_size),
                                                                             this->getMessagePayloadValue(
                                                                                     StorageServiceMessagePayload::FILE_READ_REQUEST_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        // Wait for a reply to the request
        std::shared_ptr<SimulationMessage> message = nullptr;

        try {
            message = S4U_Mailbox::getMessage(request_answer_mailbox, this->network_timeout);
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        if (auto msg = std::dynamic_pointer_cast<StorageServiceFileReadAnswerMessage>(message)) {
            // If it's not a success, throw an exception
            if (not msg->success) {
                throw WorkflowExecutionException(msg->failure_cause);
            }
        } else {
            throw std::runtime_error("StorageService::downloadFile(): Received an unexpected [" +
                                     message->getName() + "] message!");
        }

        WRENCH_INFO("File read request accepted (will receive file content on mailbox_name %s)",
                    mailbox_that_should_receive_file_content.c_str());

        if (this->buffer_size == 0) {

            throw std::runtime_error("downloadFile::writeFile(): Zero buffer size not implemented yet");

        } else {
            try {
                bool done = false;
                // Receive the first chunk
                auto msg = S4U_Mailbox::getMessage(mailbox_that_should_receive_file_content);
                if (auto file_content_chunk_msg =
                        std::dynamic_pointer_cast<StorageServiceFileContentChunkMessage>(msg)) {
                    done = file_content_chunk_msg->last_chunk;
                } else {
                    throw std::runtime_error("FileTransferThread::downloadFile() : Received an unexpected [" +
                                             msg->getName() + "] message!");
                }

                // Receive chunks and write them to disk
                while (not done) {
                    // Issue the receive
                    auto req = S4U_Mailbox::igetMessage(mailbox_that_should_receive_file_content);
                    // Do the I/O
                    S4U_Simulation::writeToDisk(msg->payload, dst_mountpoint);

                    // Wait for the comm to finish
                    msg = req->wait();
                    if (auto file_content_chunk_msg =
                            std::dynamic_pointer_cast<StorageServiceFileContentChunkMessage>(msg)) {
                        done = file_content_chunk_msg->last_chunk;
                    } else {
                        throw std::runtime_error("FileTransferThread::downloadFile() : Received an unexpected [" +
                                                 msg->getName() + "] message!");
                    }
                }
                // Do the I/O for the last chunk
                S4U_Simulation::writeToDisk(msg->payload, dst_mountpoint);
            } catch (std::shared_ptr<NetworkError> &e) {
                throw WorkflowExecutionException(e);
            }
        }

    }
};
