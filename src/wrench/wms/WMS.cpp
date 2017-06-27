/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wms/WMS.h"

namespace wrench {

    /**
     * @brief Create a WMS with a workflow instance and a scheduler implementation
     *
     * @param workflow: a workflow to execute
     * @param scheduler: a scheduler implementation
     * @param hostname: the name of the host on which to run the WMS
     * @param suffix: a string to append to the process name
     *
     * @throw std::invalid_argument
     */
    WMS::WMS(Workflow *workflow,
             std::unique_ptr<Scheduler> scheduler,
             std::string hostname,
             std::string suffix) :
            S4U_DaemonWithMailbox("wms_" + suffix, "wms_" + suffix),
            workflow(workflow),
            scheduler(std::move(scheduler)) {

      this->hostname = hostname;
    }

    /**
     * @brief Add a dynamic optimization to the list of optimizations. Optimizations are
     * executed in order of insertion
     *
     * @param optimization: a dynamic optimization implementation
     */
    void WMS::addDynamicOptimization(std::unique_ptr<DynamicOptimization> optimization) {
      this->dynamic_optimizations.push_back(std::move(optimization));
    }

    /**
     * @brief Add a static optimization to the list of optimizations. Optimizations are
     * executed in order of insertion
     *
     * @param optimization: a static optimization implementation
     */
    void WMS::addStaticOptimization(std::unique_ptr<StaticOptimization> optimization) {
      this->static_optimizations.push_back(std::move(optimization));
    }

    /**
     * @brief Perform dynamic optimizations. Optimizations are executed in order of insertion
     */
    void WMS::runDynamicOptimizations() {
      for (auto &opt : this->dynamic_optimizations) {
        opt.get()->process(this->workflow);
      }
    }

    /**
     * @brief Perform static optimizations. Optimizations are executed in order of insertion
     */
    void WMS::runStaticOptimizations() {
      for (auto &opt : this->static_optimizations) {
        opt.get()->process(this->workflow);
      }
    }

    /**
     * @brief Set the simulation
     *
     * @param simulation: the current simulation
     *
     * @throw std::invalid_argument
     */
    void WMS::setSimulation(Simulation *simulation) {
      this->simulation = simulation;

      // Start the daemon
      try {
        this->start(this->hostname);
      } catch (std::invalid_argument &e) {
        throw;
      }
    }

    /**
     * @brief Get the name of the host on which the WMS is running
     *
     * @return the hostname
     */
    std::string WMS::getHostname() {
      return this->hostname;
    }

};
