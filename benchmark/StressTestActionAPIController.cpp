/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <set>
#include "StressTestActionAPIController.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(stress_test_action_controller, "Log category for Stress Test Action Controller");

namespace wrench {

    int StressTestActionAPIController::main() {

#if 0
        std::shared_ptr<JobManager> job_manager = this->createJobManager();

        unsigned long max_num_pending_jobs = 10;
        unsigned long num_pending_jobs = 0;
        unsigned long num_completed_jobs = 0;


        while (num_completed_jobs < num_jobs) {

            while ((num_pending_jobs < max_num_pending_jobs) and (num_pending_jobs + num_completed_jobs < num_jobs)) {

                WRENCH_INFO("Creating a new job");

                auto job = job_manager->createCompoundJob("job_" + std::to_string(num_completed_jobs));

                // Pick a random compute
                auto cs_it(compute_services.begin());
                advance(cs_it, rand() % compute_services.size());
                auto target_cs = *cs_it;
                // Pick a random storage_service
                auto ss_it(storage_services.begin());
                advance(ss_it, rand() % storage_services.size());
                auto target_ss = *ss_it;

                auto infile = Simulation::addFile("infile_" + std::to_string(num_completed_jobs + num_pending_jobs), 100000000);
                auto outfile = Simulation::addFile("outfile_" + std::to_string(num_completed_jobs + num_pending_jobs), 100000000);
                StorageService::createFileAtLocation(FileLocation::LOCATION(target_ss, infile));

                auto read_file_action = job->addFileReadAction("read_" + std::to_string(num_completed_jobs + num_pending_jobs), FileLocation::LOCATION(target_ss, infile));
                auto compute_action = job->addComputeAction("compute_" + std::to_string(num_completed_jobs + num_pending_jobs), 1000.0, 0, 1, 1, ParallelModel::CONSTANTEFFICIENCY(1.0));
                auto write_file_action = job->addFileWriteAction("write_" + std::to_string(num_completed_jobs + num_pending_jobs), FileLocation::LOCATION(target_ss, outfile));
                job->addActionDependency(read_file_action, compute_action);
                job->addActionDependency(compute_action, write_file_action);

                job_manager->submitJob(job, target_cs);
                num_pending_jobs++;
            }

            std::shared_ptr<wrench::ExecutionEvent> event;
            event = this->waitForNextEvent();
            if (auto real_event = dynamic_cast<wrench::CompoundJobCompletedEvent *>(event.get())) {
                auto job = real_event->job;

                num_completed_jobs++;
                num_pending_jobs--;
                // Erase the files
                for (auto const &action: job->getActions()) {
                    if (std::dynamic_pointer_cast<FileReadAction>(action)) {
                        StorageService::removeFileAtLocation(std::dynamic_pointer_cast<FileReadAction>(action)->getFileLocations().at(0));
                    } else if (std::dynamic_pointer_cast<FileWriteAction>(action)) {
                        StorageService::removeFileAtLocation(std::dynamic_pointer_cast<FileWriteAction>(action)->getFileLocation());
                    }
                }

            } else if (auto real_event = dynamic_cast<wrench::CompoundJobFailedEvent *>(event.get())) {
                throw std::runtime_error(real_event->failure_cause->toString());
            } else {
                throw std::runtime_error("Got an unexpected event!");
            }
        }
#endif
        return 0;
    }

};// namespace wrench
