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

    class WorkflowExecutionFailureCause {

    public:

        enum Cause {
            NO_STORAGE_SERVICE_FOR_FILE,
            FILE_NOT_FOUND,
            STORAGE_NO_ENOUGH_SPACE,
            SERVICE_TERMINATED,
            JOB_TYPE_NOT_SUPPORTED,
            NOT_ENOUGH_CORES,
            NETWORK_ERROR
        };

        WorkflowExecutionFailureCause(Cause cause);

        virtual std::string toString() = 0;

        Cause getCause();

    protected:
        Cause cause;
    };


    class NoStorageServiceForFile : public WorkflowExecutionFailureCause {

    public:
        NoStorageServiceForFile(WorkflowFile *file);

        WorkflowFile *getFile();
        std::string toString();

    private:
        WorkflowFile *file;
    };


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

    class ServiceIsDown : public WorkflowExecutionFailureCause {
    public:
        ServiceIsDown(Service *service);
        Service *getService();
        std::string toString();

    private:
        Service *service;
    };

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

    class NetworkError : public WorkflowExecutionFailureCause {
    public:
        NetworkError();
        std::string toString();

    private:
    };
};


#endif //WRENCH_STANDARDJOBFAILURECAUSE_H
