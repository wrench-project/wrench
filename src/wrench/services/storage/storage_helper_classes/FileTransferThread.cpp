/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/simgrid_S4U_util/S4U_CommPort.h>
#include <wrench/services/storage/StorageServiceMessage.h>
#include <wrench/services/storage/storage_helpers/FileTransferThread.h>
#include "wrench/services/storage/storage_helpers/FileTransferThreadMessage.h"

#include <wrench-dev.h>
#include <boost/algorithm/string/classification.hpp>
#include <utility>
//#include <wrench/services/memory/MemoryManager.h>

WRENCH_LOG_CATEGORY(wrench_core_file_transfer_thread, "Log category for File Transfer Thread");

namespace wrench {
    /**
     * @brief Constructor
     * @param hostname: host on which to run
     * @param parent: the parent storage service
     * @param file: the file corresponding to the connection
     * @param num_bytes_to_transfer: number of bytes to transfer
     * @param src_commport: the a source commport to receive data from
     * @param dst_location: a location to write data to
     * @param dst_opened_file: an open file to write to
     * @param answer_commport_if_read: the commport to send an answer to in case this was a file read ("" if none). This
     *        will simply be reported to the parent service, who may use it as needed
     * @param answer_commport_if_write: the commport to send an answer to in case this was a file write ("" if none). This
     *        will simply be reported to the parent service, who may use it as needed
     * @param answer_commport_if_copy: the commport to send an answer to in case this was a file copy ("" if none). This
     *        will simply be reported to the parent service, who may use it as needed
     * @param buffer_size: the buffer size to use
     */
    FileTransferThread::FileTransferThread(const std::string& hostname,
                                           std::shared_ptr<StorageService> parent,
                                           std::shared_ptr<DataFile> file,
                                           sg_size_t num_bytes_to_transfer,
                                           S4U_CommPort *src_commport,
                                           std::shared_ptr<FileLocation> dst_location,
                                           std::shared_ptr<simgrid::fsmod::File> dst_opened_file,
                                           S4U_CommPort *answer_commport_if_read,
                                           S4U_CommPort *answer_commport_if_write,
                                           S4U_CommPort *answer_commport_if_copy,
                                           sg_size_t buffer_size) : Service(hostname, "file_transfer_thread"),
                                                                 parent(std::move(parent)),
                                                                 file(std::move(file)),
                                                                 num_bytes_to_transfer(num_bytes_to_transfer),
                                                                 answer_commport_if_read(answer_commport_if_read),
                                                                 answer_commport_if_write(answer_commport_if_write),
                                                                 answer_commport_if_copy(answer_commport_if_copy),
                                                                 buffer_size(buffer_size) {
        this->src_commport = src_commport;
        this->src_location = nullptr;
        this->dst_commport = nullptr;
        this->dst_location = std::move(dst_location);
        this->dst_opened_file = std::move(dst_opened_file);
    }

    /**
     * @brief Constructor
     * @param hostname: host on which to run
     * @param parent: the parent storage service
     * @param file: the file corresponding to the connection
     * @param num_bytes_to_transfer: number of bytes to transfer
     * @param src_location: a location to read data from
     * @param src_opened_file: a open file to read from
     * @param dst_commport: a commport to send data to
     * @param answer_commport_if_read: the commport to send an answer to in case this was a file read ("" if none). This
     *        will simply be reported to the parent service, who may use it as needed
     * @param answer_commport_if_write: the commport to send an answer to in case this was a file write ("" if none). This
     *        will simply be reported to the parent service, who may use it as needed
     * @param answer_commport_if_copy: the commport to send an answer to in case this was a file copy ("" if none). This
     *        will simply be reported to the parent service, who may use it as needed
     * @param buffer_size: the buffer size to use
     */
    FileTransferThread::FileTransferThread(const std::string& hostname,
                                           std::shared_ptr<StorageService> parent,
                                           std::shared_ptr<DataFile> file,
                                           sg_size_t num_bytes_to_transfer,
                                           std::shared_ptr<FileLocation> src_location,
                                           std::shared_ptr<simgrid::fsmod::File> src_opened_file,
                                           S4U_CommPort *dst_commport,
                                           S4U_CommPort *answer_commport_if_read,
                                           S4U_CommPort *answer_commport_if_write,
                                           S4U_CommPort *answer_commport_if_copy,
                                           sg_size_t buffer_size) : Service(hostname, "file_transfer_thread"),
                                                                 parent(std::move(parent)),
                                                                 file(std::move(file)),
                                                                 num_bytes_to_transfer(num_bytes_to_transfer),
                                                                 answer_commport_if_read(answer_commport_if_read),
                                                                 answer_commport_if_write(answer_commport_if_write),
                                                                 answer_commport_if_copy(answer_commport_if_copy),
                                                                 buffer_size(buffer_size) {
        this->src_commport = nullptr;
        this->src_location = std::move(src_location);
        this->src_opened_file = std::move(src_opened_file);
        this->dst_commport = dst_commport;
        this->dst_location = nullptr;
    }

