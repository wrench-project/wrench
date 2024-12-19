/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/storage/StorageServiceProperty.h>
#include <wrench/exceptions/ExecutionException.h>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/services/storage/StorageService.h>
#include <wrench/services/storage/compound/CompoundStorageService.h>
#include <wrench/services/compute/cloud/CloudComputeService.h>
#include "wrench/services/storage/StorageServiceMessage.h"
#include <wrench/services/storage/StorageServiceMessagePayload.h>
#include <wrench/simgrid_S4U_util/S4U_CommPort.h>
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
     */
    StorageService::StorageService(const std::string &hostname,
                                   const std::string &service_name) : Service(hostname, service_name) {

        this->state = StorageService::UP;
        this->is_scratch_ = false;
    }

    /**
     * @brief Indicate whether this storage service is a scratch service of a ComputeService
     * @param is_scratch: true if scratch, false otherwise
     */
    void StorageService::setIsScratch(bool is_scratch) {
        this->is_scratch_ = is_scratch;
    }


    //    /**
    //     * @brief Store a file at a particular location BEFORE the simulation is launched
    //     *
    //     * @param location: a file location
    //     *
    //     */
    //    void StorageService::stageFile(const std::shared_ptr<FileLocation> &location) {
    //        location->getStorageService()->stageFile(location->getFile(), location->getMountPoint(),
    //                                                 location->getAbsolutePathAtMountPoint());
    //    }
    //
    //    /**
    //     * @brief Store a file at a particular mount point and directory BEFORE the simulation is launched
    //     *
    //     * @param file: a file
    //     * @param mountpoint: a mount point
    //     * @param path: a path at the mount point
    //     */
    //    void StorageService::stageFile(const std::shared_ptr<DataFile> &file, const std::string &mountpoint, std::string path) {
    //        auto fs = this->file_systems[mountpoint].get();
    //
    //        try {
    //            fs->stageFile(file, std::move(path));
    //        } catch (std::exception &e) {
    //            throw;
    //        }
    //    }

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
     * @brief Synchronously write a file to the storage service
     *
     * @param answer_commport: the commport on which to expect the answer
     * @param location: the location
     * @param num_bytes_to_write: the number of bytes to write to the file.
     * @param wait_for_answer: whether to wait for the answer
     *
     */
    void StorageService::writeFile(S4U_CommPort *answer_commport,
                                   const std::shared_ptr<FileLocation> &location,
                                   sg_size_t num_bytes_to_write,
                                   bool wait_for_answer) {

        if (location == nullptr) {
            throw std::invalid_argument("StorageService::writeFile(): Invalid arguments");
        }

        if (location->getStorageService() != this->getSharedPtr<StorageService>()) {
            throw std::invalid_argument("StorageService::writeFile(): Can only read from a location at that same storage service");
        }

        this->assertServiceIsUp();

        this->commport->putMessage(
                new StorageServiceFileWriteRequestMessage(
                        answer_commport,
                        simgrid::s4u::this_actor::get_host(),
                        location,
                        num_bytes_to_write,
                        this->getMessagePayloadValue(
                                StorageServiceMessagePayload::FILE_WRITE_REQUEST_MESSAGE_PAYLOAD)));

        // Wait for a reply

        {
            auto msg = answer_commport->getMessage<StorageServiceFileWriteAnswerMessage>(this->network_timeout, "StorageService::writeFile(): Received a totally");

            if (not msg->success) {
                throw ExecutionException(msg->failure_cause);
            }

            // Update buffer size according to which storage service actually answered.
            auto buffer_size = msg->buffer_size;

            if (buffer_size < 1) {
                // just wait for the final ack (no timeout!)
                answer_commport->getMessage<StorageServiceAckMessage>("StorageService::writeFile(): Received an");


            } else {
                auto file = location->getFile();
                for (auto const &dwmb: msg->data_write_commport_and_bytes) {
                    // Bufferized
                    sg_size_t remaining = dwmb.second;
                    while (remaining > buffer_size) {

                        dwmb.first->putMessage(
                                new StorageServiceFileContentChunkMessage(
                                        file, buffer_size, false));
                        remaining -= buffer_size;
                    }
                    dwmb.first->putMessage(new StorageServiceFileContentChunkMessage(
                            file, remaining, true));

                    //Waiting for the final ack
                    answer_commport->getMessage<StorageServiceAckMessage>(this->network_timeout, "StorageService::writeFile(): Received an");
                }
            }
        }
    }


    /***************************************************************/
    /****** EVERYTHING BELOW ARE INTERACTIONS WITH THE DAEMON ******/
    /***************************************************************/

    /**
     * @brief Synchronously asks the storage service for its total free space capacity
     * @return A number of bytes
     *
     */
    sg_size_t StorageService::getTotalFreeSpace() {
        return getTotalFreeSpaceAtPath("");
    }

    /**
     * @brief Synchronously asks the storage service for its total free space capacity
     * at a particular path (i.e., at the partition that holds that path). If the path
     * is the empty string, then it's the sum total free space across all partitions. If the
     * path is invalid, then this method returns 0.
     *  @param path a path
     *
     *  @return A number of bytes (or 0 if the path is invalid)
     */
    sg_size_t StorageService::getTotalFreeSpaceAtPath(const std::string &path) {
        assertServiceIsUp();

        // Send a message to the daemon
        auto answer_commport = S4U_Daemon::getRunningActorRecvCommPort();
        this->commport->putMessage(new StorageServiceFreeSpaceRequestMessage(
                answer_commport,
                path,
                this->getMessagePayloadValue(
                        StorageServiceMessagePayload::FREE_SPACE_REQUEST_MESSAGE_PAYLOAD)));

        // Wait for a reply
        auto msg = answer_commport->getMessage<StorageServiceFreeSpaceAnswerMessage>(this->network_timeout, "StorageService::getTotalFreeSpaceAtPath() Received an");

        return msg->free_space;
    }

    /**
     * @brief Asks the storage service whether it holds a file
     *
     * @param answer_commport: the answer commport to which the reply from the server should be sent
     * @param location: the location to lookup
     *
     * @return true if the file is present, false otherwise
     */
    bool StorageService::lookupFile(S4U_CommPort *answer_commport,
                                    const std::shared_ptr<FileLocation> &location) {

        if (!answer_commport or !location) {
            throw std::invalid_argument("StorageService::lookupFile(): Invalid nullptr arguments");
        }

        assertServiceIsUp(this->getSharedPtr<Service>());

        // Send a message to the daemon
        this->commport->putMessage(
                new StorageServiceFileLookupRequestMessage(
                        answer_commport,
                        location,
                        this->getMessagePayloadValue(
                                StorageServiceMessagePayload::FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD)));

        // Wait for a reply
        auto msg = answer_commport->getMessage<StorageServiceFileLookupAnswerMessage>(this->network_timeout, "StorageService::lookupFile():");

        return msg->file_is_available;
    }


    /**
     * @brief Read a file from the storage service
     *
     * @param answer_commport: the commport on which to expect the answer
     * @param location: the location
     * @param num_bytes: the number of bytes to read
     * @param wait_for_answer: whether to wait for the answer
     */
    void StorageService::readFile(S4U_CommPort *answer_commport,
                                  const std::shared_ptr<FileLocation> &location,
                                  sg_size_t num_bytes,
                                  bool wait_for_answer) {

        if (!answer_commport or !location or (num_bytes < 0.0)) {
            throw std::invalid_argument("StorageService::readFile(): Invalid nullptr/0 arguments");
        }

        if (location->getStorageService() != this->getSharedPtr<StorageService>()) {
            throw std::invalid_argument("StorageService::readFile(): Can only read from a location at that same storage service");
        }

        assertServiceIsUp(this->getSharedPtr<Service>());

        this->commport->putMessage(
                new StorageServiceFileReadRequestMessage(
                        answer_commport,
                        simgrid::s4u::this_actor::get_host(),
                        location,
                        num_bytes,
                        this->getMessagePayloadValue(StorageServiceMessagePayload::FILE_READ_REQUEST_MESSAGE_PAYLOAD)));


        if (wait_for_answer) {
            // Wait for a reply
            auto msg = answer_commport->getMessage<StorageServiceFileReadAnswerMessage>(this->network_timeout, "StorageService::readFile(): Received an");

            // If it's not a success, throw an exception
            if (not msg->success) {
                std::shared_ptr<FailureCause> cause = msg->failure_cause;
                throw ExecutionException(cause);
            }

            if (msg->buffer_size < 1) {
                // Non-Bufferized
                // Just wait for the final ack (no timeout!)
                answer_commport->getMessage<StorageServiceAckMessage>("StorageService::readFile(): Received an");
            } else {
                unsigned long number_of_sources = msg->number_of_sources;

                // Otherwise, retrieve the file chunks until the last one is received
                // Noting that we have multiple sources
                unsigned long num_final_chunks_received = 0;
                while (true) {
                    std::shared_ptr<StorageServiceFileContentChunkMessage> file_content_chunk_msg = nullptr;
                    try {
                        file_content_chunk_msg = msg->commport_to_receive_the_file_content->getMessage<StorageServiceFileContentChunkMessage>("StorageService::readFile(): Received an");
                    } catch (...) {
                        S4U_CommPort::retireTemporaryCommPort(msg->commport_to_receive_the_file_content);
                        throw;
                    }
                    if (file_content_chunk_msg->last_chunk) {
                        num_final_chunks_received++;
                        if (num_final_chunks_received == msg->number_of_sources) {
                            break;
                        }
                    }
                }

                //Waiting for all the final acks
                for (unsigned long source = 0; source < number_of_sources; source++) {
                    answer_commport->getMessage<StorageServiceAckMessage>(this->network_timeout, "StorageService::readFile(): Received an");
                }

                S4U_CommPort::retireTemporaryCommPort(msg->commport_to_receive_the_file_content);
            }
        }
    }


    /**
     * @brief Synchronously and sequentially read a set of files from storage services
     *
     * @param locations: a map of files to locations
     *
     */
    void StorageService::readFiles(std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> locations) {
        StorageService::writeOrReadFiles(READ, std::move(locations));
    }

    /**
     * @brief Synchronously and sequentially upload a set of files from storage services
     *
     * @param locations: a map of files to locations
     *
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
                StorageService::readFileAtLocation(location);

                WRENCH_INFO("File %s read", file->getID().c_str());

            } else {
                WRENCH_INFO("Writing file %s to location %s",
                            file->getID().c_str(),
                            location->toString().c_str());
                StorageService::writeFileAtLocation(location);

                WRENCH_INFO("File %s written", file->getID().c_str());
            }
        }
    }

    /**
     * @brief Delete a file on the storage service
     *
     * @param answer_commport: the answer commport to which the reply from the server should be sent
     * @param location: the location to delete
     * @param wait_for_answer: whether this call should
     */
    void StorageService::deleteFile(S4U_CommPort *answer_commport,
                                    const std::shared_ptr<FileLocation> &location,
                                    bool wait_for_answer) {
        if (!answer_commport or !location) {
            throw std::invalid_argument("StorageService::deleteFile(): Invalid nullptr arguments");
        }

        if (location->isScratch()) {
            throw std::invalid_argument("StorageService::deleteFile(): Cannot be called on a SCRATCH location");
        }

        assertServiceIsUp(this->getSharedPtr<Service>());

        // Send a message to the storage service's daemon
        this->commport->putMessage(
                new StorageServiceFileDeleteRequestMessage(
                        answer_commport,
                        location,
                        this->getMessagePayloadValue(StorageServiceMessagePayload::FILE_DELETE_REQUEST_MESSAGE_PAYLOAD)));

        if (wait_for_answer) {

            // Wait for a reply
            std::unique_ptr<SimulationMessage> message = nullptr;

            auto msg = answer_commport->getMessage<StorageServiceFileDeleteAnswerMessage>(this->network_timeout, "StorageService::deleteFile():");
            // On failure, throw an exception
            if (!msg->success) {
                throw ExecutionException(std::move(msg->failure_cause));
            }
            WRENCH_INFO("Deleted file at %s", location->toString().c_str());
        }
    }

    /**
     * @brief Synchronously ask the storage service to read a file from another storage service
     *
     * @param src_location: the location where to read the file
     * @param dst_location: the location where to write the file
     *
     */
    void StorageService::copyFile(const std::shared_ptr<FileLocation> &src_location,
                                  const std::shared_ptr<FileLocation> &dst_location) {
        if ((src_location == nullptr) || (dst_location == nullptr)) {
            throw std::invalid_argument("StorageService::copyFile(): Invalid nullptr arguments");
        }
        if (src_location->getFile() != dst_location->getFile()) {
            throw std::invalid_argument("StorageService::copyFile(): src and dst locations should be for the same file");
        }

        if (std::dynamic_pointer_cast<CompoundStorageService>(src_location->getStorageService()) or
            std::dynamic_pointer_cast<CompoundStorageService>(dst_location->getStorageService())) {
            return CompoundStorageService::copyFile(src_location, dst_location);
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

        S4U_CommPort *commport_to_contact;
        if (dst_is_non_bufferized) {
            commport_to_contact = dst_location->getStorageService()->commport;
        } else if (src_is_non_bufferized) {
            commport_to_contact = src_location->getStorageService()->commport;
        } else {
            commport_to_contact = dst_location->getStorageService()->commport;
        }


        // Send a message to the daemon of the dst service
        auto answer_commport = S4U_Daemon::getRunningActorRecvCommPort();
        src_location->getStorageService()->getSimulation()->getOutput().addTimestampFileCopyStart(Simulation::getCurrentSimulatedDate(), file,
                                                                                             src_location,
                                                                                             dst_location);
        commport_to_contact->putMessage(
                new StorageServiceFileCopyRequestMessage(
                        answer_commport,
                        src_location,
                        dst_location,
                        dst_location->getStorageService()->getMessagePayloadValue(
                                StorageServiceMessagePayload::FILE_COPY_REQUEST_MESSAGE_PAYLOAD)));

        // Wait for a reply
        std::unique_ptr<SimulationMessage> message = nullptr;


        auto msg = answer_commport->getMessage<StorageServiceFileCopyAnswerMessage>();

        if (msg->failure_cause) {
            WRENCH_DEBUG("%s", msg->failure_cause->toString().c_str());
            throw ExecutionException(std::move(msg->failure_cause));
        }
    }

    /**
     * @brief Asynchronously ask for a file copy between two storage services
     *
     * @param answer_commport: the commport to which a notification message will be sent
     * @param src_location: the source location
     * @param dst_location: the destination location
     *
     */
    void StorageService::initiateFileCopy(S4U_CommPort *answer_commport,
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

        S4U_CommPort *commport_to_contact;
        if (dst_is_non_bufferized) {
            commport_to_contact = dst_location->getStorageService()->commport;
        } else if (src_is_non_bufferized) {
            commport_to_contact = src_location->getStorageService()->commport;
        } else {
            commport_to_contact = dst_location->getStorageService()->commport;
        }

        src_location->getStorageService()->getSimulation()->getOutput().addTimestampFileCopyStart(Simulation::getCurrentSimulatedDate(), file,
                                                                                             src_location,
                                                                                             dst_location);

        // Send a message to the daemon on the dst location
        commport_to_contact->putMessage(
                new StorageServiceFileCopyRequestMessage(
                        answer_commport,
                        src_location,
                        dst_location,
                        dst_location->getStorageService()->getMessagePayloadValue(
                                StorageServiceMessagePayload::FILE_COPY_REQUEST_MESSAGE_PAYLOAD)));
    }


    //    /**
    //     * @brief Store a file at a particular mount point ex-nihilo. Doesn't notify a file registry service and will do nothing (and won't complain) if the file already exists
    //     * at that location.
    //     *
    //     * @param location: a file location, must be the same object as the function is invoked on
    //     *
    //     */
    //    void StorageService::createFile(const std::shared_ptr<FileLocation> &location) {
    //        if (location->getStorageService() != this->getSharedPtr<StorageService>()) {
    //            throw std::invalid_argument("StorageService::createFile(file,location) must be called on the same StorageService that the location uses");
    //        }
    //        stageFile(location->getFile(), location->getPath());
    //    }

    //    /**
    // * @brief Store a file at a particular mount point ex-nihilo. Doesn't notify a file registry service and will do nothing (and won't complain) if the file already exists
    // * at that location.
    //
    //*
    //* @param file: a file
    //* @param path: path to file
    //*
    //*/
    //    void StorageService::createFile(const std::shared_ptr<DataFile> &file, const std::string &path) {
    //
    //        createFile(FileLocation::LOCATION(this->getSharedPtr<StorageService>(), path, file));
    //    }

    //    /**
    // * @brief Store a file at a particular mount point ex-nihilo. Doesn't notify a file registry service and will do nothing (and won't complain) if the file already exists
    // * at that location.
    // * @param file: a file
    // *
    // */
    //    void StorageService::createFile(const std::shared_ptr<DataFile> &file) {
    //
    //        createFile(FileLocation::LOCATION(this->getSharedPtr<StorageService>(), getMountPoint(), file));
    //    }

    //    /**
    //     * @brief Determines whether the storage service is bufferized
    //     * @return true if bufferized, false otherwise
    //     */
    //    bool StorageService::isBufferized() const {
    //        return this->buffer_size > 1;
    //    }

    //    /**
    //     * @brief Determines whether the storage service has the file. This doesn't simulate anything and is merely a zero-simulated-time data structure lookup.
    //     * If you want to simulate the overhead of querying the StorageService, instead use lookupFile().
    //     *
    //     * @param file a file
    //     *
    //     * @return true if the file is present, false otherwise
    //     */
    //    bool StorageService::hasFile(const shared_ptr<DataFile> &file) {
    //        return this->hasFile(file, this->getMountPoint());
    //    }


}// namespace wrench
