/**
 * Copyright (c) 2017-2020. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_SIMPLESTORAGESERVICE_H
#define WRENCH_SIMPLESTORAGESERVICE_H

#include "wrench/services/storage/storage_helpers/FileTransferThread.h"
#include "wrench/services/storage/StorageService.h"
#include "SimpleStorageServiceProperty.h"
#include "SimpleStorageServiceMessagePayload.h"
#include "wrench/services/memory/MemoryManager.h"
#include "wrench/simgrid_S4U_util/S4U_PendingCommunication.h"

namespace wrench {

    class SimulationMessage;

    class SimulationTimestampFileCopyStart;

    class S4U_PendingCommunication;

    /**
     * @brief A storage service that provides direct access to some storage resources (e.g., one or more disks).
     *        An important (configurable) property of the storage service is
     *        SimpleStorageServiceProperty::BUFFER_SIZE (see documentation thereof), which defines the
     *        buffer size that the storage service uses. Specifically, when the storage service
     *        receives/sends data from/to the network, it does so in a loop over data "chunks",
     *        with pipelined network and disk I/O operations. The smaller the buffer size the more "fluid"
     *        the model, but the more time-consuming the simulation. A large buffer size, however, may
     *        lead to less realistic simulations. At the extreme, an infinite buffer size would correspond
     *        to fully sequential executions (first a network receive/send, and then a disk write/read).
     *        Setting the buffer size to "0" corresponds to a fully fluid model in which individual
     *        data chunk operations are not simulated, thus achieving both accuracy (unless one specifically wishes
     *        to study the effects of buffering) and quick simulation times. For now, setting the buffer
     *        size to "0" is not implemented. The default buffer size is 10 MiB (note that the user can
     *        always declare a disk with arbitrary bandwidth in the platform description XML).
     */
    class SimpleStorageService : public StorageService {

    protected:
        /** @brief Default property values */
        WRENCH_PROPERTY_COLLECTION_TYPE default_property_values = {
                {SimpleStorageServiceProperty::MAX_NUM_CONCURRENT_DATA_CONNECTIONS, "infinity"},
                {SimpleStorageServiceProperty::BUFFER_SIZE, "10000000"},// 10 MEGA BYTE
                {SimpleStorageServiceProperty::CACHING_BEHAVIOR, "NONE"}};

        /** @brief Default message payload values */
        WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE default_messagepayload_values = {
                {SimpleStorageServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD, 1024},
                {SimpleStorageServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD, 1024},
                {SimpleStorageServiceMessagePayload::FREE_SPACE_REQUEST_MESSAGE_PAYLOAD, 1024},
                {SimpleStorageServiceMessagePayload::FREE_SPACE_ANSWER_MESSAGE_PAYLOAD, 1024},
                {SimpleStorageServiceMessagePayload::FILE_DELETE_REQUEST_MESSAGE_PAYLOAD, 1024},
                {SimpleStorageServiceMessagePayload::FILE_DELETE_ANSWER_MESSAGE_PAYLOAD, 1024},
                {SimpleStorageServiceMessagePayload::FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD, 1024},
                {SimpleStorageServiceMessagePayload::FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD, 1024},
                {SimpleStorageServiceMessagePayload::FILE_COPY_REQUEST_MESSAGE_PAYLOAD, 1024},
                {SimpleStorageServiceMessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD, 1024},
                {SimpleStorageServiceMessagePayload::FILE_WRITE_REQUEST_MESSAGE_PAYLOAD, 1024},
                {SimpleStorageServiceMessagePayload::FILE_WRITE_ANSWER_MESSAGE_PAYLOAD, 1024},
                {SimpleStorageServiceMessagePayload::FILE_READ_REQUEST_MESSAGE_PAYLOAD, 1024},
                {SimpleStorageServiceMessagePayload::FILE_READ_ANSWER_MESSAGE_PAYLOAD, 1024},
        };


    public:
        /***********************/
        /** \cond INTERNAL    **/
        /***********************/

        ~SimpleStorageService() override;

        static SimpleStorageService *createSimpleStorageService(const std::string &hostname,
                                                                std::set<std::string> mount_points,
                                                                WRENCH_PROPERTY_COLLECTION_TYPE property_list = {},
                                                                WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list = {});

        double getFileLastWriteDate(const std::shared_ptr<FileLocation> &location) override;

        bool hasFile(const std::shared_ptr<DataFile> &file, const std::string &path) override;

        /***********************/
        /** \endcond          **/
        /***********************/

    protected:
        SimpleStorageService(const std::string &hostname,
                             std::set<std::string> mount_points,
                             WRENCH_PROPERTY_COLLECTION_TYPE property_list,
                             WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list,
                             const std::string &suffix);

    protected:
        static unsigned long getNewUniqueNumber();

        /** @brief Maximum number of concurrent connections */
        unsigned long num_concurrent_connections;

        bool processStopDaemonRequest(simgrid::s4u::Mailbox *ack_mailbox);
        bool processFileDeleteRequest(const std::shared_ptr<FileLocation> &location,
                                      simgrid::s4u::Mailbox *answer_mailbox);
        bool processFileLookupRequest(const std::shared_ptr<FileLocation> &location,
                                      simgrid::s4u::Mailbox *answer_mailbox);
        bool processFreeSpaceRequest(simgrid::s4u::Mailbox *answer_mailbox);


    private:
        friend class Simulation;

        void validateProperties();

        std::shared_ptr<MemoryManager> memory_manager;
    };

}// namespace wrench

#endif//WRENCH_SIMPLESTORAGESERVICE_H
