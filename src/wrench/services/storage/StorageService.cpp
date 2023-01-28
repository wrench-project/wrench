/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/storage/StorageServiceProperty.h>
#include <wrench/services/storage/storage_helpers/LogicalFileSystem.h>
#include <wrench/exceptions/ExecutionException.h>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/services/storage/StorageService.h>
#include <wrench/services/compute/cloud/CloudComputeService.h>
#include "wrench/services/storage/StorageServiceMessage.h"
#include <wrench/services/storage/StorageServiceMessagePayload.h>
#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>
#include <wrench/simulation/Simulation.h>
#include <wrench/simgrid_S4U_util/S4U_PendingCommunication.h>

#include <memory>

WRENCH_LOG_CATEGORY(wrench_core_storage_service, "Log category for Storage Service");

#define GB (1000.0 * 1000.0 * 1000.0)

namespace wrench {
    /**
     * @brief Constructor
     *
     * @param hostname: the name of the host on which the service should run
     * @param service_name: the name of the storage service
     *
     * @throw std::invalid_argument
     */
    StorageService::StorageService(const std::string &hostname,
                                   const std::string &service_name) : Service(hostname, service_name) {

        this->state = StorageService::UP;
        this->is_scratch = false;
    }

    /**
     * @brief Determines whether the storage service is a scratch service of a ComputeService
     * @return true if it is, false otherwise
     */
    bool StorageService::isScratch() const {
        return this->is_scratch;
    }

    /**
     * @brief Indicate that this storage service is a scratch service of a ComputeService
     */
    void StorageService::setScratch() {
        this->is_scratch = true;
    }


    /**
     * @brief Store a file at a particular location BEFORE the simulation is launched
     *
     * @param location: a file location
     *
     */
    void StorageService::stageFile(const std::shared_ptr<FileLocation> &location) {
        location->getStorageService()->stageFile(location->getFile(), location->getMountPoint(),
                                                 location->getAbsolutePathAtMountPoint());
    }

    /**
     * @brief Store a file at a particular mount point and directory BEFORE the simulation is launched
     *
     * @param file: a file
     * @param mountpoint: a mount point
     * @param path: a path at the mount point
     */
    void StorageService::stageFile(const std::shared_ptr<DataFile> &file, const std::string &mountpoint, std::string path) {
        auto fs = this->file_systems[mountpoint].get();

        try {
            fs->stageFile(file, std::move(path));
        } catch (std::exception &e) {
            throw;
        }
    }

    /**
     * @brief Stop the service
     */
    void StorageService::stop() {
        // Just call the super class's method
        Service::stop();
    }

    /***************************************************************/
    /****** THESE FUNCTIONS PROVIDE AN OBJECT API FOR TALKING ******/
    /******     TO SPECIFIC STORAGE SERVERS NOT LOCATIONS     ******/
    /******    CONSTRUCTS LOCATION AND FORWARDS TO DAEMON    ******/
    /***************************************************************/

    /**
     * @brief Synchronously asks the storage service whether it holds a file
     *
     * @param file: the file
     *
     * @return true or false
     *
     * @throw ExecutionException
     * @throw std::invalid_arguments
     */
    bool StorageService::lookupFile(const std::shared_ptr<DataFile> &file) {
        return StorageService::lookupFile(FileLocation::LOCATION(static_pointer_cast<StorageService>(shared_from_this()), file));
    }

    /**
     * @brief Synchronously delete a file at a location
     *
     * @param file: the file
     * @param file_registry_service: a file registry service that should be updated once the
     *         file deletion has (successfully) completed (none if nullptr)
     *
     * @throw ExecutionException
     * @throw std::runtime_error
     * @throw std::invalid_argument
     */
    void StorageService::deleteFile(const std::shared_ptr<DataFile> &file, const std::shared_ptr<FileRegistryService> &file_registry_service) {
        StorageService::deleteFile(FileLocation::LOCATION(static_pointer_cast<StorageService>(shared_from_this()), file), file_registry_service);
    }

