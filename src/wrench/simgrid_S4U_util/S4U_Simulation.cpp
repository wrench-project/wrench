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
#include <xbt/ex.hpp>
#include <set>
#include <cfloat>
#include <wrench/services/compute/ComputeService.h>
#include <wrench/util/UnitParser.h>

#include "wrench/simgrid_S4U_util/S4U_Simulation.h"

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
     * @brief Initialize the simulated platform. Must only be called once.
     *
     * @param filename: the path to an XML platform description file
     */
    void S4U_Simulation::setupPlatform(std::string &filename) {
      try {
        this->engine->load_platform(filename.c_str());
      } catch (xbt_ex &e) {
        // TODO: S4U doesn't throw for this
      }
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
      std::vector<simgrid::s4u::Host *> host_list = this->engine->get_all_hosts();
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
      std::vector<simgrid::kernel::routing::ClusterZone*>clusters =
          this->engine->get_filtered_netzones<simgrid::kernel::routing::ClusterZone>();
      for (auto c : clusters) {
        std::vector<simgrid::s4u::Host*> host_list = c->get_all_hosts();
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
      unsigned int num_cores = 0;
      try {
        num_cores = (unsigned int) simgrid::s4u::Host::by_name(hostname)->get_core_count();
      } catch (std::out_of_range &e) {
        throw std::invalid_argument("Unknown hostname " + hostname);
      }
      return num_cores;
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
      double flop_rate = 0;
      try {
        flop_rate = simgrid::s4u::Host::by_name(hostname)->get_pstate_speed(0);
      } catch (std::out_of_range &e) {
        throw std::invalid_argument("Unknown hostname " + hostname);
      }
      return flop_rate;
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
     *
     * @param flops: the number of flops
     * @throw runtime_error;
     */
    void S4U_Simulation::compute(double flops) {
      simgrid::s4u::this_actor::execute(flops);
    }

    /**
     * @brief Simulates a sleep
     * @param duration: the number of seconds to sleep
     */
    void S4U_Simulation::sleep(double duration ) {
      simgrid::s4u::this_actor::sleep_for(duration);
    }

    /**
     * @brief Get the memory capacity of a host given a hostname
     * @param hostname: the name of the host
     * @return a memory capacity in bytes
     */
    double S4U_Simulation::getHostMemoryCapacity(std::string hostname) {
      return getHostMemoryCapacity(simgrid::s4u::Host::by_name(hostname));
    }

    /**
     * @brief Get the name of the host on which the calling actor is running
     * @return a hostname
     */
    std::string S4U_Simulation::getHostname() {
      return std::string(simgrid::s4u::Host::current()->get_name());
    }

    /**
     * @brief Get the memory capacity of a S4U host
     * @param host: the host
     * @return a memory capacity in bytes
     */
    double S4U_Simulation::getHostMemoryCapacity(simgrid::s4u::Host *host) {
      std::set<std::string> tags = {"mem", "Mem", "MEM", "ram", "Ram", "RAM", "memory", "Memory", "MEMORY"};
      double capacity_value = ComputeService::ALL_RAM;

      for (auto const &tag : tags) {
        const char *capacity_string = host->get_property(tag.c_str());
        if (capacity_string) {
          if (capacity_value != ComputeService::ALL_RAM) {
            throw std::invalid_argument("S4U_Simulation::getHostMemoryCapacity(): Host '" + std::string(host->get_cname()) + "' has multiple memory capacity specifications");
          }
          try {
            capacity_value = UnitParser::parse_size(capacity_string);
          } catch (std::invalid_argument &e) {
            throw std::invalid_argument(
                    "S4U_Simulation::getHostMemoryCapacity(): Host '" + std::string(host->get_cname()) + "'has invalid memory capacity specification '" + tag +":" +
                    std::string(capacity_string) + "'");
          }
        }
      }
      return capacity_value;
    }

};
