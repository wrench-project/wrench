/**
* Copyright (c) 2017. The WRENCH Team.
*
* This program is free software: you can redistribute it and/or modify
        * it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/

#ifndef WRENCH_SIMULATIONTIMESTAMPTYPES_H
#define WRENCH_SIMULATIONTIMESTAMPTYPES_H

#include "wrench/workflow/WorkflowTask.h"

namespace wrench {

    class WorkflowTask;
    class StorageService;

    /**
     * @brief A top-level base class for simulation timestamps
     */
    class SimulationTimestampType {
    public:
        SimulationTimestampType();
        double getDate();

    private:
        double date = -0.1;
    };

    /**
     * @brief A base class for simulation timestamps
     */
    class SimulationTimestampPair : public SimulationTimestampType {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        SimulationTimestampPair();
        SimulationTimestampPair(SimulationTimestampPair *endpoint);
        virtual ~SimulationTimestampPair() {}
        /***********************/
        /** \endcond           */
        /***********************/

        virtual SimulationTimestampPair *getEndpoint();

    protected:
        /** @brief An optional associated "endpoint" simulation timestamp */
        SimulationTimestampPair *endpoint = nullptr;
    };

    /**
    * @brief A base class for simulation timestamps regarding workflow tasks
    */
    class SimulationTimestampTask : public SimulationTimestampPair {

    public:

        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        SimulationTimestampTask(WorkflowTask *);
        /***********************/
        /** \endcond           */
        /***********************/

        WorkflowTask *getTask();
        SimulationTimestampTask *getEndpoint() override;

    protected:
        static std::map<std::string, SimulationTimestampTask *> pending_task_timestamps;

        void setEndpoints();

    private:
        WorkflowTask *task;
    };

    /**
     * @brief A simulation timestamp class for WorkflowTask start times
     */
    class SimulationTimestampTaskStart : public SimulationTimestampTask {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        SimulationTimestampTaskStart(WorkflowTask *);
        /***********************/
        /** \endcond           */
        /***********************/
    };

    /**
     * @brief A simulation timestamp class for WorkflowTask failure times
     */
    class SimulationTimestampTaskFailure : public SimulationTimestampTask {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        SimulationTimestampTaskFailure(WorkflowTask *);
        /***********************/
        /** \endcond           */
        /***********************/

    };

    /**
     * @brief A simulation timestamp class for WorkflowTask completion times
     */
    class SimulationTimestampTaskCompletion : public SimulationTimestampTask {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        SimulationTimestampTaskCompletion(WorkflowTask *);
        /***********************/
        /** \endcond           */
        /***********************/
    };

    /**
     * @brief A simulation timestamp class for WorkflowTask termination times
     */
     class SimulationTimestampTaskTermination : public SimulationTimestampTask {
     public:
         /***********************/
         /** \cond INTERNAL     */
         /***********************/
         SimulationTimestampTaskTermination(WorkflowTask *);
         /***********************/
         /** \endcond           */
         /***********************/
     };

    class SimulationTimestampFileCopyStart;
    class FileLocation;

    /**
     * @brief A base class for simulation timestamps regarding file copies
     */
    class SimulationTimestampFileCopy : public SimulationTimestampPair {
    public:

        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        SimulationTimestampFileCopy(WorkflowFile *file, std::shared_ptr<FileLocation>  src, std::shared_ptr<FileLocation> dst, SimulationTimestampFileCopyStart *start_timestamp = nullptr);
        /***********************/
        /** \endcond           */
        /***********************/

        /**
         * @brief Retrieve the matching endpoint, if any
         */
        SimulationTimestampFileCopy *getEndpoint() override;
        WorkflowFile *getFile();
        std::shared_ptr<FileLocation> getSource();
        std::shared_ptr<FileLocation> getDestination();

    protected:
        /** 
         * @brief The WorkflowFile that was being copied
         */
        WorkflowFile *file;

        /**
         * @brief The location where the WorkflowFile was being copied from
         */
        std::shared_ptr<FileLocation> source;

        /**
         * @brief The location where the WorkflowFile was being copied to
         */
        std::shared_ptr<FileLocation> destination;
    };

    class SimulationTimestampFileCopyFailure;
    class SimulationTimestampFileCopyCompletion;

    /**
     * @brief A simulation timestamp class for file copy start times
     */
    class SimulationTimestampFileCopyStart : public SimulationTimestampFileCopy {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        SimulationTimestampFileCopyStart(WorkflowFile *file, std::shared_ptr<FileLocation> src, std::shared_ptr<FileLocation> dst);
        /***********************/
        /** \endcond           */
        /***********************/

        friend class SimulationTimestampFileCopyFailure;
        friend class SimulationTimestampFileCopyCompletion;
    };

    /**
     * @brief A simulation timestamp class for file copy failure times
     */
    class SimulationTimestampFileCopyFailure : public SimulationTimestampFileCopy {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        SimulationTimestampFileCopyFailure(SimulationTimestampFileCopyStart *start_timestamp);
        /***********************/
        /** \endcond           */
        /***********************/
    };

    /**
     * @brief A simulation timestamp class for file copy completions
     */
    class SimulationTimestampFileCopyCompletion : public SimulationTimestampFileCopy {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        SimulationTimestampFileCopyCompletion(SimulationTimestampFileCopyStart *start_timestamp);
        /***********************/
        /** \endcond           */
        /***********************/
    };




    /**
     * @brief A simulation timestamp class for changes in a host's pstate
     */
    class SimulationTimestampPstateSet : public SimulationTimestampType {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        SimulationTimestampPstateSet(std::string hostname, int pstate);
        /***********************/
        /** \endcond           */
        /***********************/

        std::string getHostname();
        int getPstate();
    private:
        std::string hostname;
        int pstate;
    };

    /**
     * @brief A simulation timestamp class for energy consumption
     */
    class SimulationTimestampEnergyConsumption: public SimulationTimestampType {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        SimulationTimestampEnergyConsumption(std::string hostname, double joules);
        /***********************/
        /** \endcond           */
        /***********************/

        std::string getHostname();
        double getConsumption();

    private:
        std::string hostname;
        double joules;
    };
};

#endif //WRENCH_SIMULATIONTIMESTAMPTYPES_H
