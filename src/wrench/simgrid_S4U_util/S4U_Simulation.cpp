/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include <iostream>
#include <set>
#include <cfloat>
#include <wrench/services/compute/ComputeService.h>
#include <wrench/util/UnitParser.h>
#include <simgrid/plugins/energy.h>
#include <wrench/workflow/execution_events/FailureCause.h>
#include "wrench/logging/TerminalOutput.h"

#include "wrench/simgrid_S4U_util/S4U_Simulation.h"

WRENCH_LOG_NEW_DEFAULT_CATEGORY(s4u_simulation, "Log category for S4U_Simulation");

#include <simgrid/plugins/file_system.h>


namespace wrench {

    /**
     * @brief Initialize the Simgrid simulation
     *
     * @param argc: the C-style argument counts
     * @param argv: the C-style argument list
     */
    void S4U_Simulation::initialize(int *argc, char **argv) {
        this->engine = new simgrid::s4u::Engine(argc, argv);
        this->initialized = true;
        sg_storage_file_system_init();
    }

    /**
     * @brief Returns true if S4U_Simulation::initialize() has been called successfully previously
     *
     * @return true or false
     */
    bool S4U_Simulation::isInitialized() {
        return this->initialized;
    }

    /**
     * @brief Returns true if S4U_Simulation::setupPlatform() has been called successfully previously
     * @return true or false
     */
    bool S4U_Simulation::isPlatformSetup() {
        return this->platform_setup;
    }

    /**
     * @brief Start the simulation
     *
     * @throw std::runtime_error
     */
    void S4U_Simulation::runSimulation() {
        if (this->initialized) {
            this->engine->run();
        } else {
            throw std::runtime_error("S4U_Simulation::runSimulation(): Simulation has not been initialized");
        }
    }

    /**
     * @brief Shutdown the simulation
     */
    void S4U_Simulation::shutdown() {
        if (this->initialized) {
            this->engine->shutdown();
        }
    }

    /**
     * @brief Initialize the simulated platform. Must only be called once. Due to
     *        the current SimGrid implementation, if the file is not readable or
     *        not correctly formatted, this method will call exit() instead of throwing
     *        an exception.
     *
     * @param filename: the path to an XML platform description file
     */
    void S4U_Simulation::setupPlatform(std::string &filename) {

        // TODO: One day, perhaps SimGrid will throw an exception here...
        this->engine->load_platform(filename);
        this->platform_setup = true;
    }

    /**
     * @brief Get the hostname on which the calling actor is running
     *
     * @return the hostname as a string
     */
    std::string S4U_Simulation::getHostName() {
        return simgrid::s4u::Host::current()->get_name();
    }

    /**
     * @brief Get the list of hostnames
     *
     * @return a vector of hostnames
     */
    std::vector<std::string> S4U_Simulation::getAllHostnames() {
        std::vector<simgrid::s4u::Host *> host_list = simgrid::s4u::Engine::get_instance()->get_all_hosts();
        std::vector<std::string> hostname_list;
        hostname_list.reserve(host_list.size());
        for (auto h : host_list) {
            hostname_list.push_back(h->get_name());
        }
        return hostname_list;
    }

    /**
     * @brief Get the by-cluster structure of the platform
     * @return a map of all cluster names and their associated hostname list
     */
    std::map<std::string, std::vector<std::string>> S4U_Simulation::getAllHostnamesByCluster() {
        std::map<std::string, std::vector<std::string>> result;
        std::vector<simgrid::kernel::routing::ClusterZone *> clusters;

        auto simgrid_engine = simgrid::s4u::Engine::get_instance();

        clusters = simgrid_engine->get_filtered_netzones<simgrid::kernel::routing::ClusterZone>();
        for (auto c : clusters) {
            std::vector<simgrid::s4u::Host *> host_list = c->get_all_hosts();
            std::vector<std::string> hostname_list;
            hostname_list.reserve(host_list.size());
            for (auto h : host_list) {
                hostname_list.push_back(std::string(h->get_cname()));
            }
            result.insert({c->get_name(), hostname_list});
        }

        return result;
    }

    /**
     * @brief Determines whether a host exists for a given hostname
     * @param hostname: the name of the host
     * @return true or false
     */
    bool S4U_Simulation::hostExists(std::string hostname) {
        return (simgrid::s4u::Host::by_name_or_null(hostname) != nullptr);
    }