    /**
     * @brief Synchronously read a file from the storage service
     *
     * @param file: the file
     *
     * @throw ExecutionException
     * @throw std::invalid_arguments
     */
    void StorageService::readFile(const std::shared_ptr<DataFile> &file) {
        StorageService::readFile(FileLocation::LOCATION(static_pointer_cast<StorageService>(shared_from_this()), file));
    }
    /**
     * @brief Synchronously read a file from the storage service
     *
     * @param file: the file
     * @param num_bytes: the number of bytes to read
     *
     * @throw ExecutionException
     * @throw std::invalid_arguments
     */
    void StorageService::readFile(const std::shared_ptr<DataFile> &file, double num_bytes) {
        StorageService::readFile(FileLocation::LOCATION(static_pointer_cast<StorageService>(shared_from_this()), file), num_bytes);
    }

    /**
     * @brief Synchronously read a file from the storage service
     *
     * @param file: the file
     * @param path: the absolute file path
     *
     * @throw ExecutionException
     * @throw std::invalid_arguments
     */
    void StorageService::readFile(const std::shared_ptr<DataFile> &file, const std::string &path) {
        StorageService::readFile(FileLocation::LOCATION(static_pointer_cast<StorageService>(shared_from_this()), path, file));
    }

    /**
     * @brief Synchronously read a file from the storage service
     *
     * @param file: the file
     * @param path: the absolute file path
     * @param num_bytes: the number of bytes to read
     *
     * @throw ExecutionException
     * @throw std::invalid_arguments
     */
    void StorageService::readFile(const std::shared_ptr<DataFile> &file, const std::string &path, double num_bytes) {
        StorageService::readFile(FileLocation::LOCATION(static_pointer_cast<StorageService>(shared_from_this()), path, file), num_bytes);
    }

    /**
     * @brief Synchronously write a file to the storage service
     *
     * @param location: the file location
     *
     * @throw ExecutionException
     */
    void StorageService::writeFile(const std::shared_ptr<FileLocation> &location) {
        if (location == nullptr) {
            throw std::invalid_argument("StorageService::writeFile(): Invalid nullptr arguments");
        }
        location->getStorageService()->writeFile(location->getFile(), location->getFullAbsolutePath());
    }

    /**
     * @brief Synchronously write a file to the storage service
     *
     * @param file: the file
     *
     * @throw ExecutionException
     */
    void StorageService::writeFile(const std::shared_ptr<DataFile> &file) {
        this->writeFile(file, this->getMountPoint());
    }

    /**
     * @brief Synchronously write a file to the storage service
     *
     * @param file: the file
     * @param path: path to file
     *
     * @throw ExecutionException
     */
    void StorageService::writeFile(const std::shared_ptr<DataFile> &file, const std::string &path) {
        if (file == nullptr) {
            throw std::invalid_argument("StorageService::writeFile(): Invalid arguments");
        }

        assertServiceIsUp();

        // Send a  message to the daemon
        auto answer_mailbox = S4U_Daemon::getRunningActorRecvMailbox();

        S4U_Mailbox::putMessage(this->mailbox,
                                new StorageServiceFileWriteRequestMessage(
                                        answer_mailbox,
                                        simgrid::s4u::this_actor::get_host(),
                                        wrench::FileLocation::LOCATION(this->getSharedPtr<StorageService>(), path, file),
                                        this->buffer_size,
                                        this->getMessagePayloadValue(
                                                StorageServiceMessagePayload::FILE_WRITE_REQUEST_MESSAGE_PAYLOAD)));

        // Wait for a reply
        std::shared_ptr<SimulationMessage> message;

        message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);

