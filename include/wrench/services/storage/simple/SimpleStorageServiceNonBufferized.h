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

#include <utility>

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
        /***********************/
        /** \cond INTERNAL    **/
        /***********************/

        /**
         * @brief Internal structure to describe transaction
         */
        struct Transaction {
            /** @brief source location */
            std::shared_ptr<FileLocation> src_location;
            /** @brief source host */
            simgrid::s4u::Host *src_host;
            /** @brief source disk */
            simgrid::s4u::Disk *src_disk;
            /** @brief destination location */
            std::shared_ptr<FileLocation> dst_location;
            /** @brief destination host */
            simgrid::s4u::Host *dst_host;
            /** @brief destination disk */
            simgrid::s4u::Disk *dst_disk;
            /** @brief mailbox to report to */
            simgrid::s4u::Mailbox *mailbox;
            /** @brief transfer size */
            double transfer_size;
            /** @brief SG IO op */
            simgrid::s4u::IoPtr stream;


        public:
            /**
             * @brief Constructor
             * @param src_location: source location
             * @param src_host: source host
             * @param src_disk: source disk
             * @param dst_location: destination location
             * @param dst_host: destination host
             * @param dst_disk: destination disk
             * @param mailbox: mailbox to report to
             * @param transfer_size: transfer size
             */
            Transaction(
                    std::shared_ptr<FileLocation> src_location,
                    simgrid::s4u::Host *src_host,
                    simgrid::s4u::Disk *src_disk,
                    std::shared_ptr<FileLocation> dst_location,
                    simgrid::s4u::Host *dst_host,
                    simgrid::s4u::Disk *dst_disk,
                    simgrid::s4u::Mailbox *mailbox,
                    double transfer_size) : src_location(std::move(src_location)), src_host(src_host), src_disk(src_disk),
                                            dst_location(std::move(dst_location)), dst_host(dst_host), dst_disk(dst_disk),
                                            mailbox(mailbox), transfer_size(transfer_size), stream(nullptr) {
            }
        };
        /***********************/
        /** \endcond          **/
        /***********************/


    private:
        friend class SimpleStorageService;

        //  Constructor
        SimpleStorageServiceNonBufferized(const std::string &hostname,
                                          std::set<std::string> mount_points,
                                          WRENCH_PROPERTY_COLLECTION_TYPE property_list = {},
                                          WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list = {});

        friend class Simulation;


        void cleanup(bool has_returned_from_main, int return_value) override;
        double getLoad() override;

        int main() override;

        bool processNextMessage(SimulationMessage *message);

        bool processFileWriteRequest(std::shared_ptr<FileLocation> &location,
                                     double num_bytes_to_write,
                                     simgrid::s4u::Mailbox *answer_mailbox,
                                     simgrid::s4u::Host *requesting_host);

        bool
        processFileReadRequest(const std::shared_ptr<FileLocation> &location,
                               double num_bytes_to_read, simgrid::s4u::Mailbox *answer_mailbox,
                               simgrid::s4u::Host *requesting_host);


        bool processFileCopyRequest(
                std::shared_ptr<FileLocation> &src,
                std::shared_ptr<FileLocation> &dst,
                simgrid::s4u::Mailbox *answer_mailbox);

        bool processFileCopyRequestIAmTheSource(
                std::shared_ptr<FileLocation> &src,
                std::shared_ptr<FileLocation> &dst,
                simgrid::s4u::Mailbox *answer_mailbox);

        bool processFileCopyRequestIAmNotTheSource(
                std::shared_ptr<FileLocation> &src,
                std::shared_ptr<FileLocation> &dst,
                simgrid::s4u::Mailbox *answer_mailbox);


        void startPendingTransactions();

        void processTransactionCompletion(const std::shared_ptr<Transaction> &transaction);
        void processTransactionFailure(const std::shared_ptr<Transaction> &transaction);


        std::deque<std::shared_ptr<Transaction>> pending_transactions;
        std::vector<std::shared_ptr<Transaction>> running_transactions;

        std::map<simgrid::s4u::IoPtr, std::shared_ptr<Transaction>> stream_to_transactions;

        std::shared_ptr<MemoryManager> memory_manager;
    };

}// namespace wrench

#endif//WRENCH_SIMPLESTORAGESERVICENONBUFFERIZED_H
