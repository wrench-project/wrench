/**
* Copyright (c) 2017. The WRENCH Team.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/
#include "wrench/services/storage/xrootd/Cache.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
namespace wrench{
    namespace XRootD{
        bool Cache::isCached(std::shared_ptr<DataFile> file) {
            return cache.find(file)!=cache.end();//once timestamps are implimented also check the file is still in cache  it (remove if not)
        }
        void Cache::add(std::shared_ptr<DataFile> file,std::shared_ptr<FileLocation> location){
            wrench::S4U_Simulation::getClock();
            cache[file].insert(location);
        }
        void Cache::add(std::shared_ptr<DataFile> file, std::set<std::shared_ptr<FileLocation>> locations){
            wrench::S4U_Simulation::getClock();
            for(auto location:locations){
                cache[file].insert(location);
            }
        }
        std::set<std::shared_ptr<FileLocation>> Cache::get(std::shared_ptr<DataFile> file){
            return cache[file];

        }

        std::set<std::shared_ptr<FileLocation>> Cache::operator[](std::shared_ptr<DataFile> file){
            return get(file);
        }
        void Cache::remove(std::shared_ptr<DataFile> file){
            cache.erase(file);
        }
    }
}