    /**
     * @brief Constructor
     * @param hostname: host on which to run
     * @param parent: the parent storage service
     * @param file: the file corresponding to the connection
     * @param num_bytes_to_transfer: number of bytes to transfer
     * @param src_location: a location to read data from
     * @param src_opened_file: a open file to read from
     * @param dst_location: a location to send data to
     * @param dst_opened_file: a open file to write to
     * @param answer_commport_if_read: the commport to send an answer to in case this was a file read (nullptr if none). This
     *        will simply be reported to the parent service, who may use it as needed
     * @param answer_commport_if_write: the commport to send an answer to in case this was a file write (nullptr if none). This
     *        will simply be reported to the parent service, who may use it as needed
     * @param answer_commport_if_copy: the commport to send an answer to in case this was a file copy (nullptr if none). This
     *        will simply be reported to the parent service, who may use it as needed
     * @param buffer_size: the buffer size to use
     */
    FileTransferThread::FileTransferThread(const std::string& hostname,
                                           std::shared_ptr<StorageService> parent,
                                           std::shared_ptr<DataFile> file,
                                           sg_size_t num_bytes_to_transfer,
                                           std::shared_ptr<FileLocation> src_location,
                                           std::shared_ptr<simgrid::fsmod::File> src_opened_file,
                                           std::shared_ptr<FileLocation> dst_location,
                                           std::shared_ptr<simgrid::fsmod::File> dst_opened_file,
                                           S4U_CommPort *answer_commport_if_read,
                                           S4U_CommPort *answer_commport_if_write,
                                           S4U_CommPort *answer_commport_if_copy,
                                           sg_size_t buffer_size) : Service(hostname, "file_transfer_thread"),
                                                                 parent(std::move(parent)),
                                                                 file(std::move(file)),
                                                                 num_bytes_to_transfer(num_bytes_to_transfer),
                                                                 answer_commport_if_read(answer_commport_if_read),
                                                                 answer_commport_if_write(answer_commport_if_write),
                                                                 answer_commport_if_copy(answer_commport_if_copy),
                                                                 buffer_size(buffer_size) {
        this->src_commport = nullptr;
        this->src_location = std::move(src_location);
        this->src_opened_file = std::move(src_opened_file);
        this->dst_commport = nullptr;
        this->dst_opened_file = std::move(dst_opened_file);
        this->dst_location = std::move(dst_location);
    }

    /**
     * @brief Cleanup method
     *
     * @param has_returned_from_main: whether main() returned
     * @param return_value: the return value (if main() returned)
     */
    void FileTransferThread::cleanup(bool has_returned_from_main, int return_value) {
        //        this->release_held_mutexes();
        // Do nothing
        //        Service::cleanup(has_returned_from_main, return_value);
    }

    /**
     * @brief Main method
     * @return 0 on success, non-zero otherwise
     */
    int FileTransferThread::main() {
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_CYAN);

        FileTransferThreadNotificationMessage *msg_to_send_back = nullptr;
        std::shared_ptr<NetworkError> failure_cause = nullptr;

