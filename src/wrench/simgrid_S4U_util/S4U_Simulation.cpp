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
#include <climits>
#include <wrench/util/UnitParser.h>
#include <simgrid/plugins/energy.h>
#include <simgrid/plugins/file_system.h>
#include <wrench/failure_causes/FailureCause.h>
#include <wrench/simgrid_S4U_util/S4U_VirtualMachine.h>
#include <wrench/simgrid_S4U_util/S4U_CommPort.h>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/simulation/Simulation.h>

#include <simgrid/version.h>

#include <wrench/simgrid_S4U_util/S4U_Simulation.h>
#include "smpi/smpi.h"


WRENCH_LOG_CATEGORY(wrench_core_s4u_simulation, "Log category for S4U_Simulation");

namespace wrench {

    // simgrid::s4u::MutexPtr S4U_Simulation::global_lock;

    /**
     * @brief Initialize the Simgrid simulation
     *
     * @param argc: the C-style argument counts
     * @param argv: the C-style argument list
     */
    void S4U_Simulation::initialize(int *argc, char **argv) {
        this->engine = new simgrid::s4u::Engine(argc, argv);
        if (not Simulation::isSurfPrecisionSetByUser()) {
            simgrid::s4u::Engine::set_config("precision/timing:1e-9");
        }


        // Create the commport pool
        S4U_CommPort::createCommPortPool();
        S4U_CommPort::NULL_COMMPORT = new S4U_CommPort();
        this->initialized = true;

        // this->global_lock = simgrid::s4u::Mutex::create();
    }

    /**
     * @brief Enable the use of SMPI
     */
    void S4U_Simulation::enableSMPI() {
        static bool enabled = false;

        if (not enabled) {
            // Set the SMPI options
            simgrid::s4u::Engine::set_config("smpi/simulate-computation:no");
            simgrid::s4u::Engine::set_config("smpi/host-speed:1000Gf");
            simgrid::s4u::Engine::set_config("smpi/init:0");
            simgrid::s4u::Engine::set_config("smpi/shared-malloc:global");
            // Initialize SMPI
            SMPI_init();
        }
    }

    /**
     * @brief Returns true if S4U_Simulation::initialize() has been called successfully previously
     *
     * @return true or false
     */
    bool S4U_Simulation::isInitialized() const {
        return this->initialized;
    }

    /**
     * @brief Returns true if S4U_Simulation::setupPlatform() has been called successfully previously
     * @return true or false
     */
    bool S4U_Simulation::isPlatformSetup() const {
        return this->platform_setup;
    }

    /**
     * @brief Start the simulation
     *
     */
    void S4U_Simulation::runSimulation() {
        // Setup a handler for deadlocks
        simgrid::s4u::Engine::on_deadlock_cb([]() {
            throw std::runtime_error("The simulation has deadlocked."
                                     " Run with logs enabled (e.g., --wrench-full-log) "
                                     "to see the SimGrid-generated transcript that describes "
                                     "the current state of all deadlocked actors.");
        });

        if (this->initialized) {
            this->engine->run();
            S4U_CommPort::all_commports.clear();
            if (SMPI_is_inited()) {
                SMPI_finalize();
            }
        } else {
            throw std::runtime_error("S4U_Simulation::runSimulation(): Simulation has not been initialized");
        }
    }

    /**
     * @brief Shutdown the simulation
     */
    void S4U_Simulation::shutdown() const {
        if (this->initialized) {
            //            this->engine->shutdown();
        }
    }

    /**
     * @brief Method to check that all link bandwidths are >0
     */
    void S4U_Simulation::checkLinkBandwidths() const
    {
        auto links = this->engine->get_all_links();
        for (auto const &l: links) {
            if (l->get_bandwidth() <= 0) {
                throw std::invalid_argument("XML Platform error: link " +
                                            l->get_name() + " has zero bandwidth");
            }
        }
    }

    /**
     * @brief Initialize the simulated platform. Must only be called once.
     *
     * @param filename the path to a .xml XML platform description file or a .so/.dylib shared object platform file
     *
     */
    void S4U_Simulation::setupPlatformFromFile(const std::string &filename) {
        try {
            this->engine->load_platform(filename);
        } catch (simgrid::ParseError &e) {
            throw std::invalid_argument("Platform description file error: " + std::string(e.what()));
        } catch (std::invalid_argument &) {
            throw;
        }

        this->platform_setup = true;
    }


