/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/compute/htcondor/HTCondorCentralManagerServiceMessage.h>

#include <utility>

#include <utility>

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param payload: the message size in bytes
     */
    HTCondorCentralManagerServiceMessage::HTCondorCentralManagerServiceMessage(sg_size_t payload)
        : ServiceMessage(payload) {}

    /**
     * @brief Constructor
     *
     * @param scheduled_jobs: list of pending jobs upon negotiator completion
     * @param payload: the message size in bytes
     */
    NegotiatorCompletionMessage::NegotiatorCompletionMessage(std::set<std::shared_ptr<Job>> scheduled_jobs, sg_size_t payload)
        : HTCondorCentralManagerServiceMessage(payload), scheduled_jobs(std::move(std::move(scheduled_jobs))) {}


    /**
     * @brief Constructor
     *
     * @param payload: the message size in bytes
     */
    CentralManagerWakeUpMessage::CentralManagerWakeUpMessage(sg_size_t payload)
        : HTCondorCentralManagerServiceMessage(payload) {}

}// namespace wrench