    /**
     * @brief Get the number of cores of a host
     *
     * @param hostname: the name of the host
     * @return the number of cores of the host
     *
     * @throw std::invalid_argument
     */
    unsigned int S4U_Simulation::getHostNumCores(std::string hostname) {
        auto host = simgrid::s4u::Host::by_name_or_null(hostname);
        if (host == nullptr) {
            throw std::invalid_argument("Unknown hostname " + hostname);
        }
        return (unsigned int) host->get_core_count();
    }

    /**
     * @brief Get the number of cores of the current host
     * @return a number of cores
     */
    unsigned int S4U_Simulation::getNumCores() {
        return simgrid::s4u::Host::current()->get_core_count();
    }

    /**
     * @brief Get the flop rate of a host
     *
     * @param hostname: the name of the host
     * @return the flop rate in floating point operations per second
     *
     * @throw std::invalid_argument
     */
    double S4U_Simulation::getHostFlopRate(std::string hostname) {
        auto host = simgrid::s4u::Host::by_name_or_null(hostname);
        if (host == nullptr) {
            throw std::invalid_argument("Unknown hostname " + hostname);
        }
        return host->get_speed();
    }

    /**
     * @brief Returns whether a host is on or not
     *
     * @param hostname: the name of the host
     * @return true or false
     *
     * @throw std::invalid_argument
     */
    bool S4U_Simulation::isHostOn(std::string hostname) {
        auto host = simgrid::s4u::Host::by_name_or_null(hostname);
        if (host == nullptr) {
            throw std::invalid_argument("Unknown hostname " + hostname);
        }
        return host->is_on();
    }

    /**
     * @brief Turn off a host
     *
     * @param hostname: the name of the host to turn off
     *
     * @throw std::invalid_argument
     */
    void S4U_Simulation::turnOffHost(std::string hostname) {
        auto host = simgrid::s4u::Host::by_name_or_null(hostname);
        if (host == nullptr) {
            throw std::invalid_argument("Unknown hostname " + hostname);
        }
        host->turn_off();
    }

    /**
     * @brief Turn on a host
     *
     * @param hostname: the name of the host to turn on
     *
     * @throw std::invalid_argument
     */
    void S4U_Simulation::turnOnHost(std::string hostname) {
        auto host = simgrid::s4u::Host::by_name_or_null(hostname);
        if (host == nullptr) {
            throw std::invalid_argument("Unknown hostname " + hostname);
        }
        host->turn_on();
    }

    /**
    * @brief Returns whether a link is on or not
    *
    * @param linkname: the name of the link
    * @return true or false
    *
    * @throw std::invalid_argument
    */
    bool S4U_Simulation::isLinkOn(std::string linkname) {
        auto link = simgrid::s4u::Link::by_name_or_null(linkname);
        if (link == nullptr) {
            throw std::invalid_argument("Unknown linkname " + linkname);
        }
        return link->is_on();
    }

    /**
     * @brief Turn off a link
     *
     * @param linkname: the name of the link to turn off
     *
     * @throw std::invalid_argument
     */
    void S4U_Simulation::turnOffLink(std::string linkname) {
        auto link = simgrid::s4u::Link::by_name_or_null(linkname);
        if (link == nullptr) {
            throw std::invalid_argument("Unknown linkname " + linkname);
        }
        link->turn_off();
    }

    /**
     * @brief Turn on a link
     *
     * @param linkname: the name of the link to turn on
     *
     * @throw std::invalid_argument
     */
    void S4U_Simulation::turnOnLink(std::string linkname) {
        auto link = simgrid::s4u::Link::by_name_or_null(linkname);
        if (link == nullptr) {
            throw std::invalid_argument("Unknown linkname " + linkname);
        }
        link->turn_on();
    }

    /**
     * @brief Get the flop rate of the current host
     *
     * @return the flop rate in floating point operations per second
     *
     * @throw std::invalid_argument
     */
    double S4U_Simulation::getFlopRate() {
        return simgrid::s4u::Host::current()->get_speed(); // changed it to speed of the current pstate
    }

    /**
     * @brief Get the current simulation date
     *
     * @return the simulation clock
     */
    double S4U_Simulation::getClock() {
        return simgrid::s4u::Engine::get_clock();
    }

    /**
     * @brief Simulates a computation on host on which the calling actor is running
     * @param flops: the number of flops
     */
    void S4U_Simulation::compute(double flops) {
        simgrid::s4u::this_actor::execute(flops);
    }

