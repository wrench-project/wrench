
/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_XROOTD_CACHE_H
#define WRENCH_XROOTD_CACHE_H
#include <unordered_map>
#include <memory>
#include <vector>
#include <set>
#include "wrench/data_file/DataFile.h"
#include "wrench/services/storage/storage_helpers/FileLocation.h"
#include <utility>
#include <limits>
namespace wrench {
    namespace XRootD {

        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        /**
         * @brief A class that implements the XRootD cache
         */
        class Cache {
        private:
            /** @brief The internal cache data structure, currently just a map of data files pointers to a set of file locations, each with corresponding last update time */
            std::unordered_map<std::shared_ptr<DataFile>, std::map<std::shared_ptr<FileLocation>, double>> cache;//possibly change time to last access time
        public:
            /** @brief The maximum time an unupdated entry can remain in the cache.*/
            double maxCacheTime = std::numeric_limits<double>::infinity();
            bool isCached(std::shared_ptr<DataFile> file);
            void add(std::shared_ptr<DataFile> file, std::shared_ptr<FileLocation> location);
            void add(std::shared_ptr<DataFile> file, std::set<std::shared_ptr<FileLocation>> locations);
            std::set<std::shared_ptr<FileLocation>> get(std::shared_ptr<DataFile> file);

            std::set<std::shared_ptr<FileLocation>> operator[](std::shared_ptr<DataFile> file);
            void remove(std::shared_ptr<DataFile> file);
            void clean();
        };

        /***********************/
        /** \endcond           */
        /***********************/
    }// namespace XRootD
}// namespace wrench
#endif//WRENCH_CACHE_H
