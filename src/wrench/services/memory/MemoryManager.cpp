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

        total = S4U_Simulation::getHostMemoryCapacity(hostname);
        free = total;
        dirty = 0;
        cached = 0;
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
                                                               double dirty_ratio, int interval, int expired_time,
                                                               std::string hostname) {

        std::shared_ptr<MemoryManager> memory_manager_ptr = std::shared_ptr<MemoryManager>(
                new MemoryManager(memory, dirty_ratio, interval, expired_time, hostname));

        memory_manager_ptr->simulation = simulation;

        try {
            memory_manager_ptr->start(memory_manager_ptr, true, false); // Daemonized, no auto-restart
        } catch (std::shared_ptr<HostError> &e) {
            throw;
        }
        return memory_manager_ptr;
    }

    int MemoryManager::main() {

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_MAGENTA);WRENCH_INFO(
                "Periodic Flush starting with interval = %d , expired time = %d",
                this->interval, this->expired_time);

        while (this->getState() == State::UP) {

            double start_time = S4U_Simulation::getClock();
            pdflush();
            double end_time = S4U_Simulation::getClock();

            if (end_time - start_time < interval) {
                S4U_Simulation::sleep(interval + start_time - end_time);
            }
        }

        this->setStateToDown();
        return 0;
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

    double MemoryManager::getFreeMemory() const {
        return free;
    }

    void MemoryManager::setFreeMemory(double free_amt) {
        this->free = free_amt;
    }

    double MemoryManager::getCached() const {
        return cached;
    }

    double MemoryManager::getDirty() const {
        return dirty;
    }

    double MemoryManager::getEvictableMemory() {
        double sum = 0;
        for (int i = 0; i < inactive_list.size(); i++) {
            sum += inactive_list[i]->getSize();
        }
        return sum;
    }

    double MemoryManager::getAvailableMemory() {
        return this->free + this->cached - this->dirty;
    }

    /**
     * Flush dirty data in a LRU list
     * @param list: the LRU list whose data will be flushed
     * @param amount: the amount requested to flush
     * @return flushed amount
     */
    double MemoryManager::flushLruList(std::vector<Block *> &list, double amount, std::string excluded_filename) {

        if (amount <= 0) return 0;
        double flushed = 0;

        std::map<std::string, double> flushing_map;

        for (auto blk : list) {

            if (!excluded_filename.empty() && strcmp(excluded_filename.c_str(), blk->getFilename().c_str()) == 0) {
                continue;
            }

            if (blk->isDirty()) {
                if (flushed + blk->getSize() <= amount) {
                    // flush whole block
                    blk->setDirty(false);
                    flushed += blk->getSize();
                    flushing_map[blk->getMountpoint()] += blk->getSize();
                } else if (flushed < amount && amount < flushed + blk->getSize()) {

                    double blk_flushed = amount - flushed;
                    flushing_map[blk->getMountpoint()] += blk_flushed;
                    flushed = amount;
                    // split
                    blk->setSize(blk->getSize() - blk_flushed);

                    std::string fn = blk->getFilename();
                    inactive_list.push_back(new Block(fn, blk->getMountpoint(), blk_flushed, blk->getLastAccess(),
                                                      false));
                } else {
                    // done flushing
                    break;
                }
            }
        }

        std::vector<simgrid::s4u::IoPtr> io_ptrs;
        for (auto it = flushing_map.begin(); it != flushing_map.end(); it++) {
            s4u_Disk *disk = getDisk(it->first, this->hostname);
            io_ptrs.push_back(disk->write_async(it->second));
        }

        for (int i = 0; i < io_ptrs.size(); i++) {
            io_ptrs[i]->wait();
        }

        dirty -= flushed;

        return flushed;
    }

    /**
     * Flush dirty data from cache
     * @param amount: request amount to be flushed
     * @return flushed amount
     */
    double MemoryManager::flush(double amount, std::string excluded_filename) {
        if (amount <= 0) return 0;

        double flushed_inactive = flushLruList(inactive_list, amount, excluded_filename);

        double flushed_active = 0;
        if (flushed_inactive < amount) {
            flushed_active = flushLruList(active_list, amount - flushed_inactive, excluded_filename);
        }

        return flushed_inactive + flushed_active;
    }

    /**
     * Flush expired dirty data in a list.
     * Expired dirty data is the dirty data not accessed in a period longer than expired_time
     * @param list: the LRU to be flushed
     * @return flushed amount
     */
    double MemoryManager::flushExpiredData(std::vector<Block *> &list) {

        double flushed = 0;

        std::map<std::string, double> flushing_map;

        for (auto blk : list) {
            if (!blk->isDirty()) continue;
            if (S4U_Simulation::getClock() - blk->getLastAccess() >= expired_time) {
                blk->setDirty(false);
                flushed += blk->getSize();
                flushing_map[blk->getMountpoint()] += blk->getSize();
            }
        }

        for (auto it = flushing_map.begin(); it != flushing_map.end(); it++) {
            s4u_Disk *disk = getDisk(it->first, this->hostname);
            disk->write(it->second);
        }

        dirty -= flushed;
        return flushed;
    }

    /**
     * Periodical flushing, which flushes expired dirty data in a list.
     * Expired dirty data is the dirty data not accessed in a period longer than expired_time
     * @return flushed amount
     */
    double MemoryManager::pdflush() {
        double flushed = 0;
        flushed += flushExpiredData(inactive_list);
        flushed += flushExpiredData(active_list);
        if (flushed > 0) {
            WRENCH_INFO("Periodically flushed %lf MB", flushed / 1000000)
        }

        return flushed;
    }

    /**
     * Evicted clean data from cache.
     * @param amount: the requested amount of data to be flushed
     * @return flushed amount
     */
    double MemoryManager::evict(double amount, std::string excluded_filename) {

        if (amount <= 0) return 0;

        double evicted = 0;

        for (int i = 0; i < inactive_list.size(); i++) {

            Block *blk = inactive_list.at(i);

            if (!excluded_filename.empty() && strcmp(excluded_filename.c_str(), blk->getFilename().c_str()) == 0) {
                continue;
            }

            if (!blk->isDirty()) continue;
            if (evicted + blk->getSize() <= amount) {
                evicted += blk->getSize();
                inactive_list.erase(inactive_list.begin() + i);
                i--;
            } else if (evicted < amount && evicted + blk->getSize() > amount) {
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

    /**
     * Read data from disk to cache.
     * @param filename
     * @param amount
     */
    simgrid::s4u::IoPtr MemoryManager::readToCache(std::string filename, std::string mount_point,
                                                   double amount, bool async) {
        // Change stats
        free -= amount;
        cached += amount;
        // Push cached data to inactive list
        inactive_list.push_back(new Block(filename, mount_point, amount, S4U_Simulation::getClock(), false));
        balanceLruLists();

        s4u_Disk *disk = getDisk(mount_point, this->hostname);
        if (async) {
            return disk->read_async(amount);
        } else {
            disk->read(amount);
            return nullptr;
        }
    }

    /**
     * Simulate a read from cache, re-access and update cached file data
     * @param filename: name of the file read
     */
    simgrid::s4u::IoPtr MemoryManager::readFromCache(std::string filename, bool async) {

        double dirty_reaccessed = 0;
        double clean_reaccessed = 0;
        std::string mnt_pt = "";

        // Calculate dirty cached data
        for (int i = 0; i < inactive_list.size(); i++) {
            Block *blk = inactive_list.at(i);
            if (strcmp(blk->getFilename().c_str(), filename.c_str()) == 0) {

                if (mnt_pt.empty()) {
                    mnt_pt = blk->getMountpoint();
                }

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
            Block *blk = active_list.at(i);
            if (strcmp(blk->getFilename().c_str(), filename.c_str()) == 0) {

                if (mnt_pt.empty()) {
                    mnt_pt = blk->getMountpoint();
                }

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
            Block *new_blk = new Block(filename, mnt_pt, clean_reaccessed, S4U_Simulation::getClock(), false);
            active_list.push_back(new_blk);
        }
        if (dirty_reaccessed > 0) {
            Block *new_blk = new Block(filename, mnt_pt, dirty_reaccessed, S4U_Simulation::getClock(), true);
            active_list.push_back(new_blk);
        }

        balanceLruLists();

        return this->memory->read_async(clean_reaccessed + dirty_reaccessed);
    }

    /**
     * Simulate a read from cache, re-access and update cached file data
     * @param filename: name of the file read
     */
    void MemoryManager::readChunkFromCache(std::string filename, double amount) {

        std::string mnt_pt = "";
        double dirty_reaccessed = 0;
        double clean_reaccessed = 0;
        double read = 0;

        // Calculate dirty cached data
        for (int i = 0; i < inactive_list.size(); i++) {

            if (read >= amount) {
                break;
            }

            Block *blk = inactive_list.at(i);
            if (strcmp(blk->getFilename().c_str(), filename.c_str()) == 0) {

                if (mnt_pt.empty()) {
                    mnt_pt = blk->getMountpoint();
                }

                if (read + blk->getSize() <= amount) {
                    read += blk->getSize();
                    if (blk->isDirty()) {
                        dirty_reaccessed += blk->getSize();
                    } else {
                        clean_reaccessed += blk->getSize();
                    }
                    // remove the existing old block
                    inactive_list.erase(inactive_list.begin() + i);
                    i--;
                } else {
                    double blk_read = amount - read;
                    read += blk_read;
                    blk->split(blk->getSize() - blk_read);
                    if (blk->isDirty()) {
                        dirty_reaccessed += blk_read;
                    } else {
                        clean_reaccessed += blk_read;
                    }
                }
            }
        }

        // Calculate clean cached data
        for (int i = 0; i < active_list.size(); i++) {

            if (read >= amount) {
                break;
            }

            Block *blk = active_list.at(i);
            if (strcmp(blk->getFilename().c_str(), filename.c_str()) == 0) {

                if (mnt_pt.empty()) {
                    mnt_pt = blk->getMountpoint();
                }

                if (read + blk->getSize() <= amount) {
                    read += blk->getSize();
                    if (blk->isDirty()) {
                        dirty_reaccessed += blk->getSize();
                    } else {
                        clean_reaccessed += blk->getSize();
                    }

                    active_list.erase(active_list.begin() + i);
                    i--;
                } else {
                    double blk_read = amount - read;
                    read += blk_read;
                    blk->split(blk->getSize() - blk_read);
                    if (blk->isDirty()) {
                        dirty_reaccessed += blk_read;
                    } else {
                        clean_reaccessed += blk_read;
                    }
                }

            }
        }

        // create new blocks and put in the active list
        if (clean_reaccessed > 0) {
            Block *new_blk = new Block(filename, mnt_pt, clean_reaccessed, S4U_Simulation::getClock(), false);
            active_list.push_back(new_blk);
        }
        if (dirty_reaccessed > 0) {
            Block *new_blk = new Block(filename, mnt_pt, dirty_reaccessed, S4U_Simulation::getClock(), true);
            active_list.push_back(new_blk);
        }

        balanceLruLists();

        memory->read(clean_reaccessed + dirty_reaccessed);
    }

    /**
     * Simulate a file write to cache
     * @param filename: name of the file written
     * @param amount: amount of data written
     */
    void MemoryManager::writeToCache(std::string filename, std::string mnt_pt, double amount) {

        Block *blk = new Block(filename, mnt_pt, amount, S4U_Simulation::getClock(), false);
        inactive_list.push_back(blk);

        this->cached -= amount;
        this->free -= amount;
        this->dirty += amount;

        memory->write(amount);
    }

    bool compare_last_access(Block *blk1, Block *blk2) {
        return blk1->getLastAccess() < blk2->getLastAccess();
    }

    /**
     * Balance the amount of data in LRU lists.
     * If the amount of data in the active list is more than doubled of the inactive list,
     * move blocks from the active list to the inactive list to make their sizes equal.
     */
    void MemoryManager::balanceLruLists() {

        double inactive_size = 0;
        for (int i=0; i < inactive_list.size(); i++) {
            inactive_size += inactive_list[i]->getSize();
        }

        double active_size = 0;
        for (int i=0; i < active_list.size(); i++) {
            active_size += active_list[i]->getSize();
        }

        if (active_size > 2 * inactive_size) {

            double to_move_amt = (active_size - inactive_size) / 2;
            double moved_amt = 0;

            for (int i = 0; i < active_list.size(); i++) {
                Block *blk = active_list.at(i);

                // move the whole block
                if (to_move_amt - (moved_amt + blk->getSize()) >= 0) {
                    inactive_list.push_back(blk);
                    active_list.erase(active_list.begin() + i);
                    i--;
                } else {
                    // split the block
                    std::string fn = blk->getFilename();
                    Block *new_blk = new Block(fn, blk->getMountpoint(), to_move_amt - moved_amt, blk->getLastAccess(),
                                               blk->isDirty());
                    inactive_list.push_back(new_blk);

                    blk->setSize(blk->getSize() + moved_amt - to_move_amt);

                    // finish moving
                    break;
                }
            }

        }
    }

    /**
     * Get amount of cached data of a file in cache
     * @param filename: name of the file
     * @return the amount of cached data
     */
    double MemoryManager::getCachedAmount(std::string filename) {

        double amt = 0;

        for (int i = 0; i < inactive_list.size(); i++) {
            if (strcmp(inactive_list[i]->getFilename().c_str(), filename.c_str()) == 0) {
                amt += inactive_list[i]->getSize();
            }
        }

        for (int i = 0; i < active_list.size(); i++) {
            if (strcmp(active_list[i]->getFilename().c_str(), filename.c_str()) == 0) {
                amt += active_list[i]->getSize();
            }
        }

        return amt;
    }

    /**
     * Get list of cached blocks of a file
     * @param filename : name of the file
     * @return a vector of cached blocks of the file
     */
    std::vector<Block *> MemoryManager::getCachedBlocks(std::string filename) {

        std::vector<Block *> block_list;

        for (int i = 0; i < inactive_list.size(); i++) {
            if (strcmp(inactive_list[i]->getFilename().c_str(), filename.c_str()) == 0) {
                block_list.push_back(new Block(inactive_list[i]));
            }
        }
        for (int i = 0; i < active_list.size(); i++) {
            if (strcmp(active_list[i]->getFilename().c_str(), filename.c_str()) == 0) {
                block_list.push_back(new Block(active_list[i]));
            }
        }

        std::sort(block_list.begin(), block_list.end(), compare_last_access);

        return block_list;
    }

    /**
     * Retrieve the disk where the file is stored.
     * @param mountpoint: mountpoint on the disk where the file is stored
     * @return
     */
    s4u_Disk *MemoryManager::getDisk(std::string mountpoint, std::string hostname) {

        mountpoint = FileLocation::sanitizePath(mountpoint);

        auto host = simgrid::s4u::Host::by_name_or_null(hostname);
        if (not host) {
            throw std::invalid_argument("MemoryManager::getDisk: unknown host " + hostname);
        }

        auto disk_list = simgrid::s4u::Host::by_name(hostname)->get_disks();
        for (auto disk : disk_list) {
            std::string disk_mountpoint =
                    FileLocation::sanitizePath(std::string(std::string(disk->get_property("mount"))));
            if (disk_mountpoint == mountpoint) {
                return disk;
            }
        }

        return nullptr;
    }

    void MemoryManager::log(){
        this->time_log.push_back(this->simulation->getCurrentSimulatedDate());
        this->dirty_log.push_back(this->dirty);
        this->cached_log.push_back(this->cached);
        this->free_log.push_back(this->free);
    }

    void MemoryManager::export_log(std::string filename){
        FILE *log_file = fopen(filename.c_str(), "w");
        fprintf(log_file, "time, total_mem, dirty, cache, used_mem\n");

        double start = this->time_log.at(0);
        for (int i=0; i<this->time_log.size(); i++) {
            fprintf(log_file, "%lf, %lf, %lf, %lf, %lf\n",
                    this->time_log.at(i) - start,
                    total,
                    this->dirty_log.at(i),
                    this->cached_log.at(i),
                    total - this->free_log.at(i));
        }

        fclose(log_file);
    }

}