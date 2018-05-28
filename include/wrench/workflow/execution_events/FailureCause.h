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
     *
     */
    class FailureCause {

    public:

        /** @brief Types of failure causes */
        enum CauseType {
            /** @brief Unknown cause */
                    FATAL_FAILURE,
            /** @brief The file cannot be found anywhere */
                    NO_STORAGE_SERVICE_FOR_FILE,
            /** @brief No Scratch Space available */
                    NO_SCRATCH_SPACE,
            /** @brief The file was not found where it was supposed to be found */
                    FILE_NOT_FOUND,
            /** @brief The file to be written was already there */
                    FILE_ALREADY_THERE,
            /** @brief The file is already being copied there */
                    FILE_ALREADY_BEING_COPIED,
            /** @brief The storage service does not have enough space to support operation */
                    STORAGE_NOT_ENOUGH_SPACE,
            /** @brief The service cannot be used because it is down (likely was terminated) */
                    SERVICE_DOWN,
            /** @brief The compute service does not support this job type */
                    JOB_TYPE_NOT_SUPPORTED,
            /** @brief The compute service cannot run the job (ever) due to insufficient resources */
                    NOT_ENOUGH_COMPUTE_RESOURCES,
            /** @brief There was a network error, or an endpoint was down */
                    NETWORK_ERROR,
            /** @brief There was a network timeout (for a "with timeout" network operation) */
                    NETWORK_TIMEOUT,
            /** @brief The job cannot be terminated because it's neither pending nor running */
                    JOB_CANNOT_BE_TERMINATED,
            /** @brief The job cannot be forgotten because it's not completed */
                    JOB_CANNOT_BE_FORGOTTEN,
            /** @brief A compute thread has died */
                    COMPUTE_THREAD_HAS_DIED,
            /** @brief A functionality is not available */
                    FUNCTIONALITY_NOT_AVAILABLE,
            /** @brief A job was terminated due to a timeout */
                    JOB_TIMEOUT

        };

        FailureCause(CauseType cause);


        /**
         * @brief Return an error message that describes the failure cause (to be overriden)
         *
         * @return an error message
         */
        virtual std::string toString() = 0;

        CauseType getCauseType();

    private:
        CauseType cause;
    };


    /**
     * @brief A "file cannot be found anywhere" failure cause
     */
    class NoStorageServiceForFile : public FailureCause {

    public:
        NoStorageServiceForFile(WorkflowFile *file);

        WorkflowFile *getFile();
        std::string toString();

    private:
        WorkflowFile *file;
    };


    /**
     * @brief No Scratch Space failure cause
     */
    class NoScratchSpace : public FailureCause {

    public:
        NoScratchSpace(std::string error);
        std::string toString();
    private:
        std::string error;
    };

    /**
     * @brief A "file was not found" failure cause
     */
    class FileNotFound : public FailureCause {

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
     * @brief A "not enough space on storage service" failure cause
     */
    class StorageServiceNotEnoughSpace : public FailureCause {

    public:
        StorageServiceNotEnoughSpace(WorkflowFile *file, StorageService *storage_service);

        WorkflowFile *getFile();
        StorageService *getStorageService();
        std::string toString();


    private:
        WorkflowFile *file;
        StorageService *storage_service;
    };

    /**
     * @brief A "file already there" failure cause
     */
    class StorageServiceFileAlreadyThere : public FailureCause {

    public:
        StorageServiceFileAlreadyThere(WorkflowFile *file, StorageService *storage_service);

        WorkflowFile *getFile();
        StorageService *getStorageService();
        std::string toString();


    private:
        WorkflowFile *file;
        StorageService *storage_service;
    };

    /**
     * @brief A "file is already being copied" failure cause
     */
    class FileAlreadyBeingCopied : public FailureCause {

    public:
        FileAlreadyBeingCopied(WorkflowFile *file, StorageService *dst);

        WorkflowFile *getFile();
        StorageService *getStorageService();
        std::string toString();

    private:
        WorkflowFile *file;
        StorageService *storage_service;
    };

    /**
     * @brief A "service is down" failure cause
     */
    class ServiceIsDown : public FailureCause {
    public:
        ServiceIsDown(Service *service);
        Service *getService();
        std::string toString();

    private:
        Service *service;
    };

    /**
     * @brief A "compute service does not support requested job type" failure cause
     */
    class JobTypeNotSupported : public FailureCause {
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
     * @brief A requested functionality is not available on that service
     */
    class FunctionalityNotAvailable : public FailureCause {
    public:
        FunctionalityNotAvailable(Service *service, std::string functionality_name);
        Service *getService();
        std::string getFunctionalityName();
        std::string toString();

    private:
        Service *service;
        std::string functionality_name;
    };


    /**
     * @brief A "compute service doesn't have enough cores" failure cause
     */
    class NotEnoughComputeResources : public FailureCause {
    public:
        NotEnoughComputeResources(WorkflowJob *job, ComputeService *compute_service);
        WorkflowJob *getJob();
        ComputeService *getComputeService();
        std::string toString();

    private:
        WorkflowJob *job;
        ComputeService *compute_service;
    };

    /**
     * @brief A "network error (or endpoint is down)" failure cause
     */
    class NetworkError : public FailureCause {
    public:
        /** @brief Enumerated type to describe whether the network error occured
         * while sending or receiving
         */
        enum OperationType {
            SENDING,
            RECEIVING
        };

        NetworkError(NetworkError::OperationType, std::string mailbox);
        std::string toString();
        bool whileReceiving();
        bool whileSending();
        std::string getMailbox();

    private:
        NetworkError::OperationType operation_type;
        bool while_sending = false;
        std::string mailbox = "";
    };

    /**
    * @brief A "network timout" failure cause
    */
    class NetworkTimeout : public FailureCause {
    public:
        /** A enumerated tupe that describes the operation that led to the network timeout */
        enum OperationType {
            SENDING,
            RECEIVING
        };

        NetworkTimeout(NetworkTimeout::OperationType, std::string mailbox);
        std::string toString();
        bool whileReceiving();
        bool whileSending();
        std::string getMailbox();

    private:
        NetworkTimeout::OperationType operation_type;
        bool while_sending = false;
        std::string mailbox = "";
    };
    
    /**
     * @brief A "job cannot be terminated" failure cause
     */
    class JobCannotBeTerminated : public FailureCause {
    public:
        JobCannotBeTerminated(WorkflowJob *job);
        WorkflowJob *getJob();
        std::string toString();

    private:
        WorkflowJob *job;
    };

    /**
    * @brief A "job cannot be forgotten" failure cause
    */
    class JobCannotBeForgotten : public FailureCause {
    public:
        JobCannotBeForgotten(WorkflowJob *job);
        WorkflowJob *getJob();
        std::string toString();

    private:
        WorkflowJob *job;
    };

    /**
   * @brief A "compute thread has died" failure cause
   */
    class ComputeThreadHasDied : public FailureCause {
    public:
        ComputeThreadHasDied();
        std::string toString();

    private:
    };

    /**
   * @brief Unknown failure cause
   */
    class FatalFailure : public FailureCause {
    public:
        FatalFailure();
        std::string toString();

    private:
    };


    /**
    * @brief A "job has times out" failure cause
    */
    class JobTimeout : public FailureCause {
    public:
        JobTimeout(WorkflowJob *job);
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
