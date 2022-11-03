/**
 * Copyright (c) 2017-2020. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_SIMPLESTORAGESERVICENONBUFFERIZED_H
#define WRENCH_SIMPLESTORAGESERVICENONBUFFERIZED_H

#include "wrench/services/storage/storage_helpers/FileTransferThread.h"
#include "wrench/services/storage/simple/SimpleStorageService.h"
#include "SimpleStorageServiceProperty.h"
#include "SimpleStorageServiceMessagePayload.h"
#include "wrench/services/memory/MemoryManager.h"
#include "wrench/simgrid_S4U_util/S4U_PendingCommunication.h"

namespace wrench {

    class SimulationMessage;

    class SimulationTimestampFileCopyStart;

    class S4U_PendingCommunication;

    /**
     * @brief The non-bufferized (i.e., BUFFER_SIZE == 0) implementation
     */
    class SimpleStorageServiceNonBufferized : public SimpleStorageService {

//    private:
//        WRENCH_PROPERTY_COLLECTION_TYPE default_property_values = {
//                {SimpleStorageServiceProperty::MAX_NUM_CONCURRENT_DATA_CONNECTIONS, "infinity"},
//                {SimpleStorageServiceProperty::BUFFER_SIZE, "10485760"},// 10 MEGA BYTE
//        };
//
//        WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE default_messagepayload_values = {
//                {SimpleStorageServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD, 1024},
//                {SimpleStorageServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD, 1024},
//                {SimpleStorageServiceMessagePayload::FREE_SPACE_REQUEST_MESSAGE_PAYLOAD, 1024},
//                {SimpleStorageServiceMessagePayload::FREE_SPACE_ANSWER_MESSAGE_PAYLOAD, 1024},
//                {SimpleStorageServiceMessagePayload::FILE_DELETE_REQUEST_MESSAGE_PAYLOAD, 1024},
//                {SimpleStorageServiceMessagePayload::FILE_DELETE_ANSWER_MESSAGE_PAYLOAD, 1024},
//                {SimpleStorageServiceMessagePayload::FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD, 1024},
//                {SimpleStorageServiceMessagePayload::FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD, 1024},
//                {SimpleStorageServiceMessagePayload::FILE_COPY_REQUEST_MESSAGE_PAYLOAD, 1024},
//                {SimpleStorageServiceMessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD, 1024},
//                {SimpleStorageServiceMessagePayload::FILE_WRITE_REQUEST_MESSAGE_PAYLOAD, 1024},
//                {SimpleStorageServiceMessagePayload::FILE_WRITE_ANSWER_MESSAGE_PAYLOAD, 1024},
//                {SimpleStorageServiceMessagePayload::FILE_READ_REQUEST_MESSAGE_PAYLOAD, 1024},
//                {SimpleStorageServiceMessagePayload::FILE_READ_ANSWER_MESSAGE_PAYLOAD, 1024},
//        };

    private:
        friend class SimpleStorageService;

        // Public Constructor
        SimpleStorageServiceNonBufferized(const std::string &hostname,
                             std::set<std::string> mount_points,
                             WRENCH_PROPERTY_COLLECTION_TYPE property_list = {},
                             WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list = {});
    public:

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/

//        ~SimpleStorageServiceNonBufferized() override;

        void cleanup(bool has_returned_from_main, int return_value) override;
        double getLoad() override;
//        double countRunningFileTransferThreads();

//        double getFileLastWriteDate(const std::shared_ptr<DataFile> &file, const std::shared_ptr<FileLocation> &location);

        /***********************/
        /** \endcond          **/
        /***********************/

    private:
        friend class Simulation;

//        // Low-level Constructor
//        SimpleStorageServiceNonBufferized(const std::string &hostname,
//                             std::set<std::string> mount_points,
//                             WRENCH_PROPERTY_COLLECTION_TYPE property_list,
//                             WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list,
//                             const std::string &suffix);

        int main() override;

        bool processNextMessage();

//        static unsigned long getNewUniqueNumber();

//        bool processFileDeleteRequest(const std::shared_ptr<DataFile> &file, const std::shared_ptr<FileLocation> &location,
//                                      simgrid::s4u::Mailbox *answer_mailbox);

        bool processFileWriteRequest(const std::shared_ptr<DataFile> &file, const std::shared_ptr<FileLocation> &, simgrid::s4u::Mailbox *answer_mailbox,
                                     double buffer_size);

        bool
        processFileReadRequest(const std::shared_ptr<DataFile> &file, const std::shared_ptr<FileLocation> &location,
                               double num_bytes_to_read, simgrid::s4u::Mailbox *answer_mailbox,
                               simgrid::s4u::Mailbox *mailbox_to_receive_the_file_content);

        bool processFileCopyRequest(const std::shared_ptr<DataFile> &file,
                                    const std::shared_ptr<FileLocation> &src,
                                    const std::shared_ptr<FileLocation> &dst,
                                    simgrid::s4u::Mailbox *answer_mailbox);

        bool processFileTransferThreadNotification(
                const std::shared_ptr<FileTransferThread> &ftt,
                const std::shared_ptr<DataFile> &file,
                simgrid::s4u::Mailbox *src_mailbox,
                const std::shared_ptr<FileLocation> &src_location,
                simgrid::s4u::Mailbox *dst_mailbox,
                const std::shared_ptr<FileLocation> &dst_location,
                bool success,
                std::shared_ptr<FailureCause> failure_cause,
                simgrid::s4u::Mailbox *answer_mailbox_if_read,
                simgrid::s4u::Mailbox *answer_mailbox_if_write,
                simgrid::s4u::Mailbox *answer_mailbox_if_copy);

//        unsigned long num_concurrent_connections;

        void startPendingFileTransferThread();

        std::deque<std::shared_ptr<FileTransferThread>> pending_file_transfer_threads;
        std::set<std::shared_ptr<FileTransferThread>> running_file_transfer_threads;

        void validateProperties();

        std::shared_ptr<MemoryManager> memory_manager;
    };

};// namespace wrench

#endif//WRENCH_SIMPLESTORAGESERVICE_H
