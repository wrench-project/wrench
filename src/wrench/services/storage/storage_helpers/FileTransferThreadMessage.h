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
#include <wrench/workflow/execution_events/FailureCause.h>
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
         * @param src: the source of the transfer
         * @param dst: the destination of the transfer
         * @param answer_mailbox_if_copy: the mailbox that a "copy is done/failed" may be sent if necessary
         * @param success: whether the transfer succeeded
         * @param failure_cause: the failure cause (nullptr if success)
         * @param start_time_stamp: the start time stamp
         */
        FileTransferThreadNotificationMessage(std::shared_ptr<FileTransferThread> file_transfer_thread,
                                                   WorkflowFile *file,
                                                   std::pair<FileTransferThread::LocationType, std::string> src,
                                                   std::pair<FileTransferThread::LocationType, std::string> dst,
                                                   std::string answer_mailbox_if_copy,
                                                   bool success, std::shared_ptr<FailureCause> failure_cause,
                                                   SimulationTimestampFileCopyStart *start_time_stamp) :
                FileTransferThreadMessage("FileTransferThreadNotificationMessage", 0),
                file_transfer_thread(file_transfer_thread),
                file(file), src(src), dst(dst),
                answer_mailbox_if_copy(answer_mailbox_if_copy), success(success),
                failure_cause(failure_cause), start_time_stamp(start_time_stamp) {}

        /** @brief File transfer thread that sent this message */
        std::shared_ptr<FileTransferThread> file_transfer_thread;
        /** @brief File that was being communicated */
        WorkflowFile *file;
        /** @brief Source */
        std::pair<FileTransferThread::LocationType, std::string> src;
        /** @brief Destination */
        std::pair<FileTransferThread::LocationType, std::string> dst;
        /** @brief If this was a file copy, the mailbox to which an answer should be send */
        std::string answer_mailbox_if_copy;
        /** @brief Whether the transfer succeeded or not */
        bool success;
        /** @brief The failure cause is case of a failure */
        std::shared_ptr<FailureCause> failure_cause;
        /** @brief A start time stamp */
        SimulationTimestampFileCopyStart *start_time_stamp;
    };


    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_FILETRANSFERTHREADMESSAGE_H
