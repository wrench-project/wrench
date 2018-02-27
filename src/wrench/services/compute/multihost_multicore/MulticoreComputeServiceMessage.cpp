/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include "MulticoreComputeServiceMessage.h"

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param name: the message name
     * @param payload: the message size in bytes
     */
    MulticoreComputeServiceMessage::MulticoreComputeServiceMessage(std::string name, double payload) :
            ComputeServiceMessage("MulticoreComputeServiceMessage::" + name, payload) {
    }

    /**
     * @brief Constructor
     * @param job: the workflow job
     * @param cs: the compute service
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    MulticoreComputeServiceNotEnoughCoresMessage::MulticoreComputeServiceNotEnoughCoresMessage(WorkflowJob *job,
                                                                                               ComputeService *cs,
                                                                                               double payload)
            : MulticoreComputeServiceMessage("NOT_ENOUGH_CORES", payload), compute_service(cs) {
      if ((job == nullptr) || (cs == nullptr)) {
        throw std::invalid_argument("MulticoreComputeServiceNotEnoughCoresMessage::MulticoreComputeServiceNotEnoughCoresMessage(): Invalid arguments");
      }
      this->job = job;
    }


};
