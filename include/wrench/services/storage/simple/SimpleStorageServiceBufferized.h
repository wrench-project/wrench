/**
 * Copyright (c) 2017-2020. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_SIMPLESTORAGESERVICEBUFFERIZED_H
#define WRENCH_SIMPLESTORAGESERVICEBUFFERIZED_H

#include "wrench/services/storage/storage_helpers/FileTransferThread.h"
#include "SimpleStorageServiceProperty.h"
#include "SimpleStorageServiceMessagePayload.h"
#include "wrench/services/memory/MemoryManager.h"
#include "wrench/simgrid_S4U_util/S4U_PendingCommunication.h"
#include "wrench/services/storage/simple/SimpleStorageService.h"
#include "SimpleStorageService.h"

namespace wrench {

    class SimulationMessage;

    class SimulationTimestampFileCopyStart;

    class S4U_PendingCommunication;

    /**
     * @brief The bufferized (i.e., BUFFER_SIZE > 0) implementation
     */
    class SimpleStorageServiceBufferized : public SimpleStorageService {

    private:
        WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE default_messagepayload_values = {
                {SimpleStorageServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {SimpleStorageServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {SimpleStorageServiceMessagePayload::FREE_SPACE_REQUEST_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {SimpleStorageServiceMessagePayload::FREE_SPACE_ANSWER_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {SimpleStorageServiceMessagePayload::FILE_DELETE_REQUEST_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {SimpleStorageServiceMessagePayload::FILE_DELETE_ANSWER_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {SimpleStorageServiceMessagePayload::FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {SimpleStorageServiceMessagePayload::FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {SimpleStorageServiceMessagePayload::FILE_COPY_REQUEST_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {SimpleStorageServiceMessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {SimpleStorageServiceMessagePayload::FILE_WRITE_REQUEST_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {SimpleStorageServiceMessagePayload::FILE_WRITE_ANSWER_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {SimpleStorageServiceMessagePayload::FILE_READ_REQUEST_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {SimpleStorageServiceMessagePayload::FILE_READ_ANSWER_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
        };

        friend class SimpleStorageService;

        // Public Constructor
        SimpleStorageServiceBufferized(const std::string &hostname,
                                       std::set<std::string> mount_points,
                                       WRENCH_PROPERTY_COLLECTION_TYPE property_list = {},
                                       WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list = {});


    public:
        double getLoad() override;

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/
        void cleanup(bool has_returned_from_main, int return_value) override;
        double countRunningFileTransferThreads();

        /***********************/
        /** \endcond          **/
        /***********************/

    private:
        friend class Simulation;

        int main() override;

        bool processNextMessage();

        bool processFileWriteRequest(std::shared_ptr<FileLocation> &location,
                                     double num_bytes_to_write,
                                     S4U_CommPort *answer_commport);

        bool
        processFileReadRequest(const std::shared_ptr<FileLocation> &location,
                               double num_bytes_to_read, S4U_CommPort *answer_commport);

        bool processFileCopyRequest(
                std::shared_ptr<FileLocation> &src,
                std::shared_ptr<FileLocation> &dst,
                S4U_CommPort *answer_commport);

        bool processFileTransferThreadNotification(
                const std::shared_ptr<FileTransferThread> &ftt,
                S4U_CommPort *src_commport,
                const std::shared_ptr<FileLocation> &src_location,
                S4U_CommPort *dst_commportx,
                const std::shared_ptr<FileLocation> &dst_location,
                bool success,
                std::shared_ptr<FailureCause> failure_cause,
                S4U_CommPort *answer_commport_if_read,
                S4U_CommPort *answer_commport_if_write,
                S4U_CommPort *answer_commport_if_copy);

        void startPendingFileTransferThread();

        std::deque<std::shared_ptr<FileTransferThread>> pending_file_transfer_threads;
        std::set<std::shared_ptr<FileTransferThread>> running_file_transfer_threads;

        std::map<std::shared_ptr<FileTransferThread>, S4U_CommPort *> ongoing_tmp_commports;

        std::shared_ptr<MemoryManager> memory_manager;
    };

}// namespace wrench

#endif//WRENCH_SIMPLESTORAGESERVICEBUFFERIZED_H
