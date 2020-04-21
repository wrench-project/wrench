w/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include "CloudStandardJobScheduler.h"
//#include <climits>
//#include <numeric>

XBT_LOG_NEW_DEFAULT_CATEGORY(cloud_scheduler, "Log category for Cloud Scheduler");

namespace wrench {

    /**
     * @brief Schedule and run a set of ready tasks on available cloud resources
     *
     * @param compute_services: a set of compute services available to run jobs
     * @param tasks: a map of (ready) workflow tasks
     *
     * @throw std::runtime_error
     */
    void CloudStandardJobScheduler::scheduleTasks(const std::set<std::shared_ptr<ComputeService>> &compute_services,
                                                  const std::vector<WorkflowTask *> &tasks) {

        // Check that the right compute_services is passed
        if (compute_services.size() != 1) {
            throw std::runtime_error("This example Cloud Scheduler requires a single compute service");
        }

        auto compute_service = *compute_services.begin();
        auto cloud_service = std::dynamic_pointer_cast<CloudComputeService>(compute_service);

        if (cloud_service == nullptr) {
            throw std::runtime_error("This example Cloud Scheduler can only handle a cloud service");
        }

        WRENCH_INFO("There are %ld ready tasks to schedule", tasks.size());

        for (auto task : tasks) {

            WRENCH_INFO("Trying to schedule ready task '%s' on a currently running VM", task->getID().c_str());

            // Try to run the task on one of compute services running on previously created VMs
            std::shared_ptr<BareMetalComputeService> picked_vm_cs = nullptr;
            for (auto const &vm_cs : this->compute_services_running_on_vms) {
                unsigned long num_idle_cores;
                try {
                    num_idle_cores = vm_cs->getTotalNumIdleCores();
                } catch (WorkflowExecutionException &e) {
                    // The service has some problem, forget it
                    throw std::runtime_error("Unable to get the number of idle cores: " + e.getCause()->toString());
                }
                if (task->getMinNumCores() <= num_idle_cores) {
                    picked_vm_cs = vm_cs;
                    break;
                }
            }

            // If no current running compute service on a VM can accommodate the task, try
            // to create a new one
            if (picked_vm_cs == nullptr) {
                WRENCH_INFO("No currently VM can support task '%s', let's try to create one...", task->getID().c_str());
                unsigned long num_idle_cores;
                try {
                    num_idle_cores = cloud_service->getTotalNumIdleCores();
                } catch (WorkflowExecutionException &e) {
                    // The service has some problem, forget it
                    throw std::runtime_error("Unable to get the number of idle cores: " + e.getCause()->toString());
                }
                if (num_idle_cores >= task->getMinNumCores()) {
                    // Create and start the best VM possible for this task
                    try {
                        WRENCH_INFO("Creating a VM with %ld cores", std::min(task->getMinNumCores(), num_idle_cores));
                        auto vm = cloud_service->createVM(std::min(task->getMinNumCores(), num_idle_cores),
                                                          task->getMemoryRequirement());
                        picked_vm_cs = cloud_service->startVM(vm);
                        this->compute_services_running_on_vms.push_back(picked_vm_cs);
                    } catch (WorkflowExecutionException &e) {
                        throw std::runtime_error("Unable to create/start a VM: " + e.getCause()->toString());
                    }
                } else {
                    WRENCH_INFO("Not enough idle cores on the CloudComputeService to create a big enough VM for task '%s", task->getID().c_str());
                }
            }

            // If no VM is available to run the task, then nevermind
            if (picked_vm_cs == nullptr) {
                continue;
            }

            WRENCH_INFO("Submitting task '%s' for execution on a VM", task->getID().c_str());

            // Submitting the task
            std::map<WorkflowFile *, std::shared_ptr<FileLocation>> file_locations;
            for (auto f : task->getInputFiles()) {
                file_locations[f] = (FileLocation::LOCATION(default_storage_service));
            }
            for (auto f : task->getOutputFiles()) {
                file_locations[f] = (FileLocation::LOCATION(default_storage_service));
            }
            auto job = this->getJobManager()->createStandardJob(task, file_locations);
            this->getJobManager()->submitJob(job, picked_vm_cs);
        }
        WRENCH_INFO("Done with scheduling tasks as standard jobs");
    }

}
