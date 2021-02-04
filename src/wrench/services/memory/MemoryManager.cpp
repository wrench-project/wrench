/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include "wrench/logging/TerminalOutput.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
#include "wrench/workflow/failure_causes/HostError.h"
#include <wrench/services/memory/MemoryManager.h>

WRENCH_LOG_CATEGORY(wrench_periodic_flush, "Log category for Periodic Flush");

namespace wrench {

    /**
     * Constructor
     *
     * @param memory: disk model used to simulate memory_manager_service
     * @param dirty_ratio: dirty_ratio parameter as in the Linux kernel
     * @param interval: the interval that periodical flushing awakes in second
     * @param expired_time: the expired time of dirty data to be flushed in second
     * @param hostname: name of the host on which periodical flushing starts
     */
    MemoryManager::MemoryManager(s4u_Disk *memory, double dirty_ratio,
                                 int interval, int expired_time, std::string hostname) :
            Service(hostname, "page_cache_manager_" + hostname, "page_cache_manager_" + hostname),
            memory(memory), dirty_ratio(dirty_ratio), interval(interval), expired_time(expired_time) {

        // Get RAM disk size
        this->total = S4U_Simulation::getDiskCapacity(hostname, "/memory");
//        this->total = S4U_Simulation::getHostMemoryCapacity(hostname);
        this->free = total;
        this->dirty = 0;
        this->cached = 0;
    }

    /**
     * Initialize and start the memory_manager_service manager
     *
     * @param simulation
     * @param memory: disk model used to simulate memory_manager_service
     * @param dirty_ratio: dirty_ratio parameter as in the Linux kernel
     * @param interval: the interval that periodical flushing awakes in milliseconds
     * @param expired_time: the expired time of dirty data to be flushed in milliseconds
     * @param hostname: name of the host on which periodical flushing starts
     * @return
     */
    std::shared_ptr<MemoryManager> MemoryManager::initAndStart(wrench::Simulation *simulation, s4u_Disk *memory,
                                                               double dirty_ratio, int interval, int expired_time,
                                                               std::string hostname) {

        auto memory_manager = simulation->add(new MemoryManager(memory, dirty_ratio, interval, expired_time, hostname));
        memory_manager->start(memory_manager, true, false);
        return memory_manager;
    }

    int MemoryManager::main() {

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_MAGENTA);
        WRENCH_INFO(
                "Periodic Flush starting with interval = %d , expired time = %d",
                this->interval, this->expired_time);