        WRENCH_INFO(
                "New FileTransferThread (file=%s, bytes_to_transfer=%llu, src_commport=%s; src_location=%s; dst_commport=%s; dst_location=%s; "
                "answer_commport_if_read=%s; answer_commport_if_write=%s; answer_commport_if_copy=%s; buffer size=%llu)",
                file->getID().c_str(),
                this->num_bytes_to_transfer,
                ((src_commport == nullptr) ? "none" : src_commport->get_cname()),
                (src_location == nullptr ? "none" : src_location->toString().c_str()),
                ((dst_commport == nullptr) ? "none" : dst_commport->get_cname()),
                (dst_location == nullptr ? "none" : dst_location->toString().c_str()),
                ((answer_commport_if_read == nullptr) ? "none" : answer_commport_if_read->get_cname()),
                ((answer_commport_if_write == nullptr) ? "none" : answer_commport_if_write->get_cname()),
                ((answer_commport_if_copy == nullptr) ? "none" : answer_commport_if_copy->get_cname()),
                this->buffer_size);

        // Create a message to send back (some field of which may be overwritten below)
        msg_to_send_back = new FileTransferThreadNotificationMessage(
                this->getSharedPtr<FileTransferThread>(),
                this->file,
                this->src_commport,
                this->src_location,
                this->dst_commport,
                this->dst_location,
                this->answer_commport_if_read,
                this->answer_commport_if_write,
                this->answer_commport_if_copy,
                true, nullptr);

        if ((this->src_location) and (this->src_location->getStorageService() == this->parent) and
            (dst_commport)) {
            /** Sending a local file to the network **/
            try {
                sendLocalFileToNetwork(this->file, this->src_location, num_bytes_to_transfer, this->dst_commport);
            } catch (ExecutionException &e) {
                WRENCH_INFO(
                        "FileTransferThread::main(): Network error (%s)", e.getCause()->toString().c_str());
                msg_to_send_back->success = false;
                msg_to_send_back->failure_cause = failure_cause;
            }

        } else if ((this->src_commport) and (this->dst_location) and
                   (this->dst_location->getStorageService() == this->parent)) {
            /** Receiving a file from the network **/
            try {
                receiveFileFromNetwork(this->file, this->src_commport, this->dst_location);
            } catch (ExecutionException &e) {
                WRENCH_INFO(
                        "FileTransferThread::main() Network error (%s)", e.getCause()->toString().c_str());
                msg_to_send_back->success = false;
                msg_to_send_back->failure_cause = failure_cause;
            }

        } else if ((this->src_location) and (this->src_location->getStorageService() == this->parent) and
                   (this->dst_location) and (this->dst_location->getStorageService() == this->parent)) {
            /** Copying a local file */
            copyFileLocally(this->file, this->src_location, this->dst_location);

        } else if (((this->src_location) and (this->dst_location) and
                    (this->dst_location->getStorageService() == this->parent))) {
            /** Downloading a file from another storage service */
            try {
                downloadFileFromStorageService(this->file, this->src_location, this->dst_location);
            } catch (ExecutionException &e) {
                msg_to_send_back->success = false;
                msg_to_send_back->failure_cause = e.getCause();
            } catch (std::shared_ptr<FailureCause> &failure_cause) {
                msg_to_send_back->success = false;
                msg_to_send_back->failure_cause = failure_cause;
            }
        } else {
            throw std::runtime_error("FileTransferThread::main(): Invalid src/dst combination (" +
                                     (src_location ? src_location->toString() : "nullptr") +
                                     "; " +
                                     (dst_location ? dst_location->toString() : "nullptr") + ")");
        }


        try {
            // Send report back to the service
            // (a dput() right before death is always dicey, so this is a put())
            this->parent->commport->putMessage(msg_to_send_back);
        } catch (ExecutionException &e) {
            // oh well...
        }

