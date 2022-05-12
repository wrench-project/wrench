
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
#import <unordered_map>
#include <memory>
#include <vector>
#include <set>
#include "wrench/data_file/DataFile.h"
#include "wrench/services/storage/storage_helpers/FileLocation.h"

namespace wrench{
    namespace XRootD{
        class Cache{
        private:
            std::unordered_map< std::shared_ptr<DataFile>, std::set<std::shared_ptr<FileLocation>>> cache;//probiably change the payload of this to an object containing the file location AND its queue time stamp
        public:
            bool isCached(std::shared_ptr<DataFile> file);
            void add(std::shared_ptr<DataFile> file,std::shared_ptr<FileLocation> location);

            void add(std::shared_ptr<DataFile> file, std::set<std::shared_ptr<FileLocation>> locations);
            std::set<std::shared_ptr<FileLocation>> get(std::shared_ptr<DataFile> file);

            std::set<std::shared_ptr<FileLocation>> operator[](std::shared_ptr<DataFile> file);
            void remove(std::shared_ptr<DataFile> file);
        };

    }
}
#endif//WRENCH_CACHE_H
