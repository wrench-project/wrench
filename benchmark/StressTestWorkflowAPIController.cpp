/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <set>
#include "StressTestWorkflowAPIController.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(stress_test_workflow_controller, "Log category for Stress Test Workflow Controller");

namespace wrench {

    int StressTestWorkflowAPIController::main() {
        WRENCH_INFO("New WMS starting");

        // Creating workflow
        shared_ptr<Workflow> workflow = Workflow::createWorkflow();
        // One task per job, all independent
        for (unsigned int i=0; i < this->num_jobs; i++) {
            shared_ptr<WorkflowTask> task = workflow->addTask("task_" + std::to_string(i), 1000.0, 1, 1, 1.0);
            task->addOutputFile(workflow->addFile("outfile_" + std::to_string(i), 100000000));
            task->addInputFile(workflow->addFile("infile_" + std::to_string(i), 100000000));
        }

        std::shared_ptr<JobManager> job_manager = this->createJobManager();


        std::set<shared_ptr<WorkflowTask> > tasks_to_do;
        for (const auto& t : workflow->getTasks()) {
            tasks_to_do.insert(t);
        }
        std::set<shared_ptr<WorkflowTask> > tasks_pending;

        //REMOVE//std::set<std::shared_ptr<ComputeService>> compute_services = this->getAvailableComputeServices<ComputeService>();
        //REMOVE//std::set<std::shared_ptr<StorageService>> storage_services = this->getAvailableStorageServices();

        unsigned long max_num_pending_tasks = 10;

        WRENCH_INFO("%lu tasks to run", tasks_to_do.size());

        while ((not tasks_to_do.empty()) or (not tasks_pending.empty())) {

            while ((!tasks_to_do.empty()) and (tasks_pending.size() < max_num_pending_tasks)) {

                WRENCH_INFO("Looking at scheduling another task");
                shared_ptr<WorkflowTask> to_submit = *(tasks_to_do.begin());
                tasks_to_do.erase(to_submit);
                tasks_pending.insert(to_submit);

                auto input_file = *(to_submit->getInputFiles().begin());
                auto output_file = *(to_submit->getOutputFiles().begin());
                // Pick a random compute
                auto cs_it(compute_services.begin());
                advance(cs_it, rand() % compute_services.size());
                auto target_cs = *cs_it;
                // Pick a random storage_service
                auto ss_it(storage_services.begin());
                advance(ss_it, rand() % storage_services.size());
                auto target_ss = *ss_it;

                StorageService::createFileAtLocation(wrench::FileLocation::LOCATION(target_ss, input_file));
                auto job = job_manager->createStandardJob(to_submit,
                                                          {{input_file, wrench::FileLocation::LOCATION(target_ss, input_file)},
                                                           {output_file, wrench::FileLocation::LOCATION(target_ss, output_file)}
                                                          });
                job_manager->submitJob(job, target_cs);

            }

            std::shared_ptr <wrench::ExecutionEvent> event;
            event = this->waitForNextEvent();
            if (auto real_event = dynamic_cast<wrench::StandardJobCompletedEvent *>(event.get())) {
                shared_ptr<WorkflowTask> completed_task = *(real_event->standard_job->getTasks().begin());
                WRENCH_INFO("Task %s has completed", completed_task->getID().c_str());
                if (tasks_to_do.size() % 10 == 0) {
                    //std::cerr << ".";
                }
                // Erase the task's input and output file
                for (auto const &location : real_event->standard_job->getFileLocations()) {
                    StorageService::removeFileAtLocation(location.second.at(0));
                }

                // Erase the pending task
                tasks_pending.erase(completed_task);
            } else if (auto real_event = dynamic_cast<wrench::StandardJobFailedEvent *>(event.get())) {
                throw std::runtime_error(real_event->failure_cause->toString());
            } else {
                throw std::runtime_error("Got unexpected event!");
            }
        }

        return 0;
    }

};
