/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>
#include <wrench/services/storage/StorageServiceMessage.h>
#include <wrench/services/storage/storage_helpers/FileTransferThread.h>
#include "wrench/services/storage/storage_helpers/FileTransferThreadMessage.h"

#include <wrench-dev.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <utility>
#include <wrench/services/memory/MemoryManager.h>

WRENCH_LOG_CATEGORY(wrench_core_file_transfer_thread, "Log category for File Transfer Thread");

namespace wrench {
    /**
     * @brief Constructor
     * @param hostname: host on which to run
     * @param parent: the parent storage service
     * @param file: the file corresponding to the connection
     * @param num_bytes_to_transfer: number of bytes to transfer
     * @param src_mailbox: the a source mailbox to receive data from
     * @param dst_location: a location to write data to
     * @param answer_mailbox_if_read: the mailbox to send an answer to in case this was a file read ("" if none). This
     *        will simply be reported to the parent service, who may use it as needed
     * @param answer_mailbox_if_write: the mailbox to send an answer to in case this was a file write ("" if none). This
     *        will simply be reported to the parent service, who may use it as needed
     * @param answer_mailbox_if_copy: the mailbox to send an answer to in case this was a file copy ("" if none). This
     *        will simply be reported to the parent service, who may use it as needed
     * @param buffer_size: the buffer size to use
     */
    FileTransferThread::FileTransferThread(std::string hostname,
                                           std::shared_ptr<StorageService> parent,
                                           std::shared_ptr<DataFile> file,
                                           double num_bytes_to_transfer,
                                           simgrid::s4u::Mailbox *src_mailbox,
                                           std::shared_ptr<FileLocation> dst_location,
                                           simgrid::s4u::Mailbox *answer_mailbox_if_read,
                                           simgrid::s4u::Mailbox *answer_mailbox_if_write,
                                           simgrid::s4u::Mailbox *answer_mailbox_if_copy,
                                           double buffer_size) : Service(std::move(hostname), "file_transfer_thread"),
                                                                 parent(std::move(parent)),
                                                                 file(std::move(file)),
                                                                 num_bytes_to_transfer(num_bytes_to_transfer),
                                                                 answer_mailbox_if_read(answer_mailbox_if_read),
                                                                 answer_mailbox_if_write(answer_mailbox_if_write),
                                                                 answer_mailbox_if_copy(answer_mailbox_if_copy),
                                                                 buffer_size(buffer_size) {
        this->src_mailbox = src_mailbox;
        this->src_location = nullptr;
        this->dst_mailbox = nullptr;
        this->dst_location = std::move(dst_location);
    }

    /**
     * @brief Constructor
     * @param hostname: host on which to run
     * @param parent: the parent storage service
     * @param file: the file corresponding to the connection
     * @param num_bytes_to_transfer: number of bytes to transfer
     * @param src_location: a location to read data from
     * @param dst_mailbox: a mailbox to send data to
     * @param answer_mailbox_if_read: the mailbox to send an answer to in case this was a file read ("" if none). This
     *        will simply be reported to the parent service, who may use it as needed
     * @param answer_mailbox_if_write: the mailbox to send an answer to in case this was a file write ("" if none). This
     *        will simply be reported to the parent service, who may use it as needed
     * @param answer_mailbox_if_copy: the mailbox to send an answer to in case this was a file copy ("" if none). This
     *        will simply be reported to the parent service, who may use it as needed
     * @param buffer_size: the buffer size to use
     */
    FileTransferThread::FileTransferThread(std::string hostname,
                                           std::shared_ptr<StorageService> parent,
                                           std::shared_ptr<DataFile> file,
                                           double num_bytes_to_transfer,
                                           std::shared_ptr<FileLocation> src_location,
                                           simgrid::s4u::Mailbox *dst_mailbox,
                                           simgrid::s4u::Mailbox *answer_mailbox_if_read,
                                           simgrid::s4u::Mailbox *answer_mailbox_if_write,
                                           simgrid::s4u::Mailbox *answer_mailbox_if_copy,
                                           double buffer_size) : Service(std::move(hostname), "file_transfer_thread"),
                                                                 parent(std::move(parent)),
                                                                 file(std::move(file)),
                                                                 num_bytes_to_transfer(num_bytes_to_transfer),
                                                                 answer_mailbox_if_read(answer_mailbox_if_read),
                                                                 answer_mailbox_if_write(answer_mailbox_if_write),
                                                                 answer_mailbox_if_copy(answer_mailbox_if_copy),
                                                                 buffer_size(buffer_size) {
        this->src_mailbox = nullptr;
        this->src_location = std::move(src_location);
        this->dst_mailbox = dst_mailbox;
        this->dst_location = nullptr;
    }