        if (auto msg = dynamic_cast<StorageServiceFileWriteAnswerMessage *>(message.get())) {
            // If not a success, throw an exception
            if (not msg->success) {
                throw ExecutionException(msg->failure_cause);
            }

            if (this->buffer_size < 1) {
                // just wait for the final ack (no timeout!)
                message = S4U_Mailbox::getMessage(answer_mailbox);
                if (not dynamic_cast<StorageServiceAckMessage *>(message.get())) {
                    throw std::runtime_error("StorageService::writeFile(): Received an unexpected [" +
                                             message->getName() + "] message instead of final ack!");
                }

            } else {
                // Bufferized
                double remaining = file->getSize();
                while (remaining - this->buffer_size > DBL_EPSILON) {
                    S4U_Mailbox::putMessage(msg->data_write_mailbox,
                                            new StorageServiceFileContentChunkMessage(
                                                    file, this->buffer_size, false));
                    remaining -= this->buffer_size;
                }
                S4U_Mailbox::putMessage(msg->data_write_mailbox, new StorageServiceFileContentChunkMessage(
                                                                         file, remaining, true));

                //Waiting for the final ack
                message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
                if (not dynamic_cast<StorageServiceAckMessage *>(message.get())) {
                    throw std::runtime_error("StorageService::writeFile(): Received an unexpected [" +
                                             message->getName() + "] message instead of final ack!");
                }
            }

        } else {
            throw std::runtime_error("StorageService::writeFile(): Received a totally unexpected [" +
                                     message->getName() + "] message!");
        }
    }


    /***************************************************************/
    /****** EVERYTHING BELOW ARE INTERACTIONS WITH THE DAEMON ******/
    /***************************************************************/

    /**
     * @brief Synchronously asks the storage service for its capacity at all its
     *        mount points
     * @return The free space in bytes of each mount point, as a map
     *
     * @throw ExecutionException
     *
     * @throw std::runtime_error
     *
     */
    std::map<std::string, double> StorageService::getFreeSpace() {
        assertServiceIsUp();

        // Send a message to the daemon
        auto answer_mailbox = S4U_Daemon::getRunningActorRecvMailbox();
        S4U_Mailbox::putMessage(this->mailbox, new StorageServiceFreeSpaceRequestMessage(
                                                       answer_mailbox,
                                                       this->getMessagePayloadValue(
                                                               StorageServiceMessagePayload::FREE_SPACE_REQUEST_MESSAGE_PAYLOAD)));

        // Wait for a reply
        std::unique_ptr<SimulationMessage> message = nullptr;
        message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);

        if (auto msg = dynamic_cast<StorageServiceFreeSpaceAnswerMessage *>(message.get())) {
            return msg->free_space;
        } else {
            throw std::runtime_error("StorageService::getFreeSpace(): Unexpected [" + message->getName() + "] message");
        }
    }


    /**
     * @brief Synchronously asks the storage service whether it holds a file
     *
     * @param location: the file location
     *
     * @return true or false
     *
     * @throw ExecutionException
     * @throw std::invalid_arguments
     */
    bool StorageService::lookupFile(const std::shared_ptr<FileLocation> &location) {
        if (location == nullptr) {
            throw std::invalid_argument("StorageService::lookupFile(): Invalid nullptr argument");
        }

        auto storage_service = location->getStorageService();
        auto file = location->getFile();

        assertServiceIsUp(storage_service);

        // Send a message to the daemon
        auto answer_mailbox = S4U_Daemon::getRunningActorRecvMailbox();
        S4U_Mailbox::putMessage(
                location->getStorageService()->mailbox,
                new StorageServiceFileLookupRequestMessage(
                        answer_mailbox,
                        location,
                        storage_service->getMessagePayloadValue(
                                StorageServiceMessagePayload::FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD)));

        // Wait for a reply
        std::shared_ptr<SimulationMessage> message;
        message = S4U_Mailbox::getMessage(answer_mailbox, storage_service->network_timeout);

        if (auto msg = dynamic_cast<StorageServiceFileLookupAnswerMessage *>(message.get())) {
            return msg->file_is_available;
        } else {
            throw std::runtime_error("StorageService::lookupFile(): Unexpected [" + message->getName() + "] message");
        }
    }

    /**
     * @brief Synchronously read a file from the storage service
     *
     * @param location: the location to read the file from
     *
     * @throw ExecutionException
     * @throw std::invalid_arguments
     */

    void StorageService::readFile(const std::shared_ptr<FileLocation> &location) {
        if (location == nullptr) {
            throw std::invalid_argument("StorageService::readFile(): Invalid arguments");
        }//This check DOES need to exist, because we call file->getSize()
        auto answer_mailbox = S4U_Daemon::getRunningActorRecvMailbox();
        readFile(location, answer_mailbox, location->getFile()->getSize());
    }

    /**
     * @brief Synchronously read a file from the storage service
     *
     * @param location: the location to read the file from
     * @param num_bytes_to_read: number of bytes to read from the file
     *
     * @throw ExecutionException
     * @throw std::invalid_arguments
     */

    void StorageService::readFile(const std::shared_ptr<FileLocation> &location, double num_bytes_to_read) {
        // Get mailbox to send message too
        auto answer_mailbox = S4U_Daemon::getRunningActorRecvMailbox();
        readFile(location, answer_mailbox, num_bytes_to_read);
    }

    /**
     * @brief Synchronously read a file from the storage service
     * @param location: the file location
     * @param answer_mailbox: the answer mailbox
     * @param num_bytes_to_read: number of bytes to read
     */
    void StorageService::readFile(const std::shared_ptr<FileLocation> &location,
                                  simgrid::s4u::Mailbox *answer_mailbox,
                                  double num_bytes_to_read) {

        if ((location == nullptr) or (answer_mailbox == nullptr) or (num_bytes_to_read < 0.0)) {
            throw std::invalid_argument("StorageService::readFile(): Invalid arguments");
        }

        auto storage_service = location->getStorageService();
        auto file = location->getFile();

        assertServiceIsUp(storage_service);

        simgrid::s4u::Mailbox *chunk_receiving_mailbox;
        if (storage_service->buffer_size == 0) {
            chunk_receiving_mailbox = nullptr;
        } else {
            chunk_receiving_mailbox = S4U_Mailbox::getTemporaryMailbox();
        }

        try {
            S4U_Mailbox::putMessage(storage_service->mailbox,
                                    new StorageServiceFileReadRequestMessage(
                                            answer_mailbox,
                                            simgrid::s4u::this_actor::get_host(),
                                            chunk_receiving_mailbox,
                                            location,
                                            num_bytes_to_read,
                                            storage_service->getMessagePayloadValue(
                                                    StorageServiceMessagePayload::FILE_READ_REQUEST_MESSAGE_PAYLOAD)));
        } catch (ExecutionException &e) {
            if (chunk_receiving_mailbox) S4U_Mailbox::retireTemporaryMailbox(chunk_receiving_mailbox);
            throw;
        }

        // Wait for a reply
        std::unique_ptr<SimulationMessage> message = nullptr;

        try {
            message = S4U_Mailbox::getMessage(answer_mailbox, storage_service->network_timeout);
        } catch (ExecutionException &e) {
            if (chunk_receiving_mailbox) S4U_Mailbox::retireTemporaryMailbox(chunk_receiving_mailbox);
            throw;
        }

        if (auto msg = dynamic_cast<StorageServiceFileReadAnswerMessage *>(message.get())) {

            // If it's not a success, throw an exception
            if (not msg->success) {
                std::shared_ptr<FailureCause> cause = msg->failure_cause;
                if (chunk_receiving_mailbox) S4U_Mailbox::retireTemporaryMailbox(chunk_receiving_mailbox);
                throw ExecutionException(cause);
            }

            if (msg->buffer_size < 1) {
                // Non-Bufferized
                // Just wait for the final ack (no timeout!)
                message = S4U_Mailbox::getMessage(answer_mailbox);
                if (not dynamic_cast<StorageServiceAckMessage *>(message.get())) {
                    throw std::runtime_error("StorageService::readFile(): Received an unexpected [" +
                                             message->getName() + "] message!");
                }

            } else {
                // Otherwise, retrieve the file chunks until the last one is received
                while (true) {
                    std::shared_ptr<SimulationMessage> file_content_message = nullptr;
                    try {
                        file_content_message = S4U_Mailbox::getMessage(chunk_receiving_mailbox);
                    } catch (ExecutionException &e) {
                        S4U_Mailbox::retireTemporaryMailbox(chunk_receiving_mailbox);
                        throw;
                    }

                    if (auto file_content_chunk_msg = dynamic_cast<StorageServiceFileContentChunkMessage *>(
                                file_content_message.get())) {
                        if (file_content_chunk_msg->last_chunk) {
                            S4U_Mailbox::retireTemporaryMailbox(chunk_receiving_mailbox);
                            break;
                        }
                    } else {
                        S4U_Mailbox::retireTemporaryMailbox(chunk_receiving_mailbox);
                        throw std::runtime_error("StorageService::readFile(): Received an unexpected [" +
                                                 file_content_message->getName() + "] message!");
                    }
                }

                S4U_Mailbox::retireTemporaryMailbox(chunk_receiving_mailbox);

                //Waiting for the final ack
                message = S4U_Mailbox::getMessage(answer_mailbox, storage_service->network_timeout);
                if (not dynamic_cast<StorageServiceAckMessage *>(message.get())) {
                    throw std::runtime_error("StorageService::readFile(): Received an unexpected [" +
                                             message->getName() + "] message!");
                }
            }
        }
    }


    /**
 * @brief Synchronously and sequentially read a set of files from storage services
 *
 * @param locations: a map of files to locations
 *
 * @throw std::runtime_error
 * @throw ExecutionException
 */
    void StorageService::readFiles(std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> locations) {
        StorageService::writeOrReadFiles(READ, std::move(locations));
    }

    /**
 * @brief Synchronously and sequentially upload a set of files from storage services
 *
 * @param locations: a map of files to locations
 *
 * @throw std::runtime_error
 * @throw ExecutionException
 */
    void StorageService::writeFiles(std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> locations) {
        StorageService::writeOrReadFiles(WRITE, std::move(locations));
    }

    /**
 * @brief Synchronously and sequentially write/read a set of files to/from storage services
 *
 * @param action: FileOperation::READ (download) or FileOperation::WRITE
 * @param locations: a map of files to locations
 *
 * @throw std::runtime_error
 * @throw ExecutionException
 */
    void StorageService::writeOrReadFiles(FileOperation action,
                                          std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> locations) {
        for (auto const &f: locations) {
            if ((f.first == nullptr) or (f.second == nullptr)) {
                throw std::invalid_argument("StorageService::writeOrReadFiles(): invalid argument");
            }
        }

        // Create a temporary sorted list of files so that the order in which files are read/written is deterministic!
        std::map<std::string, std::shared_ptr<DataFile>> sorted_files;
        for (auto const &f: locations) {
            sorted_files[f.first->getID()] = f.first;
        }

        for (auto const &f: sorted_files) {
            auto file = f.second;
            auto location = locations[file];

            if (action == READ) {
                WRENCH_INFO("Reading file %s from location %s",
                            file->getID().c_str(),
                            location->toString().c_str());
                StorageService::readFile(location);

                WRENCH_INFO("File %s read", file->getID().c_str());

            } else {
                WRENCH_INFO("Writing file %s to location %s",
                            file->getID().c_str(),
                            location->toString().c_str());
                StorageService::writeFile(location);

                WRENCH_INFO("File %s written", file->getID().c_str());
            }
        }
    }

    /**
<<<<<<< HEAD
     * @brief Synchronously delete a file at a location
     *
     * @param location: the file location
     * @param file_registry_service: a file registry service that should be updated once the
     *         file deletion has (successfully) completed (none if nullptr)
     *
     * @throw ExecutionException
     * @throw std::runtime_error
     * @throw std::invalid_argument
     */
    void StorageService::deleteFile(const std::shared_ptr<FileLocation> &location,
                                    const std::shared_ptr<FileRegistryService> &file_registry_service) {
        if (location == nullptr) {
            throw std::invalid_argument("StorageService::deleteFile(): Invalid nullptr arguments");
        }

        auto storage_service = location->getStorageService();
        auto file = location->getFile();

        assertServiceIsUp(storage_service);

        bool unregister = (file_registry_service != nullptr);
        // Send a message to the daemon
        auto answer_mailbox = S4U_Daemon::getRunningActorRecvMailbox();
        S4U_Mailbox::putMessage(storage_service->mailbox,
                                new StorageServiceFileDeleteRequestMessage(
                                        answer_mailbox,
                                        location,
                                        storage_service->getMessagePayloadValue(
                                                StorageServiceMessagePayload::FILE_DELETE_REQUEST_MESSAGE_PAYLOAD)));

        // Wait for a reply
        std::unique_ptr<SimulationMessage> message = nullptr;

        message = S4U_Mailbox::getMessage(answer_mailbox, storage_service->network_timeout);

        if (auto msg = dynamic_cast<StorageServiceFileDeleteAnswerMessage *>(message.get())) {
            // On failure, throw an exception
            if (!msg->success) {
                throw ExecutionException(std::move(msg->failure_cause));
            }
            WRENCH_INFO("Deleted file %s at location %s",
                        file->getID().c_str(), location->toString().c_str());

            if (unregister) {
                file_registry_service->removeEntry(location);
            }

        } else {
            throw std::runtime_error("StorageService::deleteFile(): Unexpected [" + message->getName() + "] message");
        }
    }

    /**
     * @brief Synchronously ask the storage service to read a file from another storage service
     *
     * @param src_location: the location where to read the file
     * @param dst_location: the location where to write the file
     *
     * @throw ExecutionException
     * @throw std::invalid_argument
     */
    void StorageService::copyFile(const std::shared_ptr<FileLocation> &src_location,
                                  const std::shared_ptr<FileLocation> &dst_location) {
        if ((src_location == nullptr) || (dst_location == nullptr)) {
            throw std::invalid_argument("StorageService::copyFile(): Invalid nullptr arguments");
        }
        if (src_location->getFile() != dst_location->getFile()) {
            throw std::invalid_argument("StorageService::copyFile(): src and dst locations should be for the same file");
        }

        assertServiceIsUp(src_location->getStorageService());
        assertServiceIsUp(dst_location->getStorageService());

        auto file = src_location->getFile();
        bool src_is_bufferized = src_location->getStorageService()->isBufferized();
        bool src_is_non_bufferized = not src_is_bufferized;
        bool dst_is_bufferized = dst_location->getStorageService()->isBufferized();
        bool dst_is_non_bufferized = not dst_is_bufferized;

        //        if (src_is_non_bufferized and dst_is_bufferized) {
        //            throw std::runtime_error("Cannot copy a file from a non-bufferized storage service to a bufferized storage service (not implemented, yet)");
        //        }

        simgrid::s4u::Mailbox *mailbox_to_contact;
        if (dst_is_non_bufferized) {
            mailbox_to_contact = dst_location->getStorageService()->mailbox;
        } else if (src_is_non_bufferized) {
            mailbox_to_contact = src_location->getStorageService()->mailbox;
        } else {
            mailbox_to_contact = dst_location->getStorageService()->mailbox;
        }

        // Send a message to the daemon of the dst service
        auto answer_mailbox = S4U_Daemon::getRunningActorRecvMailbox();
        src_location->getStorageService()->simulation->getOutput().addTimestampFileCopyStart(Simulation::getCurrentSimulatedDate(), file,
                                                                                             src_location,
                                                                                             dst_location);

        S4U_Mailbox::putMessage(
                mailbox_to_contact,
                new StorageServiceFileCopyRequestMessage(
                        answer_mailbox,
                        src_location,
                        dst_location,
                        nullptr,
                        dst_location->getStorageService()->getMessagePayloadValue(
                                StorageServiceMessagePayload::FILE_COPY_REQUEST_MESSAGE_PAYLOAD)));

        // Wait for a reply
        std::unique_ptr<SimulationMessage> message = nullptr;

        message = S4U_Mailbox::getMessage(answer_mailbox);

        if (auto msg = dynamic_cast<StorageServiceFileCopyAnswerMessage *>(message.get())) {
            if (msg->failure_cause) {
                throw ExecutionException(std::move(msg->failure_cause));
            }
        } else {
            throw std::runtime_error("StorageService::copyFile(): Unexpected [" + message->getName() + "] message");
        }
    }

    /**
     * @brief Asynchronously ask for a file copy between two storage services
     *
     * @param answer_mailbox: the mailbox to which a notification message will be sent
     * @param src_location: the source location
     * @param dst_location: the destination location
     *
     * @throw ExecutionException
     * @throw std::invalid_argument
     *
     */
    void StorageService::initiateFileCopy(simgrid::s4u::Mailbox *answer_mailbox,
                                          const std::shared_ptr<FileLocation> &src_location,
                                          const std::shared_ptr<FileLocation> &dst_location) {
        if ((src_location == nullptr) || (dst_location == nullptr)) {
            throw std::invalid_argument("StorageService::initiateFileCopy(): Invalid nullptr arguments");
        }
        if (src_location->getFile() != dst_location->getFile()) {
            throw std::invalid_argument("StorageService::initiateFileCopy(): src and dst locations should be for the same file");
        }

        assertServiceIsUp(src_location->getStorageService());
        assertServiceIsUp(dst_location->getStorageService());

        auto file = src_location->getFile();
        bool src_is_bufferized = src_location->getStorageService()->isBufferized();
        bool src_is_non_bufferized = not src_is_bufferized;
        bool dst_is_bufferized = dst_location->getStorageService()->isBufferized();
        bool dst_is_non_bufferized = not dst_is_bufferized;

        //        if (src_is_non_bufferized and dst_is_bufferized) {
        //            throw std::runtime_error("Cannot copy a file from a non-bufferized storage service to a bufferized storage service (not implemented, yet)");
        //        }

        simgrid::s4u::Mailbox *mailbox_to_contact;
        if (dst_is_non_bufferized) {
            mailbox_to_contact = dst_location->getStorageService()->mailbox;
        } else if (src_is_non_bufferized) {
            mailbox_to_contact = src_location->getStorageService()->mailbox;
        } else {
            mailbox_to_contact = dst_location->getStorageService()->mailbox;
        }

        src_location->getStorageService()->simulation->getOutput().addTimestampFileCopyStart(Simulation::getCurrentSimulatedDate(), file,
                                                                                             src_location,
                                                                                             dst_location);

        // Send a message to the daemon on the dst location
        S4U_Mailbox::putMessage(
                mailbox_to_contact,
                new StorageServiceFileCopyRequestMessage(
                        answer_mailbox,
                        src_location,
                        dst_location,
                        nullptr,
                        dst_location->getStorageService()->getMessagePayloadValue(
                                StorageServiceMessagePayload::FILE_COPY_REQUEST_MESSAGE_PAYLOAD)));
    }

    /**
 * @brief Get the total static capacity of the storage service (in zero simulation time)
 * @return capacity of the storage service (double) for each mount point, in a map
 */
    std::map<std::string, double> StorageService::getTotalSpace() {
        std::map<std::string, double> to_return;
        for (auto const &fs: this->file_systems) {
            to_return[fs.first] = fs.second->getTotalCapacity();
        }
        return to_return;
    }

    /**
 * @brief Get the mount point (will throw is more than one)
 * @return the (sole) mount point of the service
 */
    std::string StorageService::getMountPoint() {
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
    std::set<std::string> StorageService::getMountPoints() {
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
    bool StorageService::hasMultipleMountPoints() {
        return (this->file_systems.size() > 1);
    }

    /**
* @brief Checked whether the storage service has a particular mount point
* @param mp: a mount point
*
* @return true whether the service has that mount point
*/
    bool StorageService::hasMountPoint(const std::string &mp) {
        return (this->file_systems.find(mp) != this->file_systems.end());
    }

    /**
     * @brief Store a file at a particular mount point ex-nihilo. Doesn't notify a file registry service and will do nothing (and won't complain) if the file already exists
     * at that location.
     *
     * @param location: a file location, must be the same object as the function is invoked on
     *
     * @throw std::invalid_argument
     */
    void StorageService::createFile(const std::shared_ptr<FileLocation> &location) {
        if (location->getStorageService() != this->getSharedPtr<StorageService>()) {
            throw std::invalid_argument("StorageService::createFile(file,location) must be called on the same StorageService that the location uses");
        }
        stageFile(location->getFile(), location->getMountPoint(),
                  location->getAbsolutePathAtMountPoint());
    }

    /**
 * @brief Store a file at a particular mount point ex-nihilo. Doesn't notify a file registry service and will do nothing (and won't complain) if the file already exists
 * at that location.

*
* @param file: a file
* @param path: path to file
*
*/
    void StorageService::createFile(const std::shared_ptr<DataFile> &file, const std::string &path) {

        createFile(FileLocation::LOCATION(this->getSharedPtr<StorageService>(), path, file));
    }

    /**
 * @brief Store a file at a particular mount point ex-nihilo. Doesn't notify a file registry service and will do nothing (and won't complain) if the file already exists
 * at that location.
 * @param file: a file
 *
 */
    void StorageService::createFile(const std::shared_ptr<DataFile> &file) {

        createFile(FileLocation::LOCATION(this->getSharedPtr<StorageService>(), getMountPoint(), file));
    }

    /**
     * @brief Determines whether the storage service is bufferized
     * @return true if bufferized, false otherwise
     */
    bool StorageService::isBufferized() const {
        return this->buffer_size > 1;
    }

    /**
     * @brief Determines whether the storage service has the file. This doesn't simulate anything and is merely a zero-simulated-time data structure lookup.
     * If you want to simulate the overhead of querying the StorageService, instead use lookupFile().
     * @param file a file
     * @return true if the file is present, false otherwise
     */
    bool StorageService::hasFile(const shared_ptr<DataFile> &file) {
        return this->hasFile(file, this->getMountPoint());
    }


}// namespace wrench