    /**
     * @brief Simulates a disk write
     * @param num_bytes: bumber of written bytes
     * @param partition_name: partition name
     */
    void S4U_Simulation::writeToDisk(double num_bytes, std::string partition_name) {
        // TODO: Change this once I/O in SimGrid is better: the data transfer rate
        // should be based on the I/O device associated to the partition
        // Right now, this takes ZERO time!!!
    }

    /**
     * @brief Simulates a disk read
     * @param num_bytes: bumber of read bytes
     * @param partition_name: partition name
     */
    void S4U_Simulation::readFromDisk(double num_bytes, std::string partition_name) {
        // TODO: Change this once I/O in SimGrid is better: the data transfer rate
        // should be based on the I/O device associated to the partition
        // Right now, this takes ZERO time!!!
    }



    /**
     * @brief Simulates a sleep
     * @param duration: the number of seconds to sleep
     */
    void S4U_Simulation::sleep(double duration) {
        simgrid::s4u::this_actor::sleep_for(duration);
    }

    /**
     * @brief Simulates a yield
     */
    void S4U_Simulation::yield() {
        simgrid::s4u::this_actor::yield();
    }

    /**
     * @brief Get the memory capacity of a host given a hostname
     * @param hostname: the name of the host
     * @return a memory capacity in bytes
     */
    double S4U_Simulation::getHostMemoryCapacity(std::string hostname) {
        auto host = simgrid::s4u::Host::by_name_or_null(hostname);
        if (host == nullptr) {
            throw std::invalid_argument("Unknown hostname " + hostname);
        }
        return getHostMemoryCapacity(host);
    }

    /**
     * @brief Get the memory capacity of the current host
     * @return a memory capacity in bytes
     */
    double S4U_Simulation::getMemoryCapacity() {
        return getHostMemoryCapacity(simgrid::s4u::Host::current());
    }

    /**
     * @brief Get the memory capacity of a S4U host
     * @param host: the host
     * @return a memory capacity in bytes
     */
    double S4U_Simulation::getHostMemoryCapacity(simgrid::s4u::Host *host) {
        std::set<std::string> tags = {"mem", "Mem", "MEM", "ram", "Ram", "RAM", "memory", "Memory", "MEMORY"};
        double capacity_value = S4U_Simulation::DEFAULT_RAM;

        for (auto const &tag : tags) {
            const char *capacity_string = host->get_property(tag);
            if (capacity_string) {
                if (capacity_value != S4U_Simulation::DEFAULT_RAM) {
                    throw std::invalid_argument(
                            "S4U_Simulation::getHostMemoryCapacity(): Host '" + std::string(host->get_cname()) +
                            "' has multiple memory capacity specifications");
                }
                try {
                    capacity_value = UnitParser::parse_size(capacity_string);
                } catch (std::invalid_argument &e) {
                    throw std::invalid_argument(
                            "S4U_Simulation::getHostMemoryCapacity(): Host '" + std::string(host->get_cname()) +
                            "'has invalid memory capacity specification '" + tag + ":" +
                            std::string(capacity_string) + "'");
                }
            }
        }
        return capacity_value;
    }

    /**
     * @brief Get the property associated to the host specified in the platform file
     * @param hostname: the host name
     * @param property_name: the property name
     * @return a string relating to the property specified in the platform file
     */
    std::string S4U_Simulation::getHostProperty(std::string hostname, std::string property_name) {
        auto host = simgrid::s4u::Host::by_name_or_null(hostname);
        if (host == nullptr) {
            throw std::invalid_argument("Unknown hostname " + hostname);
        }
        if (host->get_properties()->find(property_name) == host->get_properties()->end()) {
            throw std::invalid_argument("Unknown property " + property_name);
        }
        return host->get_property(property_name);
    }

