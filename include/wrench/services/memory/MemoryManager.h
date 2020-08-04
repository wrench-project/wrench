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
        std::vector<Block> inactive_list;
        std::vector<Block> active_list;


        MemoryManager(s4u_Disk *memory, double dirty_ratio, int interval, int expired_time, std::string hostname);

        int main() override;

    public:

        static std::shared_ptr<MemoryManager> initAndStart(Simulation *simulation, s4u_Disk *memory,
                double dirty_ratio, int interval, int expired_time, std::string hostname);

        void kill();

        s4u_Disk* getMemory() const;

        void setMemory(s4u_Disk *memory);

        double getDirtyRatio() const;

        void setDirtyRatio(double dirtyRatio);

        const std::vector<Block> &getInactiveList() const;

        const std::vector<Block> &getActiveList() const;

        long flush(long amount);

        long flush_expired_data();

        long evict(long amount);

        void cache_read(std::string filename);

        void cache_write(std::string filename, long amount);

    };

}

#endif //WRENCH_MEMORYMANAGER_H
