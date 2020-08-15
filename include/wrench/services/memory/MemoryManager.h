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

        // We keep track of these properties since we don't want to traverse through two LRU lists to get them.
        double free;
        double cached;
        double dirty;


        MemoryManager(s4u_Disk *memory, double dirty_ratio, int interval, int expired_time, std::string hostname);

        int main() override;

        void balanceLruLists();

        void balanceAndSortCache();

        double pdflush();

        double flushExpiredData(std::vector<Block *> &list);

        double flushLruList(std::vector<Block *> &list, double amount);

    public:

        static std::shared_ptr<MemoryManager> initAndStart(Simulation *simulation, s4u_Disk *memory,
                                                           double dirty_ratio, int interval, int expired_time,
                                                           std::string hostname);

        void kill();

        s4u_Disk *getMemory() const;

        void setMemory(s4u_Disk *memory);

        double getDirtyRatio() const;

        void setDirtyRatio(double dirtyRatio);

        double getFree() const;

        void setFree(double free_amt);

        double getCached() const;

        double getDirty() const;

        double getEvictable();

        double flush(std::string mountpoint, double amount);

        double evict(double amount);

        simgrid::s4u::IoPtr readToCache(std::string filename, std::string mount_point, double amount, bool async);

        simgrid::s4u::IoPtr readFromCache(std::string filename, bool async);

        void writeToCache(std::string &filename, double amount);

        double getCachedData(std::string filename);

        static s4u_Disk *getDisk(std::string filename, std::string hostname);

    };

}

#endif //WRENCH_MEMORYMANAGER_H
