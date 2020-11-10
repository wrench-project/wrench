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
        simulation->getOutput().addTimestamp<CondorGridStartTimestamp>(new CondorGridStartTimestamp);

        /* Set the logging output to GREEN */
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job manager
        auto job_manager = this->createJobManager();


        auto htcondor_cs = *(this->getAvailableComputeServices<wrench::HTCondorComputeService>().begin());


        std::map<WorkflowFile *, std::shared_ptr<FileLocation>> file_locations;
        std::shared_ptr<wrench::StorageService> storage_service;


        //Only set up for one storage service
        for (const auto &ss : this->getAvailableStorageServices()) {
            storage_service = ss;
        }

        //Loop through files and indicate they are stored on the storage service.
        for( auto &file : this->getWorkflow()->getFiles()){
            file_locations[file] = FileLocation::LOCATION(storage_service);
        }






        bool first_task = true;



        while (not this->getWorkflow()->isDone()) {

            auto tasks = this->getWorkflow()->getReadyTasks();
            std::map<std::string, std::string> test_service_specs;
            test_service_specs.insert(std::pair<std::string, std::string>("universe", "grid"));
            if(first_task){
                first_task = false;
                test_service_specs.insert(std::pair<std::string, std::string>("grid_start", "grid"));
            }
            if(tasks.size()==1){
                test_service_specs.insert(std::pair<std::string, std::string>("grid_end", "grid"));
            }

            wrench::StandardJob *grid_job = job_manager->createStandardJob(
                    tasks, file_locations);
            //printf("Submitting job with %lu tasks\n",tasks.size());

            //test_service_specs.insert(std::pair<std::string, std::string>("grid_pre_delay","50.0"));
            //test_service_specs.insert(std::pair<std::string, std::string>("grid_post_delay","400.0"));

            // Submit the 2-task job for execution
            try {
                //printf("Submitting job with %i tasks\n",);
                job_manager->submitJob(grid_job, htcondor_cs, test_service_specs);
            } catch (wrench::WorkflowExecutionException &e) {
                throw std::runtime_error(e.what());
            }
            //Waiting for job to finish.
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
