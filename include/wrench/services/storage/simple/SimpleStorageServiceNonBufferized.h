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

    public:
        struct Transaction {
            std::shared_ptr<DataFile> file;
            std::shared_ptr<FileLocation> src_location;
            simgrid::s4u::Host *src_host;
            simgrid::s4u::Disk *src_disk;
            std::shared_ptr<FileLocation> dst_location;
            simgrid::s4u::Host *dst_host;
            simgrid::s4u::Disk *dst_disk;
            simgrid::s4u::Mailbox *mailbox;
            simgrid::s4u::IoPtr stream;

        public:
            Transaction(std::shared_ptr<DataFile> file,
                        std::shared_ptr<FileLocation> src_location,
                        simgrid::s4u::Host *src_host,
                        simgrid::s4u::Disk *src_disk,
                        std::shared_ptr<FileLocation> dst_location,
                        simgrid::s4u::Host *dst_host,
                        simgrid::s4u::Disk *dst_disk,
                        simgrid::s4u::Mailbox *mailbox) :
                    file(file), src_location(src_location), src_host(src_host), src_disk(src_disk),
                    dst_location(dst_location), dst_host(dst_host), dst_disk(dst_disk),
                    mailbox(mailbox), stream(nullptr) {
            }

        };


    private:
        friend class SimpleStorageService;

        //  Constructor
        SimpleStorageServiceNonBufferized(const std::string &hostname,
                                          std::set<std::string> mount_points,
                                          WRENCH_PROPERTY_COLLECTION_TYPE property_list = {},
                                          WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list = {});
    private:
        friend class Simulation;

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/

        void cleanup(bool has_returned_from_main, int return_value) override;
        double getLoad() override;

        /***********************/
        /** \endcond          **/
        /***********************/

        int main() override;

        bool processNextMessage(SimulationMessage *message);

        bool processFileWriteRequest(const std::shared_ptr<DataFile> &file, const std::shared_ptr<FileLocation> &location,
                                     simgrid::s4u::Mailbox *answer_mailbox, simgrid::s4u::Host *requesting_host,
                                     double buffer_size);

        bool
        processFileReadRequest(const std::shared_ptr<DataFile> &file, const std::shared_ptr<FileLocation> &location,
                               double num_bytes_to_read, simgrid::s4u::Mailbox *answer_mailbox,
                               simgrid::s4u::Host *requesting_host);

        bool processFileCopyRequest(const std::shared_ptr<DataFile> &file,
                                    const std::shared_ptr<FileLocation> &src,
                                    const std::shared_ptr<FileLocation> &dst,
                                    simgrid::s4u::Mailbox *answer_mailbox);


        void startPendingTransactions();

        void processTransactionCompletion(const std::shared_ptr<Transaction>& transaction);
        void processTransactionFailure(const std::shared_ptr<Transaction>& transaction);


//        std::deque<simgrid::s4u::IoPtr> pending_sg_iostreams;
//        std::vector<simgrid::s4u::IoPtr> running_sg_iostreams;
//        std::unordered_map<simgrid::s4u::IoPtr, std::shared_ptr<Transaction>> transactions;

        std::deque<std::shared_ptr<Transaction>> pending_transactions;
        std::vector<std::shared_ptr<Transaction>> running_transactions;
        std::unordered_map<simgrid::s4u::IoPtr, std::shared_ptr<Transaction>> stream_to_transactions;

        std::shared_ptr<MemoryManager> memory_manager;
    };

};// namespace wrench

#endif//WRENCH_SIMPLESTORAGESERVICE_H