    /**
     * @brief Initialize the simulated platform. Must only be called once.
     *
     * @param creation_function void() function to create the platform
     *
     */
    void S4U_Simulation::setupPlatformFromLambda(const std::function<void()> &creation_function) {
        creation_function();
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
     * @brief Get the list of physical hostnames
     *
     * @return a vector of hostnames
     */
    std::vector<std::string> S4U_Simulation::getAllHostnames() {
        std::vector<simgrid::s4u::Host *> host_list = simgrid::s4u::Engine::get_instance()->get_all_hosts();
        std::vector<std::string> hostname_list;
        for (auto h: host_list) {
            // Ignore VMs!
            if (S4U_VirtualMachine::vm_to_pm_map.find(h->get_name()) != S4U_VirtualMachine::vm_to_pm_map.end()) {
                continue;
            }
            hostname_list.push_back(h->get_name());
        }
        return hostname_list;
    }

    /**
     * @brief Get the list of link names
     *
     * @return a vector of link names
     */
    std::vector<std::string> S4U_Simulation::getAllLinknames() {
        std::vector<simgrid::s4u::Link *> link_list = simgrid::s4u::Engine::get_instance()->get_all_links();
        std::vector<std::string> link_name_list;
        link_name_list.reserve(link_list.size());
        for (auto h: link_list) {
            link_name_list.push_back(h->get_name());
        }
        return link_name_list;
    }

    /**
     * @brief Get a link's bandwidth
     * @param name: the link's name
     *
     * @return a bandwidth in Bps
     */
    double S4U_Simulation::getLinkBandwidth(const std::string &name) {
        auto links = simgrid::s4u::Engine::get_instance()->get_filtered_links(
                [name](simgrid::s4u::Link *l) { return (l->get_name() == name); });
        if (links.empty()) {
            throw std::invalid_argument("S4U_Simulation::getLinkBandwidth(): unknown link " + name);
        }
        return links.at(0)->get_bandwidth();
    }
    /**
     * @brief Set a link's new bandwidth
     * @param name: the link's name
     * @param bandwidth: the new bandwidth
     *
     */
    void S4U_Simulation::setLinkBandwidth(const std::string &name, double bandwidth) {
        auto links = simgrid::s4u::Engine::get_instance()->get_filtered_links(
                [name](simgrid::s4u::Link *l) { return (l->get_name() == name); });
        if (links.empty()) {
            throw std::invalid_argument("S4U_Simulation::setLinkBandwidth(): unknown link " + name);
        }
        links.at(0)->set_bandwidth(bandwidth);
    }
    /**
     * @brief Get a link's bandwidth usage
     * @param name: the link's name
     *
     * @return a bandwidth usage in Bps
     */
    double S4U_Simulation::getLinkUsage(const std::string &name) {
        auto links = simgrid::s4u::Engine::get_instance()->get_filtered_links(
                [name](simgrid::s4u::Link *l) { return (l->get_name() == name); });
        if (links.empty()) {
            throw std::invalid_argument("S4U_Simulation::getLinkUsage(): unknown link " + name);
        }
        return links.at(0)->get_load();
    }

    /**
 * @brief Get the list of hostnames in each ClusterZone in the platform (<cluster/> XML tag in the platform XML description)
 * @return a map of all cluster ids and lists of hostnames
 */
    std::map<std::string, std::vector<std::string>> S4U_Simulation::getAllHostnamesByCluster() {
        std::map<std::string, std::vector<std::string>> result;
        auto simgrid_engine = simgrid::s4u::Engine::get_instance();
        S4U_Simulation::traverseAllNetZonesRecursive(simgrid_engine->get_netzone_root(), result, false, false, false, true);
        return result;
    }

    /**
 * @brief Get the list of hostnames in each NetZone (<zone> and <cluster> tags in the platform XML description).
 *        Note that this method does not recurse into sub-zones, so it only returns the hosts that are declared
 *        directly under the <zone> and or <cluster> tags.
 * @return a map of all zone ids and lists of hostnames
 */
    std::map<std::string, std::vector<std::string>> S4U_Simulation::getAllHostnamesByZone() {
        std::map<std::string, std::vector<std::string>> result;
        auto simgrid_engine = simgrid::s4u::Engine::get_instance();
        S4U_Simulation::traverseAllNetZonesRecursive(simgrid_engine->get_netzone_root(), result, false, false, true, false);
        return result;
    }


    /**
    * @brief Get the list of ids of all (sub-)zones in the platform within each zone (<zone/> XML tag in the platform XML description)
    * @return a map of zone ids and the list of (sub-)zone ids
    */
    std::map<std::string, std::vector<std::string>> S4U_Simulation::getAllSubZoneIDsByZone() {
        std::map<std::string, std::vector<std::string>> result;
        auto simgrid_engine = simgrid::s4u::Engine::get_instance();
        S4U_Simulation::traverseAllNetZonesRecursive(simgrid_engine->get_netzone_root(), result, true, false, false, false);
        return result;
    }

    /**
    * @brief Get the list of ids of all ClusterZone in the platform within each zone (<cluster/> XML tag in the platform XML description)
    * @return a map of zone ids and the list of cluster ids
    */
    std::map<std::string, std::vector<std::string>> S4U_Simulation::getAllClusterIDsByZone() {
        std::map<std::string, std::vector<std::string>> result;
        auto simgrid_engine = simgrid::s4u::Engine::get_instance();
        S4U_Simulation::traverseAllNetZonesRecursive(simgrid_engine->get_netzone_root(), result, false, true, false, false);
        return result;
    }


    /**
     * @brief A method to recursively explore sub-zones
     * @param nz: the root netzone
     * @param result: a reference to a map that's being incrementally built recursively
     * @param get_subzones: true if this should return (sub-)zones
     * @param get_clusters: true if this should return (sub-)clusters
     * @param get_hosts_from_zones: true if this should return hostnames from zones
     * @param get_hosts_from_clusters: true if this should return hostnames from clusters
     */
    void S4U_Simulation::traverseAllNetZonesRecursive(simgrid::s4u::NetZone *nz, std::map<std::string, std::vector<std::string>> &result, bool get_subzones, bool get_clusters, bool get_hosts_from_zones, bool get_hosts_from_clusters) {
        //        std::cerr << "PROCESSING ROOT " << nz->get_name() << "\n";
        for (auto const &child: nz->get_children()) {
            //            std::cerr << "PROCESSING CHILD " << child->get_name() << "\n";
            bool is_cluster = dynamic_cast<simgrid::kernel::routing::ClusterZone *>(child->get_impl());
            bool is_zone = not is_cluster;
            if ((is_cluster and get_clusters) or (get_subzones and is_zone)) {
                if (result.find(nz->get_name()) == result.end()) {
                    result[nz->get_name()] = {};
                }
                result[nz->get_name()].push_back(child->get_name());
            } else if ((get_hosts_from_clusters and is_cluster) or (get_hosts_from_zones and is_zone)) {
                std::vector<std::string> hosts;
                for (auto const &h: child->get_all_hosts()) {
                    if (h->get_englobing_zone() == child) {
                        hosts.push_back(h->get_name());
                    }
                }
                if (not hosts.empty()) {
                    result[child->get_name()] = hosts;
                }
            }
            if (is_zone) {
                S4U_Simulation::traverseAllNetZonesRecursive(child, result, get_subzones, get_clusters, get_hosts_from_zones, get_hosts_from_clusters);
            } else {
                continue;
            }
        }
    }


    /**
 * @brief Determines whether a host exists for a given hostname (does not include VMs)
 * @param hostname: the name of the host
 * @return true or false
 */
    bool S4U_Simulation::hostExists(const std::string &hostname) {
        return (simgrid::s4u::Host::by_name_or_null(hostname) != nullptr);
    }

    /**
 * @brief Determines whether a link exists for a given link_name
 * @param link_name: the name of the link
 * @return true or false
 */
    bool S4U_Simulation::linkExists(const std::string &link_name) {
        return (simgrid::s4u::Link::by_name_or_null(link_name) != nullptr);
    }

    /**
 * @brief Get the number of cores of a host
 *
 * @param hostname: the name of the host
 * @return the number of cores of the host
 *
 */
    unsigned int S4U_Simulation::getHostNumCores(const std::string &hostname) {
        auto host = S4U_Simulation::get_host_or_vm_by_name_or_null(hostname);
        if (host == nullptr) {
            throw std::invalid_argument("Unknown hostname " + hostname);
        }
        return static_cast<unsigned int>(host->get_core_count());
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
 */
    double S4U_Simulation::getHostFlopRate(const std::string &hostname) {
        auto host = S4U_Simulation::get_host_or_vm_by_name_or_null(hostname);
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
 */
    bool S4U_Simulation::isHostOn(const std::string &hostname) {

        auto host = S4U_Simulation::get_host_or_vm_by_name_or_null(hostname);
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
 */
    void S4U_Simulation::turnOffHost(const std::string &hostname) {

        auto host = S4U_Simulation::get_host_or_vm_by_name_or_null(hostname);
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
 */
    void S4U_Simulation::turnOnHost(const std::string &hostname) {

        auto host = S4U_Simulation::get_host_or_vm_by_name_or_null(hostname);
        if (host == nullptr) {
            throw std::invalid_argument("Unknown hostname " + hostname);
        }
        host->turn_on();
    }

    /**
* @brief Returns whether a link is on or not
*
* @param link_name: the name of the link
* @return true or false
*
*/
    bool S4U_Simulation::isLinkOn(const std::string &link_name) {
        auto link = simgrid::s4u::Link::by_name_or_null(link_name);
        if (link == nullptr) {
            throw std::invalid_argument("Unknown link_name " + link_name);
        }
        return link->is_on();
    }

    /**
 * @brief Turn off a link
 *
 * @param link_name: the name of the link to turn off
 *
 */
    void S4U_Simulation::turnOffLink(const std::string &link_name) {
        auto link = simgrid::s4u::Link::by_name_or_null(link_name);
        if (link == nullptr) {
            throw std::invalid_argument("Unknown link_name " + link_name);
        }
        link->turn_off();
    }

    /**
 * @brief Turn on a link
 *
 * @param link_name: the name of the link to turn on
 *
 */
    void S4U_Simulation::turnOnLink(const std::string &link_name) {
        auto link = simgrid::s4u::Link::by_name_or_null(link_name);
        if (link == nullptr) {
            throw std::invalid_argument("Unknown link_name " + link_name);
        }
        link->turn_on();
    }

    /**
 * @brief Get the flop rate of the current host
 *
 * @return the flop rate in floating point operations per second
 *
 */
    double S4U_Simulation::getFlopRate() {
        return simgrid::s4u::Host::current()->get_speed();// changed it to speed of the current pstate
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
     * @brief Simulates a multi-threaded computation
     * @param num_threads: the number of threads
     * @param thread_creation_overhead: the thread creation overhead in seconds
     * @param sequential_work: the sequential work (in flops)
     * @param parallel_per_thread_work: the parallel per thread work (in flops)
     */
    void S4U_Simulation::compute_multi_threaded(unsigned long num_threads,
                                                double thread_creation_overhead,
                                                double sequential_work,
                                                double parallel_per_thread_work) {
        // Overhead
        S4U_Simulation::sleep(static_cast<int>(num_threads) * thread_creation_overhead);
        if (num_threads == 1) {
            simgrid::s4u::this_actor::execute(sequential_work + parallel_per_thread_work);
        } else {
            // Launch compute-heavy thread
            auto bottleneck_thread = simgrid::s4u::this_actor::exec_async(sequential_work + parallel_per_thread_work);
            // Launch all other threads
            simgrid::s4u::this_actor::thread_execute(simgrid::s4u::this_actor::get_host(), parallel_per_thread_work, static_cast<int>(num_threads) - 1);
            // Wait for the compute-heavy thread
            bottleneck_thread->wait();
        }
    }

    /**
    * @brief Simulates a disk write
    *
    * @param num_bytes: number of bytes to write
    * @param hostname: name of host to which disk is attached (or empty if disk is known)
    * @param mount_point: mount point (or empty if disk is known)
    * @param disk: a disk to write to, if known (if known, this method will run much faster)
    */
    void S4U_Simulation::writeToDisk(sg_size_t num_bytes, const std::string &hostname, std::string mount_point, simgrid::s4u::Disk *disk) {
        mount_point = FileLocation::sanitizePath(mount_point);

        WRENCH_DEBUG("Writing %llu bytes to disk %s:%s", num_bytes, hostname.c_str(), mount_point.c_str());

        if (not disk) {
            auto host = S4U_Simulation::get_host_or_vm_by_name_or_null(hostname);
            if (host == nullptr) {
                throw std::invalid_argument("S4U_Simulation::writeToDisk(): unknown host " + hostname);
            }
            auto disk_list = host->get_disks();
            for (auto const &d: disk_list) {
                std::string disk_mountpoint =
                        FileLocation::sanitizePath(std::string(d->get_property("mount")));
                if (disk_mountpoint == mount_point) {
                    disk = d;
                    break;
                }
            }
            if (not disk) {
                throw std::invalid_argument("S4U_Simulation::writeToDisk(): unknown path " +
                                            mount_point + " at host " + hostname);
            }
        }
        disk->write((sg_size_t) num_bytes);
    }


    /**
* @brief Read from a local disk and write to a local disk concurrently
*
* @param num_bytes_to_read: number of bytes to read
* @param num_bytes_to_write: number of bytes to write
* @param hostname: the host at which the disks are located
* @param read_mount_point: the mountpoint to read from
* @param write_mount_point: the mountpoint to write to
* @param src_disk: source disk (nullptr if not known)
* @param dst_disk: dst disk (nullptr if not known)
*/
    void S4U_Simulation::readFromDiskAndWriteToDiskConcurrently(sg_size_t num_bytes_to_read, sg_size_t num_bytes_to_write,
                                                                const std::string &hostname,
                                                                const std::string &read_mount_point,
                                                                const std::string &write_mount_point,
                                                                simgrid::s4u::Disk *src_disk,
                                                                simgrid::s4u::Disk *dst_disk) {
        WRENCH_DEBUG("Reading %llu bytes from disk %s:%s and writing %llu bytes to disk %s:%s",
                     num_bytes_to_read, hostname.c_str(), read_mount_point.c_str(),
                     num_bytes_to_write, hostname.c_str(), write_mount_point.c_str());

        if ((not src_disk) or (not dst_disk)) {

            auto sanitized_read_mount_point = FileLocation::sanitizePath(read_mount_point);
            auto sanitized_write_mount_point = FileLocation::sanitizePath(write_mount_point);
            auto host = S4U_Simulation::get_host_or_vm_by_name_or_null(hostname);
            if (host == nullptr) {
                throw std::invalid_argument("S4U_Simulation::readFromDiskAndWriteToDiskConcurrently(): unknown host " + hostname);
            }
            auto disk_list = S4U_Simulation::get_host_or_vm_by_name(hostname)->get_disks();
            for (auto const &disk: disk_list) {
                std::string disk_mountpoint =
                        FileLocation::sanitizePath(std::string(std::string(disk->get_property("mount"))));
                if (disk_mountpoint == sanitized_read_mount_point) {
                    src_disk = disk;
                }
                if (disk_mountpoint == sanitized_write_mount_point) {
                    dst_disk = disk;
                }
                if (src_disk and dst_disk) {
                    break;
                }
            }
        }

        if ((not src_disk) or (not dst_disk)) {
            throw std::invalid_argument("S4U_Simulation::readFromDiskAndWriteToDiskConcurrently(): Couldn't find src or dst disk!");
        }


        // Start asynchronous read
        auto read_activity = src_disk->io_init((sg_size_t) num_bytes_to_read, simgrid::s4u::Io::OpType::READ);
        read_activity->start();
        // Do synchronous write
        dst_disk->write((sg_size_t) num_bytes_to_write);
        // Wait for asynchronous read to be done
        read_activity->wait();
    }


    /**
* @brief Simulates a disk read
*
* @param num_bytes: number of bytes to read
* @param hostname: name of host to which disk is attached
* @param mount_point: mount point
 * @param disk: disk to read from (nullptr if not known)
*/
    void S4U_Simulation::readFromDisk(sg_size_t num_bytes, const std::string &hostname, std::string mount_point, simgrid::s4u::Disk *disk) {
        mount_point = FileLocation::sanitizePath(mount_point);

        WRENCH_DEBUG("Reading %llu bytes from disk %s:%s", num_bytes, hostname.c_str(), mount_point.c_str());

        if (not disk) {
            auto host = S4U_Simulation::get_host_or_vm_by_name_or_null(hostname);
            if (not host) {
                throw std::invalid_argument("S4U_Simulation::readFromDisk(): unknown host " + hostname);
            }

            auto disk_list = host->get_disks();
            for (auto const &d: disk_list) {
                std::string disk_mountpoint =
                        FileLocation::sanitizePath(std::string(d->get_property("mount")));

                if (disk_mountpoint == mount_point) {
                    disk = d;
                    break;
                }
            }
            if (not disk) {
                throw std::invalid_argument("S4U_Simulation::readFromDisk(): invalid mount point " +
                                            mount_point + " at host " + hostname);
            }
        }
        disk->read((sg_size_t) num_bytes);
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
* @brief Get the memory_manager_service capacity of a host given a hostname
* @param hostname: the name of the host
* @return a memory_manager_service capacity in bytes
*/
    sg_size_t S4U_Simulation::getHostMemoryCapacity(const std::string &hostname) {
        auto host = S4U_Simulation::get_host_or_vm_by_name_or_null(hostname);
        if (host == nullptr) {
            throw std::invalid_argument("Unknown hostname " + hostname);
        }
        return getHostMemoryCapacity(host);
    }

    /**
* @brief Get the memory_manager_service capacity of the current host
* @return a memory_manager_service capacity in bytes
*/
    sg_size_t S4U_Simulation::getMemoryCapacity() {
        return getHostMemoryCapacity(simgrid::s4u::Host::current());
    }

    /**
* @brief Get the memory_manager_service capacity of a S4U host
* @param host: the host
* @return a memory_manager_service capacity in bytes
*/
    sg_size_t S4U_Simulation::getHostMemoryCapacity(simgrid::s4u::Host *host) {
        static std::map<simgrid::s4u::Host *, sg_size_t> memoized;

        if (memoized.find(host) != memoized.end()) {
            return memoized[host];
        }

        std::set<std::string> tags = {"mem", "Mem", "MEM", "ram", "Ram", "RAM", "memory_manager_service", "Memory", "MEMORY"};
        sg_size_t capacity_value = S4U_Simulation::DEFAULT_RAM;

        for (auto const &tag: tags) {
            const char *capacity_string = host->get_property(tag);
            if (capacity_string) {
                if (capacity_value != S4U_Simulation::DEFAULT_RAM) {
                    throw std::invalid_argument(
                            "S4U_Simulation::getHostMemoryCapacity(): Host '" + std::string(host->get_cname()) +
                            "' has multiple memory_manager_service capacity specifications");
                }
                try {
                    capacity_value = UnitParser::parse_size(capacity_string);
                } catch (std::invalid_argument &e) {
                    throw std::invalid_argument(
                            "S4U_Simulation::getHostMemoryCapacity(): Host '" + std::string(host->get_cname()) +
                            "'has invalid memory_manager_service capacity specification '" + tag + ":" +
                            std::string(capacity_string) + "'");
                }
            }
        }
        memoized[host] = capacity_value;
        return capacity_value;
    }

    /**
* @brief Get the property associated to a host specified in the platform file
* @param hostname: the host name
* @param property_name: the property name
* @return a string relating to the property specified in the platform file
*/
    std::string S4U_Simulation::getHostProperty(const std::string &hostname, const std::string &property_name) {
        auto host = simgrid::s4u::Host::by_name_or_null(hostname);
        if (host == nullptr) {
            throw std::invalid_argument("S4U_Simulation::getHostProperty(): Unknown hostname " + hostname);
        }
        if (host->get_properties()->find(property_name) == host->get_properties()->end()) {
            throw std::invalid_argument("S4U_Simulation::getHostProperty(): Unknown property " + property_name);
        }
        return host->get_property(property_name);
    }

    /**
* @brief Get the property associated to a cluster specified in the platform file
* @param cluster_id: the cluster id
* @param property_name: the property name
* @return a string relating to the property specified in the platform file
*/
    std::string S4U_Simulation::getClusterProperty(const std::string &cluster_id, const std::string &property_name) {
        auto simgrid_engine = simgrid::s4u::Engine::get_instance();
        auto clusters = simgrid_engine->get_filtered_netzones<simgrid::kernel::routing::ClusterZone>();
        for (auto c: clusters) {
            if (c->get_name() == cluster_id) {
                if (c->get_properties()->find(property_name) == c->get_properties()->end()) {
                    throw std::invalid_argument("S4U_Simulation::getClusterProperty(): Unknown property " + property_name);
                }
                return c->get_property(property_name);
            }
        }
        throw std::invalid_argument("S4U_Simulation::getClusterProperty(): Unknown cluster " + cluster_id);
    }


    /**
* @brief Set a property associated to a host specified in the platform file (does not include VMs)
* @param hostname: the host name
* @param property_name: the property name
* @param property_value: the property value
*/
    void S4U_Simulation::setHostProperty(const std::string &hostname, const std::string &property_name, const std::string &property_value) {
        auto host = simgrid::s4u::Host::by_name_or_null(hostname);
        if (host == nullptr) {
            throw std::invalid_argument("S4U_Simulation::getHostProperty(): Unknown hostname " + hostname);
        }
        host->set_property(property_name, property_value);
    }

    /**
* @brief Get the energy consumed by the host up to now
* @param hostname: the host name
* @return the energy consumed by the host in Joules
*/
    double S4U_Simulation::getEnergyConsumedByHost(const std::string &hostname) {
        double energy_consumed;

        auto host = simgrid::s4u::Host::by_name_or_null(hostname);
        if (host == nullptr) {
            throw std::invalid_argument("Unknown hostname " + hostname);
        }
        try {
            energy_consumed = sg_host_get_consumed_energy(host);
        } catch (std::exception &) {
            throw std::runtime_error(
                    "S4U_Simulation::getEnergyConsumedByHost(): Was not able to get the "
                    "energy consumed by the host. Make sure the energy plugin is enabled (--wrench-energy-simulation)");
        }
        return energy_consumed;
    }

#if 0
    /**
     * @brief Get the total energy consumed by a set of hosts
     * @param hostnames: the list of hostnames
     * @return The total energy consumed by all the hosts in Joules
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
*/
    void S4U_Simulation::setPstate(const std::string &hostname, unsigned long pstate) {
        auto host = simgrid::s4u::Host::by_name_or_null(hostname);
        if (host == nullptr) {
            throw std::invalid_argument("S4U_Simulation::setPstate(): Unknown hostname " + hostname);
        }
        if (host->get_pstate_count() < pstate) {
            throw std::invalid_argument("S4U_Simulation::setPstate(): Invalid pstate index (host " + hostname + " only has " +
                                        std::to_string(host->get_pstate_count()) + " pstates)");
        }
        try {
            host->set_pstate(pstate);
        } catch (std::exception &e) {
            throw std::runtime_error(
                    "S4U_Simulation::setPstate(): Was not able to set the pstate of the host. "
                    "Make sure the energy is plugin is enabled (--wrench-energy-simulation) and "
                    "the pstate is within range of pstates available to the host");
        }
    }

    /**
* @brief Get the total number of power states of a host
* @param hostname: the host name
* @return The number of power states available for the host (as specified in the platform xml description file)
*/
    int S4U_Simulation::getNumberofPstates(const std::string &hostname) {
        auto host = simgrid::s4u::Host::by_name_or_null(hostname);
        if (host == nullptr) {
            throw std::invalid_argument("Unknown hostname " + hostname);
        }
        try {
            return static_cast<int>(host->get_pstate_count());
        } catch (std::exception &) {
            throw std::runtime_error(
                    "S4U_Simulation::getNumberofPstates():: Was not able to get the energy consumed by the host. "
                    "Make sure the energy plugin is enabled (--wrench-energy-simulation) ");
        }
    }

    /**
* @brief Get the current power state of a host
* @param hostname: the host name
* @return The index of the current pstate of the host (as specified in the platform xml description file)
*/
    unsigned long S4U_Simulation::getCurrentPstate(const std::string &hostname) {
        auto host = simgrid::s4u::Host::by_name_or_null(hostname);
        if (host == nullptr) {
            throw std::invalid_argument("Unknown hostname " + hostname);
        }
        try {
            return host->get_pstate();
        } catch (std::exception &e) {
            throw std::runtime_error(
                    "S4U_Simulation::getNumberofPstates():: Was not able to get the energy consumed by the host. "
                    "Make sure the energy plugin is enabled (--wrench-energy-simulation) ");
        }
    }

    /**
    * @brief Get the minimum power consumption (i.e., idling) for a host at its current pstate
    * @param hostname: the host name
    * @return The power consumption for this host if idle (as specified in the platform xml description file)
    */
    double S4U_Simulation::getMinPowerConsumption(const std::string &hostname) {
        auto host = simgrid::s4u::Host::by_name_or_null(hostname);
        if (host == nullptr) {
            throw std::invalid_argument("Unknown hostname " + hostname);
        }
        try {
            return sg_host_get_wattmin_at(host, static_cast<int>(host->get_pstate()));
        } catch (std::exception &) {
            throw std::runtime_error(
                    "S4U_Simulation::getMinPowerConsumption(): Was not able to get the min power available to the host. "
                    "Make sure the energy plugin is enabled (--wrench-energy-simulation) ");
        }
    }

    /**
    * @brief Get the maximum power consumption (i.e., 100% load) for a host at its current pstate
    * @param hostname: the host name
    * @return The power consumption for this host if 100% used (as specified in the platform xml description file)
    */
    double S4U_Simulation::getMaxPowerConsumption(const std::string &hostname) {
        auto host = simgrid::s4u::Host::by_name_or_null(hostname);
        if (host == nullptr) {
            throw std::invalid_argument("Unknown hostname " + hostname);
        }
        try {
            return sg_host_get_wattmax_at(simgrid::s4u::Host::by_name(hostname),
                                          static_cast<int>((simgrid::s4u::Host::by_name(hostname))->get_pstate()));
        } catch (std::exception &) {
            throw std::runtime_error(
                    "S4U_Simulation::getMaxPowerConsumption():: Was not able to get the max power possible for the host. "
                    "Make sure the energy plugin is enabled (--wrench-energy-simulation)"

            );
        }
    }

    /**
    * @brief Get the list of power states available for a host
    * @param hostname: the host name
    * @return a list of power states available for the host (as specified in the platform xml description file)
    */
    std::vector<int> S4U_Simulation::getListOfPstates(const std::string &hostname) {
        auto host = simgrid::s4u::Host::by_name_or_null(hostname);
        if (host == nullptr) {
            throw std::invalid_argument("Unknown hostname " + hostname);
        }

        std::vector<int> list = {};
        try {
            int num_pstates = getNumberofPstates(hostname);
            for (int i = 0; i < num_pstates; i++) {
                list.push_back(i);
            }
        } catch (std::exception &) {
            throw std::runtime_error(
                    "S4U_Simulation::getListOfPstates(): Was not able to get the list of pstates for the host. "
                    "Make sure the energy plugin is enabled (--wrench-energy-simulation) ");
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
    * @return a vector of mount points
    *
    */
    std::vector<std::string> S4U_Simulation::getDisks(const std::string &hostname) {
        simgrid::s4u::Host *host;
        try {
            host = S4U_Simulation::get_host_or_vm_by_name(hostname);
        } catch (std::exception &) {
            throw std::invalid_argument("S4U_Simulation::getDisks(): Unknown host " + hostname);
        }

        std::vector<std::string> mount_points;
        for (auto const &d: host->get_disks()) {
            // Get the disk's mount point
            const char *p = d->get_property("mount");
            if (!p) {
                p = "/";
            }
            std::string mount_point = std::string(p);
            mount_points.push_back(mount_point);
        }

        return mount_points;
    }

    /**
* @brief Determines whether a mount point is defined at a host
* @param hostname: the host's name
* @param mount_point: the mount point
* @return a simgrid disk if the host has a disk attached to the specified mount point, nullptr otherwise
*/
    simgrid::s4u::Disk *S4U_Simulation::hostHasMountPoint(const std::string &hostname, const std::string &mount_point) {
        simgrid::s4u::Host *host;
        try {
            host = simgrid::s4u::Host::by_name(hostname);
        } catch (std::exception &) {
            throw std::invalid_argument("S4U_Simulation::hostHasMountPoint(): Unknown host " + hostname);
        }


        std::set<std::string> mount_points;
        for (auto const &d: host->get_disks()) {
            // Get the disk's mount point
            const char *p = d->get_property("mount");
            if (!p) {
                p = "/";
            }

            if (FileLocation::sanitizePath(std::string(p)) == FileLocation::sanitizePath(mount_point)) {
                return d;
            }
        }

        return nullptr;
    }


    /**
 * @brief Get the list of link names on the route between two hosts
 * @param src_host: src hostname
 * @param dst_host: dst hostname
 * @return a vector of link names
 */
    std::vector<std::string> S4U_Simulation::getRoute(std::string &src_host, std::string &dst_host) {
        simgrid::s4u::Host *src, *dst;
        try {
            src = S4U_Simulation::get_host_or_vm_by_name(src_host);
        } catch (std::exception &) {
            throw std::invalid_argument("S4U_Simulation::getRoute(): Unknown host " + src_host);
        }
        try {
            dst = S4U_Simulation::get_host_or_vm_by_name(dst_host);
        } catch (std::exception &) {
            throw std::invalid_argument("S4U_Simulation::getRoute(): Unknown host " + dst_host);
        }
        std::vector<simgrid::s4u::Link *> links;
        src->route_to(dst, links, nullptr);
        std::vector<std::string> to_return;
        to_return.reserve(links.size());
        for (auto const &l: links) {
            to_return.emplace_back(l->get_name());
        }
        return to_return;
    }


    /**
* @brief Gets the capacity of a disk attached to some host for a given mount point
* @param hostname: the host's name
* @param mount_point: the mount point (e.g.,  "/home")
* @return the capacity of the disk / mount point
*
*/
    sg_size_t S4U_Simulation::getDiskCapacity(const std::string &hostname, std::string mount_point) {
        //        WRENCH_INFO("==== %s %s ==== ", hostname.c_str(), mount_point.c_str());
        simgrid::s4u::Host *host;
        try {
            host = S4U_Simulation::get_host_or_vm_by_name(hostname);
        } catch (std::exception &) {
            throw std::invalid_argument("S4U_Simulation::getDiskCapacity(): Unknown host " + hostname);
        }

        mount_point = FileLocation::sanitizePath(mount_point + "/");

        for (auto const &d: host->get_disks()) {
            // Get the disk's mount point
            const char *mp = d->get_property("mount");
            if (!mp) {
                mp = "/";
            }

            std::string dmp = FileLocation::sanitizePath(std::string(mp) + "/");

            // This is not the mount point you're looking for
            if (dmp != mount_point) {
                continue;
            }

            sg_size_t capacity;
            const char *capacity_str = d->get_property("size");

            if (capacity_str) {
                try {
                    capacity = UnitParser::parse_size(capacity_str);
                } catch (std::invalid_argument &) {
                    throw std::invalid_argument("S4U_Simulation::getDiskCapacity(): Disk " + d->get_name() +
                                                " at host " + hostname + " has invalid size");
                }
            } else {
                capacity = LLONG_MAX;// Default size if no size property specified
            }

            return capacity;
        }

        throw std::invalid_argument("S4U_Simulation::getDiskCapacity(): Unknown mount point " +
                                    mount_point + " at host " + hostname);
    }


    /**
 * @brief Method to create, programmatically, a new disk
 * @param hostname: the name of the host to which the disk should be attached
 * @param disk_id: the nae of the disk
 * @param read_bandwidth_in_bytes_per_sec: the disk's read bandwidth in byte/sec
 * @param write_bandwidth_in_bytes_per_sec: the disk's write bandwidth in byte/sec
 * @param capacity_in_bytes: the disk's capacity in bytes
 * @param mount_point: the disk's mount point (most people use "/")
 */
    void S4U_Simulation::createNewDisk(const std::string &hostname, const std::string &disk_id,
                                       double read_bandwidth_in_bytes_per_sec,
                                       double write_bandwidth_in_bytes_per_sec,
                                       sg_size_t capacity_in_bytes,
                                       const std::string &mount_point) {
        // Get the host
        auto host = simgrid::s4u::Host::by_name_or_null(hostname);
        if (not host) {
            throw std::invalid_argument("S4U_Simulation::createNewDisk(): unknown host " + hostname);
        }
        // Check that no similar disk exists
        for (auto const &d: host->get_disks()) {
            if (d->get_name() == disk_id) {
                throw std::invalid_argument("S4U_Simulation::createNewDisk(): a disk with id " + disk_id + " already exists at host " + hostname);
            }
            if (d->get_property("mount") == mount_point) {
                throw std::invalid_argument(
                        "S4U_Simulation::createNewDisk(): a disk with mount point " + mount_point +
                        " already exists at host " + hostname);
            }
        }
        // Create the disk
        auto disk = host->add_disk(disk_id, read_bandwidth_in_bytes_per_sec, write_bandwidth_in_bytes_per_sec);
        // Add the required disk properties
        disk->set_property("size", std::to_string(capacity_in_bytes) + "B");
        disk->set_property("mount", mount_point);
        disk->seal();
    }

    /**
 * @brief Convenient s4u wrapper to retrieve a Host (which can be a VM) based on a name
 * @param name: the host/vm name or null if none
 * @return a SimGrid host
 */
    simgrid::s4u::Host *S4U_Simulation::get_host_or_vm_by_name_or_null(const std::string &name) {
        auto host = simgrid::s4u::Host::by_name_or_null(name);
        if (!host) {
            // Perhaps it's a VM
            if (S4U_VirtualMachine::vm_to_pm_map.find(name) != S4U_VirtualMachine::vm_to_pm_map.end()) {
                auto physical_host = simgrid::s4u::Host::by_name_or_null(S4U_VirtualMachine::vm_to_pm_map[name]);
                host = physical_host->vm_by_name_or_null(name);
            }
        }
        return host;
    }

    /**
 * @brief Convenient s4u wrapper to retrieve a Host (which can be a VM) based on a name
 * @param name: the host/vm name
 * @return a SimGrid host or nullptr if name==""
 */
    simgrid::s4u::Host *S4U_Simulation::get_host_or_vm_by_name(const std::string &name) {
        if (name.empty()) return nullptr;
        const auto host = S4U_Simulation::get_host_or_vm_by_name_or_null(name);
        if (!host) {
            throw std::invalid_argument("S4U_Simulation::get_host_or_vm_by_name(): Unknown host '" + name + "'");
        }
        return host;
    }


}// namespace wrench
