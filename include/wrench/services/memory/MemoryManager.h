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
        long free;
        long cached;
        long dirty;


        MemoryManager(s4u_Disk *memory, double dirty_ratio, int interval, int expired_time, std::string hostname);

        int main() override;

        void balance_lru_lists();
        void cache_balance_and_sort();

    public:

        static std::shared_ptr<MemoryManager> initAndStart(Simulation *simulation, s4u_Disk *memory,
                double dirty_ratio, int interval, int expired_time, std::string hostname);

        void kill();

        s4u_Disk* getMemory() const;

        void setMemory(s4u_Disk *memory);

        double getDirtyRatio() const;

        void setDirtyRatio(double dirtyRatio);

        long getFree() const;

        long getCached() const;

        long getDirty() const;

        long flush(long amount);

        long flush_expired_data();

        long evict(long amount);

        void read_to_cache(std::string &filename, long amount);

        void cache_read(std::string &filename);

        void cache_write(std::string &filename, long amount);

    };

}

#endif //WRENCH_MEMORYMANAGER_H
