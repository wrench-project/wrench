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
#include "wrench/workflow/job/StandardJob.h"
#include "wrench/workflow/job/WorkflowJob.h"

#include <vector>

namespace wrench {

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
        NegotiatorCompletionMessage(std::vector<WorkflowJob *> scheduled_jobs, double payload);

        /** @brief List of scheduled jobs */
        std::vector<WorkflowJob *> scheduled_jobs;
    };

    /**
     * @brief
     */
    class ScheduleStandardJobForPilotMessage : public HTCondorCentralManagerServiceMessage {
    public:
        ScheduleStandardJobForPilotMessage(const std::string &answer_mailbox, StandardJob *job,
                                           std::map<std::string, std::string> &service_specific_arguments,
                                           double payload
        );

        std::string answer_mailbox;
        /** **/
        StandardJob *job;
        /** **/
        std::map<std::string, std::string> service_specific_arguments;
    };

    /**
     * @brief
     */
    class ScheduleStandardJobForPilotAnswerMessage : public HTCondorCentralManagerServiceMessage {
    public:
        ScheduleStandardJobForPilotAnswerMessage(bool success, std::shared_ptr<FailureCause> failure_cause,
                                                 double payload);

        /** **/
        bool success;
        /** @brief The failure cause (or nullptr if success) */
        std::shared_ptr<FailureCause> failure_cause;
    };
}

#endif //WRENCH_HTCONDORCENTRALMANAGERSERVICEMESSAGE_H
