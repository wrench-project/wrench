/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/services/compute/htcondor/HTCondorCentralManagerServiceMessage.h"

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param name: the message name
     * @param payload: the message size in bytes
     */
    HTCondorCentralManagerServiceMessage::HTCondorCentralManagerServiceMessage(std::string name, double payload)
            : ServiceMessage("HTCondorCentralManagerServiceMessage::" + name, payload) {}

    /**
     * @brief Constructor
     *
     * @param scheduled_jobs: list of pending jobs upon negotiator completion
     * @param payload: the message size in bytes
     */
    NegotiatorCompletionMessage::NegotiatorCompletionMessage(std::vector<WorkflowJob *> scheduled_jobs, double payload)
            : HTCondorCentralManagerServiceMessage("NEGOTIATOR_DONE", payload), scheduled_jobs(scheduled_jobs) {}

    /**
     * @brief
     *
     * @param job:
     * @param service_specific_arguments:
     * @param payload:
     */
    ScheduleStandardJobForPilotMessage::ScheduleStandardJobForPilotMessage(
            const std::string &answer_mailbox, StandardJob *job,
            std::map<std::string, std::string> &service_specific_arguments, double payload) :
            HTCondorCentralManagerServiceMessage("SCHEDULE_JOB_FOR_PILOT", payload), answer_mailbox(answer_mailbox),
            job(job), service_specific_arguments(service_specific_arguments) {}

    /**
     * @brief
     *
     * @param success:
     * @param payload:
     */
    ScheduleStandardJobForPilotAnswerMessage::ScheduleStandardJobForPilotAnswerMessage(
            bool success,
            std::shared_ptr<FailureCause> failure_cause,
            double payload)
            : HTCondorCentralManagerServiceMessage("SCHEDULED_FOR_PILOT_ANSWER", payload), success(success),
              failure_cause(failure_cause) {}
}
