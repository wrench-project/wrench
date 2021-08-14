/**
 * Copyright (c) 2020. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <iostream>

#include "CondorWMS.h"
#include "CondorTimestamp.h"

WRENCH_LOG_CATEGORY(custom_wms, "Log category for Custom WMS");


namespace wrench {

    CondorWMS::CondorWMS(const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                         const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                         std::string &hostname) : WMS(
                    nullptr, nullptr,
                    compute_services,
                    storage_services,
                    {},
                    nullptr,
                    hostname,
                    "condor-grid"){}

    /**
     * @brief main method of the CondorWMS daemon
     *
     * @return 0 on completion
     *
     * @throw std::runtime_error
     */
    int CondorWMS::main() {

        /* Set the logging output to GREEN */
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);

        /* Create a data movement manager */
        auto data_movement_manager = this->createDataMovementManager();

        /* Create a job manager */
        auto job_manager = this->createJobManager();

        /* Get reference to the storage service */
        auto ss = *(this->getAvailableStorageServices().begin());

        /* Get references to all compute services */
        auto htcondor_cs = *(this->getAvailableComputeServices<wrench::HTCondorComputeService>().begin());
        auto batch_cs = *(this->getAvailableComputeServices<wrench::BatchComputeService>().begin());
        auto cloud_cs = *(this->getAvailableComputeServices<wrench::CloudComputeService>().begin());

        /* Create and start a 5-core VM with 32GB of RAM on the Cloud compute service */
        cloud_cs->createVM(5, 32*1000*1000*1000, "my_vm", {}, {});
        auto vm_cs = cloud_cs->startVM("my_vm");

        /* Add the VM's BareMetalComputeService to the HTCondor compute service */
        htcondor_cs->addComputeService(vm_cs);

        /* At this point, HTCondor has access to: .... */

        /* Create a map of files, which are all supposed to be on the local SS */
        std::map<WorkflowFile *, std::shared_ptr<FileLocation>> file_locations;
        for (auto const &t : this->getWorkflow()->getTasks()) {
            for (auto const &f : t->getInputFiles()) {
                file_locations[f] = wrench::FileLocation::LOCATION(ss);
            }
            for (auto const &f : t->getOutputFiles()) {
                file_locations[f] = wrench::FileLocation::LOCATION(ss);
            }
        }

        /* Split the 10 tasks into two groups of 5 tasks */
        std::vector<wrench::WorkflowTask *> first_five_tasks;
        std::vector<wrench::WorkflowTask *> last_five_tasks;
        int task_count = 0;
        for (auto const &t : this->getWorkflow()->getTasks()) {
            if (task_count < 5) {
                first_five_tasks.push_back(t);
            } else {
                last_five_tasks.push_back(t);
            }
            task_count++;
        }
        std::cerr << "CONDOR ALL SETUP \n";

        /* Submit the first 5 tasks as part of a single "grid universe" job to HTCondor */
        auto grid_universe_job = job_manager->createStandardJob(first_five_tasks, file_locations);
        std::map<std::string, std::string> htcondor_service_specific_arguments;
        htcondor_service_specific_arguments["universe"] = "grid";
        htcondor_service_specific_arguments["-N"] = "3";
        htcondor_service_specific_arguments["-c"] = "5";
        htcondor_service_specific_arguments["-t"] = "3600";
        htcondor_service_specific_arguments["-service"] = batch_cs->getName();
        job_manager->submitJob(grid_universe_job, htcondor_cs, htcondor_service_specific_arguments);

        std::cerr << "CONDOR ALL SETUP \n";
        /* Submit the next 5 tasks as individual non "grid universe" jobs to HTCondor */
        for (auto const &t : last_five_tasks) {
            auto job = job_manager->createStandardJob(t, file_locations);
            job_manager->submitJob(job, htcondor_cs);
        }

        /* Wait for all execution events */
        while (not this->getWorkflow()->isDone()) {
            this->waitForAndProcessNextEvent();
        }

        htcondor_cs->stop();
        return 0;
    }

    /**
     * @brief Process a standard job completion event
     *
     * @param event: the event
     */
    void CondorWMS::processEventStandardJobCompletion(std::shared_ptr<StandardJobCompletedEvent> event) {
        /* Retrieve the job that this event is for */
        auto job = event->standard_job;
        /* Retrieve the job's tasks */
        for (auto const &task : job->getTasks()) {
            WRENCH_INFO("Notified that a standard job has completed task %s",
                        task->getID().c_str())
        }
        simulation->getOutput().addTimestamp<CondorGridEndTimestamp>(new CondorGridEndTimestamp);
    }



}
