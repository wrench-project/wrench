//
// Created by Dzung Do on 2020-08-03.
//

#include "wrench/logging/TerminalOutput.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
#include "wrench/workflow/failure_causes/HostError.h"
#include <wrench/services/memory/MemoryManager.h>

WRENCH_LOG_CATEGORY(wrench_periodic_flush, "Log category for Periodic Flush");

namespace wrench {

    /**
     * Constructor
     *
     * @param memory: disk model used to simulate memory
     * @param dirty_ratio: dirty_ratio parameter as in the Linux kernel
     * @param interval: the interval that periodical flushing awakes in second
     * @param expired_time: the expired time of dirty data to be flushed in second
     * @param hostname: name of the host on which periodical flushing starts
     */
    MemoryManager::MemoryManager(s4u_Disk *memory, double dirty_ratio,
            int interval, int expired_time, std::string hostname) :
            Service(hostname, "periodic_flush_" + hostname, "periodic_flush_" + hostname),
            memory(memory), dirty_ratio(dirty_ratio), interval(interval), expired_time(expired_time) {

        free = S4U_Simulation::getHostMemoryCapacity(hostname);
        dirty = 0;
        cached = 0;
    }

    int MemoryManager::main() {

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_MAGENTA);
        WRENCH_INFO("Periodic Flush starting with interval = %d , expired time = %d",
                this->interval, this->expired_time);

        while (this->getState() == State::UP) {
            WRENCH_INFO("Flush on host %s", this->getHostname().c_str());
            S4U_Simulation::sleep(this->interval - 0);
        }