    /**
     * @brief Get the energy consumed by the host up to now
     * @param hostname: the host name
     * @return the energy consumed by the host in Joules
     * @throw std::runtime_error
     */
    double S4U_Simulation::getEnergyConsumedByHost(const std::string &hostname) {
        double energy_consumed = 0;
        try {
            energy_consumed = sg_host_get_consumed_energy(simgrid::s4u::Host::by_name(hostname));
        } catch (std::exception &e) {
            throw std::runtime_error(
                    "S4U_Simulation::getEnergyConsumedByHost(): Was not able to get the energy consumed by the host. Make sure energy plugin is enabled and "
                    "the host name is correct"
            );
        }
        return energy_consumed;
    }

#if 0
    /**
     * @brief Get the total energy consumed by a set of hosts
     * @param hostnames: the list of hostnames
     * @return The total energy consumed by all the hosts in Joules
     * @throw std::runtime_error
     */
    double S4U_Simulation::getTotalEnergyConsumed(const std::vector<std::string> &hostnames) {
        double total_energy = 0;
        try {
            for (auto hostname: hostnames) {
                total_energy += sg_host_get_consumed_energy(simgrid::s4u::Host::by_name(hostname));
            }
        } catch (std::exception &e) {
            throw std::runtime_error(
                    "S4U_Simulation::getTotalEnergyConsumed(): Was not able to get the total energy consumed by the host. Make sure energy plugin is enabled and "
                    "the host name is correct"
            );
        }
        return total_energy;
    }
#endif

    /**
     * @brief Set the power state of the host
     *
     * @param hostname: the host name
     * @param pstate: the power state index (the power state index is specified in the platform xml description file)
     * @throw std::runtime_error
     */
    void S4U_Simulation::setPstate(const std::string &hostname, int pstate) {
        try {
            simgrid::s4u::Host::by_name(hostname)->set_pstate(pstate);
        } catch (std::exception &e) {
            throw std::runtime_error(
                    "S4U_Simulation::setPstate(): Was not able to set the pstate of the host. Make sure energy is plugin is enabled and "
                    "the host name is correct and the pstate is within range of pstates available to the host"
            );
        }
    }

    /**
     * @brief Get the total number of power states of a host
     * @param hostname: the host name
     * @return The number of power states available for the host (as specified in the platform xml description file)
     * @throw std::runtime_error
     */
    int S4U_Simulation::getNumberofPstates(const std::string &hostname) {
        try {
            return simgrid::s4u::Host::by_name(hostname)->get_pstate_count();
        } catch (std::exception &e) {
            throw std::runtime_error(
                    "S4U_Simulation::getNumberofPstates():: Was not able to get the energy consumed by the host. Make sure energy plugin is enabled and "
                    "the host name is correct"
            );
        }
    }

    /**
     * @brief Get the current power state of a host
     * @param hostname: the host name
     * @return The index of the current pstate of the host (as specified in the platform xml description file)
     * @throw std::runtime_error
     */
    int S4U_Simulation::getCurrentPstate(const std::string &hostname) {
        try {
            return simgrid::s4u::Host::by_name(hostname)->get_pstate();
        } catch (std::exception &e) {
            throw std::runtime_error(
                    "S4U_Simulation::getCurrentPstate(): Was not able to get the number of pstates of the host. Make sure energy plugin is enabled and "
                    "the host name is correct"
            );
        }
    }

    /**
     * @brief Get the minimum power consumption (i.e., idling) for a host at its current pstate
     * @param hostname: the host name
     * @return The power consumption for this host if idle (as specified in the platform xml description file)
     * @throw std::runtime_error
     */
    double S4U_Simulation::getMinPowerConsumption(const std::string &hostname) {
        try {
            return sg_host_get_wattmin_at(simgrid::s4u::Host::by_name(hostname),
                                          (simgrid::s4u::Host::by_name(hostname))->get_pstate());
        } catch (std::exception &e) {
            throw std::runtime_error(
                    "S4U_Simulation::getMinPowerConsumption(): Was not able to get the min power available to the host. Make sure energy plugin is enabled and "
                    "the host name is correct"
            );
        }
    }

    /**
     * @brief Get the maximum power consumption (i.e., 100% load) for a host at its current pstate
     * @param hostname: the host name
     * @return The power consumption for this host if 100% used (as specified in the platform xml description file)
     * @throw std::runtime_error
     */
    double S4U_Simulation::getMaxPowerConsumption(const std::string &hostname) {
        try {
            return sg_host_get_wattmax_at(simgrid::s4u::Host::by_name(hostname),
                                          (simgrid::s4u::Host::by_name(hostname))->get_pstate());
        } catch (std::exception &e) {
            throw std::runtime_error(
                    "S4U_Simulation::getMaxPowerConsumption():: Was not able to get the max power possible for the host. Make sure energy is plugin is enabled and "
                    "the host name is correct"
            );
        }
    }