    /**
     * @brief Constructor
     * @param hostname: host on which to run
     * @param parent: the parent storage service
     * @param file: the file corresponding to the connection
     * @param num_bytes_to_transfer: number of bytes to transfer
     * @param src_location: a location to read data from
     * @param dst_location: a location to send data to
     * @param answer_mailbox_if_read: the mailbox to send an answer to in case this was a file read (nullptr if none). This
     *        will simply be reported to the parent service, who may use it as needed
     * @param answer_mailbox_if_write: the mailbox to send an answer to in case this was a file write (nullptr if none). This
     *        will simply be reported to the parent service, who may use it as needed
     * @param answer_mailbox_if_copy: the mailbox to send an answer to in case this was a file copy (nullptr if none). This
     *        will simply be reported to the parent service, who may use it as needed
     * @param buffer_size: the buffer size to use
     */
    FileTransferThread::FileTransferThread(std::string hostname,
                                           std::shared_ptr<StorageService> parent,
                                           std::shared_ptr<DataFile> file,
                                           double num_bytes_to_transfer,
                                           std::shared_ptr<FileLocation> src_location,
                                           std::shared_ptr<FileLocation> dst_location,
                                           simgrid::s4u::Mailbox *answer_mailbox_if_read,
                                           simgrid::s4u::Mailbox *answer_mailbox_if_write,
                                           simgrid::s4u::Mailbox *answer_mailbox_if_copy,
                                           double buffer_size) : Service(std::move(hostname), "file_transfer_thread"),
                                                                 parent(std::move(parent)),
                                                                 file(std::move(file)),
                                                                 num_bytes_to_transfer(num_bytes_to_transfer),
                                                                 answer_mailbox_if_read(answer_mailbox_if_read),
                                                                 answer_mailbox_if_write(answer_mailbox_if_write),
                                                                 answer_mailbox_if_copy(answer_mailbox_if_copy),
                                                                 buffer_size(buffer_size) {
        this->src_mailbox = nullptr;
        this->src_location = std::move(src_location);
        this->dst_mailbox = nullptr;
        this->dst_location = std::move(dst_location);
    }

    /**
     * @brief Cleanup method
     *
     * @param has_returned_from_main: whether main() returned
     * @param return_value: the return value (if main() returned)
     */
    void FileTransferThread::cleanup(bool has_returned_from_main, int return_value) {
        // Do nothing. It's fine to just die
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
                "New FileTransferThread (file=%s, bytes_to_transfer=%.2lf, src_mailbox=%s; src_location=%s; dst_mailbox=%s; dst_location=%s; "
                "answer_mailbox_if_read=%s; answer_mailbox_if_write=%s; answer_mailbox_if_copy=%s; buffer size=%.2lf",
                file->getID().c_str(),
                this->num_bytes_to_transfer,
                ((src_mailbox == nullptr) ? "none" : src_mailbox->get_cname()),
                (src_location == nullptr ? "none" : src_location->toString().c_str()),
                ((dst_mailbox == nullptr) ? "none" : dst_mailbox->get_cname()),
                (dst_location == nullptr ? "none" : dst_location->toString().c_str()),
                ((answer_mailbox_if_read == nullptr) ? "none" : answer_mailbox_if_read->get_cname()),
                ((answer_mailbox_if_write == nullptr) ? "none" : answer_mailbox_if_write->get_cname()),
                ((answer_mailbox_if_copy == nullptr) ? "none" : answer_mailbox_if_copy->get_cname()),
                this->buffer_size);

        // Create a message to send back (some field of which may be overwritten below)
        msg_to_send_back = new FileTransferThreadNotificationMessage(
                this->getSharedPtr<FileTransferThread>(),
                this->file,
                this->src_mailbox,
                this->src_location,
                this->dst_mailbox,
                this->dst_location,
                this->answer_mailbox_if_read,
                this->answer_mailbox_if_write,
                this->answer_mailbox_if_copy,
                true, nullptr);

