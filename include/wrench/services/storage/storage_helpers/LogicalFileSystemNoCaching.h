/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_LOGICALFILESYSTEMNOCACHING_H
#define WRENCH_LOGICALFILESYSTEMNOCACHING_H

#include <stdexcept>
#include <string>
#include <map>
#include <unordered_map>
#include <set>
#include <memory>

#include <simgrid/disk.h>


#include <wrench/data_file/DataFile.h>
#include <wrench/services/storage/storage_helpers/LogicalFileSystem.h>

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/


    class StorageService;

    /**
     * @brief  A class that implements a weak file system abstraction
     */
    class LogicalFileSystemNoCaching : public LogicalFileSystem {

        class FileOnDiskNoCaching : public FileOnDisk {
        public:
            explicit FileOnDiskNoCaching(double last_write_date) : FileOnDisk(last_write_date) {}
        };

    public:
        void storeFileInDirectory(const std::shared_ptr<DataFile> &file, const std::string &absolute_path) override;
        void removeFileFromDirectory(const std::shared_ptr<DataFile> &file, const std::string &absolute_path) override;
        void removeAllFilesInDirectory(const std::string &absolute_path) override;
        void updateReadDate(const std::shared_ptr<DataFile> &file, const std::string &absolute_path) override;
        void incrementNumRunningTransactionsForFileInDirectory(const std::shared_ptr<DataFile> &file, const std::string &absolute_path) override;
        void decrementNumRunningTransactionsForFileInDirectory(const std::shared_ptr<DataFile> &file, const std::string &absolute_path) override;

    protected:
        friend class StorageService;
        bool evictFiles(double needed_free_space) override;

    private:
        friend class LogicalFileSystem;
        explicit LogicalFileSystemNoCaching(const std::string &hostname,
                                            StorageService *storage_service,
                                            const std::string &mount_point);


    private:
    };


    /***********************/
    /** \endcond           */
    /***********************/

}// namespace wrench


#endif//WRENCH_LOGICALFILESYSTEMNOCACHING_H
