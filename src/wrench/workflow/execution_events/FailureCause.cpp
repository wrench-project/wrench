/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/workflow/execution_events/FailureCause.h"
#include "wrench/workflow/WorkflowFile.h"
#include "wrench/workflow/job/WorkflowJob.h"
#include "wrench/services/storage/StorageService.h"
#include "wrench/services/compute/ComputeService.h"

namespace wrench {

    /**
     * @brief Constructor
     * @param cause: the type of the failure cause
     */
    FailureCause::FailureCause(CauseType cause) {
      this->cause = cause;
    }

    /**
     * @brief Retrieve the type of the failure cause
     * @return
     */
    FailureCause::CauseType FailureCause::getCauseType() {
      return this->cause;
    }

    /**
     * @brief Constructor
     * @param file: the file that could not be found on any storage service
     */
    NoStorageServiceForFile::NoStorageServiceForFile(WorkflowFile *file) : FailureCause(
            NO_STORAGE_SERVICE_FOR_FILE) {
      this->file = file;
    }

    /**
     * @brief Constructor
     * @param error: error message
     */
    NoScratchSpace::NoScratchSpace(std::string error) : FailureCause(
            NO_SCRATCH_SPACE) {
      this->error = error;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string NoScratchSpace::toString() {
      return error;
    }

    /**
     * @brief Getter
     * @return the file
     */
    WorkflowFile *NoStorageServiceForFile::getFile() {
      return this->file;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string NoStorageServiceForFile::toString() {
      return "No Storage Service location is specified for file " + this->file->getId();
    }

    /**
     * @brief Constructor
     * @param file: the file that could not be found
     * @param storage_service: the storage service on which it was not found
     */
    FileNotFound::FileNotFound(WorkflowFile *file, StorageService *storage_service) : FailureCause(
            FILE_NOT_FOUND) {
      this->file = file;
      this->storage_service = storage_service;
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
    StorageService *FileNotFound::getStorageService() {
      return this->storage_service;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string FileNotFound::toString() {
      return "Couldn't find file " + this->file->getId() + " at Storage Service " + this->storage_service->getName();
    }

    /**
     * @brief Constructor
     * @param file: the file that could not be written
     * @param storage_service:  the storage service that ran out of spacee
     */
    StorageServiceNotEnoughSpace::StorageServiceNotEnoughSpace(WorkflowFile *file, StorageService *storage_service)
            : FailureCause(STORAGE_NOT_ENOUGH_SPACE) {
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
    StorageService *StorageServiceNotEnoughSpace::getStorageService() {
      return this->storage_service;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string StorageServiceNotEnoughSpace::toString() {
      return "Cannot write file " + this->file->getId() + " to Storage Service " +
             this->storage_service->getName() + " due to lack of storage space";
    }

    /**
     * @brief Constructor
     * @param service: the service that was down
     */
    ServiceIsDown::ServiceIsDown(Service *service) : FailureCause(SERVICE_DOWN) {
      this->service = service;
    }

    /**
     * @brief Getter
     * @return the service
     */
    Service *ServiceIsDown::getService() {
      return this->service;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string ServiceIsDown::toString() {
      return "Service " + this->service->getName() + " on host " + this->service->getHostname() + " was terminated ";
    }

    /**
     * @brief Constructor
     * @param job: the job that wasn't supported
     * @param compute_service: the compute service that did not support it
     */
    JobTypeNotSupported::JobTypeNotSupported(WorkflowJob *job, ComputeService *compute_service)
            : FailureCause(JOB_TYPE_NOT_SUPPORTED) {
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
    ComputeService *JobTypeNotSupported::getComputeService() {
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
     * @param job: the job that could not be executed
     * @param compute_service: the compute service that didn't have enough cores
     */
    NotEnoughComputeResources::NotEnoughComputeResources(WorkflowJob *job, ComputeService *compute_service) : FailureCause(
            NOT_ENOUGH_COMPUTE_RESOURCES) {
      this->job = job;
      this->compute_service = compute_service;
    }

    /**
     * @brief Getter
     * @return the job
     */
    WorkflowJob *NotEnoughComputeResources::getJob() {
      return this->job;
    }

    /**
     * @brief Getter
     * @return the compute service
     */
    ComputeService *NotEnoughComputeResources::getComputeService() {
      return this->compute_service;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string NotEnoughComputeResources::toString() {
      return "Compute service " + this->compute_service->getName() + " on host " +
             this->compute_service->getHostname() + " does not have enough compute resources to support job " + job->getName();
    }

    /**
     * @brief Constructor
     *
     * @param operation_type: NetworkError:OperationType::SENDING or NetworkError::OperationType::RECEIVING
     * @param mailbox: the name of a mailbox
     */
    NetworkError::NetworkError(NetworkError::OperationType operation_type, std::string mailbox) : FailureCause(NETWORK_ERROR) {
      this->operation_type = operation_type;
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
      return "Network error (link failure, or communication peer died) while " + operation + " mailbox_name " + this->mailbox;
    };

    /**
     * @brief Constructor
     *
     * @param operation_type: NetworkTimeout::OperationType::SENDING or NetworkTimeout::OperationType::RECEIVING
     * @param mailbox: the mailbox name
     */
    NetworkTimeout::NetworkTimeout(NetworkTimeout::OperationType operation_type, std::string mailbox) : FailureCause(NETWORK_TIMEOUT) {
      this->operation_type = operation_type;
      this->mailbox = mailbox;
    }

    /**
     * @brief Returns whether the network error occurred while receiving
     * @return true or false
     */
    bool NetworkTimeout::whileReceiving() {
      return (this->operation_type == NetworkTimeout::RECEIVING);
    }

    /**
     * @brief Returns whether the network error occurred while sending
     * @return true or false
     */
    bool NetworkTimeout::whileSending() {
      return (this->operation_type == NetworkTimeout::SENDING);
    }

    /**
     * @brief Returns the mailbox name on which the error occurred
     * @return the mailbox name
     */
    std::string NetworkTimeout::getMailbox() {
      return this->mailbox;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string NetworkTimeout::toString() {
      std::string operation;
      if (this->while_sending) {
        operation = "sending to";
      } else {
        operation = "receiving from";
      }
      return "Network timeout while " + operation + " mailbox_name " + this->mailbox;
    };

    /**
     * @brief Constructor
     *
     * @param job: the job that couldn't be terminated
     */
    JobCannotBeTerminated::JobCannotBeTerminated(WorkflowJob *job) : FailureCause(
            JOB_CANNOT_BE_TERMINATED) {
      this->job = job;
    }


    /**
     * @brief Getter
     * @return the job
     */
    WorkflowJob *JobCannotBeTerminated::getJob() {
      return this->job;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string JobCannotBeTerminated::toString() {
      return "Job cannot be terminated (because it's neither pending nor running)";
    };


    /**
    * @brief Constructor
    *
    * @param job: the job that couldn't be forgotten
    */
    JobCannotBeForgotten::JobCannotBeForgotten(WorkflowJob *job) : FailureCause(
            JOB_CANNOT_BE_FORGOTTEN) {
      this->job = job;
    }


    /**
     * @brief Getter
     * @return the job
     */
    WorkflowJob *JobCannotBeForgotten::getJob() {
      return this->job;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string JobCannotBeForgotten::toString() {
      return "Job cannot be forgotten (because it's not completed or failed)";
    };

    /**
     * @brief Constructor
     * @param file: the file that is already there
     * @param storage_service:  the storage service on which it is
     */
    StorageServiceFileAlreadyThere::StorageServiceFileAlreadyThere(WorkflowFile *file, StorageService *storage_service)
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
    StorageService *StorageServiceFileAlreadyThere::getStorageService() {
      return this->storage_service;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string StorageServiceFileAlreadyThere::toString() {
      return "Cannot write file " + this->file->getId() + " to Storage Service " +
             this->storage_service->getName() + " because it's already stored there";
    }


    /**
     * @brief Constructor
     * @param file: the file that is already being copied
     * @param storage_service:  the storage service to which is is being copied
     */
    FileAlreadyBeingCopied::FileAlreadyBeingCopied(WorkflowFile *file, StorageService *storage_service)
            : FailureCause(FILE_ALREADY_BEING_COPIED) {
      this->file = file;
      this->storage_service = storage_service;
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
     * @return the storage service
     */
    StorageService *FileAlreadyBeingCopied::getStorageService() {
      return this->storage_service;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string FileAlreadyBeingCopied::toString() {
      return "File " + this->file->getId() + " is already being copied to  Storage Service " +
             this->storage_service->getName();
    }


    /**
     * @brief Constructor
     *
     */
    ComputeThreadHasDied::ComputeThreadHasDied() : FailureCause(
            COMPUTE_THREAD_HAS_DIED) {
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
    FatalFailure::FatalFailure() : FailureCause(
            FATAL_FAILURE) {
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
    FunctionalityNotAvailable::FunctionalityNotAvailable(Service *service, std::string functionality_name) : FailureCause(FUNCTIONALITY_NOT_AVAILABLE) {
      this->service = service;
      this->functionality_name = std::move(functionality_name);
    }

    /**
     * @brief Getter
     * @return the service
     */
    Service* FunctionalityNotAvailable::getService() {
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
      return "The request functionality (" + this->functionality_name + ") is not available on service " + this->service->getName();
    }

    /**
    * @brief Constructor
    *
    * @param job: the job that has timed out
    */
    JobTimeout::JobTimeout(WorkflowJob *job) : FailureCause(
            JOB_TIMEOUT) {
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

};
