/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_MULTICOREJOBEXECUTORMESSAGE_H
#define WRENCH_MULTICOREJOBEXECUTORMESSAGE_H


#include <services/compute_services/ComputeServiceMessage.h>

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    class MulticoreJobExecutorMessage : public ComputeServiceMessage {
    protected:
        MulticoreJobExecutorMessage(std::string name, double payload);
    };

    /**
     * @brief "NOT_ENOUGH_CORES" SimulationMessage class
     */
    class MulticoreJobExecutorNotEnoughCoresMessage : public MulticoreJobExecutorMessage {
    public:
        MulticoreJobExecutorNotEnoughCoresMessage(WorkflowJob *, ComputeService *, double payload);

        WorkflowJob *job;
        ComputeService *compute_service;
    };


    /**
     * @brief "NUM_CORES_REQUEST" SimulationMessage class
     */
    class MulticoreJobExecutorNumCoresRequestMessage : public MulticoreJobExecutorMessage {
    public:
        MulticoreJobExecutorNumCoresRequestMessage(std::string answer_mailbox, double payload);

        std::string answer_mailbox;
    };

    /**
     * @brief "NUM_CORES_CORES_ANSWER" SimulationMessage class
     */
    class MulticoreJobExecutorNumCoresAnswerMessage : public MulticoreJobExecutorMessage {
    public:
        MulticoreJobExecutorNumCoresAnswerMessage(unsigned int num, double payload);

        unsigned int num_cores;
    };

    /**
  * @brief "NUM_IDLE_CORES_REQUEST" SimulationMessage class
  */
    class MulticoreJobExecutorNumIdleCoresRequestMessage : public MulticoreJobExecutorMessage {
    public:
        MulticoreJobExecutorNumIdleCoresRequestMessage(std::string answer_mailbox, double payload);

        std::string answer_mailbox;
    };

    /**
     * @brief "NUM_IDLE_CORES_ANSWER" SimulationMessage class
     */
    class MulticoreJobExecutorNumIdleCoresAnswerMessage : public MulticoreJobExecutorMessage {
    public:
        MulticoreJobExecutorNumIdleCoresAnswerMessage(unsigned int num, double payload);

        unsigned int num_idle_cores;
    };

    /**
     * @brief "TTL_REQUEST" SimulationMessage class
     */
    class MulticoreJobExecutorTTLRequestMessage : public MulticoreJobExecutorMessage {
    public:
        MulticoreJobExecutorTTLRequestMessage(std::string answer_mailbox, double payload);

        std::string answer_mailbox;
    };

    /**
     * @brief "TTL_ANSWER" SimulationMessage class
     */
    class MulticoreJobExecutorTTLAnswerMessage : public MulticoreJobExecutorMessage {
    public:
        MulticoreJobExecutorTTLAnswerMessage(double ttl, double payload);

        double ttl;
    };

    /**
     * @brief "FLOP_RATE_REQUEST" SimulationMessage class
     */
    class MulticoreJobExecutorFlopRateRequestMessage : public MulticoreJobExecutorMessage {
    public:
        MulticoreJobExecutorFlopRateRequestMessage(std::string answer_mailbox, double payload);

        std::string answer_mailbox;
    };

    /**
     * @brief "FLOP_RATE_ANSWER" SimulationMessage class
     */
    class MulticoreJobExecutorFlopRateAnswerMessage : public MulticoreJobExecutorMessage {
    public:
        MulticoreJobExecutorFlopRateAnswerMessage(double flop_rate, double payload);

        double flop_rate;
    };

    /***********************/
    /** \endcond           */
    /***********************/
};


#endif //WRENCH_MULTICOREJOBEXECUTORMESSAGE_H
