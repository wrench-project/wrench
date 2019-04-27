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

#include "wrench/services/Service.h"
#include "wrench/services/storage/StorageService.h"
#include "wrench/services/compute/ComputeService.h"

namespace wrench {

    class Service;
    class WorkflowFile;
    class WorkflowJob;

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/


    /**
     * @brief A top-level class to describe all simulation-valid failures that can occur during
     *        workflow execution (and should/could be handled by a WMS)
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
            /** @brief No Scratch Space is available */
                    NO_SCRATCH_SPACE,
            /** @brief The file was not found where it was supposed to be found */
                    FILE_NOT_FOUND,
            /** @brief The file to be written is already there */
                    FILE_ALREADY_THERE,
            /** @brief The file is already being copied there */
                    FILE_ALREADY_BEING_COPIED,
            /** @brief The storage service does not have enough space to support the requested operation */
                    STORAGE_NOT_ENOUGH_SPACE,
            /** @brief The service cannot be used because it is down (likely it was terminated) */
                    SERVICE_DOWN,
            /** @brief The service cannot be used because it is suspended (likely it was terminated) */
                    SERVICE_SUSPENDED,
            /** @brief The compute service does not support this job type */
                    JOB_TYPE_NOT_SUPPORTED,
            /** @brief The compute service cannot run the job (ever) due to insufficient resources (cores and/or ram) */
                    NOT_ENOUGH_RESOURCES,
            /** @brief There was a network error, or an endpoint was down */
                    NETWORK_ERROR,
            /** @brief There was a network timeout (for a "with timeout" network operation) */
                    NETWORK_TIMEOUT,
            /** @brief A Job has been killed (likely because the service performing it was terminated  or killed) */
                    JOB_KILLED,
            /** @brief The job cannot be terminated because it is neither pending nor running */
                    JOB_CANNOT_BE_TERMINATED,
            /** @brief The job cannot be forgotten because it is not completed */
                    JOB_CANNOT_BE_FORGOTTEN,
            /** @brief A compute thread has died */
                    COMPUTE_THREAD_HAS_DIED,
            /** @brief A functionality is not available */
                    FUNCTIONALITY_NOT_AVAILABLE,
            /** @brief A job was terminated due to a timeout */
                    JOB_TIMEOUT,
            /** @brief The host went down (while computing, while sleeping) **/
                    HOST_ERROR,
            /** @brief Operation is not allowed **/
                    NOT_ALLOWED

        };

        explicit FailureCause(CauseType cause);

        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        virtual ~FailureCause() = default;
        /***********************/
        /** \endcond           */
        /***********************/


        /**
         * @brief Return an error message that describes the failure cause (to be overridden)
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
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        NoStorageServiceForFile(WorkflowFile *file);
        /***********************/
        /** \endcond           */
        /***********************/

        WorkflowFile *getFile();
        std::string toString();

    private:
        WorkflowFile *file;
    };


    /**
     * @brief A "no scratch space" failure cause
     */
    class NoScratchSpace : public FailureCause {

    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        NoScratchSpace(std::string error);
        /***********************/
        /** \endcond           */
        /***********************/

        std::string toString();
    private:
        std::string error;
    };

    /**
     * @brief A "file was not found" failure cause
     */
    class FileNotFound : public FailureCause {

    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        FileNotFound(WorkflowFile *file, std::shared_ptr<StorageService>  storage_service);
        /***********************/
        /** \endcond           */
        /***********************/

        WorkflowFile *getFile();
        std::shared_ptr<StorageService>  getStorageService();
        std::string toString();


    private:
        WorkflowFile *file;
        std::shared_ptr<StorageService>  storage_service;
    };

    /**
     * @brief A "not enough space on storage service" failure cause
     */
    class StorageServiceNotEnoughSpace : public FailureCause {

    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        StorageServiceNotEnoughSpace(WorkflowFile *file, std::shared_ptr<StorageService>  storage_service);
        /***********************/
        /** \endcond           */
        /***********************/

        WorkflowFile *getFile();
        std::shared_ptr<StorageService>  getStorageService();
        std::string toString();


    private:
        WorkflowFile *file;
        std::shared_ptr<StorageService>  storage_service;
    };

