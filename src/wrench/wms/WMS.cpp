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
		 * @param simulation: a pointer to a simulation object
		 * @param workflow: a pointer to a workflow to execute
		 * @param scheduler: a pointer to a scheduler implementation
		 * @param hostname: the name of the host
     * @param suffix: a string to append to the process name
     */
    WMS::WMS(Simulation *simulation,
             Workflow *workflow,
             std::unique_ptr<Scheduler> scheduler,
             std::string hostname,
             std::string suffix) :
            S4U_DaemonWithMailbox("wms_" + suffix, "wms_" + suffix),
            simulation(simulation),
            workflow(workflow),
            scheduler(std::move(scheduler)) {

      // Start the daemon
      this->start(hostname);
    }

    /**
     * @brief Add a static optimization to the list of optimizations. Optimizations are
     * executed in order of insertion
     *
     * @param optimization: a pointer to a static optimization implementation
     */
    void WMS::addStaticOptimization(std::unique_ptr<StaticOptimization> optimization) {
      this->static_optimizations.push_back(std::move(optimization));
    }

    /**
     * @brief Perform static optimizations. Optimizations are executed in order of insertion
     */
    void WMS::runStaticOptimizations() {
      for (auto &opt : this->static_optimizations) {
        opt.get()->process(this->workflow);
      }
    }
}
