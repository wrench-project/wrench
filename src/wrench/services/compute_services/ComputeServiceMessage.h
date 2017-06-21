/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_COMPUTESERVICEMESSAGE_H
#define WRENCH_COMPUTESERVICEMESSAGE_H


#include <services/ServiceMessage.h>

namespace wrench {

    class StandardJob;
    class PilotJob;

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief Top-level ComputeServiceMessage class
     */
    class ComputeServiceMessage : public ServiceMessage {
    protected:
        ComputeServiceMessage(std::string name, double payload);
    };

    /**
     * @brief "JOB_TYPE_NOT_SUPPORTED" SimulationMessage class
     */
    class ComputeServiceJobTypeNotSupportedMessage : public ComputeServiceMessage {
    public:
        ComputeServiceJobTypeNotSupportedMessage(WorkflowJob *, ComputeService *, double payload);

        WorkflowJob *job;
        ComputeService *compute_service;
    };

    /**
 * @brief "SUBMIT_STANDARD_JOB_REQUEST" SimulationMessage class
 */
    class ComputeServiceSubmitStandardJobRequestMessage : public ComputeServiceMessage {
    public:
        ComputeServiceSubmitStandardJobRequestMessage(std::string answer_mailbox, StandardJob *, double payload);

        std::string answer_mailbox;
        StandardJob *job;
    };

    /**
     * @brief "SUBMIT_STANDARD_JOB_ANSWER" SimulationMessage class
     */
    class ComputeServiceSubmitStandardJobAnswerMessage : public ComputeServiceMessage {
    public:
        ComputeServiceSubmitStandardJobAnswerMessage(StandardJob *, ComputeService *, bool success, WorkflowExecutionFailureCause *failure_cause, double payload);

        StandardJob *job;
        ComputeService *compute_service;
        bool success;
        WorkflowExecutionFailureCause *failure_cause;
    };

    /**
     * @brief "STANDARD_JOB_DONE" SimulationMessage class
     */
    class ComputeServiceStandardJobDoneMessage : public ComputeServiceMessage {
    public:
        ComputeServiceStandardJobDoneMessage(StandardJob *, ComputeService *, double payload);

        StandardJob *job;
        ComputeService *compute_service;
    };

    /**
     * @brief "STANDARD_JOB_FAILED" SimulationMessage class
     */
    class ComputeServiceStandardJobFailedMessage : public ComputeServiceMessage {
    public:
        ComputeServiceStandardJobFailedMessage(StandardJob *, ComputeService *, WorkflowExecutionFailureCause *, double payload);

        StandardJob *job;
        ComputeService *compute_service;
        WorkflowExecutionFailureCause *cause;
    };

    /**
     * @brief "SUBMIT_PILOT_REQUEST_JOB" SimulationMessage class
     */
    class ComputeServiceSubmitPilotJobRequestMessage : public ComputeServiceMessage {
    public:
        ComputeServiceSubmitPilotJobRequestMessage(std::string answer_mailbox, PilotJob *, double payload);

        std::string answer_mailbox;
        PilotJob *job;
    };

    /**
    * @brief "SUBMIT_PILOT_ANSWER_JOB" SimulationMessage class
    */
    class ComputeServiceSubmitPilotJobAnswerMessage : public ComputeServiceMessage {
    public:
        ComputeServiceSubmitPilotJobAnswerMessage(PilotJob *, ComputeService *, bool success, WorkflowExecutionFailureCause *, double payload);

        PilotJob *job;
        ComputeService *compute_service;
        bool success;
        WorkflowExecutionFailureCause *failure_cause;
    };


    /**
     * @brief "PILOT_JOB_STARTED" SimulationMessage class
     */
    class ComputeServicePilotJobStartedMessage : public ComputeServiceMessage {
    public:
        ComputeServicePilotJobStartedMessage(PilotJob *, ComputeService *, double payload);

        PilotJob *job;
        ComputeService *compute_service;
    };

    /**
     * @brief "PILOT_JOB_EXPIRED" SimulationMessage class
     */
    class ComputeServicePilotJobExpiredMessage : public ComputeServiceMessage {
    public:
        ComputeServicePilotJobExpiredMessage(PilotJob *, ComputeService *, double payload);

        PilotJob *job;
        ComputeService *compute_service;
    };

    /**
     * @brief "PILOT_JOB_FAILED" SimulationMessage class
     */
    class ComputeServicePilotJobFailedMessage : public ComputeServiceMessage {
    public:
        ComputeServicePilotJobFailedMessage(PilotJob *, ComputeService *, double payload);

        PilotJob *job;
        ComputeService *compute_service;
    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_COMPUTESERVICEMESSAGE_H
