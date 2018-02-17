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
     * @param filename: the path to an XML platform file
     */
    void S4U_Simulation::setupPlatform(std::string &filename) {
      try {
        this->engine->loadPlatform(filename.c_str());
      } catch (xbt_ex &e) {
        // TODO: S4U doesn't throw for this
      }
      this->platform_setup = true;
    }

    /**
     * @brief Retrieves the hostname on which the calling actor is running
     *
     * @return the hostname as a string
     */
    std::string S4U_Simulation::getHostName() {
      return simgrid::s4u::Host::current()->getName();
    }

    /**
     * @brief Retrieves the list of hostnames
     *
     * @return a vector of hostnames
     */
    std::vector<std::string> S4U_Simulation::getAllHostnames() {
      std::vector<simgrid::s4u::Host *> host_list;
      this->engine->getHostList(&host_list);
      std::vector<std::string> hostname_list;
      hostname_list.reserve(host_list.size());
      for (auto h : host_list) {
        hostname_list.push_back(h->getName());
      }
      return hostname_list;
    }

    /**
     * @brief Determines whether a host exists for a given hostname
     * @param hostname: the hostname
     * @return true or false
     */
    bool S4U_Simulation::hostExists(std::string hostname) {
      return (simgrid::s4u::Host::by_name(hostname) != nullptr);
    }

    /**
     * @brief Retrieve the number of cores of a host
     *
     * @param hostname: the name of the host
     * @return the number of cores of the host
     *
     * @throw std::invalid_argument
     */
    unsigned int S4U_Simulation::getNumCores(std::string hostname) {
      unsigned int num_cores = 0;
      try {
        num_cores = (unsigned int) simgrid::s4u::Host::by_name(hostname)->getCoreCount();
      } catch (std::out_of_range &e) {
        throw std::invalid_argument("Unknown hostname " + hostname);
      }
      return num_cores;
    }

    /**
     * @brief Retrieve the flop rate of a host
     *
     * @param hostname: the name of the host
     * @return the flop rate in floating point operations per second
     *
     * @throw std::invalid_argument
     */
    double S4U_Simulation::getFlopRate(std::string hostname) {
      double flop_rate = 0;
      try {
        flop_rate = simgrid::s4u::Host::by_name(hostname)->getPstateSpeed(0);
      } catch (std::out_of_range &e) {
        throw std::invalid_argument("Unknown hostname " + hostname);
      }
      return flop_rate;
    }



    /**
     * @brief Retrieves the current simulation date
     *
     * @return the simulation clock
     */
    double S4U_Simulation::getClock() {
      return simgrid::s4u::Engine::getClock();
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
    void S4U_Simulation::sleep(double duration) {
      simgrid::s4u::this_actor::sleep_for(duration);
    }

    /**
     * @brief Get the memory capacity of a host given a hostname
     * @param hostname: the hostname
     * @return the memory capacity in bytes
     */
    double S4U_Simulation::getMemoryCapacity(std::string hostname) {
      return getHostMemoryCapacity(simgrid::s4u::Host::by_name(hostname));
    }

    /**
     * @brief Get the memory capacity of the current host
     * @return the memory capacity in bytes
     */
    double S4U_Simulation::getMemoryCapacity() {
      return S4U_Simulation::getHostMemoryCapacity(simgrid::s4u::Host::current());
    }

    /**
     * @brief Get the memory capacity of a S4U host
     * @param host: the host
     * @return the memory capacity in bytes
     */
    double S4U_Simulation::getHostMemoryCapacity(simgrid::s4u::Host *host) {
      std::set<std::string> tags = {"mem", "Mem", "MEM", "ram", "Ram", "RAM", "memory", "Memory", "MEMORY"};
      double capacity_value = ComputeService::ALL_RAM;

      for (auto tag : tags) {
        const char *capacity_string = host->getProperty(tag.c_str());
        if (capacity_string) {
          if (capacity_value != ComputeService::ALL_RAM) {
            throw std::invalid_argument("S4U_Simulation::getMemoryCapacity(): Host '" + host->getName() + "' has multiple memory capacity specifications");
          }
          if (sscanf(capacity_string, "%lf", &capacity_value) != 1) {
            throw std::invalid_argument(
                    "S4U_Simulation::getMemoryCapacity(): Host '" + host->getName() + "'has invalid memory capacity specification '" + tag +":" +
                    std::string(capacity_string) + "'");
          }
        }
      }
      return capacity_value;
    }

};
