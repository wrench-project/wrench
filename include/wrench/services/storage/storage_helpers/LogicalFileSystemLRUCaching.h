///**
// * Copyright (c) 2017. The WRENCH Team.
// *
// * This program is free software: you can redistribute it and/or modify
// * it under the terms of the GNU General Public License as published by
// * the Free Software Foundation, either version 3 of the License, or
// * (at your option) any later version.
// */
//
//#ifndef WRENCH_LOGICALFILESYSTEMLRUCACHING_H
//#define WRENCH_LOGICALFILESYSTEMLRUCACHING_H
//
//#include <stdexcept>
//#include <string>
//#include <map>
//#include <unordered_map>
//#include <set>
//#include <memory>
//
//#include <simgrid/disk.h>
//
//
//#include <wrench/data_file/DataFile.h>
//
//namespace wrench {
//
//    /***********************/
//    /** \cond INTERNAL     */
//    /***********************/
//
//
//    class StorageService;
//
//    /**
//     * @brief  A class that implements a weak file system abstraction
//     */
//    class LogicalFileSystemLRUCaching : public LogicalFileSystem {
//
//    public:
//        /**
//	 * @brief A helper struct to describe a file instance on disk
//	 */
//        struct FileOnDiskLRUCaching : public FileOnDisk {
//        public:
//            /**
//             * @brief Constructor
//             * @param last_write_date Last write date
//             * @param lru_sequence_number LRU sequence number
//             * @param num_current_transactions Number of current transactions using this file on disk
//             */
//            FileOnDiskLRUCaching(double last_write_date,
//                                 unsigned int lru_sequence_number,
//                                 unsigned short num_current_transactions) : FileOnDisk(last_write_date),
//                                                                            lru_sequence_number(lru_sequence_number),
//                                                                            num_current_transactions(num_current_transactions) {}
//
//            /**
//             * @brief The LRU sequence number (lower means older)
//             */
//            unsigned int lru_sequence_number;
//            /**
//             * @brief The number of transactions that involve this file, meaning that it's not evictable is > 0
//             */
//            unsigned short num_current_transactions;
//        };
//
//    public:
//        /**
//         * @brief Next LRU sequence number
//         */
//        unsigned int next_lru_sequence_number = 0;
//
//        void storeFileInDirectory(const std::shared_ptr<DataFile> &file, const std::string &absolute_path) override;
//        void removeFileFromDirectory(const std::shared_ptr<DataFile> &file, const std::string &absolute_path) override;
//        void removeAllFilesInDirectory(const std::string &absolute_path) override;
//        void updateReadDate(const std::shared_ptr<DataFile> &file, const std::string &absolute_path) override;
//        void incrementNumRunningTransactionsForFileInDirectory(const std::shared_ptr<DataFile> &file, const std::string &absolute_path) override;
//        void decrementNumRunningTransactionsForFileInDirectory(const std::shared_ptr<DataFile> &file, const std::string &absolute_path) override;
//
//    protected:
//        bool evictFiles(double needed_free_space) override;
//
//    private:
//        friend class StorageService;
//        friend class LogicalFileSystem;
//
//        explicit LogicalFileSystemLRUCaching(const std::string &hostname,
//                                             StorageService *storage_service,
//                                             const std::string &mount_point);
//
//
//        std::map<unsigned int, std::tuple<std::string, std::shared_ptr<DataFile>>> lru_list;
//
//        void print_lru_list() {
//            std::cerr << "LRU LIST:\n";
//            for (auto const &lru: this->lru_list) {
//                std::cerr << "[" << lru.first << "] " << std::get<0>(lru.second) << ":" << std::get<1>(lru.second)->getID() << "\n";
//            }
//        }
//
//    private:
//    };
//
//
//    /***********************/
//    /** \endcond           */
//    /***********************/
//
//}// namespace wrench
//
//
//#endif//WRENCH_LOGICALFILESYSTEMLRUCACHING_H
