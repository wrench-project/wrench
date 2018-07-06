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

    class SimulationTimestampType {
    public:
        SimulationTimestampType();
        SimulationTimestampType(SimulationTimestampType *endpoint);

        virtual ~SimulationTimestampType() {}

        double getDate();

        virtual SimulationTimestampType *getEndpoint();

    protected:
        SimulationTimestampType *endpoint;

    private:
        double date = -1.0;
    };

    /**
    * @brief A base class for simulation timestamps regarding workflow tasks
    */
    class SimulationTimestampTask : public SimulationTimestampType {

    public:

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        /**
         * @brief Constructor
         * @param task: a workflow task
         */
        SimulationTimestampTask(WorkflowTask *);

        /***********************/
        /** \endcond           */
        /***********************/

        /**
         * @brief Retrieve the task that has completed
         *
         * @return the task
         */
        WorkflowTask *getTask();
        SimulationTimestampTask *getEndpoint();

    protected:
        static std::map<std::string, SimulationTimestampTask *> pending_task_timestamps;

        void setEndpoints();

    private:
        WorkflowTask *task;
    };

    class SimulationTimestampTaskStart : public SimulationTimestampTask {
    public:
        SimulationTimestampTaskStart(WorkflowTask *);
    };

    class SimulationTimestampTaskFailure : public SimulationTimestampTask {
    public:
        SimulationTimestampTaskFailure(WorkflowTask *);
    };

    class SimulationTimestampTaskCompletion : public SimulationTimestampTask {
    public:
        SimulationTimestampTaskCompletion(WorkflowTask *);
    };

    class SimulationTimestampFileCopyStart;

    class SimulationTimestampFileCopy : public SimulationTimestampType {
    public:

        SimulationTimestampFileCopy(WorkflowFile *file, StorageService *src, std::string src_partition, StorageService *dst, std::string dst_partition, SimulationTimestampFileCopyStart *start_timestamp = nullptr);

        struct FileLocation {
            StorageService *storage_service;
            std::string partition;

            FileLocation(StorageService *storage_service, std::string partition) : storage_service(storage_service), partition(partition) {

            }

            bool operator==(const FileLocation &rhs) {
                return (this->storage_service == rhs.storage_service) && (this->partition == rhs.partition);
            }

            bool operator!=(const FileLocation &rhs) {
                return !FileLocation::operator==(rhs);
            }
        };

        SimulationTimestampFileCopy *getEndpoint() override;
        WorkflowFile *getFile();
        FileLocation getSource();
        FileLocation getDestination();

    protected:
        WorkflowFile *file;

        FileLocation source;
        FileLocation destination;
    };

    class SimulationTimestampFileCopyFailure;
    class SimulationTimestampFileCopyCompletion;

    class SimulationTimestampFileCopyStart : public SimulationTimestampFileCopy {
    public:
        SimulationTimestampFileCopyStart(WorkflowFile *file, StorageService *src, std::string src_partition, StorageService *dst, std::string dst_partition);

        friend class SimulationTimestampFileCopyFailure;
        friend class SimulationTimestampFileCopyCompletion;
    };

    class SimulationTimestampFileCopyFailure : public SimulationTimestampFileCopy {
    public:
        SimulationTimestampFileCopyFailure(SimulationTimestampFileCopyStart *start_timestamp);
    };

    class SimulationTimestampFileCopyCompletion : public SimulationTimestampFileCopy {
    public:
        SimulationTimestampFileCopyCompletion(SimulationTimestampFileCopyStart *start_timestamp);
    };
};

#endif //WRENCH_SIMULATIONTIMESTAMPTYPES_H
