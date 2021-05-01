/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef WRENCH_MEMORYMANAGER_H
#define WRENCH_MEMORYMANAGER_H

#include <string>
#include <wrench/services/Service.h>
#include <wrench/simulation/Simulation.h>
#include "Block.h"

namespace wrench {

    /***********************/
    /** \cond INTERNAL    */
    /***********************/

    /**
     * @brief A class that implemnets a MemoryManager service to simulate Linux in-memory 
     * page caching for I/O operations
     */
    class MemoryManager : public Service {

    public:

    private:
        simgrid::s4u::Disk *memory;
        double dirty_ratio;
        int interval;
        int expired_time;
        std::vector<Block *> inactive_list;
        std::vector<Block *> active_list;
        double total;

        // We keep track of these properties since we don't want to traverse through two LRU lists to get them.
        double free;
        double cached;
        double dirty;

        std::vector<double> time_log;
        std::vector<double> dirty_log;
        std::vector<double> cached_log;
        std::vector<double> free_log;


        MemoryManager(simgrid::s4u::Disk *memory, double dirty_ratio, int interval, int expired_time, std::string hostname);

        int main() override;

        void balanceLruLists();

        double pdflush();

        double flushExpiredData(std::vector<Block *> &list);

        double flushLruList(std::vector<Block *> &list, double amount, std::string excluded_filename);

    public:

        static std::shared_ptr<MemoryManager> initAndStart(Simulation *simulation, simgrid::s4u::Disk *memory,
                                                           double dirty_ratio, int interval, int expired_time,
                                                           std::string hostname);

        void kill();

        simgrid::s4u::Disk *getMemory() const;

        void setMemory(simgrid::s4u::Disk *memory);

        double getDirtyRatio() const;

        void setDirtyRatio(double dirty_ratio);

        double getFreeMemory() const;

        void releaseMemory(double released_amt);

        void useAnonymousMemory(double used_amt);

        double getTotalCachedAmount() const;

        double getDirty() const;

        double getEvictableMemory();

        double getAvailableMemory();

        double getTotalMemory();

        double flush(double amount, std::string excluded_filename);

        double evict(double amount, std::string excluded_filename);

        simgrid::s4u::IoPtr readToCache(std::string filename, std::shared_ptr<FileLocation> location,
                double amount, bool async);

        void readChunkFromCache(std::string filename, double amount);

        void writebackToCache(std::string filename, std::shared_ptr<FileLocation> location, double amount, bool is_dirty);

        void addToCache(std::string filename, std::shared_ptr<FileLocation> location, double amount, bool is_dirty);

        double getCachedAmount(std::string filename);

        std::vector<Block*> getCachedBlocks(std::string filename);

        static simgrid::s4u::Disk *getDisk(std::string mountpoint, std::string hostname);

        void log();

        void export_log(std::string filename);

        /***********************/
        /** \endcond          */
        /***********************/
    };

}

#endif //WRENCH_MEMORYMANAGER_H
