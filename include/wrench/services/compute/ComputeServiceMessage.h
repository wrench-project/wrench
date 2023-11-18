/**
 * Copyright (c) 2017-2019. The WRENCH Team.
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
#include "wrench/failure_causes/FailureCause.h"
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
        ComputeServiceMessage(double payload);
    };


    //    /**
    //    * @brief A message sent to a ComputeService to submit a StandardJob for execution
    //    */
    //    class ComputeServiceSubmitStandardJobRequestMessage : public ComputeServiceMessage {
    //    public:
    //        ComputeServiceSubmitStandardJobRequestMessage(const std::string answer_commport, std::shared_ptr<StandardJob> ,
    //                                                      const std::map<std::string, std::string> service_specific_args,
    //                                                      double payload);
    //
    //        /** @brief The commport_name to which the answer message should be sent */
    //        std::string answer_commport;
    //        /** @brief The submitted job */
    //        std::shared_ptr<StandardJob> job;
    //        /** @brief Service specific arguments */
    //        std::map<std::string, std::string> service_specific_args;
    //    };

    //    /**
    //     * @brief  A message sent by a ComputeService in answer to a StandardJob submission request
    //     */
    //    class ComputeServiceSubmitStandardJobAnswerMessage : public ComputeServiceMessage {
    //    public:
    //        ComputeServiceSubmitStandardJobAnswerMessage(std::shared_ptr<StandardJob> , std::shared_ptr<ComputeService>, bool success,
    //                                                     std::shared_ptr<FailureCause> failure_cause, double payload);
    //
    //        /** @brief The standard job that was submitted */
    //        std::shared_ptr<StandardJob> job;
    //        /** @brief The compute service to which the job was submitted */
    //        std::shared_ptr<ComputeService> compute_service;
    //        /** @brief Whether to job submission was successful */
    //        bool success;
    //        /** @brief The cause of the failure, or nullptr on success */
    //        std::shared_ptr<FailureCause> failure_cause;
    //    };

    //    /**
    //     * @brief A message sent by a ComputeService when a StandardJob has completed execution
    //     */
    //    class ComputeServiceStandardJobDoneMessage : public ComputeServiceMessage {
    //    public:
    //        ComputeServiceStandardJobDoneMessage(std::shared_ptr<StandardJob> , std::shared_ptr<ComputeService>, double payload);
    //
    //        /** @brief The job that has completed */
    //        std::shared_ptr<StandardJob> job;
    //        /** @brief The compute service on which the job has completed */
    //        std::shared_ptr<ComputeService> compute_service;
    //    };
    //
    //    /**
    //     * @brief A message sent by a ComputeService when a StandardJob has failed to execute
    //     */
    //    class ComputeServiceStandardJobFailedMessage : public ComputeServiceMessage {
    //    public:
    //        ComputeServiceStandardJobFailedMessage(std::shared_ptr<StandardJob> , std::shared_ptr<ComputeService>,
    //                                               std::shared_ptr<FailureCause> cause,
    //                                               double payload);
    //
    //        /** @brief The job that has failed */
    //        std::shared_ptr<StandardJob> job;
    //        /** @brief The compute service on which the job has failed */
    //        std::shared_ptr<ComputeService> compute_service;
    //        /** @brief The cause of the failure */
    //        std::shared_ptr<FailureCause> cause;
    //    };
    //
    //    /**
    //    * @brief A message sent to a ComputeService to terminate a StandardJob previously submitted for execution
    //    */
    //    class ComputeServiceTerminateStandardJobRequestMessage : public ComputeServiceMessage {
    //    public:
    //        ComputeServiceTerminateStandardJobRequestMessage(std::string answer_commport, std::shared_ptr<StandardJob> , double payload);
    //
    //        /** @brief The commport_name to which the answer message should be sent */
    //        std::string answer_commport;
    //        /** @brief The job to terminate*/
    //        std::shared_ptr<StandardJob> job;
    //    };
    //
    //    /**
    //     * @brief A message sent by a ComputeService in answer to a StandardJob termination request
    //     */
    //    class ComputeServiceTerminateStandardJobAnswerMessage : public ComputeServiceMessage {
    //    public:
    //        ComputeServiceTerminateStandardJobAnswerMessage(std::shared_ptr<StandardJob> , std::shared_ptr<ComputeService>, bool success,
    //                                                        std::shared_ptr<FailureCause> failure_cause, double payload);
    //
    //        /** @brief The standard job to terminate */
    //        std::shared_ptr<StandardJob> job;
    //        /** @brief The compute service to which the job had been submitted */
    //        std::shared_ptr<ComputeService> compute_service;
    //        /** @brief Whether to job termination was successful */
    //        bool success;
    //        /** @brief The cause of the failure, or nullptr on success */
    //        std::shared_ptr<FailureCause> failure_cause;
    //    };

    /**
     * @brief A message sent to a ComputeService to submit a CompoundJob for execution
     */
    class ComputeServiceSubmitCompoundJobRequestMessage : public ComputeServiceMessage {
    public:
        ComputeServiceSubmitCompoundJobRequestMessage(S4U_CommPort *answer_commport,
                                                      std::shared_ptr<CompoundJob> job,
                                                      std::map<std::string, std::string> service_specific_args,
                                                      double payload);

        /** @brief The commport_name to which the answer message should be sent */
        S4U_CommPort *answer_commport;
        /** @brief The submitted job */
        std::shared_ptr<CompoundJob> job;
        /** @brief Service specific arguments */
        std::map<std::string, std::string> service_specific_args;
    };

    /**
     * @brief  A message sent by a ComputeService in answer to a CompoundJob submission request
     */
    class ComputeServiceSubmitCompoundJobAnswerMessage : public ComputeServiceMessage {
    public:
        ComputeServiceSubmitCompoundJobAnswerMessage(std::shared_ptr<CompoundJob>, std::shared_ptr<ComputeService>, bool success,
                                                     std::shared_ptr<FailureCause> failure_cause, double payload);

        /** @brief The standard job that was submitted */
        std::shared_ptr<CompoundJob> job;
        /** @brief The compute service to which the job was submitted */
        std::shared_ptr<ComputeService> compute_service;
        /** @brief Whether to job submission was successful */
        bool success;
        /** @brief The cause of the failure, or nullptr on success */
        std::shared_ptr<FailureCause> failure_cause;
    };

    /**
     * @brief A message sent by a ComputeService when a CompoundJob has completed execution
     */
    class ComputeServiceCompoundJobDoneMessage : public ComputeServiceMessage {
    public:
        ComputeServiceCompoundJobDoneMessage(std::shared_ptr<CompoundJob>, std::shared_ptr<ComputeService>, double payload);

        /** @brief The job that has completed */
        std::shared_ptr<CompoundJob> job;
        /** @brief The compute service on which the job has completed */
        std::shared_ptr<ComputeService> compute_service;
    };

    /**
     * @brief A message sent by a ComputeService when a CompoundJob has failed to execute
     */
    class ComputeServiceCompoundJobFailedMessage : public ComputeServiceMessage {
    public:
        ComputeServiceCompoundJobFailedMessage(std::shared_ptr<CompoundJob>, std::shared_ptr<ComputeService>,
                                               double payload);

        /** @brief The job that has failed */
        std::shared_ptr<CompoundJob> job;
        /** @brief The compute service on which the job has failed */
        std::shared_ptr<ComputeService> compute_service;
    };

    /**
    * @brief A message sent to a ComputeService to terminate a CompoundJob previously submitted for execution
    */
    class ComputeServiceTerminateCompoundJobRequestMessage : public ComputeServiceMessage {
    public:
        ComputeServiceTerminateCompoundJobRequestMessage(S4U_CommPort *answer_commport, std::shared_ptr<CompoundJob>, double payload);

        /** @brief The commport_name to which the answer message should be sent */
        S4U_CommPort *answer_commport;
        /** @brief The job to terminate*/
        std::shared_ptr<CompoundJob> job;
    };

    /**
     * @brief A message sent by a ComputeService in answer to a CompoundJob termination request
     */
    class ComputeServiceTerminateCompoundJobAnswerMessage : public ComputeServiceMessage {
    public:
        ComputeServiceTerminateCompoundJobAnswerMessage(std::shared_ptr<CompoundJob>, std::shared_ptr<ComputeService>, bool success,
                                                        std::shared_ptr<FailureCause> failure_cause, double payload);

        /** @brief The standard job to terminate */
        std::shared_ptr<CompoundJob> job;
        /** @brief The compute service to which the job had been submitted */
        std::shared_ptr<ComputeService> compute_service;
        /** @brief Whether to job termination was successful */
        bool success;
        /** @brief The cause of the failure, or nullptr on success */
        std::shared_ptr<FailureCause> failure_cause;
    };


    /**
     * @brief A message sent by a ComputeService when a PilotJob has started its execution
     */
    class ComputeServicePilotJobStartedMessage : public ComputeServiceMessage {
    public:
        ComputeServicePilotJobStartedMessage(std::shared_ptr<PilotJob>, std::shared_ptr<ComputeService>, double payload);

        /** @brief The pilot job that has started */
        std::shared_ptr<PilotJob> job;
        /** @brief The compute service on which the pilot job has started */
        std::shared_ptr<ComputeService> compute_service;
    };

    /**
     * @brief A message sent by a ComputeService when a PilotJob has expired
     */
    class ComputeServicePilotJobExpiredMessage : public ComputeServiceMessage {
    public:
        ComputeServicePilotJobExpiredMessage(std::shared_ptr<PilotJob>, std::shared_ptr<ComputeService>, double payload);

        /** @brief The pilot job that has expired */
        std::shared_ptr<PilotJob> job;
        /** @brief The compute service on which the pilot job has expired */
        std::shared_ptr<ComputeService> compute_service;
    };


    /**
     * @brief A message sent to a ComputeService to request information on its compute resources
    */
    class ComputeServiceResourceInformationRequestMessage : public ComputeServiceMessage {
    public:
        ComputeServiceResourceInformationRequestMessage(S4U_CommPort *answer_commport, const std::string &key, double payload);

        /** @brief The commport_name to which the answer should be sent */
        S4U_CommPort *answer_commport;
        /** @brief The key (i.e., resource information name) desired */
        std::string key;
    };

    /**
     * @brief A message sent by a ComputeService in answer to a resource information request
     */
    class ComputeServiceResourceInformationAnswerMessage : public ComputeServiceMessage {
    public:
        ComputeServiceResourceInformationAnswerMessage(std::map<std::string, double> info,
                                                       double payload);

        /** @brief The resource information map */
        std::map<std::string, double> info;
    };

    /**
     * @brief A message sent to a ComputeService to asks if at least one host has some available resources right now
     */
    class ComputeServiceIsThereAtLeastOneHostWithAvailableResourcesRequestMessage : public ComputeServiceMessage {
    public:
        ComputeServiceIsThereAtLeastOneHostWithAvailableResourcesRequestMessage(S4U_CommPort *answer_commport,
                                                                                unsigned long num_cores,
                                                                                double ram, double payload);

        /** @brief The commport_name to which a reply should be sent */
        S4U_CommPort *answer_commport;
        /** @brief The number of cores desired */
        unsigned long num_cores;
        /** @brief The RAM desired */
        double ram;
    };

    /**
     * @brief A message sent by a ComputeService in answer to a "does at least one host have these available resources" request
     */
    class ComputeServiceIsThereAtLeastOneHostWithAvailableResourcesAnswerMessage : public ComputeServiceMessage {
    public:
        ComputeServiceIsThereAtLeastOneHostWithAvailableResourcesAnswerMessage(bool answer, double payload);

        /** @brief The true/false answer */
        bool answer;
    };

    /***********************/
    /** \endcond           */
    /***********************/

}// namespace wrench

#endif//WRENCH_COMPUTESERVICEMESSAGE_H
