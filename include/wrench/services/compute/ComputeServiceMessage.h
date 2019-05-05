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
     * @brief Top-level class for messages received/sent by a ComputeService
     */
    class ComputeServiceMessage : public ServiceMessage {
    protected:
        ComputeServiceMessage(std::string name, double payload);
    };


    /**
     * @brief A message sent to a ComputeService to submit a StandardJob for execution
     */
    class ComputeServiceSubmitStandardJobRequestMessage : public ComputeServiceMessage {
    public:
        ComputeServiceSubmitStandardJobRequestMessage(const std::string answer_mailbox, StandardJob *,
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
     * @brief  A message sent by a ComputeService in answer to a StandardJob submission request
     */
    class ComputeServiceSubmitStandardJobAnswerMessage : public ComputeServiceMessage {
    public:
        ComputeServiceSubmitStandardJobAnswerMessage(StandardJob *, std::shared_ptr<ComputeService>  , bool success,
                                                     std::shared_ptr<FailureCause> failure_cause, double payload);

        /** @brief The standard job that was submitted */
        StandardJob *job;
        /** @brief The compute service to which the job was submitted */
        std::shared_ptr<ComputeService>  compute_service;
        /** @brief Whether to job submission was successful */
        bool success;
        /** @brief The cause of the failure, or nullptr on success */
        std::shared_ptr<FailureCause> failure_cause;
    };

    /**
     * @brief A message sent by a ComputeService when a StandardJob has completed execution
     */
    class ComputeServiceStandardJobDoneMessage : public ComputeServiceMessage {
    public:
        ComputeServiceStandardJobDoneMessage(StandardJob *, std::shared_ptr<ComputeService>  , double payload);

        /** @brief The job that has completed */
        StandardJob *job;
        /** @brief The compute service on which the job has completed */
        std::shared_ptr<ComputeService>  compute_service;
    };

    /**
     * @brief A message sent by a ComputeService when a StandardJob has failed to execute
     */
    class ComputeServiceStandardJobFailedMessage : public ComputeServiceMessage {
    public:
        ComputeServiceStandardJobFailedMessage(StandardJob *, std::shared_ptr<ComputeService>  , std::shared_ptr<FailureCause> cause,
                                               double payload);

        /** @brief The job that has failed */
        StandardJob *job;
        /** @brief The compute service on which the job has failed */
        std::shared_ptr<ComputeService>  compute_service;
        /** @brief The cause of the failure */
        std::shared_ptr<FailureCause> cause;
    };

    /**
    * @brief A message sent to a ComputeService to terminate a StandardJob previously submitted for execution
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
     * @brief A message sent by a ComputeService in answer to a StandardJob termination request
     */
    class ComputeServiceTerminateStandardJobAnswerMessage : public ComputeServiceMessage {
    public:
        ComputeServiceTerminateStandardJobAnswerMessage(StandardJob *, std::shared_ptr<ComputeService>  , bool success,
                                                        std::shared_ptr<FailureCause> failure_cause, double payload);

        /** @brief The standard job to terminate */
        StandardJob *job;
        /** @brief The compute service to which the job had been submitted */
        std::shared_ptr<ComputeService>  compute_service;
        /** @brief Whether to job termination was successful */
        bool success;
        /** @brief The cause of the failure, or nullptr on success */
        std::shared_ptr<FailureCause> failure_cause;
    };

    /**
     * @brief A message sent to a ComputeService to submit a PilotJob for execution
     */
    class ComputeServiceSubmitPilotJobRequestMessage : public ComputeServiceMessage {
    public:
        ComputeServiceSubmitPilotJobRequestMessage(std::string answer_mailbox, PilotJob *, std::map<std::string, std::string> &service_specific_args, double payload);

        /** @brief The mailbox to which the answer message should be sent */
        std::string answer_mailbox;
        /** @brief The submitted pilot job */
        PilotJob *job;
        /** @brief Service specific arguments */
        std::map<std::string, std::string> service_specific_args;
    };

    /**
    * @brief A message sent by a ComputeService in answer to a PilotJob submission request
    */
    class ComputeServiceSubmitPilotJobAnswerMessage : public ComputeServiceMessage {
    public:
        ComputeServiceSubmitPilotJobAnswerMessage(PilotJob *, std::shared_ptr<ComputeService>  , bool success,
                                                  std::shared_ptr<FailureCause> cause,
                                                  double payload);

        /** @brief The submitted pilot job */
        PilotJob *job;
        /** @brief The compute service to which the job was submitted */
        std::shared_ptr<ComputeService>  compute_service;
        /** @brief Whether the job submission was successful or not */
        bool success;
        /** @brief The cause of the failure, or nullptr on success */
        std::shared_ptr<FailureCause> failure_cause;
    };


    /**
     * @brief A message sent by a ComputeService when a PilotJob has started its execution
     */
    class ComputeServicePilotJobStartedMessage : public ComputeServiceMessage {
    public:
        ComputeServicePilotJobStartedMessage(PilotJob *, std::shared_ptr<ComputeService>  , double payload);

        /** @brief The pilot job that has started */
        PilotJob *job;
        /** @brief The compute service on which the pilot job has started */
        std::shared_ptr<ComputeService>  compute_service;
    };

    /**
     * @brief A message sent by a ComputeService when a PilotJob has expired
     */
    class ComputeServicePilotJobExpiredMessage : public ComputeServiceMessage {
    public:
        ComputeServicePilotJobExpiredMessage(PilotJob *, std::shared_ptr<ComputeService>  , double payload);

        /** @brief The pilot job that has expired */
        PilotJob *job;
        /** @brief The compute service on which the pilot job has expired */
        std::shared_ptr<ComputeService>  compute_service;
    };

    /**
     * @brief A message sent by a ComputeService when a PilotJob has failed
     */
    class ComputeServicePilotJobFailedMessage : public ComputeServiceMessage {
    public:
        ComputeServicePilotJobFailedMessage(PilotJob *, std::shared_ptr<ComputeService>  , double payload);

        /** @brief The pilot job that has failed */
        PilotJob *job;
        /** @brief The compute service on which the pilot job failed */
        std::shared_ptr<ComputeService>  compute_service;
    };

    /**
    * @brief A message sent to a ComputeService to terminate a PilotJob previously submitted for execution
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
     * @brief A message sent by a ComputeService in answer to a PilotJob termination request
     */
    class ComputeServiceTerminatePilotJobAnswerMessage : public ComputeServiceMessage {
    public:
        ComputeServiceTerminatePilotJobAnswerMessage(PilotJob *, std::shared_ptr<ComputeService> compute_service, bool success,
                                                     std::shared_ptr<FailureCause> failure_cause, double payload);

        /** @brief The job to terminate */
        PilotJob *job;
        /** @brief The compute service to which the job had been submitted */
        std::shared_ptr<ComputeService>  compute_service;
        /** @brief Whether to job termination was successful */
        bool success;
        /** @brief The cause of the failure, or nullptr on success */
        std::shared_ptr<FailureCause> failure_cause;
    };


    /**
     * @brief A message sent to a ComputeService to request information on its compute resources
    */
    class ComputeServiceResourceInformationRequestMessage : public ComputeServiceMessage {
    public:
        ComputeServiceResourceInformationRequestMessage(std::string answer_mailbox, double payload);

        /** @brief The mailbox to which the answer should be sent */
        std::string answer_mailbox;
    };

    /**
     * @brief A message sent by a ComputeService in answer to a resource information request
     */
    class ComputeServiceResourceInformationAnswerMessage : public ComputeServiceMessage {
    public:
        ComputeServiceResourceInformationAnswerMessage(std::map<std::string, std::map<std::string, double>> info, double payload);

        /** @brief The resource information map */
        std::map<std::string, std::map<std::string, double>> info;
    };


    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_COMPUTESERVICEMESSAGE_H