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
        /**
         * @brief Check the cache for a file
         * @param file: The file to check the cache for
         * @return true if the file is cached and if timestamp (not implemented) is valid, false otherwise
         */
        bool Cache::isCached(std::shared_ptr<DataFile> file) {
            double earliestAllowedTime = wrench::S4U_Simulation::getClock()-maxCacheTime;
            auto entries=cache[file];
            //after getting all possible cache entries, loop through them "all" and check the timestamps, removing any that fail.  If you find even 1 success, return true and stop cleaning.
            for(auto ittr=entries.begin();ittr!=entries.end();){//intentionaly blank 3rd term
                auto entry=*ittr;
                if(entry.second<earliestAllowedTime){
                    ittr=entries.erase(ittr);
                }else{
                    return true;
                }
            }
            return false;

        }
        /**
         * @brief Add a file to the cache
         * @param file: The file to add to the cache
         * @param location: The location to add to the cache
         */
        void Cache::add(std::shared_ptr<DataFile> file,std::shared_ptr<FileLocation> location){
            double currentSimTime = wrench::S4U_Simulation::getClock();
            cache[file][location]=currentSimTime;
        }

        /**
        * @brief Add a file to the cache
        * @param file: The file to add to the cache
        * @param locations: The locations to add to the cache
        */
        void Cache::add(std::shared_ptr<DataFile> file, std::set<std::shared_ptr<FileLocation>> locations){
            double currentSimTime = wrench::S4U_Simulation::getClock();
            for(auto location:locations){
                cache[file][location]=currentSimTime;
            }
        }
        /**
         * @brief get all valid cached copies.
         * @param file: The file to check the cache for
         * @return the set of valid cached copies.  (empty set if not found)
         */
        std::set<std::shared_ptr<FileLocation>> Cache::get(std::shared_ptr<DataFile> file){
            double earliestAllowedTime = wrench::S4U_Simulation::getClock()-maxCacheTime;
            auto entries=cache[file];
            std::set<std::shared_ptr<FileLocation>> ret;
            for(auto ittr=entries.begin();ittr!=entries.end();){//intentionaly blank 3rd term
                auto entry=*ittr;
                if(entry.second<earliestAllowedTime){
                    ittr=entries.erase(ittr);
                }else{
                    ret.insert(entry.first);
                    ittr++;

                }
            }
            return ret;

        }/**
         * @brief get all valid cached copies.
         * @param file: The file to check the cache for
         * @return the set of valid cached copies.  (empty set if not found)
         */

        std::set<std::shared_ptr<FileLocation>> Cache::operator[](std::shared_ptr<DataFile> file){
            return get(file);
        }/**
         * @brief remove all copies of a file from the cache
         * @param file: The file to check the cache for
         */

        void Cache::remove(std::shared_ptr<DataFile> file){
            cache.erase(file);
        }
        /**
         * @brief Deap Clean the cache of outdated entries.  Possibly slow, but if the cache size is getting out of hand, this can help.
         * @param file: The file to check the cache for
         */

        void Cache::clean(){
            double earliestAllowedTime = wrench::S4U_Simulation::getClock()-maxCacheTime;
            //itterate all files
            for(auto ettr=cache.begin();ettr!=cache.end();) {//intentionaly blank 3rd term
                auto entries = ettr->second;
                //iterate all locations
                for (auto ittr = entries.begin(); ittr != entries.end();) {//intentionaly blank 3rd term
                    auto entry = *ittr;
                    //check all timestamps
                    if (entry.second < earliestAllowedTime) {
                        ittr = entries.erase(ittr);
                    } else {
                        ittr++;
                    }
                }
                //remove empty files
                if(cache[ettr->first].empty()){
                    ettr=cache.erase(ettr);
                }else{
                    ettr++;
                }
            }
        }
    }
}