        if ((this->src_location) and (this->src_location->getStorageService() == this->parent) and
            (dst_mailbox)) {
            /** Sending a local file to the network **/
            try {
                sendLocalFileToNetwork(this->file, this->src_location, num_bytes_to_transfer, this->dst_mailbox);
            } catch (ExecutionException &e) {
                WRENCH_INFO(
                        "FileTransferThread::main(): Network error (%s)", e.getCause()->toString().c_str());
                msg_to_send_back->success = false;
                msg_to_send_back->failure_cause = failure_cause;
            }

        } else if ((this->src_mailbox) and (this->dst_location) and
                   (this->dst_location->getStorageService() == this->parent)) {
            /** Receiving a file from the network **/
            try {
                receiveFileFromNetwork(this->file, this->src_mailbox, this->dst_location);
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

        // Call retire on all mailboxes passed, which is pretty brute force but should work
        if (this->dst_mailbox) {
            S4U_Mailbox::retireTemporaryMailbox(this->dst_mailbox);
        }
        if (this->src_mailbox) {
            S4U_Mailbox::retireTemporaryMailbox(this->src_mailbox);
        }

        try {
            // Send report back to the service
            // (a dput() right before death is always dicey, so this is a put())
            S4U_Mailbox::putMessage(this->parent->mailbox, msg_to_send_back);
        } catch (ExecutionException &e) {
            // oh well...
        }

        return 0;
    }

    /**
    * @brief Method to received a f from the network onto the local disk
    * @param f: the f
    * @param mailbox: the source mailbox
    * @param location: the destination location
    *
    * @throw shared_ptr<FailureCause>
    */
    void FileTransferThread::receiveFileFromNetwork(const std::shared_ptr<DataFile> &f,
                                                    simgrid::s4u::Mailbox *mailbox,
                                                    const std::shared_ptr<FileLocation> &location) {
        /** Ideal Fluid model buffer size */
        if (this->buffer_size < DBL_EPSILON) {
            throw std::runtime_error(
                    "FileTransferThread::receiveFileFromNetwork(): Zero buffer size not implemented yet");

        } else {
            /** Non-zero buffer size */
            bool done = false;

            // Receive the first chunk
            auto msg = S4U_Mailbox::getMessage(mailbox);
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
                    auto req = S4U_Mailbox::igetMessage(mailbox);

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
                        simulation->writeToDisk(msg->payload, location->getStorageService()->hostname,
                                                dst_ss->getPathMountPoint(location->getPath()), dst_location->getDiskOrNull());
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
                    simulation->writeToDisk(msg->payload, ss->hostname,
                                            ss->getPathMountPoint(location->getPath()), location->getDiskOrNull());
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
     * @brief Method to send a f from the local disk to the network
     * @param f: the f
     * @param location: the source location
     * @param num_bytes: number of bytes to transfer
     * @param mailbox: the destination mailbox
     *
     * @throw shared_ptr<FailureCause>
     */
    void FileTransferThread::sendLocalFileToNetwork(const std::shared_ptr<DataFile> &f,
                                                    const std::shared_ptr<FileLocation> &location,
                                                    double num_bytes,
                                                    simgrid::s4u::Mailbox *mailbox) {
        /** Ideal Fluid model buffer size */
        if (this->buffer_size < DBL_EPSILON) {
            throw std::runtime_error(
                    "FileTransferThread::sendLocalFileToNetwork(): Zero buffer size not implemented yet");

        } else {
            try {
                /** Non-zero buffer size */
                std::shared_ptr<S4U_PendingCommunication> req = nullptr;
                // Sending a zero-byte f is really sending a 1-byte f
                double remaining = std::max<double>(1, num_bytes);

#ifdef PAGE_CACHE_SIMULATION
                if (Simulation::isPageCachingEnabled()) {
                    simulation->getMemoryManagerByHost(location->getStorageService()->hostname)->log();
                }
#endif

                while (remaining > DBL_EPSILON) {
                    double chunk_size = std::min<double>(this->buffer_size, remaining);

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
                        simulation->readFromDisk(chunk_size, ss->hostname,
                                                 ss->getPathMountPoint(location->getPath()), location->getDiskOrNull());
#ifdef PAGE_CACHE_SIMULATION
                    }
#endif

                    remaining -= this->buffer_size;
                    if (req) {
                        req->wait();
                        //                        WRENCH_INFO("Bytes sent over the network were received");
                    }
                    //                    WRENCH_INFO("Asynchronously sending %s bytes over the network", std::to_string(chunk_size).c_str());
                    req = S4U_Mailbox::iputMessage(mailbox,
                                                   new StorageServiceFileContentChunkMessage(
                                                           this->file,
                                                           chunk_size, (remaining <= 0)));
                }
#ifdef PAGE_CACHE_SIMULATION
                if (Simulation::isPageCachingEnabled()) {
                    simulation->getMemoryManagerByHost(location->getStorageService()->hostname)->log();
                }
#endif
                req->wait();
                WRENCH_INFO("Bytes sent over the network were received");
            } catch (std::shared_ptr<NetworkError> &e) {
                throw;
            }
        }
    }

    /**
     * @brief Method to copy a f locally
     * @param f: the f to copy
     * @param src_loc: the source location
     * @param dst_loc: the destination location
     */
    void FileTransferThread::copyFileLocally(const std::shared_ptr<DataFile> &f,
                                             const std::shared_ptr<FileLocation> &src_loc,
                                             const std::shared_ptr<FileLocation> &dst_loc) {

        double remaining = f->getSize();
        double to_send = std::min<double>(this->buffer_size, remaining);

        if ((src_loc->getStorageService() == dst_loc->getStorageService()) and
            (src_loc->getPath() == dst_loc->getPath())) {
            WRENCH_INFO(
                    "FileTransferThread::copyFileLocally(): Copying f %s onto itself at location %s... ignoring",
                    f->getID().c_str(), src_loc->toString().c_str());
            return;
        }

        /** Ideal Fluid model buffer size */
        if (this->buffer_size < DBL_EPSILON) {
            throw std::runtime_error(
                    "FileTransferThread::copyFileLocally(): Zero buffer size not implemented yet");

        } else {
            auto src_ss = std::dynamic_pointer_cast<SimpleStorageService>(src_loc->getStorageService());
            if (!src_ss) {
                throw std::runtime_error("FileTransferThread::receiveFileFromNetwork(): Disk operation can only be to a SimpleStorageService");
            }
            auto src_mount_point = src_ss->getPathMountPoint(src_loc->getPath());
            auto dst_ss = std::dynamic_pointer_cast<SimpleStorageService>(dst_loc->getStorageService());
            if (!dst_ss) {
                throw std::runtime_error("FileTransferThread::receiveFileFromNetwork(): Disk operation can only be to a SimpleStorageService");
            }
            auto dst_mount_point = dst_ss->getPathMountPoint(dst_loc->getPath());

            // Read the first chunk
            simulation->readFromDisk(to_send, src_ss->hostname, src_mount_point, src_loc->getDiskOrNull());
            // start the pipeline
            while (remaining - this->buffer_size > DBL_EPSILON) {
                simulation->readFromDiskAndWriteToDiskConcurrently(
                        this->buffer_size, this->buffer_size, src_loc->getStorageService()->hostname,
                        src_mount_point, dst_mount_point, src_loc->getDiskOrNull(), dst_loc->getDiskOrNull());

                remaining -= this->buffer_size;
            }
            // Write the last chunk
            simulation->writeToDisk(remaining, dst_loc->getStorageService()->hostname,
                                    dst_mount_point, dst_loc->getDiskOrNull());
        }
    }

    /**
     * @brief Download a f to a local partition/disk
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
        if (((this->buffer_size < DBL_EPSILON) && (src_loc->getStorageService()->getBufferSize() > DBL_EPSILON)) or
            ((this->buffer_size > DBL_EPSILON) && (src_loc->getStorageService()->getBufferSize() < DBL_EPSILON))) {
            throw std::invalid_argument("FileTransferThread::downloadFileFromStorageService(): "
                                        "Incompatible buffer size specs (both must be zero, or both must be non-zero");
        }

        // Send a message to the source
        auto request_answer_mailbox = S4U_Daemon::getRunningActorRecvMailbox();
        //        auto mailbox_that_should_receive_file_content = S4U_Mailbox::generateUniqueMailbox("works_by_itself");

        //        simgrid::s4u::Mailbox *mailbox_that_should_receive_file_content;
        //        if (src_loc->getStorageService()->buffer_size > DBL_EPSILON) {
        //            mailbox_that_should_receive_file_content = S4U_Mailbox::getTemporaryMailbox();
        //        } else {
        //            mailbox_that_should_receive_file_content = nullptr;
        //        }

        S4U_Mailbox::putMessage(
                src_loc->getStorageService()->mailbox,
                new StorageServiceFileReadRequestMessage(
                        request_answer_mailbox,
                        simgrid::s4u::this_actor::get_host(),
                        src_loc,
                        f->getSize(),
                        src_loc->getStorageService()->getMessagePayloadValue(
                                StorageServiceMessagePayload::FILE_READ_REQUEST_MESSAGE_PAYLOAD)));

        // Wait for a reply to the request
        std::unique_ptr<SimulationMessage> message = nullptr;

        message = S4U_Mailbox::getMessage(request_answer_mailbox, this->network_timeout);

        simgrid::s4u::Mailbox *mailbox_to_receive_the_file_content;
        if (auto msg = dynamic_cast<StorageServiceFileReadAnswerMessage *>(message.get())) {
            // If it's not a success, throw an exception
            if (not msg->success) {
                throw ExecutionException(msg->failure_cause);
            }
            mailbox_to_receive_the_file_content = msg->mailbox_to_receive_the_file_content;
            WRENCH_INFO("Download request accepted (will receive file content on mailbox_name %s)",
                        mailbox_to_receive_the_file_content->get_cname());
        } else {
            throw std::runtime_error("FileTransferThread::downloadFileFromStorageService(): Received an unexpected [" +
                                     message->getName() + "] message!");
        }


        try {
            bool done = false;
            // Receive the first chunk
            auto msg = S4U_Mailbox::getMessage(mailbox_to_receive_the_file_content);
            if (auto file_content_chunk_msg =
                        dynamic_cast<StorageServiceFileContentChunkMessage *>(msg.get())) {
                done = file_content_chunk_msg->last_chunk;
            } else {
                S4U_Mailbox::retireTemporaryMailbox(mailbox_to_receive_the_file_content);
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
                auto req = S4U_Mailbox::igetMessage(mailbox_to_receive_the_file_content);
                //                    WRENCH_INFO("Downloaded of %f of f  %s from location %s",
                //                                msg->payload, f->getID().c_str(), src_loc->toString().c_str());
                // Do the I/O
#ifdef PAGE_CACHE_SIMULATION
                if (Simulation::isPageCachingEnabled()) {
                    simulation->writebackWithMemoryCache(f, msg->payload, dst_loc, false);
                } else {
#endif
                    // Write to disk
                    simulation->writeToDisk(msg->payload,
                                            dst_ss->getHostname(),
                                            dst_ss->getPathMountPoint(dst_loc->getPath()), dst_loc->getDiskOrNull());
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
                    S4U_Mailbox::retireTemporaryMailbox(mailbox_to_receive_the_file_content);
                    throw std::runtime_error(
                            "FileTransferThread::downloadFileFromStorageService(): Received an unexpected [" +
                            msg->getName() + "] message!");
                }
            }
            S4U_Mailbox::retireTemporaryMailbox(mailbox_to_receive_the_file_content);

            // Do the I/O for the last chunk
#ifdef PAGE_CACHE_SIMULATION
            if (Simulation::isPageCachingEnabled()) {
                simulation->writebackWithMemoryCache(f, msg->payload, dst_loc, false);
            } else {
#endif
                // Write to disk
                simulation->writeToDisk(msg->payload,
                                        dst_ss->getHostname(),
                                        dst_ss->getPathMountPoint(dst_loc->getPath()), dst_loc->getDiskOrNull());
#ifdef PAGE_CACHE_SIMULATION
            }
#endif
        } catch (ExecutionException &e) {
            S4U_Mailbox::retireTemporaryMailbox(mailbox_to_receive_the_file_content);
            throw;
        }
    }

}// namespace wrench