//    /**
//     * @brief A "file is already there" failure cause
//     */
//    class StorageServiceFileAlreadyThere : public FailureCause {
//
//    public:
//        /***********************/
//        /** \cond INTERNAL     */
//        /***********************/
//        StorageServiceFileAlreadyThere(WorkflowFile *file, std::shared_ptr<StorageService>  storage_service);
//        /***********************/
//        /** \endcond           */
//        /***********************/
//
//        WorkflowFile *getFile();
//        std::shared_ptr<StorageService>  getStorageService();
//        std::string toString();
//
//
//    private:
//        WorkflowFile *file;
//        std::shared_ptr<StorageService>  storage_service;
//    };

    /**
     * @brief A "file is already being copied" failure cause
     */
    class FileAlreadyBeingCopied : public FailureCause {

    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        FileAlreadyBeingCopied(WorkflowFile *file, std::shared_ptr<StorageService> dst, std::string dst_partition);
        /***********************/
        /** \endcond           */
        /***********************/

        WorkflowFile *getFile();
        std::shared_ptr<StorageService> getStorageService();
        std::string getPartition();
        std::string toString();

    private:
        WorkflowFile *file;
        std::shared_ptr<StorageService> storage_service;
        std::string dst_partition;
    };

    /**
     * @brief A "service is down" failure cause
     */
    class
    ServiceIsDown : public FailureCause {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        explicit ServiceIsDown(std::shared_ptr<Service> service);
        /***********************/
        /** \endcond           */
        /***********************/

        std::shared_ptr<Service> getService();
        std::string toString() override;

    private:
        std::shared_ptr<Service> service;
    };

    /**
     * @brief A "service is suspended" failure cause
     */
    class
    ServiceIsSuspended : public FailureCause {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        explicit ServiceIsSuspended(std::shared_ptr<Service> service);
        /***********************/
        /** \endcond           */
        /***********************/

        std::shared_ptr<Service> getService();
        std::string toString() override;

    private:
        std::shared_ptr<Service> service;
    };

    /**
     * @brief A "compute service does not support requested job type" failure cause
     */
    class JobTypeNotSupported : public FailureCause {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        JobTypeNotSupported(WorkflowJob *job, std::shared_ptr<ComputeService>  compute_service);
        /***********************/
        /** \endcond           */
        /***********************/

        WorkflowJob *getJob();
        std::shared_ptr<ComputeService>  getComputeService();
        std::string toString();

    private:
        WorkflowJob *job;
        std::shared_ptr<ComputeService>  compute_service;
    };

    /**
     * @brief A "requested functionality is not available on that service" failure cause
     */
    class FunctionalityNotAvailable : public FailureCause {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        FunctionalityNotAvailable(std::shared_ptr<Service>  service, std::string functionality_name);
        /***********************/
        /** \endcond           */
        /***********************/

        std::shared_ptr<Service>  getService();
        std::string getFunctionalityName();
        std::string toString();

    private:
        std::shared_ptr<Service>  service;
        std::string functionality_name;
    };


    /**
     * @brief A "compute service doesn't have enough cores" failure cause
     */
    class NotEnoughResources : public FailureCause {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        NotEnoughResources(WorkflowJob *job, std::shared_ptr<ComputeService> compute_service);
        /***********************/
        /** \endcond           */
        /***********************/

        WorkflowJob *getJob();
        std::shared_ptr<ComputeService> getComputeService();
        std::string toString();

    private:
        WorkflowJob *job;
        std::shared_ptr<ComputeService> compute_service;
    };

    /**
    * @brief A "job has been killed" failure cause
    */
    class JobKilled : public FailureCause {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        JobKilled(WorkflowJob *job, std::shared_ptr<ComputeService> compute_service);
        /***********************/
        /** \endcond           */
        /***********************/

        WorkflowJob *getJob();
        std::shared_ptr<ComputeService> getComputeService();
        std::string toString();

    private:
        WorkflowJob *job;
        std::shared_ptr<ComputeService> compute_service;
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

        /** @brief Enumerated type to describe the type of the network error
         */
        enum ErrorType {
            TIMEOUT,
            FAILURE
        };

        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        NetworkError(NetworkError::OperationType, NetworkError::ErrorType, std::string mailbox);
        /***********************/
        /** \endcond           */
        /***********************/

        std::string toString();
        bool whileReceiving();
        bool whileSending();
        bool isTimeout();
        std::string getMailbox();

    private:
        NetworkError::OperationType operation_type;
        NetworkError::ErrorType error_type;
        bool while_sending = false;
        std::string mailbox = "";
    };

    /**
 * @brief A "host error" failure cause (e.g., attempted to start a daemon on a host that is off)
 */
    class HostError : public FailureCause {
    public:

        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        HostError(std::string hostname);

        /***********************/
        /** \endcond           */
        /***********************/

        std::string toString();

    private:
        std::string hostname;
    };


    /**
     * @brief A "job cannot be terminated" failure cause
     */
    class JobCannotBeTerminated : public FailureCause {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        JobCannotBeTerminated(WorkflowJob *job);
        /***********************/
        /** \endcond           */
        /***********************/

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
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        JobCannotBeForgotten(WorkflowJob *job);
        /***********************/
        /** \endcond           */
        /***********************/

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
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        ComputeThreadHasDied();
        /***********************/
        /** \endcond           */
        /***********************/

        std::string toString();

    private:
    };

    /**
    * @brief An "Unknown" failure cause (should not happen)
    */
    class FatalFailure : public FailureCause {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        FatalFailure();
        /***********************/
        /** \endcond           */
        /***********************/

        std::string toString();

    private:
    };


    /**
    * @brief A "job has times out" failure cause
    */
    class JobTimeout : public FailureCause {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        JobTimeout(WorkflowJob *job);
        /***********************/
        /** \endcond           */
        /***********************/

        WorkflowJob *getJob();
        std::string toString();

    private:
        WorkflowJob *job;
    };

    /**
    * @brief A "operation not allowed" failure cause
    */
    class NotAllowed : public FailureCause {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        NotAllowed(std::shared_ptr<Service> service, std::string &error_message);
        /***********************/
        /** \endcond           */
        /***********************/

        std::shared_ptr<Service> getService();
        std::string toString();

    private:
        std::shared_ptr<Service> service;
        std::string error_message;
    };



    /***********************/
    /** \endcond           */
    /***********************/
};


#endif //WRENCH_STANDARDJOBFAILURECAUSE_H
