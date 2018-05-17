/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include <cfloat>
#include <numeric>
#include "wrench/logging/TerminalOutput.h"
#include "wrench/services/compute/cloud/CloudService.h"


XBT_LOG_NEW_DEFAULT_CATEGORY(cloud_service, "Log category for Cloud Service");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param hostname: the hostname on which to start the service
     * @param supports_standard_jobs: true if the compute service should support standard jobs
     * @param supports_pilot_jobs: true if the compute service should support pilot jobs
     * @param execution_hosts: the hosts available for running virtual machines
     * @param default_storage_service: a storage service (or nullptr)
     * @param plist: a property list ({} means "use all defaults")
     *
     * @throw std::runtime_error
     */
    CloudService::CloudService(const std::string &hostname,
                               bool supports_standard_jobs,
                               bool supports_pilot_jobs,
                               std::vector<std::string> &execution_hosts,
                               std::map<std::string, std::string> plist,
                               int scratch_size) :
            VirtualizedClusterService(hostname, supports_standard_jobs, supports_pilot_jobs, execution_hosts,
                                      plist, scratch_size) {}

    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int CloudService::main() {

      TerminalOutput::setThisProcessLoggingColor(COLOR_RED);
      WRENCH_INFO("Cloud Service starting on host %s listening on mailbox_name %s",
                  this->hostname.c_str(),
                  this->mailbox_name.c_str());

      /** Main loop **/
      while (this->processNextMessage()) {
        // no specific action
      }

      WRENCH_INFO("Cloud Service on host %s terminated!", S4U_Simulation::getHostName().c_str());
      return 0;
    }
}
