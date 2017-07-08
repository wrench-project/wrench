/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <xbt/ex.hpp>
#include <wrench-dev.h>
#include "S4U_PendingCommunication.h"
#include <simulation/SimulationMessage.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(pending_communication, "Log category for Pending Communication");

namespace wrench {

    /**
     * @brief Wait for a pending communication
     *
     * @throw std::runtime_error
     *          - "network_error" (e.g., other end has died)
     */
    std::unique_ptr<SimulationMessage> S4U_PendingCommunication::wait() {

      try {
        if (this->comm_ptr->getState() != finished) {
          this->comm_ptr->wait();
        }
      } catch (xbt_ex &e) {
        if (e.category == network_error) {
          WRENCH_INFO("Network error while doing a dputMessage()");
          throw std::runtime_error("network_error");
        }
      }
      return std::unique_ptr<SimulationMessage>(this->simulation_message);
    }


    /**
     * @brief Wait for any completion
     * @param pending_comms: pending communications
     * @return the index of the comm to which something happened
     *
     * @throw std::invalid_argument
     */
    unsigned long  S4U_PendingCommunication::waitForSomethingToHappen(
            std::vector<std::unique_ptr<S4U_PendingCommunication>> *pending_comms) {

      std::set<S4U_PendingCommunication *> completed_comms;

      if (pending_comms->size() == 0) {
        throw std::invalid_argument("S4U_PendingCommunication::waitForSomethingToHappen(): invalid argument");
      }

      std::vector<simgrid::s4u::CommPtr> pending_s4u_comms;
      for (auto it=pending_comms->begin(); it < pending_comms->end(); it++) {
        pending_s4u_comms.push_back((*it)->comm_ptr);
      }

      return (unsigned long) simgrid::s4u::Comm::wait_any(&pending_s4u_comms);
    }

    /**
     * @brief Constructor
     */
    S4U_PendingCommunication::S4U_PendingCommunication() {
    }


};