/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_HTCONDORCENTRALMANAGERSERVICEMESSAGE_H
#define WRENCH_HTCONDORCENTRALMANAGERSERVICEMESSAGE_H

#include "wrench/services/ServiceMessage.h"
#include "wrench/job/StandardJob.h"
#include "wrench/job/WorkflowJob.h"

#include <vector>

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief Top-level class for messages received/sent by a HTCondorCentralManagerService
     */
    class HTCondorCentralManagerServiceMessage : public ServiceMessage {
    protected:
        HTCondorCentralManagerServiceMessage(std::string name, double payload);
    };

    /**
     * @brief A message received by a HTCondorCentralManagerService so that it is notified of a negotiator
     *        cycle completion
     */
    class NegotiatorCompletionMessage : public HTCondorCentralManagerServiceMessage {
    public:
        NegotiatorCompletionMessage(std::vector<std::shared_ptr<WorkflowJob>> scheduled_jobs, double payload);

        /** @brief List of scheduled jobs */
        std::vector<std::shared_ptr<WorkflowJob>> scheduled_jobs;
    };

    /**
     * @brief A message received by a HTCondorCentralManagerService so that it wakes up and
     * tries to dispatch jobs again
     */
    class CentralManagerWakeUpMessage : public HTCondorCentralManagerServiceMessage {
    public:
        CentralManagerWakeUpMessage(double payload);

    };

    /***********************/
    /** \endcond INTERNAL  */
    /***********************/
}

#endif //WRENCH_HTCONDORCENTRALMANAGERSERVICEMESSAGE_H
