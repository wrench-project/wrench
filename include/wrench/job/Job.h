/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_JOB_H
#define WRENCH_JOB_H


#include <string>
#include <stack>
#include <map>
#include <memory>

#include "wrench/managers/job_manager/JobManager.h"

namespace wrench {

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    class Workflow;
    class ComputeService;


    /**
     * @brief Abstraction of a job used for executing tasks in a Workflow
     */
    class Job {

    public:
        std::string getName();

        double getSubmitDate() const;
        double getEndDate() const;

        /***********************/
        /** \cond INTERNAL     */
        /***********************/

        S4U_CommPort *popCallbackCommPort();

        void printCallbackCommPortStack();

        void pushCallbackCommPort(S4U_CommPort *commport);

        S4U_CommPort *getCallbackCommPort();

        S4U_CommPort *getOriginCallbackCommPort();

        void setParentComputeService(std::shared_ptr<ComputeService> compute_service);

        std::shared_ptr<ComputeService> getParentComputeService();


        virtual ~Job();

        virtual void setPriority(double p);

        double getPriority() const;

        std::map<std::string, std::string> &getServiceSpecificArguments();

    protected:
        friend class JobManager;

        Job(std::string name, std::shared_ptr<JobManager> job_manager);

        void setServiceSpecificArguments(std::map<std::string, std::string> args);

        static unsigned long getNewUniqueNumber();

        /** @brief Service-specific arguments used during job submission **/
        std::map<std::string, std::string> service_specific_args;

        /** @brief Stack of callback commports (to pop notifications) */
        std::stack<S4U_CommPort *> callback_commport_stack;
        /** @brief The Job Manager in charge of this job **/
        std::shared_ptr<JobManager> job_manager;
        /** @brief The originator's commport_name */
        S4U_CommPort *originator_commport;

        /** @brief The job's name */
        std::string name;
        /** @brief The date at which the job was last submitted */
        double submit_date;
        /** @brief The date at which the job ended (with success or failure) */
        double end_date;
        /** @brief The compute service to which the job was submitted */
        std::shared_ptr<ComputeService> parent_compute_service;

        /** @brief Whether the job has already been submitted to the job manager */
        bool already_submitted_to_job_manager = false;

        /** @brief The job's priority (the higher the number, the higher the priority) */
        double priority = 0.0;

    private:
        /***********************/
        /** \endcond           */
        /***********************/
    };

    /***********************/
    /** \endcond           */
    /***********************/

}// namespace wrench


#endif//WRENCH_JOB_H
