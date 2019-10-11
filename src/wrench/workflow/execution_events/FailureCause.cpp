/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/workflow/execution_events/FailureCause.h>

#include "wrench/logging/TerminalOutput.h"
#include "wrench/workflow/execution_events/FailureCause.h"
#include "wrench/workflow/WorkflowFile.h"
#include "wrench/workflow/job/WorkflowJob.h"
#include "wrench/services/storage/StorageService.h"
#include "wrench/services/compute/ComputeService.h"

WRENCH_LOG_NEW_DEFAULT_CATEGORY(falure_cause, "Log category for FailureCause");


namespace wrench {


    /**
     * @brief Constructor
     * @param error: error message
     */
    NoScratchSpace::NoScratchSpace(std::string error) {
        this->error = error;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string NoScratchSpace::toString() {
        return error;}


    /**
     * @brief Constructor
     * @param file: the file that could not be found
     * @param location: the location at which it could not be found
     */
    FileNotFound::FileNotFound(WorkflowFile *file, std::shared_ptr<FileLocation> location) {
        this->file = file;
        this->location = location;
    }

    /**
     * @brief Getter
     * @return the file
     */
    WorkflowFile *FileNotFound::getFile() {
        return this->file;
    }

    /**
     * @brief Getter
     * @return the storage service
     */
    std::shared_ptr<FileLocation> FileNotFound::getLocation() {
        return this->location;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string FileNotFound::toString() {
        return "Couldn't find file " + this->file->getID() + " at location " + this->location->toString();
    }

    /**
     * @brief Constructor
     * @param file: the file that could not be written
     * @param storage_service:  the storage service that ran out of spacee
     */
    StorageServiceNotEnoughSpace::StorageServiceNotEnoughSpace(WorkflowFile *file,
                                                               std::shared_ptr<StorageService> storage_service) {
        this->file = file;
        this->storage_service = storage_service;
    }

    /**
     * @brief Getter
     * @return the file
     */
    WorkflowFile *StorageServiceNotEnoughSpace::getFile() {
        return this->file;
    }

    /**
     * @brief Getter
     * @return the storage service
     */
    std::shared_ptr<StorageService> StorageServiceNotEnoughSpace::getStorageService() {
        return this->storage_service;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string StorageServiceNotEnoughSpace::toString() {
        return "Cannot write file " + this->file->getID() + " to Storage Service " +
               this->storage_service->getName() + " due to lack of storage space";
    }

    /**
     * @brief Constructor
     * @param service: the service that was down
     */
    ServiceIsDown::ServiceIsDown(std::shared_ptr<Service> service) {
        this->service = service;
    }

    /**
     * @brief Getter
     * @return the service
     */
    std::shared_ptr<Service> ServiceIsDown::getService() {
        return this->service;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string ServiceIsDown::toString() {
        return "Service " + this->service->getName() + " on host " + this->service->getHostname() + " was terminated";
    }

    /**
     * @brief Constructor
     * @param service: the service that was suspended
     */
    ServiceIsSuspended::ServiceIsSuspended(std::shared_ptr<Service> service) {
        this->service = service;
    }

    /**
     * @brief Getter
     * @return the service
     */
    std::shared_ptr<Service> ServiceIsSuspended::getService() {
        return this->service;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string ServiceIsSuspended::toString() {
        return "Service " + this->service->getName() + " on host " + this->service->getHostname() + " is suspended";
    }

    /**
     * @brief Constructor
     * @param job: the job that wasn't supported
     * @param compute_service: the compute service that did not support it
     */
    JobTypeNotSupported::JobTypeNotSupported(WorkflowJob *job, std::shared_ptr<ComputeService> compute_service) {
        this->job = job;
        this->compute_service = compute_service;
    }

    /**
     * @brief Getter
     * @return the job
     */
    WorkflowJob *JobTypeNotSupported::getJob() {
        return this->job;
    }

    /**
     * @brief Getter
     * @return the compute service
     */
    std::shared_ptr<ComputeService> JobTypeNotSupported::getComputeService() {
        return this->compute_service;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string JobTypeNotSupported::toString() {
        return "Compute service " + this->compute_service->getName() + " on host " +
               this->compute_service->getHostname() + " does not support jobs of type " + this->job->getTypeAsString();
    }

    /**
     * @brief Constructor
     * @param job: the job that could not be executed (or nullptr if no job was involved)
     * @param compute_service: the compute service that didn't have enough cores or ram
     */
    NotEnoughResources::NotEnoughResources(WorkflowJob *job, std::shared_ptr<ComputeService> compute_service) {
        this->job = job;
        this->compute_service = compute_service;
    }

    /**
     * @brief Getter
     * @return the job
     */
    WorkflowJob *NotEnoughResources::getJob() {
        return this->job;
    }

    /**
     * @brief Getter
     * @return the compute service
     */
    std::shared_ptr<ComputeService> NotEnoughResources::getComputeService() {
        return this->compute_service;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string NotEnoughResources::toString() {
        std::string text_msg = "Compute service " + this->compute_service->getName() + " on host " +
                               this->compute_service->getHostname() + " does not have enough compute resources";
        if (job) {
            text_msg += " to support job " + job->getName();
        }
        return text_msg;
    }

    /**
    * @brief Constructor
    * @param job: the job that could not be executed
    * @param compute_service: the compute service that didn't have enough cores
    */
    JobKilled::JobKilled(WorkflowJob *job, std::shared_ptr<ComputeService> compute_service) {
        this->job = job;
        this->compute_service = compute_service;
    }

    /**
     * @brief Getter
     * @return the job
     */
    WorkflowJob *JobKilled::getJob() {
        return this->job;
    }

    /**
     * @brief Getter
     * @return the compute service
     */
    std::shared_ptr<ComputeService> JobKilled::getComputeService() {
        return this->compute_service;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string JobKilled::toString() {
        return "Job " + this->job->getName() + " on service " +
               this->compute_service->getName() + " was killed (likely the service was stopped/terminated)";
    }

    /**
     * @brief Constructor
     *
     * @param operation_type: NetworkError:OperationType::SENDING or NetworkError::OperationType::RECEIVING or
     *        NetworkError::OperationType::UNKNOWN
     * @param error_type: the error type 
     * @param mailbox: the name of a mailbox (or "" if unknown)
     */
    NetworkError::NetworkError(NetworkError::OperationType operation_type,
                               NetworkError::ErrorType error_type,
                               std::string mailbox) {
        if (mailbox.empty()) {
            throw std::invalid_argument("NetworkError::NetworkError(): invalid arguments");
        }
        this->operation_type = operation_type;
        this->error_type = error_type;
        this->mailbox = mailbox;
    }

    /**
     * @brief Returns whether the network error occurred while receiving
     * @return true or false
     */
    bool NetworkError::whileReceiving() {
        return (this->operation_type == NetworkError::RECEIVING);
    }

    /**
     * @brief Returns whether the network error occurred while sending
     * @return true or false
     */
    bool NetworkError::whileSending() {
        return (this->operation_type == NetworkError::SENDING);
    }

    /**
     * @brief Returns whether the network error was a timeout
     * @return true or false
     */
    bool NetworkError::isTimeout() {
        return (this->error_type == NetworkError::TIMEOUT);
    }

    /**
     * @brief Returns the mailbox name on which the error occurred
     * @return the mailbox name
     */
    std::string NetworkError::getMailbox() {
        return this->mailbox;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string NetworkError::toString() {
        std::string operation;
        if (this->while_sending) {
            operation = "sending to";
        } else {
            operation = "receiving from";
        }
        std::string error;
        if (this->isTimeout()) {
            error = "timeout";
        } else {
            error = "link failure, or communication peer died";
        }
        return "Network error (" + error + ") while " + operation + " mailbox_name " + this->mailbox;
    };


#if 0

    /**
     * @brief Constructor
     * @param file: the file that is already there
     * @param storage_service:  the storage service on which it is
     */
    StorageServiceFileAlreadyThere::StorageServiceFileAlreadyThere(WorkflowFile *file, std::shared_ptr<StorageService>  storage_service)
            : FailureCause(FILE_ALREADY_THERE) {
      this->file = file;
      this->storage_service = storage_service;
    }

    /**
     * @brief Getter
     * @return the file
     */
    WorkflowFile *StorageServiceFileAlreadyThere::getFile() {
      return this->file;
    }

    /**
     * @brief Getter
     * @return the storage service
     */
    std::shared_ptr<StorageService>  StorageServiceFileAlreadyThere::getStorageService() {
      return this->storage_service;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string StorageServiceFileAlreadyThere::toString() {
      return "Cannot write file " + this->file->getID() + " to Storage Service " +
             this->storage_service->getName() + " because it's already stored there";
    }


#endif

    /**
     * @brief Constructor
     * @param file: the file that is already being copied
     * @param src: the source location
     * @param dst: the destination location
     */
    FileAlreadyBeingCopied::FileAlreadyBeingCopied(WorkflowFile *file,
                                                   std::shared_ptr<FileLocation> src,
                                                   std::shared_ptr<FileLocation> dst) {
        this->file = file;
        this->src_location = src;
        this->dst_location = dst;
    }

    /**
     * @brief Getter
     * @return the file
     */
    WorkflowFile *FileAlreadyBeingCopied::getFile() {
        return this->file;
    }

    /**
     * @brief Getter
     * @return the source location
     */
    std::shared_ptr<FileLocation> FileAlreadyBeingCopied::getSourceLocation() {
        return this->src_location;
    }

    /**
    * @brief Getter
    * @return the source location
    */
    std::shared_ptr<FileLocation> FileAlreadyBeingCopied::getDestinationLocation() {
        return this->dst_location;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string FileAlreadyBeingCopied::toString() {
        return "File " + this->file->getID() + " is already being copied ("+
               "src = " + this->src_location->toString() + "; " +
               "dst = " + this->dst_location->toString() + ")";
    }


    /**
     * @brief Constructor
     *
     */
    ComputeThreadHasDied::ComputeThreadHasDied() {
    }


    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string ComputeThreadHasDied::toString() {
        return "A compute thread has died";
    };

    /**
    * @brief Constructor
    *
    */
    FatalFailure::FatalFailure() {
    }


    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string FatalFailure::toString() {
        return "Internal implementation, likely a WRENCH bug";
    };

    /**
     * @brief Constructor
     * @param service: the service
     * @param functionality_name: a description of the functionality that's not available
     */
    FunctionalityNotAvailable::FunctionalityNotAvailable(std::shared_ptr<Service> service,
                                                         std::string functionality_name) {
        this->service = service;
        this->functionality_name = std::move(functionality_name);
    }

    /**
     * @brief Getter
     * @return the service
     */
    std::shared_ptr<Service> FunctionalityNotAvailable::getService() {
        return this->service;
    }

    /**
     * @brief Getter
     * @return the functionality name
     */
    std::string FunctionalityNotAvailable::getFunctionalityName() {
        return this->functionality_name;
    }


    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string FunctionalityNotAvailable::toString() {
        return "The request functionality (" + this->functionality_name + ") is not available on service " +
               this->service->getName();
    }

    /**
    * @brief Constructor
    *
    * @param job: the job that has timed out
    */
    JobTimeout::JobTimeout(WorkflowJob *job) {
        this->job = job;
    }


    /**
     * @brief Getter
     * @return the job
     */
    WorkflowJob *JobTimeout::getJob() {
        return this->job;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string JobTimeout::toString() {
        return "Job has timed out - likely not enough time was requested from a (batch-scheduled?) compute service";
    };

    /**
     * @brief Constructor
     * @param hostname: the name of the host that experienced the error
     */
    HostError::HostError(std::string hostname) {
        this->hostname = hostname;
    }

    /** @brief Get the human-readable failure message
     * @return the message
     */
    std::string HostError::toString() {
        return "The host (" + this->hostname + ") is down and no service cannot be started on it";
    }

    /**
     * @brief Constructor
     * @param service: the service that cause the error (or nullptr if no known service for the error)
     * @param error_message: a custom error message
     */
    NotAllowed::NotAllowed(std::shared_ptr<Service> service, std::string &error_message) {
        this->service = service;
        this->error_message = error_message;
    }

    /**
     * @brief Get the service that caused the error
     * @return the service
     */
    std::shared_ptr<Service> NotAllowed::getService() {
        return this->service;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string NotAllowed::toString() {
        if (this->service) {
            return "The service (" + this->service->getName() + ") does not allow the operation (" + this->error_message + ")";
        } else {
            return "Operation not allowed (" + this->error_message + ")";
        }
    }

    /**
     * @brief Constructor
     * @param storage_service: the storage service at which the mount point was unknown
     * @param invalid_path: the invalid path
     */
    InvalidDirectoryPath::InvalidDirectoryPath(std::shared_ptr<StorageService> storage_service,
                                               std::string invalid_path) {
        this->storage_service = storage_service;
        this->invalid_path = invalid_path;
    }

    /**
     * @brief Get the storage service at which the path was invalid
     * @return a storage service
     */
    std::shared_ptr<StorageService> InvalidDirectoryPath::getStorageService() {
        return this->storage_service;
    }

    /**
     * @brief Get the invalid path
     * @return a path
     */
    std::string InvalidDirectoryPath::getInvalidPath() {
        return this->invalid_path;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string InvalidDirectoryPath::toString() {
        return "Storage service " + this->storage_service->getName() + " doesn't have a " +
               this->getInvalidPath() + " path";
    }

};
