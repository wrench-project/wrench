/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_FILETRANSFERTHREADMESSAGE_H
#define WRENCH_FILETRANSFERTHREADMESSAGE_H


#include <memory>

#include <wrench/services/ServiceMessage.h>
#include <wrench/failure_causes/FailureCause.h>
#include <wrench/services/file_registry/FileRegistryService.h>
#include <wrench/simulation/SimulationTimestampTypes.h>
#include <wrench/simulation/Simulation.h>
#include <wrench/simulation/SimulationOutput.h>
#include "wrench/services/storage/storage_helpers/FileTransferThread.h"

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief Top-level class for messages received/sent by a DataCommunicationThread
     */
    class FileTransferThreadMessage : public ServiceMessage {
    protected:
        /**
         * @brief Constructor
         * @param name: the message name
         * @param payload: the message payload
         */
        FileTransferThreadMessage(std::string name, double payload) :
                ServiceMessage("FileTransferThreadMessage::" + name, payload) {}
    };


    /**
     * @brief A message sent to by a FileTransferThread to report on success/failure of the transfer
     */
    class FileTransferThreadNotificationMessage : public FileTransferThreadMessage {
    public:
        /**
         * @brief Constructor
         *
         * @param file_transfer_thread: the FileTransferThread that sent this message
         * @param file: the file that was being transfered
         * @param src_mailbox: the source mailbox of the transfer (or "" if source wasn't a mailbox)
         * @param src_location: the source location of the transfer (or nullptr if source wasn't a location)
         * @param dst_mailbox: the destination mailbox of the transfer (or "" if source wasn't a mailbox)
         * @param dst_location: the destination location of the transfer (or nullptr if source wasn't a location)
         * @param answer_mailbox_if_read: the mailbox that a "read is done" may be sent to if necessary
         * @param answer_mailbox_if_write: the mailbox that a "write is done" may be sent to if necessary
         * @param answer_mailbox_if_copy: the mailbox that a "copy is done/failed" may be sent if necessary
         * @param success: whether the transfer succeeded
         * @param failure_cause: the failure cause (nullptr if success)
         */
        FileTransferThreadNotificationMessage(std::shared_ptr<FileTransferThread> file_transfer_thread,
                                              WorkflowFile *file,
                                              std::string src_mailbox,
                                              std::shared_ptr<FileLocation> src_location,
                                              std::string dst_mailbox,
                                              std::shared_ptr<FileLocation> dst_location,
                                              std::string answer_mailbox_if_read,
                                              std::string answer_mailbox_if_write,
                                              std::string answer_mailbox_if_copy,
                                              bool success, std::shared_ptr<FailureCause> failure_cause) :
                FileTransferThreadMessage("FileTransferThreadNotificationMessage", 0),
                file_transfer_thread(file_transfer_thread),
                file(file),
                src_mailbox(src_mailbox), src_location(src_location),
                dst_mailbox(dst_mailbox), dst_location(dst_location),
                answer_mailbox_if_read(answer_mailbox_if_read),
                answer_mailbox_if_write(answer_mailbox_if_write),
                answer_mailbox_if_copy(answer_mailbox_if_copy),
                success(success),
                failure_cause(failure_cause) {}

        /** @brief File transfer thread that sent this message */
        std::shared_ptr<FileTransferThread> file_transfer_thread;
        /** @brief File that was being communicated */
        WorkflowFile *file;

        /** @brief Source mailbox (or "" if source wasn't a mailbox) */
        std::string src_mailbox;
        /** @brief Source location (or nullptr if source wasn't a location) */
        std::shared_ptr<FileLocation> src_location;

        /** @brief Destination mailbox (or "" if destination wasn't a mailbox) */
        std::string dst_mailbox;
        /** @brief Destination location (or nullptr if source wasn't a location) */
        std::shared_ptr<FileLocation> dst_location;

        /** @brief If this was a file read, the mailbox to which an answer should be send */
        std::string answer_mailbox_if_read;
        /** @brief If this was a file write, the mailbox to which an answer should be send */
        std::string answer_mailbox_if_write;
        /** @brief If this was a file copy, the mailbox to which an answer should be send */
        std::string answer_mailbox_if_copy;
        /** @brief Whether the transfer succeeded or not */
        bool success;
        /** @brief The failure cause is case of a failure */
        std::shared_ptr<FailureCause> failure_cause;
    };


    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_FILETRANSFERTHREADMESSAGE_H