    /**
     * @brief Get the list of power states available for a host
     * @param hostname: the host name
     * @return a list of power states available for the host (as specified in the platform xml description file)
     * @throw std::runtime_error
     */
    std::vector<int> S4U_Simulation::getListOfPstates(const std::string &hostname) {
        std::vector<int> list = {};
        try {
            int num_pstates = getNumberofPstates(hostname);
            for (int i = 0; i < num_pstates; i++) {
                list.push_back(i);
            }
        } catch (std::exception &e) {
            throw std::runtime_error(
                    "S4U_Simulation::getListOfPstates(): Was not able to get the list of pstates for the host. Make sure energy plugin is enabled and "
                    "the host name is correct"
            );
        }
        return list;
    }

    /**
     * @brief Compute zero flop, which take zero time but will block if the host's pstate
     *        has a zero flop/sec speed, until the host's pstate is changed to a pstate with
     *        non-zero flop/sec speed.
     */
    void S4U_Simulation::computeZeroFlop() {
        if (S4U_Simulation::getFlopRate() <= 0) {
            S4U_Simulation::compute(0);
        }
    }


    /**
     * @brief Gets set of disks, i.e., mount points, available at a host
     * @param hostname: the host's name
     * @return a set of mount points
     *
     * @htrow std::invalid_argument
     */
    std::set<std::string> S4U_Simulation::getDisks(std::string hostname) {

        simgrid::s4u::Host *host;
        try {
            host = simgrid::s4u::Host::by_name(hostname);
        } catch (std::exception &e) {
            throw std::invalid_argument("S4U_Simulation::getDisks(): Unknown host " + hostname);
        }

        std::set<std::string> mount_points;
        for (auto const &d : host->get_disks()) {
            // Get the disk's mount point
            const char *p = d->get_property("mount");
            if (!p) {
                p = "/";
            }
            std::string mount_point = std::string(p);
            if (mount_points.find(mount_point) != mount_points.end()) {
                throw std::invalid_argument("S4U_Simulation::getDisks(): At host " + hostname +
                " it seems that several disks share the same mount point (or have the same default '/' mount point)."
                "You likely need to fix your platform XML");
            }
            mount_points.insert(mount_point);
        }

        return mount_points;
    }

    /**
     * @brief Determines whether a mount point is defined at a host
     * @param hostname: the host's name
     * @param mount_point: the mount point
     * @return true if the host has a disk attached to the specified mount point, false otherwise
     */
    bool S4U_Simulation::hostHasDisk(std::string hostname, std::string mount_point) {

        simgrid::s4u::Host *host;
        try {
            host = simgrid::s4u::Host::by_name(hostname);
        } catch (std::exception &e) {
            throw std::invalid_argument("S4U_Simulation::getDisks(): Unknown host " + hostname);
        }

        std::set<std::string> mount_points;
        for (auto const &d : host->get_disks()) {
            // Get the disk's mount point
            const char *p = d->get_property("mount");
            if (!p) {
                p = "/";
            }
            if (std::string(p) == mount_point) {
                return true;
            }
        }

        return false;
    }


    /**
     * @brief Gets the capacity of a disk attached to some host for a given mount point
     * @param hostname: the host's name
     * @param mount_point: the mount point (e.g.,  "/home")
     * @return the capacity of the disk / mount point
     *
     * @throw std::invalid_argument
     */
    double S4U_Simulation::getDiskCapacity(std::string hostname, std::string mount_point) {

        simgrid::s4u::Host *host;
        try {
            host = simgrid::s4u::Host::by_name(hostname);
        } catch (std::exception &e) {
            throw std::invalid_argument("S4U_Simulation::getDiskCapacity(): Unknown host " + hostname);
        }

        for (auto const &d : host->get_disks()) {

            // Get the disk's mount point
            const char *mp = d->get_property("mount");
            if (!mp) {
                mp = "/";
            }

            // This is not the mount point you're looking for
            if (strcmp(mp, mount_point.c_str()) != 0) {
                continue;
            }

            double capacity;
            const char *capacity_str = d->get_property("size");

            if (capacity_str) {
                try {
                    capacity = UnitParser::parse_size(capacity_str);
                } catch (std::invalid_argument &e) {
                    throw std::invalid_argument("S4U_Simulation::getDiskCapacity(): Disk " + d->get_name() +
                                                " at host " + hostname + " has invalid size");
                }
            } else {
                capacity = DBL_MAX; // Default size if no size property specified
            }

            return capacity;
        }

        throw std::invalid_argument("S4U_Simulation::getDiskCapacity(): Unknown mount point " + mount_point);
    }



};
