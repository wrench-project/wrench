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
#include "WorkUnitMulticoreExecutor.h"

namespace wrench {

    class WorkflowTask;

    class WorkUnitMultiCoreExecutor;

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief Top-level MulticoreComputeServiceMessage class
     */
    class MulticoreComputeServiceMessage : public ComputeServiceMessage {
    protected:
        MulticoreComputeServiceMessage(std::string name, double payload);
    };

    /**
     * @brief MulticoreComputeServiceNotEnoughCoresMessage class
     */
    class MulticoreComputeServiceNotEnoughCoresMessage : public MulticoreComputeServiceMessage {
    public:
        MulticoreComputeServiceNotEnoughCoresMessage(WorkflowJob *, ComputeService *, double payload);

        /** @brief The job that couldn't run due to not enough cores */
        WorkflowJob *job;
        /** @brief The compute service on which there weren't enough cores */
        ComputeService *compute_service;
    };

    /**
     * @brief MulticoreComputeServiceNumCoresRequestMessage class
     */
    class MulticoreComputeServiceNumCoresRequestMessage : public MulticoreComputeServiceMessage {
    public:
        MulticoreComputeServiceNumCoresRequestMessage(std::string answer_mailbox, double payload);

        /** @brief The mailbox to which the answer message should be sent */
        std::string answer_mailbox;
    };

    /**
     * @brief MulticoreComputeServiceNumCoresAnswerMessage class
     */
    class MulticoreComputeServiceNumCoresAnswerMessage : public MulticoreComputeServiceMessage {
    public:
        MulticoreComputeServiceNumCoresAnswerMessage(unsigned int num, double payload);

        /** @brief The number of cores */
        unsigned int num_cores;
    };

    /**
  * @brief MulticoreComputeServiceNumIdleCoresRequestMessage class
  */
    class MulticoreComputeServiceNumIdleCoresRequestMessage : public MulticoreComputeServiceMessage {
    public:
        MulticoreComputeServiceNumIdleCoresRequestMessage(std::string answer_mailbox, double payload);

        /** @brief The mailbox to which the answer message should be sent */
        std::string answer_mailbox;
    };

    /**
     * @brief MulticoreComputeServiceNumIdleCoresAnswerMessage class
     */
    class MulticoreComputeServiceNumIdleCoresAnswerMessage : public MulticoreComputeServiceMessage {
    public:
        MulticoreComputeServiceNumIdleCoresAnswerMessage(unsigned int num, double payload);

        /** @brief The number of idle cores */
        unsigned int num_idle_cores;
    };

    /**
     * @brief MulticoreComputeServiceTTLRequestMessage class
     */
    class MulticoreComputeServiceTTLRequestMessage : public MulticoreComputeServiceMessage {
    public:
        MulticoreComputeServiceTTLRequestMessage(std::string answer_mailbox, double payload);

        /** @brief The mailbox to which the answer message should be sent */
        std::string answer_mailbox;
    };

    /**
     * @brief MulticoreComputeServiceTTLAnswerMessage class
     */
    class MulticoreComputeServiceTTLAnswerMessage : public MulticoreComputeServiceMessage {
    public:
        MulticoreComputeServiceTTLAnswerMessage(double ttl, double payload);

        /** @brief The time-to-live */
        double ttl;
    };

    /**
     * @brief MulticoreComputeServiceFlopRateRequestMessage class
     */
    class MulticoreComputeServiceFlopRateRequestMessage : public MulticoreComputeServiceMessage {
    public:
        MulticoreComputeServiceFlopRateRequestMessage(std::string answer_mailbox, double payload);

        /** @brief The mailbox to which the answer message should be sent */
        std::string answer_mailbox;
    };

    /**
     * @brief MulticoreComputeServiceFlopRateAnswerMessage class
     */
    class MulticoreComputeServiceFlopRateAnswerMessage : public MulticoreComputeServiceMessage {
    public:
        MulticoreComputeServiceFlopRateAnswerMessage(double flop_rate, double payload);

        /** @brief The flop rate */
        double flop_rate;
    };

    /**
     * @brief WorkerThreadDoWorkRequestMessage class
     */
    class WorkerThreadDoWorkRequestMessage : public MulticoreComputeServiceMessage {
    public:
        WorkerThreadDoWorkRequestMessage(
                WorkUnit *work,
                double payload);

        /** @brief The work to be performed */
        WorkUnit *work;
    };

    /**
     * @brief WorkerThreadWorkDoneMessage class
     */
    class WorkerThreadWorkDoneMessage : public MulticoreComputeServiceMessage {
    public:
        WorkerThreadWorkDoneMessage(
                WorkUnitMultiCoreExecutor *worker_thread,
                std::shared_ptr<WorkUnit> work_unit,
                double payload);

        /** @brief The worker thread that performed the work */
        WorkUnitMultiCoreExecutor *worker_thread;
        /** @brief The work that was performed */
        std::shared_ptr<WorkUnit> work;

    };

    /**
     * @brief WorkerThreadWorkFailedMessage class
     */
    class WorkerThreadWorkFailedMessage : public MulticoreComputeServiceMessage {
    public:
        WorkerThreadWorkFailedMessage(
                WorkUnitMultiCoreExecutor *worker_thread,
                std::shared_ptr<WorkUnit> work,
                std::shared_ptr<FailureCause> cause,
                double payload);

        /** @brief The worker thread that failed to perform the work */
        WorkUnitMultiCoreExecutor *worker_thread;
        /** @brief The work that failed */
        std::shared_ptr<WorkUnit> work;
        /** @brief The cause of the failure */
        std::shared_ptr<FailureCause> cause;
    };

    /***********************/
    /** \endcond           */
    /***********************/
};

#endif //WRENCH_MULTICORECOMPUTESERVICEMESSAGE_H
