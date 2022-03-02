/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/compute/htcondor/HTCondorCentralManagerServiceMessage.h>

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param payload: the message size in bytes
     */
    HTCondorCentralManagerServiceMessage::HTCondorCentralManagerServiceMessage(double payload)
            : ServiceMessage( payload) {}

    /**
     * @brief Constructor
     *
     * @param scheduled_jobs: list of pending jobs upon negotiator completion
     * @param payload: the message size in bytes
     */
    NegotiatorCompletionMessage::NegotiatorCompletionMessage(std::vector<std::shared_ptr<Job>> scheduled_jobs, double payload)
            : HTCondorCentralManagerServiceMessage( payload), scheduled_jobs(scheduled_jobs) {}


    /**
     * @brief Constructor
     *
     * @param payload: the message size in bytes
     */
    CentralManagerWakeUpMessage::CentralManagerWakeUpMessage(double payload)
            : HTCondorCentralManagerServiceMessage( payload) {}
            
}
