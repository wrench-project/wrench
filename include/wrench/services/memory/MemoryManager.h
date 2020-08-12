//
// Created by Dzung Do on 2020-08-03.
//

#ifndef WRENCH_MEMORYMANAGER_H
#define WRENCH_MEMORYMANAGER_H

#include <string>
#include <wrench/services/Service.h>
#include "Block.h"

namespace wrench {

    class Simulation;

    class MemoryManager : public Service {

    private:
        s4u_Disk* memory;
        double dirty_ratio;
        int interval;
        int expired_time;
        std::vector<Block*> inactive_list;
        std::vector<Block*> active_list;

        // We keep track of these properties since we don't want to traverse through two LRU lists to get them.
        double free;
        double cached;
        double dirty;


        MemoryManager(s4u_Disk *memory, double dirty_ratio, int interval, int expired_time, std::string hostname);

        int main() override;

        void balanceLruLists();
        void balanceAndSortCache();
        double flushLruList(std::vector<Block *> &list, double amount);
        double flushExpiredData(std::vector<Block *> &list);
        s4u_Disk* getDisk(const std::string &filename);

    public:

        static std::shared_ptr<MemoryManager> initAndStart(Simulation *simulation, s4u_Disk *memory,
                double dirty_ratio, int interval, int expired_time, std::string hostname);

        void kill();

        s4u_Disk* getMemory() const;

        void setMemory(s4u_Disk *memory);

        double getDirtyRatio() const;

        void setDirtyRatio(double dirtyRatio);

        double getFree() const;

        double getCached() const;

        double getDirty() const;

        double flush(double amount);

        double pdflush();

        double evict(double amount);

        void readToCache(std::string &filename, double amount);

        void readFromCache(std::string &filename);

        void writeToCache(std::string &filename, double amount);

    };

}

#endif //WRENCH_MEMORYMANAGER_H
