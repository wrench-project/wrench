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

    WorkflowExecutionFailureCause::WorkflowExecutionFailureCause(Cause cause) {
      this->cause = cause;
    }

    WorkflowExecutionFailureCause::Cause WorkflowExecutionFailureCause::getCause() {
      return this->cause;
    }

    NoStorageServiceForFile::NoStorageServiceForFile(WorkflowFile *file) : WorkflowExecutionFailureCause(
            NO_STORAGE_SERVICE_FOR_FILE) {
      this->file = file;
    }

    WorkflowFile *NoStorageServiceForFile::getFile() {
      return this->file;
    }

    std::string NoStorageServiceForFile::toString() {
      return "No Storage Service location is specified for file " + this->file->getId();
    }

    FileNotFound::FileNotFound(WorkflowFile *file, StorageService *storage_service) : WorkflowExecutionFailureCause(
            FILE_NOT_FOUND) {
      this->file = file;
      this->storage_service = storage_service;
    }

    WorkflowFile *FileNotFound::getFile() {
      return this->file;
    }

    StorageService *FileNotFound::getStorageService() {
      return this->storage_service;
    }

    std::string FileNotFound::toString() {
      return "Couldn't find file " + this->file->getId() + " at Storage Service " + this->storage_service->getName();
    }

    StorageServiceFull::StorageServiceFull(WorkflowFile *file, StorageService *storage_service)
            : WorkflowExecutionFailureCause(STORAGE_SERVICE_FULL) {
      this->file = file;
      this->storage_service = storage_service;
    }

    WorkflowFile *StorageServiceFull::getFile() {
      return this->file;
    }

    StorageService *StorageServiceFull::getStorageService() {
      return this->storage_service;
    }

    std::string StorageServiceFull::toString() {
      return "Cannot write file " + this->file->getId() + " to Storage Service " +
             this->storage_service->getName() + " due to lack of storage space";
    }

    ServiceIsDown::ServiceIsDown(Service *service) : WorkflowExecutionFailureCause(SERVICE_TERMINATED) {
      this->service = service;
    }

    Service *ServiceIsDown::getService() {
      return this->service;
    }

    std::string ServiceIsDown::toString() {
      return "Service " + this->service->getName() + " on host " + this->service->getHostname() + " was terminated ";
    }

    JobTypeNotSupported::JobTypeNotSupported(WorkflowJob *job, ComputeService *compute_service)
            : WorkflowExecutionFailureCause(JOB_TYPE_NOT_SUPPORTED) {
      this->job = job;
      this->compute_service = compute_service;
    }

    WorkflowJob *JobTypeNotSupported::getJob() {
      return this->job;
    }

    ComputeService *JobTypeNotSupported::getComputeservice() {
      return this->compute_service;
    }

    std::string JobTypeNotSupported::toString() {
      return "Compute service " + this->compute_service->getName() + " on host " +
             this->compute_service->getHostname() + " does not support jobs of type " + this->job->getTypeAsString();
    }

    NotEnoughCores::NotEnoughCores(WorkflowJob *job, ComputeService *compute_service) : WorkflowExecutionFailureCause(
            NOT_ENOUGH_CORES) {
      this->job = job;
      this->compute_service = compute_service;
    }

    WorkflowJob *NotEnoughCores::getJob() {
      return this->job;
    }

    ComputeService *NotEnoughCores::getComputeservice() {
      return this->compute_service;
    }

    std::string NotEnoughCores::toString() {
      return "Compute service " + this->compute_service->getName() + " on host " +
             this->compute_service->getHostname() + " does not have enough cores to support job " + job->getName();
    }
};
