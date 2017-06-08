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
#include "simgrid_S4U_util/S4U_Simulation.h"

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
    void S4U_Simulation::setupPlatform(std::string filename) {
      this->engine->loadPlatform(filename.c_str());
      this->platform_setup = true;
    }

    /**
     * @brief Retrieves the hostname on which the calling actor is running
     *
     * @return the hostname as a string
     */
    std::string S4U_Simulation::getHostName() {
      return simgrid::s4u::Host::current()->name();
    }

    /**
     * @brief Retrives the list of hostnames
     *
     * @return a vector of hostnames
     */
    std::vector<std::string> S4U_Simulation::getAllHostnames() {
      std::vector<simgrid::s4u::Host *> host_list;
      this->engine->hostList(&host_list);
      std::vector<std::string> hostname_list;
      for (auto h : host_list) {
        hostname_list.push_back(h->name());
      }
      return hostname_list;
    }

    /**
     * @brief Retrieves the number of cores of a host
     *
     * @param hostname: the name of the host
     * @return the number of cores of the host
     */
    int S4U_Simulation::getNumCores(std::string hostname) {
      return simgrid::s4u::Host::by_name(hostname)->coreCount();
    }

    /**
     * @brief Retrieves the current simulation date
     *
     * @return the simulation clock
     */
    double S4U_Simulation::getClock() {
      return simgrid::s4u::Engine::instance()->getClock();
    }

    /**
     * @brief Simulates a computation on host on which the calling actor is running
     *
     * @param flops: the number of flops
     */
    void S4U_Simulation::compute(double flops) {
      e_smx_state_t state = simgrid::s4u::this_actor::execute(flops);
//      std::cerr << "STATE RETURNED BY COMPUTE = " << state << std::endl;
    }

    /**
     * @brief Simulates a sleep
     * @param duration: the number of seconds to sleep
     */
    void S4U_Simulation::sleep(double duration) {
      return simgrid::s4u::this_actor::sleep_for(duration);
    }

};
