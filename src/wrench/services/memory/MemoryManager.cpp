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
     * @param interval: the interval that periodical flushing awakes in second
     * @param expired_time: the expired time of dirty data to be flushed in second
     * @param hostname: name of the host on which periodical flushing starts
     */
    MemoryManager::MemoryManager(s4u_Disk *memory, double dirty_ratio,
            int interval, int expired_time, std::string hostname) :
            Service(hostname, "periodic_flush_" + hostname, "periodic_flush_" + hostname),
            memory(memory), dirty_ratio(dirty_ratio), interval(interval), expired_time(expired_time) {}

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
     * Initialize and start a periodical flushing thread
     *
     * @param simulation
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

    const std::vector<Block> &MemoryManager::getInactiveList() const {
        return inactive_list;
    }

    const std::vector<Block> &MemoryManager::getActiveList() const {
        return active_list;
    }

    long MemoryManager::flush(long amount) {
        return 0;
    }

    long MemoryManager::flush_expired_data() {
        return 0;
    }

    long MemoryManager::evict(long amount) {
        return 0;
    }

    void MemoryManager::cache_read(std::string filename) {

    }

    void MemoryManager::cache_write(std::string filename, long amount) {

    }

}