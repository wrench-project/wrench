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

#include "wrench/services/ServiceMessage.h"
#include "wrench/failure_causes/FailureCause.h"
#include "wrench/services/file_registry/FileRegistryService.h"
#include "wrench/simulation/SimulationTimestampTypes.h"
#include "wrench/simulation/Simulation.h"
#include "wrench/simulation/SimulationOutput.h"
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
         * @param payload: the message payload
         */
        FileTransferThreadMessage(sg_size_t payload) : ServiceMessage(payload) {}
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
         * @param src_commport: the source commport_name of the transfer (or "" if source wasn't a commport_name)
         * @param src_location: the source location of the transfer (or nullptr if source wasn't a location)
         * @param dst_commport: the destination commport_name of the transfer (or "" if source wasn't a commport_name)
         * @param dst_location: the destination location of the transfer (or nullptr if source wasn't a location)
         * @param answer_commport_if_read: the commport_name that a "read is done" may be sent to if necessary
         * @param answer_commport_if_write: the commport_name that a "write is done" may be sent to if necessary
         * @param answer_commport_if_copy: the commport_name that a "copy is done/failed" may be sent if necessary
         * @param success: whether the transfer succeeded
         * @param failure_cause: the failure cause (nullptr if success)
         */
        FileTransferThreadNotificationMessage(std::shared_ptr<FileTransferThread> file_transfer_thread,
                                              std::shared_ptr<DataFile> file,
                                              S4U_CommPort *src_commport,
                                              std::shared_ptr<FileLocation> src_location,
                                              S4U_CommPort *dst_commport,
                                              std::shared_ptr<FileLocation> dst_location,
                                              S4U_CommPort *answer_commport_if_read,
                                              S4U_CommPort *answer_commport_if_write,
                                              S4U_CommPort *answer_commport_if_copy,
                                              bool success, std::shared_ptr<FailureCause> failure_cause) : FileTransferThreadMessage(0),
                                                                                                           file_transfer_thread(file_transfer_thread),
                                                                                                           file(file),
                                                                                                           src_commport(src_commport), src_location(src_location),
                                                                                                           dst_commport(dst_commport), dst_location(dst_location),
                                                                                                           answer_commport_if_read(answer_commport_if_read),
                                                                                                           answer_commport_if_write(answer_commport_if_write),
                                                                                                           answer_commport_if_copy(answer_commport_if_copy),
                                                                                                           success(success),
                                                                                                           failure_cause(failure_cause) {}

        /** @brief File transfer thread that sent this message */
        std::shared_ptr<FileTransferThread> file_transfer_thread;
        /** @brief File that was being communicated */
        std::shared_ptr<DataFile> file;

        /** @brief Source commport_name (or "" if source wasn't a commport_name) */
        S4U_CommPort *src_commport;
        /** @brief Source location (or nullptr if source wasn't a location) */
        std::shared_ptr<FileLocation> src_location;

        /** @brief Destination commport_name (or "" if destination wasn't a commport_name) */
        S4U_CommPort *dst_commport;
        /** @brief Destination location (or nullptr if source wasn't a location) */
        std::shared_ptr<FileLocation> dst_location;

        /** @brief If this was a file read, the commport_name to which an answer should be send */
        S4U_CommPort *answer_commport_if_read;
        /** @brief If this was a file write, the commport_name to which an answer should be send */
        S4U_CommPort *answer_commport_if_write;
        /** @brief If this was a file copy, the commport_name to which an answer should be send */
        S4U_CommPort *answer_commport_if_copy;
        /** @brief Whether the transfer succeeded or not */
        bool success;
        /** @brief The failure cause is case of a failure */
        std::shared_ptr<FailureCause> failure_cause;
    };


    /***********************/
    /** \endcond           */
    /***********************/

}// namespace wrench


#endif//WRENCH_FILETRANSFERTHREADMESSAGE_H
