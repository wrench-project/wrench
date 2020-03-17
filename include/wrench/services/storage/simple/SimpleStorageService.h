/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_SIMPLESTORAGESERVICE_H
#define WRENCH_SIMPLESTORAGESERVICE_H


#include <wrench/services/storage/storage_helpers/FileTransferThread.h>
#include "wrench/services/storage/StorageService.h"
#include "SimpleStorageServiceProperty.h"
#include "SimpleStorageServiceMessagePayload.h"
#include "wrench/simgrid_S4U_util/S4U_PendingCommunication.h"

namespace wrench {

    class SimulationMessage;
    class SimulationTimestampFileCopyStart;

    class S4U_PendingCommunication;

    /**
     * @brief A storage service that provides direct access to some storage resources (e.g., one or more disks).
     *        An important (configurable) property of the storage service is
     *        SimpleStorageServiceProperty::BUFFER_SIZE (see documentation thereof), which defines the
     *        buffer size that the storage service uses. More specifically, when the storage service
     *        receives / sends data from / to the network, it does so in a loop over data "chunks",
     *        with pipelined network and disk I/O operations. The smaller the buffer size the more "fluid"
     *        the model, but the more time-consuming the simulation. A large buffer size, however, may
     *        lead to less realistic simulations. At the extreme, an infinite buffer size would correspond
     *        to fully sequential executions (first a network receive/send, and then a disk write/read).
     *        Setting the buffer size to "0" corresponds to a fully fluid model in which individual
     *        data chunk operations are not simulated, thus achieving both accuracy (unless one specifically wishes
     *        to study the effects of buffering) and quick simulation times. For now, setting the buffer
     *        size to "0" is not implemented. The default buffer size is 1 MiB (note that the user can
     *        always declare a disk with arbitrary bandwidth in the platform description XML).
     */
    class SimpleStorageService : public StorageService {

    private:
        std::map<std::string, std::string> default_property_values = {
                 {SimpleStorageServiceProperty::MAX_NUM_CONCURRENT_DATA_CONNECTIONS,  "infinity"},
                 {SimpleStorageServiceProperty::BUFFER_SIZE,  "1048576"}, // 1 MEGA BYTE
                };

        std::map<std::string, double> default_messagepayload_values = {
                 {SimpleStorageServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD,         1024},
                 {SimpleStorageServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD,      1024},
                 {SimpleStorageServiceMessagePayload::FREE_SPACE_REQUEST_MESSAGE_PAYLOAD,  1024},
                 {SimpleStorageServiceMessagePayload::FREE_SPACE_ANSWER_MESSAGE_PAYLOAD,   1024},
                 {SimpleStorageServiceMessagePayload::FILE_DELETE_REQUEST_MESSAGE_PAYLOAD, 1024},
                 {SimpleStorageServiceMessagePayload::FILE_DELETE_ANSWER_MESSAGE_PAYLOAD,  1024},
                 {SimpleStorageServiceMessagePayload::FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD, 1024},
                 {SimpleStorageServiceMessagePayload::FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD,  1024},
                 {SimpleStorageServiceMessagePayload::FILE_COPY_REQUEST_MESSAGE_PAYLOAD,   1024},
                 {SimpleStorageServiceMessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD,    1024},
                 {SimpleStorageServiceMessagePayload::FILE_WRITE_REQUEST_MESSAGE_PAYLOAD,  1024},
                 {SimpleStorageServiceMessagePayload::FILE_WRITE_ANSWER_MESSAGE_PAYLOAD,   1024},
                 {SimpleStorageServiceMessagePayload::FILE_READ_REQUEST_MESSAGE_PAYLOAD,   1024},
                 {SimpleStorageServiceMessagePayload::FILE_READ_ANSWER_MESSAGE_PAYLOAD,    1024},
                };

    public:

        // Public Constructor
        SimpleStorageService(std::string hostname,
                             std::set<std::string> mount_points,
                             std::map<std::string, std::string> property_list = {},
                             std::map<std::string, double> messagepayload_list = {});

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/

        ~SimpleStorageService() override;
        void cleanup(bool has_returned_from_main, int return_value) override;


        /***********************/
        /** \endcond          **/
        /***********************/

    private:

        friend class Simulation;

        // Low-level Constructor
        SimpleStorageService(std::string hostname,
                             std::set<std::string> mount_points,
                             std::map<std::string, std::string> property_list,
                             std::map<std::string, double> messagepayload_list,
                             std::string suffix);

        int main() override;

        bool processNextMessage();

        unsigned long getNewUniqueNumber();

        bool processFileDeleteRequest(WorkflowFile *file, std::shared_ptr<FileLocation> location, std::string answer_mailbox);

        bool processFileWriteRequest(WorkflowFile *file, std::shared_ptr<FileLocation>, std::string answer_mailbox, unsigned long buffer_size);

        bool processFileReadRequest(WorkflowFile *file, std::shared_ptr<FileLocation> location, std::string answer_mailbox,
                                    std::string mailbox_to_receive_the_file_content, unsigned long buffer_size);

        bool processFileCopyRequest(WorkflowFile *file,
                std::shared_ptr<FileLocation> src,
                std::shared_ptr<FileLocation> dst,
                std::string answer_mailbox);

        bool processFileTransferThreadNotification(
                std::shared_ptr<FileTransferThread> ftt,
                WorkflowFile *file,
                std::string src_mailbox,
                std::shared_ptr<FileLocation> src_location,
                std::string dst_mailbox,
                std::shared_ptr<FileLocation> dst_location,
                bool success,
                std::shared_ptr<FailureCause> failure_cause,
                std::string answer_mailbox_if_read,
                std::string answer_mailbox_if_write,
                std::string answer_mailbox_if_copy);

        unsigned long num_concurrent_connections;

        void startPendingFileTransferThread();

        std::deque<std::shared_ptr<FileTransferThread>> pending_file_transfer_threads;
        std::set<std::shared_ptr<FileTransferThread>> running_file_transfer_threads;

        void validateProperties();

    };

};


#endif //WRENCH_SIMPLESTORAGESERVICE_H
