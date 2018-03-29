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

#include <memory>
#include <vector>
#include "wrench/workflow/execution_events/FailureCause.h"
#include "wrench/services/ServiceMessage.h"

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
     * @brief ComputeServiceSubmitStandardJobRequestMessage class
     */
    class ComputeServiceSubmitStandardJobRequestMessage : public ComputeServiceMessage {
    public:
        ComputeServiceSubmitStandardJobRequestMessage(std::string answer_mailbox, StandardJob *,
                                                      std::map<std::string, std::string> &service_specific_args,
                                                      double payload);

        /** @brief The mailbox to which the answer message should be sent */
        std::string answer_mailbox;
        /** @brief The submitted job */
        StandardJob *job;
        /** @brief Service specific arguments */
        std::map<std::string, std::string> service_specific_args;
    };

    /**
     * @brief ComputeServiceSubmitStandardJobAnswerMessage class
     */
    class ComputeServiceSubmitStandardJobAnswerMessage : public ComputeServiceMessage {
    public:
        ComputeServiceSubmitStandardJobAnswerMessage(StandardJob *, ComputeService *, bool success,
                                                     std::shared_ptr<FailureCause> failure_cause, double payload);

        /** @brief The standard job that was submitted */
        StandardJob *job;
        /** @brief The compute service to which the job was submitted */
        ComputeService *compute_service;
        /** @brief Whether to job submission was successful */
        bool success;
        /** @brief The cause of the failure, or nullptr on success */
        std::shared_ptr<FailureCause> failure_cause;
    };

    /**
     * @brief ComputeServiceStandardJobDoneMessage class
     */
    class ComputeServiceStandardJobDoneMessage : public ComputeServiceMessage {
    public:
        ComputeServiceStandardJobDoneMessage(StandardJob *, ComputeService *, double payload);

        /** @brief The job that completed */
        StandardJob *job;
        /** @brief The compute service that completed the job */
        ComputeService *compute_service;
    };

    /**
     * @brief ComputeServiceStandardJobFailedMessage class
     */
    class ComputeServiceStandardJobFailedMessage : public ComputeServiceMessage {
    public:
        ComputeServiceStandardJobFailedMessage(StandardJob *, ComputeService *, std::shared_ptr<FailureCause> cause,
                                               double payload);

        /** @brief The job that failed */
        StandardJob *job;
        /** @brief The compute service on which the job failed */
        ComputeService *compute_service;
        /** @brief The cause of the failure */
        std::shared_ptr<FailureCause> cause;
    };

    /**
    * @brief ComputeServiceTerminateStandardJobRequestMessage class
    */
    class ComputeServiceTerminateStandardJobRequestMessage : public ComputeServiceMessage {
    public:
        ComputeServiceTerminateStandardJobRequestMessage(std::string answer_mailbox, StandardJob *, double payload);

        /** @brief The mailbox to which the answer message should be sent */
        std::string answer_mailbox;
        /** @brief The job to terminate*/
        StandardJob *job;
    };

    /**
     * @brief ComputeServiceTerminateStandardJobAnswerMessage class
     */
    class ComputeServiceTerminateStandardJobAnswerMessage : public ComputeServiceMessage {
    public:
        ComputeServiceTerminateStandardJobAnswerMessage(StandardJob *, ComputeService *, bool success,
                                                        std::shared_ptr<FailureCause> failure_cause, double payload);

        /** @brief The standard job to terminate */
        StandardJob *job;
        /** @brief The compute service to which the job had been submitted */
        ComputeService *compute_service;
        /** @brief Whether to job termination was successful */
        bool success;
        /** @brief The cause of the failure, or nullptr on success */
        std::shared_ptr<FailureCause> failure_cause;
    };

    /**
     * @brief ComputeServiceSubmitPilotJobRequestMessage class
     */
    class ComputeServiceSubmitPilotJobRequestMessage : public ComputeServiceMessage {
    public:
        ComputeServiceSubmitPilotJobRequestMessage(std::string answer_mailbox, PilotJob *, double payload);

        /** @brief The mailbox to which the answer message should be sent */
        std::string answer_mailbox;
        /** @brief The submitted pilot job */
        PilotJob *job;
    };

    /**
    * @brief ComputeServiceSubmitPilotJobAnswerMessage class
    */
    class ComputeServiceSubmitPilotJobAnswerMessage : public ComputeServiceMessage {
    public:
        ComputeServiceSubmitPilotJobAnswerMessage(PilotJob *, ComputeService *, bool success,
                                                  std::shared_ptr<FailureCause> cause,
                                                  double payload);

        /** @brief The submitted pilot job */
        PilotJob *job;
        /** @brief The compute service to which the job was submitted */
        ComputeService *compute_service;
        /** @brief Whether the job submission was successful or not */
        bool success;
        /** @brief The cause of the failure, or nullptr on success */
        std::shared_ptr<FailureCause> failure_cause;
    };


    /**
     * @brief ComputeServicePilotJobStartedMessage class
     */
    class ComputeServicePilotJobStartedMessage : public ComputeServiceMessage {
    public:
        ComputeServicePilotJobStartedMessage(PilotJob *, ComputeService *, double payload);

        /** @brief The pilot job that just started */
        PilotJob *job;
        /** @brief The compute service that just started the pilot job */
        ComputeService *compute_service;
    };

    /**
     * @brief ComputeServicePilotJobExpiredMessage class
     */
    class ComputeServicePilotJobExpiredMessage : public ComputeServiceMessage {
    public:
        ComputeServicePilotJobExpiredMessage(PilotJob *, ComputeService *, double payload);

        /** @brief The pilot job that expired */
        PilotJob *job;
        /** @brief The compute service on which the pilot job expired */
        ComputeService *compute_service;
    };

    /**
     * @brief ComputeServicePilotJobFailedMessage class
     */
    class ComputeServicePilotJobFailedMessage : public ComputeServiceMessage {
    public:
        ComputeServicePilotJobFailedMessage(PilotJob *, ComputeService *, double payload);

        /** @brief The pilot job that failed */
        PilotJob *job;
        /** @brief The compute service on which the pilot job failed */
        ComputeService *compute_service;
    };

    /**
    * @brief ComputeServiceTerminatePilotJobRequestMessage class
    */
    class ComputeServiceTerminatePilotJobRequestMessage : public ComputeServiceMessage {
    public:
        ComputeServiceTerminatePilotJobRequestMessage(std::string answer_mailbox, PilotJob *, double payload);

        /** @brief The mailbox to which the answer message should be sent */
        std::string answer_mailbox;
        /** @brief The job to terminate*/
        PilotJob *job;
    };

    /**
     * @brief ComputeServiceTerminatePilotJobAnswerMessage class
     */
    class ComputeServiceTerminatePilotJobAnswerMessage : public ComputeServiceMessage {
    public:
        ComputeServiceTerminatePilotJobAnswerMessage(PilotJob *, ComputeService *, bool success,
                                                     std::shared_ptr<FailureCause> failure_cause, double payload);

        /** @brief The job to terminate */
        PilotJob *job;
        /** @brief The compute service to which the job had been submitted */
        ComputeService *compute_service;
        /** @brief Whether to job termination was successful */
        bool success;
        /** @brief The cause of the failure, or nullptr on success */
        std::shared_ptr<FailureCause> failure_cause;
    };

//    /**
//     * @brief ComputeServiceInformationMessage class
//     */
//    class ComputeServiceInformationMessage : public ComputeServiceMessage {
//    public:
//        ComputeServiceInformationMessage(WorkflowJob* job, std::string information, double payload);
//
//        /** @brief The job that is replying the information*/
//        WorkflowJob* job;
//
//        /** @brief The information to reply back */
//        std::string information;
//    };


    /**
     * @brief ComputeServiceResourceDescriptionRequestMessage class
    */
    class ComputeServiceResourceInformationRequestMessage : public ComputeServiceMessage {
    public:
        ComputeServiceResourceInformationRequestMessage(std::string answer_mailbox, double payload);

        /** @brief The mailbox to which the answer message should be sent */
        std::string answer_mailbox;
    };

    /**
     * @brief ComputeServiceNumIdleCoresAnswerMessage class
     */
    class ComputeServiceResourceInformationAnswerMessage : public ComputeServiceMessage {
    public:
        ComputeServiceResourceInformationAnswerMessage(std::map<std::string, std::vector<double>> info, double payload);

        /** @brief The resource information map */
        std::map<std::string, std::vector<double>> info;
    };


    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_COMPUTESERVICEMESSAGE_H