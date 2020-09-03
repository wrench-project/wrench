//
// Created by Dzung Do on 2020-08-03.
//

#ifndef WRENCH_MEMORYMANAGER_H
#define WRENCH_MEMORYMANAGER_H

#include <string>
#include <wrench/services/Service.h>
#include <wrench/simulation/Simulation.h>
#include "Block.h"

namespace wrench {

    class MemoryManager : public Service {

    private:
        s4u_Disk *memory;
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

        MemoryManager(s4u_Disk *memory, double dirty_ratio, int interval, int expired_time, std::string hostname);

        int main() override;

        void balanceLruLists();

        double pdflush();

        double flushExpiredData(std::vector<Block *> &list);

        double flushLruList(std::vector<Block *> &list, double amount, std::string excluded_filename);

    public:

        static std::shared_ptr<MemoryManager> initAndStart(Simulation *simulation, s4u_Disk *memory,
                                                           double dirty_ratio, int interval, int expired_time,
                                                           std::string hostname);

        void kill();

        s4u_Disk *getMemory() const;

        void setMemory(s4u_Disk *memory);

        double getDirtyRatio() const;

        void setDirtyRatio(double dirtyRatio);

        double getFreeMemory() const;

        void setFreeMemory(double free_amt);

        double getCached() const;

        double getDirty() const;

        double getEvictableMemory();

        double getAvailableMemory();

        double flush(double amount, std::string excluded_filename);

        double evict(double amount, std::string excluded_filename);

        simgrid::s4u::IoPtr readToCache(std::string filename, std::string mount_point, double amount, bool async);

        simgrid::s4u::IoPtr readFromCache(std::string filename, bool async);

        void readChunkFromCache(std::string filename, double amount);

        void writeToCache(std::string filename, std::string mnt_pt, double amount);

        double getCachedAmount(std::string filename);

        std::vector<Block*> getCachedBlocks(std::string filename);

        static s4u_Disk *getDisk(std::string mountpoint, std::string hostname);

        void log();

        void export_log(std::string filename);

    };

}

#endif //WRENCH_MEMORYMANAGER_H
