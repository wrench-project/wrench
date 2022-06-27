
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
namespace wrench{
    namespace XRootD{

/***********************/
/** \cond INTERNAL     */
/***********************/
        class Cache{
        private:
            /** @brief The internal cache data structure, currently just a map of data files pointers to a set of file locations.  In future, this will contain timestamps */
            std::unordered_map< std::shared_ptr<DataFile>, std::set<std::shared_ptr<FileLocation>>> cache;//probably change the payload of this to an object containing the file location AND its queue time stamp
        public:
            bool isCached(std::shared_ptr<DataFile> file);
            void add(std::shared_ptr<DataFile> file,std::shared_ptr<FileLocation> location);

            void add(std::shared_ptr<DataFile> file, std::set<std::shared_ptr<FileLocation>> locations);
            std::set<std::shared_ptr<FileLocation>> get(std::shared_ptr<DataFile> file);

            std::set<std::shared_ptr<FileLocation>> operator[](std::shared_ptr<DataFile> file);
            void remove(std::shared_ptr<DataFile> file);

        };

/***********************/
/** \endcond           */
/***********************/
    }
}
#endif//WRENCH_CACHE_H
