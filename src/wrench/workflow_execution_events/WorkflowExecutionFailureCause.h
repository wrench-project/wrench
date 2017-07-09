/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_STANDARDJOBFAILURECAUSE_H
#define WRENCH_STANDARDJOBFAILURECAUSE_H


#include <set>
#include <string>

namespace wrench {

    class Service;
    class WorkflowFile;
    class StorageService;
    class ComputeService;
    class WorkflowJob;

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
     * @brief A top-level class to describe all simulation-valid failures that can occur during
     *        workflow execution
     */
    class WorkflowExecutionFailureCause {

    public:

        /** @brief Types of failure causes */
        enum CauseType {
            /** @brief The file cannot be found anywhere */
            NO_STORAGE_SERVICE_FOR_FILE,
            /** @brief The file was not found where it was supposed to be found */
            FILE_NOT_FOUND,
            /** @brief The storage service does not have enough space to support operation */
            STORAGE_NO_ENOUGH_SPACE,
            /** @brief The service cannot be used because it was terminated */
            SERVICE_TERMINATED,
            /** @brief The compute service does not support this job type */
            JOB_TYPE_NOT_SUPPORTED,
            /** @brief The compute service cannot run the job due to insufficient total number of cores */
            NOT_ENOUGH_CORES,
            /** @brief There was a network error, or an endpoint was down */
            NETWORK_ERROR,
            /** @brief The job cannot be terminated because it's neither pending nor running */
            JOB_CANNOT_BE_TERMINATED,
            /** @brief The job cannot be forgotten because it's not completed */
            JOB_CANNOT_BE_FORGOTTEN

        };

        WorkflowExecutionFailureCause(CauseType cause);

        virtual std::string toString() = 0;

        CauseType getCauseType();

    private:
        CauseType cause;
    };


    /**
     * @brief A "file cannot be found anywhere" workflow execution failure cause
     */
    class NoStorageServiceForFile : public WorkflowExecutionFailureCause {

    public:
        NoStorageServiceForFile(WorkflowFile *file);

        WorkflowFile *getFile();
        std::string toString();

    private:
        WorkflowFile *file;
    };

    /**
     * @brief A "file is not found" workflow execution failure cause
     */
    class FileNotFound : public WorkflowExecutionFailureCause {

    public:
        FileNotFound(WorkflowFile *file, StorageService *storage_service);

        WorkflowFile *getFile();
        StorageService *getStorageService();
        std::string toString();


    private:
        WorkflowFile *file;
        StorageService *storage_service;
    };

    /**
     * @brief A "no space left on storage service" workflow execution failure cause
     */
    class StorageServiceFull : public WorkflowExecutionFailureCause {

    public:
        StorageServiceFull(WorkflowFile *file, StorageService *storage_service);

        WorkflowFile *getFile();
        StorageService *getStorageService();
        std::string toString();


    private:
        WorkflowFile *file;
        StorageService *storage_service;
    };

    /**
     * @brief A "service is down" workflow execution failure cause
     */
    class ServiceIsDown : public WorkflowExecutionFailureCause {
    public:
        ServiceIsDown(Service *service);
        Service *getService();
        std::string toString();

    private:
        Service *service;
    };

    /**
     * @brief A "compute service does not support requested job type" workflow execution failure cause
     */
    class JobTypeNotSupported : public WorkflowExecutionFailureCause {
    public:
        JobTypeNotSupported(WorkflowJob *job, ComputeService *compute_service);
        WorkflowJob *getJob();
        ComputeService *getComputeService();
        std::string toString();

    private:
        WorkflowJob *job;
        ComputeService *compute_service;
    };

    /**
     * @brief A "compute service doesn't have enough cores" workflow execution failure cause
     */
    class NotEnoughCores : public WorkflowExecutionFailureCause {
    public:
        NotEnoughCores(WorkflowJob *job, ComputeService *compute_service);
        WorkflowJob *getJob();
        ComputeService *getComputeService();
        std::string toString();

    private:
        WorkflowJob *job;
        ComputeService *compute_service;
    };

    /**
     * @brief A "network error (or endpoint is down)" workflow execution failure cause
     */
    class NetworkError : public WorkflowExecutionFailureCause {
    public:
        NetworkError();
        std::string toString();

    private:
    };

    /**
     * @brief A "job cannot be terminated" workflow execution failure cause
     */
    class JobCannotBeTerminated : public WorkflowExecutionFailureCause {
    public:
        JobCannotBeTerminated(WorkflowJob *job);
        WorkflowJob *getJob();
        std::string toString();

    private:
        WorkflowJob *job;
    };

    /**
    * @brief A "job cannot be forgotten" workflow execution failure cause
    */
    class JobCannotBeForgotten : public WorkflowExecutionFailureCause {
    public:
        JobCannotBeForgotten(WorkflowJob *job);
        WorkflowJob *getJob();
        std::string toString();

    private:
        WorkflowJob *job;
    };

    /***********************/
    /** \endcond           */
    /***********************/
};


#endif //WRENCH_STANDARDJOBFAILURECAUSE_H
