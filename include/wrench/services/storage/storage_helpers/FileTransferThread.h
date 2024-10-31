/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_FILETRANSFERTHREAD_H
#define WRENCH_FILETRANSFERTHREAD_H

#include <string>
#include "wrench/simgrid_S4U_util/S4U_Daemon.h"
#include "wrench/services/Service.h"
#include "wrench/services/storage/StorageService.h"
#include <iostream>

namespace wrench {

    class DataFile;
    class SimulationTimestampFileCopyStart;

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /** @brief A helper class that implements the concept of a communication
     *  thread that performs a file transfer
     */
    class FileTransferThread : public Service {

    public:
        ~FileTransferThread() {
        }


        FileTransferThread(const std::string& hostname,
                           std::shared_ptr<StorageService> parent,
                           std::shared_ptr<DataFile> file,
                           sg_size_t num_bytes_to_transfer,
                           S4U_CommPort *src_commport,
                           std::shared_ptr<FileLocation> dst_location,
                           std::shared_ptr<simgrid::fsmod::File> dst_opened_file,
                           S4U_CommPort *answer_commport_if_read,
                           S4U_CommPort *answer_commport_if_write,
                           S4U_CommPort *answer_commport_if_copy,
                           sg_size_t buffer_size);

        FileTransferThread(const std::string& hostname,
                           std::shared_ptr<StorageService> parent,
                           std::shared_ptr<DataFile> file,
                           sg_size_t num_bytes_to_transfer,
                           std::shared_ptr<FileLocation> src_location,
                           std::shared_ptr<simgrid::fsmod::File> src_opened_file,
                           S4U_CommPort *dst_commport,
                           S4U_CommPort *answer_commport_if_read,
                           S4U_CommPort *answer_commport_if_write,
                           S4U_CommPort *answer_commport_if_copy,
                           sg_size_t buffer_size);

        FileTransferThread(const std::string& hostname,
                           std::shared_ptr<StorageService> parent,
                           std::shared_ptr<DataFile> file,
                           sg_size_t num_bytes_to_transfer,
                           std::shared_ptr<FileLocation> src_location,
                           std::shared_ptr<simgrid::fsmod::File> src_opened_file,
                           std::shared_ptr<FileLocation> dsg_location,
                           std::shared_ptr<simgrid::fsmod::File> dst_opened_file,
                           S4U_CommPort *answer_commport_if_read,
                           S4U_CommPort *answer_commport_if_write,
                           S4U_CommPort *answer_commport_if_copy,
                           sg_size_t buffer_size);

        int main() override;
        void cleanup(bool has_returned_from_main, int return_value) override;


    private:
        friend class SimpleStorageServiceBufferized;

        std::shared_ptr<StorageService> parent;
        std::shared_ptr<DataFile> file;

        // Only one of these two is valid
        S4U_CommPort *src_commport;
        std::shared_ptr<FileLocation> src_location;
        std::shared_ptr<simgrid::fsmod::File> src_opened_file;

        // Only one of these two is valid
        S4U_CommPort *dst_commport;
        std::shared_ptr<FileLocation> dst_location;
        std::shared_ptr<simgrid::fsmod::File> dst_opened_file;

        sg_size_t num_bytes_to_transfer;

        S4U_CommPort *answer_commport_if_read;
        S4U_CommPort *answer_commport_if_write;
        S4U_CommPort *answer_commport_if_copy;
        sg_size_t buffer_size;

        void receiveFileFromNetwork(const std::shared_ptr<DataFile> &f, S4U_CommPort *commport, const std::shared_ptr<FileLocation> &location);
        void sendLocalFileToNetwork(const std::shared_ptr<DataFile> &f, const std::shared_ptr<FileLocation> &location, sg_size_t num_bytes, S4U_CommPort *commport);
        void downloadFileFromStorageService(const std::shared_ptr<DataFile> &f, const std::shared_ptr<FileLocation> &src_loc, const std::shared_ptr<FileLocation> &dst_loc);
        void copyFileLocally(const std::shared_ptr<DataFile> &f, const std::shared_ptr<FileLocation> &src_loc, const std::shared_ptr<FileLocation> &dst_loc);
    };

    /***********************/
    /** \endcond           */
    /***********************/
}// namespace wrench

#endif//WRENCH_FILETRANSFERTHREAD_H