        // Call retire on all commports passed, which is pretty brute force but should work since
        // I synchronized with the parent!
        // BUT IT SHOULDN'T BE MY JOB!!!!
        //        if (this->dst_commport) {
        //            S4U_CommPort::retireTemporaryCommPort(this->dst_commport);
        //        }
        //        if (this->src_commport) {
        //            S4U_CommPort::retireTemporaryCommPort(this->src_commport);
        //        }
        return 0;
    }

    /**
    * @brief Method to received a file from the network onto the local disk
    * @param f: the file to receive
    * @param commport: the source commport
    * @param location: the destination location
    *
    */
    void FileTransferThread::receiveFileFromNetwork(const std::shared_ptr<DataFile> &f,
                                                    S4U_CommPort *commport,
                                                    const std::shared_ptr<FileLocation> &location) {
        /** Ideal Fluid model buffer size */
        if (this->buffer_size == 0) {
            throw std::runtime_error(
                    "FileTransferThread::receiveFileFromNetwork(): Zero buffer size not implemented yet");

        } else {
            /** Non-zero buffer size */
            bool done = false;

            // Receive the first chunk
            auto msg = commport->getMessage();
            if (auto file_content_chunk_msg = dynamic_cast<StorageServiceFileContentChunkMessage *>(msg.get())) {
                done = file_content_chunk_msg->last_chunk;
            } else {
                throw std::runtime_error("FileTransferThread::receiveFileFromNetwork() : Received an unexpected [" +
                                         msg->getName() + "] message!");
            }

            try {
#ifdef PAGE_CACHE_SIMULATION
                if (Simulation::isPageCachingEnabled()) {
                    simulation->getMemoryManagerByHost(location->getStorageService()->hostname)->log();
                }
#endif

                // Receive chunks and write them to disk
                while (not done) {
                    // Issue the receive
                    auto req = commport->igetMessage();

                    // In NFS, write to cache only if the current host not the server host where the f is stored
                    // If the current host is f server, write to disk directly
#ifdef PAGE_CACHE_SIMULATION
                    if (Simulation::isPageCachingEnabled()) {
                        bool write_locally = location->getServerStorageService() == nullptr;

                        if (write_locally) {
                            simulation->writebackWithMemoryCache(f, msg->payload, location, true);
                        } else {
                            simulation->writeThroughWithMemoryCache(f, msg->payload, location);
                        }
                    } else {
#endif
                    // Write to disk
                    auto dst_ss = std::dynamic_pointer_cast<SimpleStorageService>(dst_location->getStorageService());
                    if (!dst_ss) {
                        throw std::runtime_error("FileTransferThread::receiveFileFromNetwork(): Storage Service should be a SimpleStorageService for disk write");
                    }
                    int unique_disk_sequence_number_write = this->simulation_->getOutput().addTimestampDiskWriteStart(Simulation::getCurrentSimulatedDate(), hostname, dst_opened_file->get_path(), msg->payload);
                    this->dst_opened_file->write(msg->payload);
                    this->simulation_->getOutput().addTimestampDiskWriteCompletion(Simulation::getCurrentSimulatedDate(), hostname, dst_opened_file->get_path(), msg->payload, unique_disk_sequence_number_write);
//                        simulation->writeToDisk(msg->payload, location->getStorageService()->hostname,
//                                                dst_location->getDiskOrNull());
#ifdef PAGE_CACHE_SIMULATION
                    }
#endif

                    // Wait for the comm to finish
                    msg = req->wait();
                    if (auto file_content_chunk_msg =
                            dynamic_cast<StorageServiceFileContentChunkMessage *>(msg.get())) {
                        done = file_content_chunk_msg->last_chunk;
                    } else {
                        throw std::runtime_error(
                                "FileTransferThread::receiveFileFromNetwork() : Received an unexpected [" +
                                msg->getName() + "] message!");
                    }
                }

                // I/O for the last chunk
#ifdef PAGE_CACHE_SIMULATION
                if (Simulation::isPageCachingEnabled()) {
                    bool write_locally = location->getServerStorageService() == nullptr;

                    if (write_locally) {
                        simulation->writebackWithMemoryCache(f, msg->payload, location, true);
                    } else {
                        simulation->writeThroughWithMemoryCache(f, msg->payload, location);
                    }
                } else {
#endif
                // Write to disk
                auto ss = std::dynamic_pointer_cast<SimpleStorageService>(location->getStorageService());
                if (!ss) {
                    throw std::runtime_error("FileTransferThread::receiveFileFromNetwork(): Writing to disk can only be to a SimpleStorageService");
                }
//                    simulation->writeToDisk(msg->payload, ss->hostname, location->getDiskOrNull());
                int unique_disk_sequence_number_write = this->simulation_->getOutput().addTimestampDiskWriteStart(Simulation::getCurrentSimulatedDate(), hostname, this->dst_opened_file->get_path(), msg->payload);
                this->dst_opened_file->write(msg->payload);
                this->simulation_->getOutput().addTimestampDiskWriteCompletion(Simulation::getCurrentSimulatedDate(), hostname, this->dst_opened_file->get_path(), msg->payload, unique_disk_sequence_number_write);


#ifdef PAGE_CACHE_SIMULATION
                }
#endif

#ifdef PAGE_CACHE_SIMULATION
                if (Simulation::isPageCachingEnabled()) {
                    simulation->getMemoryManagerByHost(location->getStorageService()->hostname)->log();
                }
#endif
            } catch (ExecutionException &e) {
                throw;
            }
        }
    }

    /**
     * @brief Method to send a file from the local disk to the network
     * @param f: the file to send
     * @param location: the source location
     * @param num_bytes: number of bytes to transfer
     * @param commport: the destination commport
     *
     */
    void FileTransferThread::sendLocalFileToNetwork(const std::shared_ptr<DataFile> &f,
                                                    const std::shared_ptr<FileLocation> &location,
                                                    sg_size_t num_bytes,
                                                    S4U_CommPort *commport) {
        /** Ideal Fluid model buffer size */
        if (this->buffer_size == 0) {
            throw std::runtime_error(
                    "FileTransferThread::sendLocalFileToNetwork(): Zero buffer size not implemented yet");

        } else {
            try {
                /** Non-zero buffer size */
                std::shared_ptr<S4U_PendingCommunication> req = nullptr;
                // Sending a zero-byte f is really sending a 1-byte f
                sg_size_t remaining = std::max<sg_size_t>(1, num_bytes);


#ifdef PAGE_CACHE_SIMULATION
                if (Simulation::isPageCachingEnabled()) {
                    simulation->getMemoryManagerByHost(location->getStorageService()->hostname)->log();
                }
#endif

                while (remaining > 0) {
                    sg_size_t chunk_size = std::min<sg_size_t>(this->buffer_size, remaining);

#ifdef PAGE_CACHE_SIMULATION
                    if (Simulation::isPageCachingEnabled()) {
                        simulation->readWithMemoryCache(f, chunk_size, location);
                    } else {
#endif
                    WRENCH_INFO("Reading %s bytes from disk", std::to_string(chunk_size).c_str());
                    auto ss = std::dynamic_pointer_cast<SimpleStorageService>(location->getStorageService());
                    if (!ss) {
                        throw std::runtime_error("FileTransferThread::receiveFileFromNetwork(): Writing to disk can only be to a SimpleStorageService");
                    }
//                        simulation->readFromDisk(chunk_size, ss->hostname, location->getDiskOrNull());
                    int unique_disk_sequence_number_read = this->simulation_->getOutput().addTimestampDiskReadStart(Simulation::getCurrentSimulatedDate(), hostname, src_opened_file->get_path(), chunk_size);
                    this->src_opened_file->read(chunk_size);
                    this->simulation_->getOutput().addTimestampDiskReadCompletion(Simulation::getCurrentSimulatedDate(), hostname, src_opened_file->get_path(), chunk_size, unique_disk_sequence_number_read);
#ifdef PAGE_CACHE_SIMULATION
                    }
#endif

                    remaining -= chunk_size;
                    if (req) {
                        req->wait();
                        //                        WRENCH_INFO("Bytes sent over the network were received");
                    }
                    //                    WRENCH_INFO("Asynchronously sending %s bytes over the network", std::to_string(chunk_size).c_str());
                    req = commport->iputMessage(new StorageServiceFileContentChunkMessage(
                            this->file,
                            chunk_size, (remaining <= 0)));
                }
#ifdef PAGE_CACHE_SIMULATION
                if (Simulation::isPageCachingEnabled()) {
                    simulation->getMemoryManagerByHost(location->getStorageService()->hostname)->log();
                }
#endif
                req->wait();
                //                WRENCH_INFO("Bytes sent over the network were received");
            } catch (std::shared_ptr<NetworkError> &e) {
                throw;
            }
        }
    }

    /**
     * @brief Method to copy a file locally
     * @param f: the f to copy
     * @param src_loc: the source location
     * @param dst_loc: the destination location
     */
    void FileTransferThread::copyFileLocally(const std::shared_ptr<DataFile> &f,
                                             const std::shared_ptr<FileLocation> &src_loc,
                                             const std::shared_ptr<FileLocation> &dst_loc) {
        if ((src_loc->getStorageService() == dst_loc->getStorageService()) and
            (src_loc->getDirectoryPath() == dst_loc->getDirectoryPath())) {
            WRENCH_INFO(
                    "FileTransferThread::copyFileLocally(): Copying f %s onto itself at location %s... ignoring",
                    f->getID().c_str(), src_loc->toString().c_str());
            return;
        }

        auto src_ss = std::dynamic_pointer_cast<SimpleStorageService>(src_loc->getStorageService());
        if (!src_ss) {
            throw std::runtime_error("FileTransferThread::receiveFileFromNetwork(): Disk operation can only be to a SimpleStorageService");
        }
        auto dst_ss = std::dynamic_pointer_cast<SimpleStorageService>(dst_loc->getStorageService());
        if (!dst_ss) {
            throw std::runtime_error("FileTransferThread::receiveFileFromNetwork(): Disk operation can only be to a SimpleStorageService");
        }

        // Read the first chunk
        sg_size_t remaining = f->getSize();
        while (remaining > 0) {
            sg_size_t to_read = std::min<sg_size_t>(remaining, this->buffer_size);
            int unique_disk_sequence_number_read = this->simulation_->getOutput().addTimestampDiskReadStart(Simulation::getCurrentSimulatedDate(), hostname, src_opened_file->get_path(), to_read);
            src_opened_file->read(to_read);
            this->simulation_->getOutput().addTimestampDiskReadCompletion(Simulation::getCurrentSimulatedDate(), hostname, src_opened_file->get_path(), to_read, unique_disk_sequence_number_read);
            int unique_disk_sequence_number_write = this->simulation_->getOutput().addTimestampDiskWriteStart(Simulation::getCurrentSimulatedDate(), hostname, dst_opened_file->get_path(), to_read);
            dst_opened_file->write(to_read);
            this->simulation_->getOutput().addTimestampDiskReadCompletion(Simulation::getCurrentSimulatedDate(), hostname, dst_opened_file->get_path(), to_read, unique_disk_sequence_number_write);
            remaining -= to_read;
        }
    }

    /**
     * @brief Download a file to a local partition/disk
     * @param f: the f to download
     * @param src_loc: the source location
     * @param dst_loc: the destination location
     */
    void FileTransferThread::downloadFileFromStorageService(const std::shared_ptr<DataFile> &f,
                                                            const std::shared_ptr<FileLocation> &src_loc,
                                                            const std::shared_ptr<FileLocation> &dst_loc) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if (f == nullptr) {
            throw std::invalid_argument("StorageService::downloadFile(): Invalid arguments");
        }
