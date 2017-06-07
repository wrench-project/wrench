/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_MULTICORECOMPUTESERVICEMESSAGE_H
#define WRENCH_MULTICORECOMPUTESERVICEMESSAGE_H


#include <services/compute_services/ComputeServiceMessage.h>
#include <vector>
#include "WorkerThread.h"

namespace wrench {

    class WorkflowTask;
    class WorkerThread;

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    class MulticoreComputeServiceMessage : public ComputeServiceMessage {
    protected:
        MulticoreComputeServiceMessage(std::string name, double payload);
    };

    /**
     * @brief "NOT_ENOUGH_CORES" SimulationMessage class
     */
    class MulticoreComputeServiceNotEnoughCoresMessage : public MulticoreComputeServiceMessage {
    public:
        MulticoreComputeServiceNotEnoughCoresMessage(WorkflowJob *, ComputeService *, double payload);

        WorkflowJob *job;
        ComputeService *compute_service;
    };


    /**
     * @brief "NUM_CORES_REQUEST" SimulationMessage class
     */
    class MulticoreComputeServiceNumCoresRequestMessage : public MulticoreComputeServiceMessage {
    public:
        MulticoreComputeServiceNumCoresRequestMessage(std::string answer_mailbox, double payload);

        std::string answer_mailbox;
    };

    /**
     * @brief "NUM_CORES_CORES_ANSWER" SimulationMessage class
     */
    class MulticoreComputeServiceNumCoresAnswerMessage : public MulticoreComputeServiceMessage {
    public:
        MulticoreComputeServiceNumCoresAnswerMessage(unsigned int num, double payload);

        unsigned int num_cores;
    };

    /**
  * @brief "NUM_IDLE_CORES_REQUEST" SimulationMessage class
  */
    class MulticoreComputeServiceNumIdleCoresRequestMessage : public MulticoreComputeServiceMessage {
    public:
        MulticoreComputeServiceNumIdleCoresRequestMessage(std::string answer_mailbox, double payload);

        std::string answer_mailbox;
    };

    /**
     * @brief "NUM_IDLE_CORES_ANSWER" SimulationMessage class
     */
    class MulticoreComputeServiceNumIdleCoresAnswerMessage : public MulticoreComputeServiceMessage {
    public:
        MulticoreComputeServiceNumIdleCoresAnswerMessage(unsigned int num, double payload);

        unsigned int num_idle_cores;
    };

    /**
     * @brief "TTL_REQUEST" SimulationMessage class
     */
    class MulticoreComputeServiceTTLRequestMessage : public MulticoreComputeServiceMessage {
    public:
        MulticoreComputeServiceTTLRequestMessage(std::string answer_mailbox, double payload);

        std::string answer_mailbox;
    };

    /**
     * @brief "TTL_ANSWER" SimulationMessage class
     */
    class MulticoreComputeServiceTTLAnswerMessage : public MulticoreComputeServiceMessage {
    public:
        MulticoreComputeServiceTTLAnswerMessage(double ttl, double payload);

        double ttl;
    };

    /**
     * @brief "FLOP_RATE_REQUEST" SimulationMessage class
     */
    class MulticoreComputeServiceFlopRateRequestMessage : public MulticoreComputeServiceMessage {
    public:
        MulticoreComputeServiceFlopRateRequestMessage(std::string answer_mailbox, double payload);

        std::string answer_mailbox;
    };

    /**
     * @brief "FLOP_RATE_ANSWER" SimulationMessage class
     */
    class MulticoreComputeServiceFlopRateAnswerMessage : public MulticoreComputeServiceMessage {
    public:
        MulticoreComputeServiceFlopRateAnswerMessage(double flop_rate, double payload);

        double flop_rate;
    };

    /**
     * @brief "WORKER_THREAD_DO_WORK_REQUEST" SimulationMessage class
     */
    class WorkerThreadDoWorkRequestMessage : public MulticoreComputeServiceMessage {
    public:
        WorkerThreadDoWorkRequestMessage(
                WorkUnit *work,
                double payload);

        WorkUnit *work;
    };

    /**
     * @brief "WORKER_THREAD_WORK_DONE" SimulationMessage class
     */
    class WorkerThreadWorkDoneMessage : public MulticoreComputeServiceMessage {
    public:
        WorkerThreadWorkDoneMessage(
                WorkerThread *worker_thread,
                WorkUnit *work_unit,
                double payload);

        WorkerThread *worker_thread;
        WorkUnit *work;

    };

    /**
     * @brief "WORKER_THREAD_WORK_FAILED" SimulationMessage class
     */
    class WorkerThreadWorkFailedMessage : public MulticoreComputeServiceMessage {
    public:
        WorkerThreadWorkFailedMessage(
                WorkerThread *worker_thread,
                WorkUnit *work,
                WorkflowExecutionFailureCause *cause,
                double payload);

        WorkerThread *worker_thread;
        WorkUnit *work;

        WorkflowExecutionFailureCause *cause;
    };


    /***********************/
    /** \endcond           */
    /***********************/
};


#endif //WRENCH_MULTICORECOMPUTESERVICEMESSAGE_H