        while (this->getState() == State::UP) {

            double start_time = S4U_Simulation::getClock();
            double amt = pdflush();
            double end_time = S4U_Simulation::getClock();
            if (amt > 0) { WRENCH_INFO("Periodically flushed %lf MB in %lf, %lf MB dirty data left",
                                       amt / 1000000, end_time - start_time, this->dirty / 1000000)
            }

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

    void MemoryManager::releaseMemory(double released_amt) {
        this->free += released_amt;
        if (this->free > this->total) this->free = this->total;
    }

    void MemoryManager::useAnonymousMemory(double used_amt) {
        this->free -= used_amt;
        if (this->free < 0) this->free = 0;
    }

    double MemoryManager::getTotalCachedAmount() const {
        return cached;
    }

    double MemoryManager::getDirty() const {
        return dirty;
    }

    double MemoryManager::getEvictableMemory() {
        double sum = 0;
        for (unsigned int i = 0; i < inactive_list.size(); i++) {
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

            if (!excluded_filename.empty() && blk->getFileId().compare(excluded_filename) == 0) {
                continue;
            }

            if (blk->isDirty()) {
                if (flushed + blk->getSize() <= amount) {
                    // flush whole block
                    blk->setDirty(false);
                    flushed += blk->getSize();
                    flushing_map[blk->getLocation()->getMountPoint()] += blk->getSize();
                } else if (flushed < amount && amount < flushed + blk->getSize()) {

                    double blk_flushed = amount - flushed;
                    flushing_map[blk->getLocation()->getMountPoint()] += blk_flushed;

                    flushed = amount;
                    // split
                    blk->setSize(blk->getSize() - blk_flushed);

                    std::string fn = blk->getFileId();
                    inactive_list.push_back(new Block(fn, blk->getLocation(), blk_flushed, blk->getLastAccess(),
                                                      false, blk->getDirtyTime()));
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
            disk->write_async(it->second);
        }

        for (unsigned int i = 0; i < io_ptrs.size(); i++) {
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

        if (flushed_inactive > 0) { WRENCH_INFO("Flushed %lf from inactive list", flushed_inactive)
        }
        if (flushed_active > 0) { WRENCH_INFO("Flushed %lf from active list", flushed_active)
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

        while(true) {
            this->acquireDaemonLock();
            Block *block_to_deal_with = nullptr;
            for (auto blk : list) {
                if (!blk->isDirty()) continue;
                if (S4U_Simulation::getClock() - blk->getDirtyTime() < expired_time) continue;
                block_to_deal_with = blk;
                break;
            }
            this->releaseDaemonLock();
            if (block_to_deal_with == nullptr) break;

            block_to_deal_with->setDirty(false);
            flushed += block_to_deal_with->getSize();

            s4u_Disk *disk = getDisk(block_to_deal_with->getLocation()->getMountPoint(), this->hostname);
            disk->write(block_to_deal_with->getSize());

            this->dirty -= block_to_deal_with->getSize();
            flushed += block_to_deal_with->getSize();

        }

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

        for (unsigned int i = 0; i < inactive_list.size(); i++) {

            Block *blk = inactive_list.at(i);

            if (!excluded_filename.empty() && blk->getFileId().compare(excluded_filename) == 0) {
                continue;
            }

            if (blk->isDirty()) continue;
            if (evicted + blk->getSize() <= amount) {
                evicted += blk->getSize();
                inactive_list.erase(inactive_list.begin() + i);
                i--;
            } else if (evicted < amount && evicted + blk->getSize() > amount) {
                blk->setSize(blk->getSize() - amount + evicted);
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
    simgrid::s4u::IoPtr MemoryManager::readToCache(std::string filename, std::shared_ptr<FileLocation> location,
                                                   double amount, bool async) {
        // Change stats
        free -= amount;
        cached += amount;
        // Push cached data to inactive list
        inactive_list.push_back(new Block(filename, location, amount, S4U_Simulation::getClock(), false, 0));
        balanceLruLists();

        s4u_Disk *disk = getDisk(location->getMountPoint(), this->hostname);
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
    void MemoryManager::readChunkFromCache(std::string filename, double amount) {

        std::shared_ptr<FileLocation> location = nullptr;
        double dirty_reaccessed = 0;
        double clean_reaccessed = 0;
        double read = 0;

        for (unsigned int i = 0; i < inactive_list.size(); i++) {

            if (read >= amount) {
                break;
            }

            Block *blk = inactive_list.at(i);
            if (blk->getFileId().compare(filename) == 0) {

                if (location == nullptr) {
                    location = blk->getLocation();
                }

                if (read + blk->getSize() <= amount) {
                    read += blk->getSize();
                    if (blk->isDirty()) {
                        dirty_reaccessed += blk->getSize();
                        this->active_list.push_back(blk);
                    } else {
                        clean_reaccessed += blk->getSize();
                    }
                    // remove the existing old block from inactive list
                    inactive_list.erase(inactive_list.begin() + i);
                    i--;
                } else {
                    double blk_read_amt = amount - read;
                    read += blk_read_amt;
                    Block *read_blk = blk->split(blk->getSize() - blk_read_amt);
                    if (blk->isDirty()) {
                        dirty_reaccessed += blk_read_amt;
                        this->active_list.push_back(read_blk);
                    } else {
                        clean_reaccessed += blk_read_amt;
                    }
                }
            }
        }

        for (unsigned int i = 0; i < active_list.size(); i++) {

            if (read >= amount) {
                break;
            }

            Block *blk = active_list.at(i);
            if (blk->getFileId().compare(filename) == 0) {

                if (location == nullptr) {
                    location = blk->getLocation();
                }

                if (read + blk->getSize() <= amount) {
                    read += blk->getSize();
                    if (blk->isDirty()) {
                        // move the block to the end of the list
                        dirty_reaccessed += blk->getSize();
                        std::rotate(active_list.begin() + i, active_list.begin() + i + 1, active_list.end());
                    } else {
                        // delete to create a new clean block
                        clean_reaccessed += blk->getSize();
                        active_list.erase(active_list.begin() + i);
                    }
                    i--;
                } else {
                    double blk_read_amt = amount - read;
                    read += blk_read_amt;
                    Block *read_blk = blk->split(blk->getSize() - blk_read_amt);
                    if (blk->isDirty()) {
                        dirty_reaccessed += blk_read_amt;
                        this->active_list.push_back(read_blk);
                    } else {
                        clean_reaccessed += blk_read_amt;
                    }
                }

            }
        }

        // create new blocks and put in the active list
        if (clean_reaccessed > 0) {
            Block *new_blk = new Block(filename, location, clean_reaccessed, S4U_Simulation::getClock(), false, 0);
            active_list.push_back(new_blk);
        }
//        if (dirty_reaccessed > 0) {
//            Block *new_blk = new Block(filename, mnt_pt, dirty_reaccessed, S4U_Simulation::getClock(), true);
//            active_list.push_back(new_blk);
//        }

        balanceLruLists();

        memory->read(clean_reaccessed + dirty_reaccessed);
    }

    /**
     * Simulate a file write to cache
     * @param filename: name of the file written
     * @param amount: amount of data written
     */
    void MemoryManager::writebackToCache(std::string filename, std::shared_ptr<FileLocation> location,
                                         double amount, bool is_dirty) {

        this->addToCache(filename, location, amount, is_dirty);
        memory->write(amount);
    }

    void MemoryManager::addToCache(std::string filename, std::shared_ptr<FileLocation> location, double amount, bool is_dirty) {
        Block *bl = new Block(filename, location, amount, S4U_Simulation::getClock(), is_dirty, S4U_Simulation::getClock());
        inactive_list.push_back(bl);

        this->cached += amount;
        this->free -= amount;
        if (is_dirty) {
            this->dirty += amount;
        }
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
        for (unsigned int i=0; i < inactive_list.size(); i++) {
            inactive_size += inactive_list[i]->getSize();
        }

        double active_size = 0;
        for (unsigned int i=0; i < active_list.size(); i++) {
            active_size += active_list[i]->getSize();
        }

        // Active list should not be large then twice the size of the inactive list
        // Balance the lists: make their sizes equal
        if (active_size > 2 * inactive_size) {

            double to_move_amt = (active_size - inactive_size) / 2;
            double moved_amt = 0;

            for (unsigned int i = 0; i < active_list.size(); i++) {
                Block *blk = active_list.at(i);

                // move the whole block
                if (to_move_amt - (moved_amt + blk->getSize()) >= 0) {
                    inactive_list.push_back(blk);
                    active_list.erase(active_list.begin() + i);
                    i--;
                } else {
                    // split the block
                    std::string fn = blk->getFileId();
                    Block *new_blk = new Block(fn, blk->getLocation(), to_move_amt - moved_amt, blk->getLastAccess(),
                                               blk->isDirty(), blk->getDirtyTime());
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

        for (unsigned int i = 0; i < inactive_list.size(); i++) {
            if (inactive_list[i]->getFileId().compare(filename) == 0) {
                amt += inactive_list[i]->getSize();
            }
        }

        for (unsigned int i = 0; i < active_list.size(); i++) {
            if (active_list[i]->getFileId().compare(filename) == 0) {
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

        for (unsigned int i = 0; i < inactive_list.size(); i++) {
            if (inactive_list[i]->getFileId().compare(filename) == 0) {
                block_list.push_back(new Block(inactive_list[i]));
            }
        }
        for (unsigned int i = 0; i < active_list.size(); i++) {
            if (active_list[i]->getFileId().compare(filename) == 0) {
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

    void MemoryManager::log() {
        this->time_log.push_back(this->simulation->getCurrentSimulatedDate());
        this->dirty_log.push_back(this->dirty);
        this->cached_log.push_back(this->cached);
        this->free_log.push_back(this->free);
    }

    void MemoryManager::export_log(std::string filename) {
        FILE *log_file = fopen(filename.c_str(), "w");
        fprintf(log_file, "time, total_mem, dirty, cache, used_mem\n");

        double start = this->time_log.at(0);
        for (unsigned int i=0; i<this->time_log.size(); i++) {
            fprintf(log_file, "%lf, %lf, %lf, %lf, %lf\n",
                    this->time_log.at(i) - start,
                    total / 1000000.0,
                    this->dirty_log.at(i) / 1000000.0,
                    this->cached_log.at(i) / 1000000.0,
                    (total - this->free_log.at(i)) / 1000000.0);
        }

        fclose(log_file);
    }

}