        this->setStateToDown();
        return 0;
    }

    /**
     * Initialize and start the memory manager
     *
     * @param simulation
     * @param memory: disk model used to simulate memory
     * @param dirty_ratio: dirty_ratio parameter as in the Linux kernel
     * @param interval: the interval that periodical flushing awakes in milliseconds
     * @param expired_time: the expired time of dirty data to be flushed in milliseconds
     * @param hostname: name of the host on which periodical flushing starts
     * @return
     */
    std::shared_ptr<MemoryManager> MemoryManager::initAndStart(wrench::Simulation *simulation, s4u_Disk *memory,
            double dirty_ratio, int interval, int expired_time, std::string hostname) {

        std::shared_ptr<MemoryManager> flush_ptr = std::shared_ptr<MemoryManager>(
                new MemoryManager(memory, dirty_ratio, interval, expired_time, hostname));

        flush_ptr->simulation = simulation;

        try {
            flush_ptr->start(flush_ptr, true, false); // Daemonized, no auto-restart
        } catch (std::shared_ptr<HostError> &e) {
            throw;
        }
        return flush_ptr;
    }

    /**
     * @brief Immediately terminate periodical flushing
     */
    void MemoryManager::kill() {
        this->killActor();
    }

    s4u_Disk *MemoryManager::getMemory() const {
        return memory;
    }

    void MemoryManager::setMemory(s4u_Disk *memory) {
        this->memory = memory;
    }

    double MemoryManager::getDirtyRatio() const {
        return dirty_ratio;
    }

    void MemoryManager::setDirtyRatio(double dirtyRatio) {
        dirty_ratio = dirtyRatio;
    }

    long MemoryManager::getFree() const {
        return free;
    }

    long MemoryManager::getCached() const {
        return cached;
    }

    long MemoryManager::getDirty() const {
        return dirty;
    }

    long MemoryManager::flush_lru_list(std::vector<Block*> &list, long amount) {

        if (amount <= 0) return 0;
        long flushed = 0;

        for (auto blk : list) {

            if (blk->isDirty()) {
                if (flushed + blk->getSize() <= amount) {
                    // flush whole block
                    blk->setDirty(false);
                    flushed += blk->getSize();
                } else if (flushed < amount && amount < flushed + blk->getSize()) {

                    long blk_flushed = amount - flushed;
                    flushed = amount;
                    // split
                    blk->setSize(blk->getSize() - blk_flushed);

                    std::string fn = blk->getFilename();
                    inactive_list.push_back(new Block(fn, blk_flushed, blk->getLastAccess(), false));
                } else {
                    // done flushing
                    break;
                }
            }
        }

        dirty -= flushed;

        return flushed;
    }

    long MemoryManager::flush(long amount) {

        long flushed_inactive = flush_lru_list(inactive_list, amount);

        long flushed_active = 0;
        if (flushed_inactive < amount) {
            flushed_active = flush_lru_list(active_list, amount - flushed_inactive);
        }

        return flushed_inactive + flushed_active;
    }

    long MemoryManager::pdflush_lru_list(std::vector<Block*> &list) {

        long flushed = 0;

        for (auto blk : list) {
            if (!blk->isDirty()) continue;
            if (S4U_Simulation::getClock() - blk->getLastAccess() >= expired_time) {
                blk->setDirty(false);
                flushed += blk->getSize();
            }
        }

        dirty -= flushed;
        return flushed;
    }

    long MemoryManager::flush_expired_data() {
        long flushed = 0;
        flushed += pdflush_lru_list(inactive_list);
        flushed += pdflush_lru_list(active_list);
        return flushed;
    }

    long MemoryManager::evict(long amount) {

        if (amount <= 0) return 0;
        long evicted = 0;

        for (int i=0; i<inactive_list.size(); i++) {

            Block* blk = inactive_list.at(i);

            if (!blk->isDirty()) continue;
            if (evicted + blk->getSize() <= amount) {
                evicted += blk->getSize();
                inactive_list.erase(inactive_list.begin() + i);
                i--;
            } else if (evicted < amount && evicted + blk->getSize() > amount ) {
                blk->setSize(blk->getSize() - amount - evicted);
                // done eviction
                evicted = amount;
                break;
            } else {
                // done eviction
                break;
            }
        }

        cached -= evicted;
        free += evicted;

        return evicted;
    }

    void MemoryManager::read_to_cache(std::string &filename, long amount) {
        // Change stats
        free -= amount;
        cached += amount;
        // Push cached data to inactive list
        inactive_list.push_back(new Block(filename, amount, S4U_Simulation::getClock(), false));
        cache_balance_and_sort();
    }

    /**
     * Simulate a read from cache, re-access and update cached file data
     * @param filename: name of the file read
     */
    void MemoryManager::cache_read(std::string &filename) {

        long dirty_reaccessed = 0;
        long clean_reaccessed = 0;

        // Calculate dirty cached data
        for (int i = 0; i < inactive_list.size(); i++) {
            Block* blk = inactive_list.at(i);
            if (strcmp(blk->getFilename().c_str(), filename.c_str()) != 0) {
                if (blk->isDirty()) {
                    dirty_reaccessed += blk->getSize();
                } else {
                    clean_reaccessed += blk->getSize();
                }
                // remove the existing old block
                inactive_list.erase(inactive_list.begin() + i);
                i--;
            }
        }

        // Calculate clean cached data
        for (int i = 0; i < active_list.size(); i++) {
            Block* blk = active_list.at(i);
            if (strcmp(blk->getFilename().c_str(), filename.c_str()) != 0) {
                if (blk->isDirty()) {
                    dirty_reaccessed += blk->getSize();
                } else {
                    clean_reaccessed += blk->getSize();
                }

                // remove the existing old block
                active_list.erase(active_list.begin() + i);
                i--;
            }
        }

        // create new blocks and put in the active list
        if (clean_reaccessed > 0) {
            Block* new_blk = new Block(filename, clean_reaccessed, S4U_Simulation::getClock(), false);
            active_list.push_back(new_blk);
        }
        if (dirty_reaccessed > 0) {
            Block* new_blk = new Block(filename, dirty_reaccessed, S4U_Simulation::getClock(), true);
            active_list.push_back(new_blk);
        }

        cache_balance_and_sort();
    }

    /**
     * Simulate a file write to cache
     * @param filename: name of the file written
     * @param amount: amount of data written
     */
    void MemoryManager::cache_write(std::string &filename, long amount) {

        Block *blk = new Block(filename, amount, S4U_Simulation::getClock(), true);
        inactive_list.push_back(blk);

        this->cached -= amount;
        this->free -= amount;
        this->dirty += amount;
    }

    bool compare_last_access(Block &blk1,Block &blk2) {
        return blk1.getLastAccess() < blk2.getLastAccess();
    }

    void MemoryManager::cache_balance_and_sort() {
        balance_lru_lists();
        std::sort(active_list.begin(), active_list.end(), compare_last_access);
        std::sort(inactive_list.begin(), inactive_list.end(), compare_last_access);
    }

    void MemoryManager::balance_lru_lists() {

        auto sum = [&] (Block &blk1, Block &blk2) {
            return blk1.getSize() + blk2.getSize();
        };

        long inactive_size = std::accumulate(inactive_list.begin(), inactive_list.end(), 0, sum);
        long active_size = std::accumulate(active_list.begin(), active_list.end(), 0, sum);

        if (active_size > 2 * inactive_size) {

            long to_move_amt = (active_size - inactive_size) / 2;
            long moved_amt = 0;

            for (int i=0; i < active_list.size(); i++) {
                Block* blk = active_list.at(i);

                // move the whole block
                if (to_move_amt - (moved_amt + blk->getSize()) >= 0) {
                    inactive_list.push_back(blk);
                    active_list.erase(active_list.begin() + i);
                    i--;
                } else {
                    // split the block
                    std::string fn = blk->getFilename();
                    Block *new_blk = new Block(fn, to_move_amt - moved_amt, blk->getLastAccess(), blk->isDirty());
                    inactive_list.push_back(new_blk);

                    blk->setSize(blk->getSize() + moved_amt - to_move_amt);

                    // finish moving
                    break;
                }
            }

        }
    }

    s4u_Disk *MemoryManager::get_disk(std::string filename) {
        return nullptr;
    }

}