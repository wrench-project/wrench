/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "WorkflowExecutionFailureCause.h"
#include "workflow/WorkflowFile.h"
#include "workflow_job/WorkflowJob.h"
#include "services/storage_services/StorageService.h"
#include "services/compute_services/ComputeService.h"

namespace wrench {

    /**
     * @brief Constructor
     * @param cause: the type of the failure cause
     */
    WorkflowExecutionFailureCause::WorkflowExecutionFailureCause(CauseType cause) {
      this->cause = cause;
    }

    /**
     * @brief Retrieve the type of the failure cause
     * @return
     */
    WorkflowExecutionFailureCause::CauseType WorkflowExecutionFailureCause::getCauseType() {
      return this->cause;
    }

    /**
     * @brief Constructor
     * @param file: the file that cannot be find on any storage service
     */
    NoStorageServiceForFile::NoStorageServiceForFile(WorkflowFile *file) : WorkflowExecutionFailureCause(
            NO_STORAGE_SERVICE_FOR_FILE) {
      this->file = file;
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
    FileNotFound::FileNotFound(WorkflowFile *file, StorageService *storage_service) : WorkflowExecutionFailureCause(
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
    StorageServiceFull::StorageServiceFull(WorkflowFile *file, StorageService *storage_service)
            : WorkflowExecutionFailureCause(STORAGE_NO_ENOUGH_SPACE) {
      this->file = file;
      this->storage_service = storage_service;
    }

    /**
     * @brief Getter
     * @return the file
     */
    WorkflowFile *StorageServiceFull::getFile() {
      return this->file;
    }

    /**
     * @brief Getter
     * @return the storage service
     */
    StorageService *StorageServiceFull::getStorageService() {
      return this->storage_service;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string StorageServiceFull::toString() {
      return "Cannot write file " + this->file->getId() + " to Storage Service " +
             this->storage_service->getName() + " due to lack of storage space";
    }

    /**
     * @brief Constructor
     * @param service: the service that was down
     */
    ServiceIsDown::ServiceIsDown(Service *service) : WorkflowExecutionFailureCause(SERVICE_TERMINATED) {
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
            : WorkflowExecutionFailureCause(JOB_TYPE_NOT_SUPPORTED) {
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
    NotEnoughCores::NotEnoughCores(WorkflowJob *job, ComputeService *compute_service) : WorkflowExecutionFailureCause(
            NOT_ENOUGH_CORES) {
      this->job = job;
      this->compute_service = compute_service;
    }

    /**
     * @brief Getter
     * @return the job
     */
    WorkflowJob *NotEnoughCores::getJob() {
      return this->job;
    }

    /**
     * @brief Getter
     * @return the compute service
     */
    ComputeService *NotEnoughCores::getComputeService() {
      return this->compute_service;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string NotEnoughCores::toString() {
      return "Compute service " + this->compute_service->getName() + " on host " +
             this->compute_service->getHostname() + " does not have enough cores to support job " + job->getName();
    }

    /**
     * @brief Constructor
     */
    NetworkError::NetworkError() : WorkflowExecutionFailureCause(NETWORK_ERROR) {
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string NetworkError::toString() {
       return "Network error (link failure, or communication peer died)";
    };

    /**
     * @brief Constructor
     *
     * @param job: the job that couldn't be terminated
     * @param compute_service: the compute service that couldn't terminate the job
     */
    JobCannotBeTerminated::JobCannotBeTerminated(WorkflowJob *job) : WorkflowExecutionFailureCause(
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



};