#endif

        WRENCH_INFO("Downloading file  %s from location %s",
                    f->getID().c_str(), src_loc->toString().c_str());

        // Check that the buffer size is compatible
        if (((this->buffer_size == 0) && (src_loc->getStorageService()->getBufferSize() > 0)) or
            ((this->buffer_size > 0) && (src_loc->getStorageService()->getBufferSize() == 0))) {
            throw std::invalid_argument("FileTransferThread::downloadFileFromStorageService(): "
                                        "Incompatible buffer size specs (both must be zero, or both must be non-zero");
        }

        // Send a message to the source
        auto request_answer_commport = S4U_Daemon::getRunningActorRecvCommPort();

        src_loc->getStorageService()->commport->putMessage(
                new StorageServiceFileReadRequestMessage(
                        request_answer_commport,
                        simgrid::s4u::this_actor::get_host(),
                        src_loc,
                        f->getSize(),
                        src_loc->getStorageService()->getMessagePayloadValue(
                                StorageServiceMessagePayload::FILE_READ_REQUEST_MESSAGE_PAYLOAD)));

        // Wait for a reply to the request
        std::unique_ptr<SimulationMessage> message = nullptr;

        message = request_answer_commport->getMessage(this->network_timeout);

        S4U_CommPort *commport_to_receive_the_file_content;
        if (auto msg = dynamic_cast<StorageServiceFileReadAnswerMessage *>(message.get())) {
            // If it's not a success, throw an exception
            if (not msg->success) {
                throw ExecutionException(msg->failure_cause);
            }
            commport_to_receive_the_file_content = msg->commport_to_receive_the_file_content;
            WRENCH_INFO("Download request accepted (will receive file content on commport %s)",
                        commport_to_receive_the_file_content->get_cname());
        } else {
            throw std::runtime_error("FileTransferThread::downloadFileFromStorageService(): Received an unexpected [" +
                                     message->getName() + "] message!");
        }

        try {
            bool done = false;
            // Receive the first chunk
            auto msg = commport_to_receive_the_file_content->getMessage();
            if (auto file_content_chunk_msg =
                    dynamic_cast<StorageServiceFileContentChunkMessage *>(msg.get())) {
                done = file_content_chunk_msg->last_chunk;
            } else {
                throw std::runtime_error(
                        "FileTransferThread::downloadFileFromStorageService(): Received an unexpected [" +
                        msg->getName() + "] message!");
            }

            // Receive chunks and write them to disk
            auto dst_ss = std::dynamic_pointer_cast<SimpleStorageService>(dst_loc->getStorageService());
            if (!dst_ss) {
                throw std::runtime_error("FileTransferThread::receiveFileFromNetwork(): Storage Service should be a SimpleStorageService for disk operations");
            }
            while (not done) {
                // Issue the receive
                auto req = commport_to_receive_the_file_content->igetMessage();
                //                    WRENCH_INFO("Downloaded of %f of f  %s from location %s",
                //                                msg->payload, f->getID().c_str(), src_loc->toString().c_str());
                // Do the I/O
#ifdef PAGE_CACHE_SIMULATION
                if (Simulation::isPageCachingEnabled()) {
                    simulation->writebackWithMemoryCache(f, msg->payload, dst_loc, false);
                } else {
#endif
                // Write to disk
//                    simulation->writeToDisk(msg->payload,
//                                            dst_ss->getHostname(),
//                                            dst_loc->getDiskOrNull());
                int unique_disk_sequence_number_write = this->simulation_->getOutput().addTimestampDiskWriteStart(Simulation::getCurrentSimulatedDate(), hostname, dst_opened_file->get_path(), msg->payload);
                dst_opened_file->write(msg->payload);
                this->simulation_->getOutput().addTimestampDiskWriteCompletion(Simulation::getCurrentSimulatedDate(), hostname, dst_opened_file->get_path(), msg->payload, unique_disk_sequence_number_write);

#ifdef PAGE_CACHE_SIMULATION
                }
#endif
                // Wait for the comm to finish
                //                    WRENCH_INFO("Wrote of %f of f  %s", msg->payload, f->getID().c_str());
                msg = req->wait();
                if (auto file_content_chunk_msg =
                        dynamic_cast<StorageServiceFileContentChunkMessage *>(msg.get())) {
                    done = file_content_chunk_msg->last_chunk;
                } else {
                    throw std::runtime_error(
                            "FileTransferThread::downloadFileFromStorageService(): Received an unexpected [" +
                            msg->getName() + "] message!");
                }
            }

            // Do the I/O for the last chunk
#ifdef PAGE_CACHE_SIMULATION
            if (Simulation::isPageCachingEnabled()) {
                simulation->writebackWithMemoryCache(f, msg->payload, dst_loc, false);
            } else {
#endif
            // Write last chunk to disk
//                simulation->writeToDisk(msg->payload,
//                                        dst_ss->getHostname(),
//                                        dst_loc->getDiskOrNull());
            int unique_disk_sequence_number_write = this->simulation_->getOutput().addTimestampDiskWriteStart(Simulation::getCurrentSimulatedDate(), hostname, dst_opened_file->get_path(), msg->payload);
            dst_opened_file->write(msg->payload);
            this->simulation_->getOutput().addTimestampDiskWriteCompletion(Simulation::getCurrentSimulatedDate(), hostname, dst_opened_file->get_path(), msg->payload, unique_disk_sequence_number_write);

#ifdef PAGE_CACHE_SIMULATION
            }

            // Wait for final ack
            request_answer_commport->getMessage<StorageServiceAckMessage>(10, "StorageService::downloadFileFromStorageService(): Received an");
#endif
        } catch (ExecutionException &e) {
            throw;
        }
    }

}// namespace wrench
