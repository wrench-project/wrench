/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <string>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <wrench/wms/WMS.h>
#include <wrench/exceptions/ExecutionException.h>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/managers/JobManager.h>
#include <wrench/services/compute/ComputeService.h>
#include <wrench/services/ServiceMessage.h>
#include <wrench/services/compute/ComputeServiceMessage.h>
#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>
#include <wrench/simulation/SimulationMessage.h>
#include <wrench/workflow/WorkflowTask.h>
#include <wrench/job/StandardJob.h>
#include <wrench/job/CompoundJob.h>
#include <wrench/job/PilotJob.h>
#include "JobManagerMessage.h"


WRENCH_LOG_CATEGORY(wrench_core_job_manager, "Log category for Job Manager");

namespace wrench {
    /**
     * @brief Constructor
     *
     * @param wms: the wms for which this manager is working
     */
    JobManager::JobManager(std::shared_ptr<WMS> wms) :
            Service(wms->hostname, "job_manager", "job_manager") {
        this->wms = wms;
    }

    /**
     * @brief Destructor, which kills the daemon (and clears all the jobs)
     */
    JobManager::~JobManager() {
        this->jobs_to_dispatch.clear();
        this->jobs_dispatched.clear();

//        this->pending_standard_jobs.clear();
//        this->running_standard_jobs.clear();
//        this->completed_standard_jobs.clear();
//        this->failed_standard_jobs.clear();
//
//        this->pending_pilot_jobs.clear();
//        this->running_pilot_jobs.clear();
//        this->completed_pilot_jobs.clear();
    }

    /**
     * @brief Kill the job manager (brutally terminate the daemon, clears all jobs)
     */
    void JobManager::kill() {
        this->killActor();
        this->jobs_to_dispatch.clear();
        this->jobs_dispatched.clear();

//        this->pending_standard_jobs.clear();
//        this->running_standard_jobs.clear();
//        this->completed_standard_jobs.clear();
//        this->failed_standard_jobs.clear();
//
//        this->pending_pilot_jobs.clear();
//        this->running_pilot_jobs.clear();
//        this->completed_pilot_jobs.clear();
    }

    /**
     * @brief Stop the job manager
     *
     * @throw ExecutionException
     * @throw std::runtime_error
     */
    void JobManager::stop() {
        try {
            S4U_Mailbox::putMessage(this->mailbox_name, new ServiceStopDaemonMessage("", 0.0));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw ExecutionException(cause);
        }
    }

    /**
     * @brief Create a standard job
     *
     * @param tasks: a list of tasks (which must be either READY, or children of COMPLETED tasks or
     *                                   of tasks also included in the standard job)
     * @param file_locations: a map that specifies locations where input/output files, if any, should be read/written.
     *         When empty, it is assumed that the ComputeService's scratch storage space will be used.
     * @param pre_file_copies: a vector of tuples that specify which file copy operations should be completed
     *                         before task executions begin. The ComputeService::SCRATCH constant can be
     *                         used to mean "the scratch storage space of the ComputeService".
     * @param post_file_copies: a vector of tuples that specify which file copy operations should be completed
     *                         after task executions end. The ComputeService::SCRATCH constant can be
     *                         used to mean "the scratch storage space of the ComputeService".
     * @param cleanup_file_deletions: a vector of file tuples that specify file deletion operations that should be completed
     *                                at the end of the job. The ComputeService::SCRATCH constant can be
     *                         used to mean "the scratch storage space of the ComputeService".
     * @return the standard job
     *
     * @throw std::invalid_argument
     */
    std::shared_ptr<StandardJob> JobManager::createStandardJob(
            std::vector<WorkflowTask *> tasks,
            std::map<WorkflowFile *, std::shared_ptr<FileLocation>> file_locations,
            std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>  >> pre_file_copies,
            std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>  >> post_file_copies,
            std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>  >> cleanup_file_deletions) {
        // Transform the non-vector file location map into a vector file location map
        std::map<WorkflowFile *, std::vector<std::shared_ptr<FileLocation>>> file_locations_vector;
        for (auto const &e : file_locations) {
            std::vector<std::shared_ptr<FileLocation>> v;
            v.push_back(e.second);
            file_locations_vector[e.first] = v;
        }

        // Call the vectorized method
        return createStandardJob(tasks, file_locations_vector, pre_file_copies, post_file_copies,
                                 cleanup_file_deletions);
    }

    /**
     * @brief Create a standard job
     *
     * @param tasks: a list of tasks (which must be either READY, or children of COMPLETED tasks or
     *               of tasks also included in the standard job)
     * @param file_locations: a map that specifies, for each file, a list of locations, in preference order, where
     *                        input/output files should be read/written.
     *                        When unspecified, it is assumed that the ComputeService's scratch storage space will be used.
     * @param pre_file_copies: a vector of tuples that specify which file copy operations should be completed
     *                         before task executions begin. The ComputeService::SCRATCH constant can be
     *                         used to mean "the scratch storage space of the ComputeService".
     * @param post_file_copies: a vector of tuples that specify which file copy operations should be completed
     *                         after task executions end. The ComputeService::SCRATCH constant can be
     *                         used to mean "the scratch storage space of the ComputeService".
     * @param cleanup_file_deletions: a vector of file tuples that specify file deletion operations that should be
     *                                completed at the end of the job. The ComputeService::SCRATCH constant can be
     *                                used to mean "the scratch storage space of the ComputeService".
     * @return the standard job
     *
     * @throw std::invalid_argument
     */
    std::shared_ptr<StandardJob> JobManager::createStandardJob(
            std::vector<WorkflowTask *> tasks,
            std::map<WorkflowFile *, std::vector<std::shared_ptr<FileLocation>>> file_locations,
            std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>  >> pre_file_copies,
            std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>  >> post_file_copies,
            std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>  >> cleanup_file_deletions) {
        // Do a sanity check of everything (looking for nullptr)
        for (auto t : tasks) {
            if (t == nullptr) {
                throw std::invalid_argument("JobManager::createStandardJob(): nullptr task in the task vector");
            }
        }

        for (auto const &fl : file_locations) {
            if (fl.first == nullptr) {
                throw std::invalid_argument(
                        "JobManager::createStandardJob(): nullptr workflow file in the file_locations map");
            }
            if (fl.second.empty()) {
                throw std::invalid_argument(
                        "JobManager::createStandardJob(): empty location vector in the file_locations map");
            }
            for (auto const &fl_l : fl.second) {
                if (fl_l == nullptr) {
                    throw std::invalid_argument(
                            "JobManager::createStandardJob(): nullptr file location in the file_locations map");
                }
            }
        }

        for (auto fc : pre_file_copies) {
            if (std::get<0>(fc) == nullptr) {
                throw std::invalid_argument(
                        "JobManager::createStandardJob(): nullptr workflow file in the pre_file_copies set");
            }
            if (std::get<1>(fc) == nullptr) {
                throw std::invalid_argument(
                        "JobManager::createStandardJob(): nullptr src storage service in the pre_file_copies set");
            }
            if (std::get<2>(fc) == nullptr) {
                throw std::invalid_argument(
                        "JobManager::createStandardJob(): nullptr dst storage service in the pre_file_copies set");
            }
            if ((std::get<1>(fc) == FileLocation::SCRATCH) and (std::get<2>(fc) == FileLocation::SCRATCH)) {
                throw std::invalid_argument(
                        "JobManager::createStandardJob(): cannot have FileLocation::SCRATCH as both source and "
                        "destination in the pre_file_copies set");
            }
        }

        for (auto fc : post_file_copies) {
            if (std::get<0>(fc) == nullptr) {
                throw std::invalid_argument(
                        "JobManager::createStandardJob(): nullptr workflow file in the post_file_copies set");
            }
            if (std::get<1>(fc) == nullptr) {
                throw std::invalid_argument(
                        "JobManager::createStandardJob(): nullptr src storage service in the post_file_copies set");
            }
            if (std::get<2>(fc) == nullptr) {
                throw std::invalid_argument(
                        "JobManager::createStandardJob(): nullptr dst storage service in the post_file_copies set");
            }
            if ((std::get<1>(fc) == FileLocation::SCRATCH) and (std::get<2>(fc) == FileLocation::SCRATCH)) {
                throw std::invalid_argument(
                        "JobManager::createStandardJob(): cannot have FileLocation::SCRATCH as both source and "
                        "destination in the pre_file_copies set");
            }
        }

        for (auto fd : cleanup_file_deletions) {
            if (std::get<0>(fd) == nullptr) {
                throw std::invalid_argument(
                        "JobManager::createStandardJob(): nullptr workflow file in the cleanup_file_deletions set");
            }
            if (std::get<1>(fd) == nullptr) {
                throw std::invalid_argument(
                        "JobManager::createStandardJob(): nullptr storage service in the cleanup_file_deletions set");
            }
        }

        // Check that every file has a file location: NOPE USE SCARTCH B Y DEFAULT
//        for (const auto &t : tasks) {
//            for (const auto & f : t->getInputFiles()) {
//                if (file_locations.find(f) == file_locations.end()) {
//                    throw std::invalid_argument(
//                            "JobManager::createStandardJob(): A location needs to be provided for file " + f->getID());
//                }
//            }
//            for (const auto & f : t->getOutputFiles()) {
//                if (file_locations.find(f) == file_locations.end()) {
//                    throw std::invalid_argument(
//                            "JobManager::createStandardJob(): A location needs to be provided for file " + f->getID());
//                }
//            }
//        }

        auto job = std::shared_ptr<StandardJob>(
                new StandardJob(this->wms->getWorkflow(), this->getSharedPtr<JobManager>(), tasks, file_locations, pre_file_copies,
                                post_file_copies, cleanup_file_deletions));

        return job;
    }

    /**
     * @brief Create a standard job
     *
     * @param tasks: a list of tasks  (which must be either READY, or children of COMPLETED tasks or
     *                                   of tasks also included in the list)
     * @param file_locations: a map that specifies locations where files, if any, should be read/written.
     *                        When empty, it is assumed that the ComputeService's scratch storage space will be used.
     *
     * @return the standard job
     *
     * @throw std::invalid_argument
     */
    std::shared_ptr<StandardJob> JobManager::createStandardJob(
            std::vector<WorkflowTask *> tasks,
            std::map<WorkflowFile *, std::shared_ptr<FileLocation> > file_locations) {
        if (tasks.empty()) {
            throw std::invalid_argument("JobManager::createStandardJob(): Invalid arguments (empty tasks argument!)");
        }

        return this->createStandardJob(tasks, file_locations, {}, {}, {});
    }

    /**
     * @brief Create a standard job
     *
     * @param tasks: a list of tasks  (which must be either READY, or children of COMPLETED tasks or
     *                                   of tasks also included in the list)
     * @param file_locations: a map that specifies, for each file, a list of locations, in preference order, where input/output files should be read/written.
     *                When unspecified, it is assumed that the ComputeService's scratch storage space will be used.
     *
     * @return the standard job
     *
     * @throw std::invalid_argument
     */
    std::shared_ptr<StandardJob> JobManager::createStandardJob(
            std::vector<WorkflowTask *> tasks,
            std::map<WorkflowFile *, std::vector<std::shared_ptr<FileLocation>>> file_locations) {
        if (tasks.empty()) {
            throw std::invalid_argument("JobManager::createStandardJob(): Invalid arguments (empty tasks argument!)");
        }

        return this->createStandardJob(tasks, file_locations, {}, {}, {});
    }

    /**
  * @brief Create a standard job
  *
  * @param tasks: a list of tasks  (which must be either READY, or children of COMPLETED tasks or
  *                                   of tasks also included in the list)
  * @return the standard job
  *
  * @throw std::invalid_argument
  */
    std::shared_ptr<StandardJob> JobManager::createStandardJob(
            std::vector<WorkflowTask *> tasks) {
        if (tasks.empty()) {
            throw std::invalid_argument("JobManager::createStandardJob(): Invalid arguments (empty tasks argument!)");
        }

        return this->createStandardJob(tasks, (std::map<WorkflowFile *, std::vector<std::shared_ptr<FileLocation>>>) {},
                                       {}, {}, {});
    }

    /**
     * @brief Create a standard job
     *
     * @param task: a task (which must be ready)
     * @param file_locations: a map that specifies locations where input/output files should be read/written.
     *                When unspecified, it is assumed that the ComputeService's scratch storage space will be used.
     *
     * @return the standard job
     *
     * @throw std::invalid_argument
     */
    std::shared_ptr<StandardJob> JobManager::createStandardJob(
            WorkflowTask *task,
            std::map<WorkflowFile *, std::shared_ptr<FileLocation> > file_locations) {
        if (task == nullptr) {
            throw std::invalid_argument("JobManager::createStandardJob(): Invalid arguments");
        }

        std::vector<WorkflowTask *> tasks;
        tasks.push_back(task);
        return this->createStandardJob(tasks, file_locations);
    }

    /**
     * @brief Create a standard job
     *
     * @param task: a task (which must be ready)
     * @param file_locations: a map that specifies, for each file, a list of locations, in preference order, where
     *                input/output files should be read/written.
     *                When unspecified, it is assumed that the ComputeService's scratch storage space will be used.
     *
     * @return the standard job
     *
     * @throw std::invalid_argument
     */
    std::shared_ptr<StandardJob> JobManager::createStandardJob(
            WorkflowTask *task,
            std::map<WorkflowFile *, std::vector<std::shared_ptr<FileLocation>>> file_locations) {
        if (task == nullptr) {
            throw std::invalid_argument("JobManager::createStandardJob(): Invalid arguments");
        }

        std::vector<WorkflowTask *> tasks;
        tasks.push_back(task);
        return this->createStandardJob(tasks, file_locations);
    }

    /**
     * @brief Create a standard job
     *
     * @param task: a task (which must be ready)
     *
     * @return the standard job
     *
     * @throw std::invalid_argument
     */
    std::shared_ptr<StandardJob> JobManager::createStandardJob(
            WorkflowTask *task) {
        if (task == nullptr) {
            throw std::invalid_argument("JobManager::createStandardJob(): Invalid arguments");
        }

        std::vector<WorkflowTask *> tasks;
        tasks.push_back(task);
        return this->createStandardJob(tasks, std::map<WorkflowFile *, std::vector<std::shared_ptr<FileLocation>>>{});
    }



    /**
     * @brief Create a pilot job
     *
     * @return the pilot job
     *
     * @throw std::invalid_argument
     */
    std::shared_ptr<PilotJob> JobManager::createPilotJob() {
        auto job = std::shared_ptr<PilotJob>(new PilotJob(this->wms->workflow));
        return job;
    }

    /**
     * @brief Helper method to validate a job submission
     * @param job: the job to submit
     * @param compute_service: the compute service
     * @param service_specific_args: the service-specific arguments
     */
    void JobManager::validateJobSubmission(std::shared_ptr<Job> job,
                                           std::shared_ptr<ComputeService> compute_service,
                                           std::map<std::string, std::string> service_specific_args) {

        if (auto cjob = std::dynamic_pointer_cast<CompoundJob>(job)) {
            validateCompoundJobSubmission(cjob, compute_service, service_specific_args);
        } else if (auto pjob = std::dynamic_pointer_cast<PilotJob>(job)) {
            validatePilotJobSubmission(pjob, compute_service, service_specific_args);
        }
    }

    /**
     * @brief Helper method to validate a compound job submission
     * @param job: the job to submit
     * @param compute_service: the compute service
     * @param service_specific_args: the service-specific arguments
     */
    void JobManager::validateCompoundJobSubmission(std::shared_ptr<CompoundJob> job,
                                                   std::shared_ptr<ComputeService> compute_service,
                                                   std::map<std::string, std::string> service_specific_args) {

        /* make sure that service arguments are provided for valid actions in the jobs */
        for (auto const &arg : service_specific_args) {
            bool found = false;
            for (auto const &action : job->getActions()) {
                if (action->getName() == arg.first) {
                    found = true;
                    break;
                }
            }
            if (not found) {
                throw std::invalid_argument(
                        "JobManager::validateCompoundJobSubmission(): Service-specific argument provided for action with name '" +
                        arg.first + "' but there is no action with such name in the job");
            }
        }

        // Invoke the validation method on the service
        compute_service->validateServiceSpecificArguments(job, service_specific_args);

    }

    /**
    * @brief Helper method to validate a pilot job submission
    * @param job: the job to submit
    * @param compute_service: the compute service
    * @param service_specific_args: the service-specific arguments
    */
    void JobManager::validatePilotJobSubmission(std::shared_ptr<PilotJob> job,
                                                std::shared_ptr<ComputeService> compute_service,
                                                std::map<std::string, std::string> service_specific_args) {
        throw std::runtime_error("JobManager::validatePilotJobSubmission(): TO IMPLEMENT!");
    }

    /**
     * @brief Submit a job to compute service
     *
     * @param job: a workflow job
     * @param compute_service: a compute service
     * @param service_specific_args: arguments specific for compute services:
     *      - to a BareMetalComputeService: {{"taskID", "[hostname:][num_cores]}, ...}
     *           - If no value is provided for a task, then the service will choose a host and use as many cores as possible on that host.
     *           - If a "" value is provided for a task, then the service will choose a host and use as many cores as possible on that host.
     *           - If a "hostname" value is provided for a task, then the service will run the task on that
     *             host, using as many of its cores as possible
     *           - If a "num_cores" value is provided for a task, then the service will run that task with
     *             this many cores, but will choose the host on which to run it.
     *           - If a "hostname:num_cores" value is provided for a task, then the service will run that
     *             task with the specified number of cores on that host.
     *      - to a BatchComputeService: {{"-t":"<int>" (requested number of minutes)},{"-N":"<int>" (number of requested hosts)},{"-c":"<int>" (number of requested cores per host)}[,{"-u":"<string>" (username)}]}
     *      - to a VirtualizedClusterComputeService: {} (jobs should not be submitted directly to the service)}
     *      - to a CloudComputeService: {} (jobs should not be submitted directly to the service)}
     *      - to a HTCondorComputeService:
     *           - For a "grid universe" job that will be submitted to a child BatchComputeService: {{"universe":"grid", {"-t":"<int>" (requested number of minutes)},{"-N":"<int>" (number of requested hosts)},{"-c":"<int>" (number of requested cores per host)}[,{"-service":"<string>" (batch service name)}, {"-u":"<string>" (username)}]}
     *           - For a "non-grid universe" job that will be submitted to a child BareMetalComputeService: {}
     *
     *
     * @throw std::invalid_argument
     * @throw ExecutionException
     */
    void JobManager::submitJob(std::shared_ptr<Job> job,
                               std::shared_ptr<ComputeService> compute_service,
                               std::map<std::string, std::string> service_specific_args) {
        if ((job == nullptr) || (compute_service == nullptr)) {
            throw std::invalid_argument("JobManager::submitJob(): Invalid arguments");
        }

        if (job->already_submitted_to_job_manager) {
            throw std::invalid_argument("JobManager::submitJob(): Job was previously submitted");
        }

        try {
            compute_service->assertServiceIsUp();
        } catch (ExecutionException &e) {
            throw;
        }


        // Update the job state and insert it into the pending list
        if (auto sjob = std::dynamic_pointer_cast<StandardJob>(job)) {

            if (not compute_service->supportsStandardJobs()) {
                throw ExecutionException(std::make_shared<JobTypeNotSupported>(job, compute_service));
            }
//            std::map<WorkflowTask *, WorkflowTask::State> original_states;

            // Do a sanity check on task states
            for (auto t : sjob->tasks) {
                if ((t->getState() == WorkflowTask::State::COMPLETED) or
                    (t->getState() == WorkflowTask::State::PENDING)) {
                    throw std::invalid_argument("JobManager()::submitJob(): task " + t->getID() +
                                                " cannot be submitted as part of a standard job because its state is " +
                                                WorkflowTask::stateToString(t->getState()));
                }
            }

            auto cjob = this->createCompoundJob("compound_job_for_" + sjob->getName());

            // Do a sanity check on use of scratch space, and build the file location map for
            // the compound job!

            for (auto fl : sjob->file_locations) {
                for (auto const &fl_l : fl.second) {
                    if ((fl_l == FileLocation::SCRATCH) and (not compute_service->hasScratch())) {
                        throw std::invalid_argument("JobManager():submitJob(): file location for file " +
                                                    fl.first->getID() +
                                                    " is scratch  space, but the compute service to which this " +
                                                    "job is being submitted to doesn't have any!");
                    }
                }
            }

            // Create the corresponding compound job
            std::shared_ptr<Action> pre_overhead_action = nullptr;
            std::shared_ptr<Action> post_overhead_action = nullptr;
            std::vector<std::shared_ptr<Action>> pre_file_copy_actions;
            std::map<WorkflowTask*, std::vector<std::shared_ptr<Action>>> task_file_read_actions;
            std::map<WorkflowTask*, std::shared_ptr<Action>> task_compute_actions;
            std::map<WorkflowTask*, std::vector<std::shared_ptr<Action>>> task_file_write_actions;
            std::vector<std::shared_ptr<Action>> post_file_copy_actions;
            std::vector<std::shared_ptr<Action>> cleanup_actions;
            std::shared_ptr<Action> scratch_cleanup = nullptr;

            // Create pre- and post-overhead work units
            if (sjob->getPreJobOverheadInSeconds() > 0.0) {
                pre_overhead_action = cjob->addSleepAction("", sjob->getPreJobOverheadInSeconds());
            }

            if (sjob->getPostJobOverheadInSeconds() > 0.0) {
                post_overhead_action = cjob->addSleepAction("", sjob->getPostJobOverheadInSeconds());
            }

            // Create the pre- file copy actions
            for (auto const &pfc : sjob->pre_file_copies) {
                auto src_location = std::get<1>(pfc);
                auto dst_location = std::get<2>(pfc);
                if (src_location == FileLocation::SCRATCH) {
                    src_location = FileLocation::LOCATION(compute_service->getScratch());
                }
                if (dst_location == FileLocation::SCRATCH) {
                    dst_location = FileLocation::LOCATION(compute_service->getScratch());
                }

                pre_file_copy_actions.push_back(cjob->addFileCopyAction("", std::get<0>(pfc), src_location, dst_location));
            }

            // Create the post- file copy actions
            for (auto const &pfc : sjob->post_file_copies) {
                auto src_location = std::get<1>(pfc);
                auto dst_location = std::get<2>(pfc);
                if (src_location == FileLocation::SCRATCH) {
                    src_location = FileLocation::LOCATION(compute_service->getScratch());
                }
                if (dst_location == FileLocation::SCRATCH) {
                    dst_location = FileLocation::LOCATION(compute_service->getScratch());
                }

                post_file_copy_actions.push_back(cjob->addFileCopyAction("", std::get<0>(pfc), src_location, dst_location));
            }

            // Create the file cleanup actions
            for (auto const &fc: sjob->cleanup_file_deletions) {
                auto target_location = std::get<1>(fc);
                if (target_location == FileLocation::SCRATCH) {
                    target_location = FileLocation::LOCATION(compute_service->getScratch());
                }
                cleanup_actions.push_back(cjob->addFileDeleteAction("", std::get<0>(fc), target_location));
            }

            auto file_locations = sjob->getFileLocations();

            // Create the task actions
            for (auto const &task : sjob->tasks) {
                auto compute_action = cjob->addComputeAction("task_" + task->getID(), task->getFlops(), task->getMemoryRequirement(),
                                                             task->getMinNumCores(), task->getMaxNumCores(), task->getParallelModel());
                task_compute_actions[task] = compute_action;
                task_file_read_actions[task] = {};
                for (auto const &f : task->getInputFiles()) {
                    std::shared_ptr<Action> fread_action;
                    if (file_locations.find(f) != file_locations.end()) {
                        std::vector<std::shared_ptr<FileLocation>> fixed_locations;
                        for (auto const &loc : file_locations[f]) {
                            if (loc == FileLocation::SCRATCH) {
                                fixed_locations.push_back(FileLocation::LOCATION(compute_service->getScratch()));
                            } else {
                                fixed_locations.push_back(loc);
                            }
                        }
                        fread_action = cjob->addFileReadAction("", f, fixed_locations);
                    } else {
                        fread_action = cjob->addFileReadAction("", f, FileLocation::LOCATION(compute_service->getScratch()));
                    }
                    task_file_read_actions[task].push_back(fread_action);
                    cjob->addActionDependency(fread_action, compute_action);
                }
                task_file_write_actions[task] = {};
                for (auto const &f : task->getOutputFiles()) {
                    std::shared_ptr<Action> fwrite_action;
                    if (file_locations.find(f) != file_locations.end()) {
                        std::vector<std::shared_ptr<FileLocation>> fixed_locations;
                        for (auto const &loc : file_locations[f]) {
                            if (loc == FileLocation::SCRATCH) {
                                fixed_locations.push_back(FileLocation::LOCATION(compute_service->getScratch()));
                            } else {
                                fixed_locations.push_back(loc);
                            }
                        }
                        fwrite_action = cjob->addFileWriteAction("", f, fixed_locations.at(0));  // TODO: The at(0) here is ok? I mean, what does it mean to write to multiple locations....
                    } else {
                        fwrite_action = cjob->addFileWriteAction("", f, FileLocation::LOCATION(compute_service->getScratch()));
                    }
                    task_file_write_actions[task].push_back(fwrite_action);
                    cjob->addActionDependency(compute_action, fwrite_action);
                }
            }


            // TODO replace this horror with some sjob function perhaps?
            bool need_scratch_clean = false;
            for (auto const &task : sjob->getTasks()) {
                for (auto const &f : task->getInputFiles()) {
                    if (sjob->getFileLocations().find(f) == sjob->getFileLocations().end()) {
                        need_scratch_clean = true;
                        break;
                    }
                }
                if (need_scratch_clean) {
                    break;
                }
                for (auto const &f : task->getOutputFiles()) {
                    if (sjob->getFileLocations().find(f) == sjob->getFileLocations().end()) {
                        need_scratch_clean = true;
                        break;
                    }
                }
                if (need_scratch_clean) {
                    break;
                }
            }

            // Create the scratch clean up actions
            if (need_scratch_clean) {
                // Does the lambda capture of cjob_file_locations work?
                auto lambda_execute = [sjob](const std::shared_ptr<wrench::ActionExecutor> &action_executor) {
                    for (auto const &task : sjob->getTasks()) {
                        for (auto const &f : task->getInputFiles()) {
                            if (sjob->getFileLocations().find(f) == sjob->getFileLocations().end()) {
                                try {
                                    auto scratch = sjob->getParentComputeService()->getScratch();
                                    scratch->deleteFile(f, FileLocation::LOCATION(scratch));
                                } catch (ExecutionException &ignore) {
                                    // ignore
                                }
                            }
                        }
                        for (auto const &f : task->getOutputFiles()) {
                            if (sjob->getFileLocations().find(f) == sjob->getFileLocations().end()) {
                                try {
                                    auto scratch = sjob->getParentComputeService()->getScratch();
                                    scratch->deleteFile(f, FileLocation::LOCATION(scratch));
                                } catch (ExecutionException &ignore) {
                                    // ignore
                                }
                            }
                        }
                    }
                };
                auto lambda_terminate = [](const std::shared_ptr<wrench::ActionExecutor> &action_executor) {};
                scratch_cleanup = cjob->addCustomAction("", lambda_execute, lambda_terminate);

            }

            // Add all inter-task dependencies
            for (auto const &parent_task : sjob->getTasks()) {
                for (auto const &child_task : parent_task->getChildren()) {
                    if (task_compute_actions.find(child_task) == task_compute_actions.end()) {
                        continue;
                    }
                    std::vector<std::shared_ptr<Action>> parent_actions;
                    if (not task_file_write_actions[parent_task].empty()) {
                        parent_actions = task_file_write_actions[parent_task];
                    } else {
                        parent_actions = {task_compute_actions[parent_task]};
                    }
                    std::vector<std::shared_ptr<Action>> child_actions;
                    if (not task_file_read_actions[child_task].empty()) {
                        child_actions = task_file_read_actions[child_task];
                    } else {
                        child_actions = {task_compute_actions[child_task]};
                    }
                    for (auto const &parent_action : parent_actions) {
                        for (auto const &child_action: child_actions) {
                            cjob->addActionDependency(parent_action, child_action);
                        }
                    }
                }
            }

            // Create  4 dummy tasks
            std::shared_ptr<Action> pre_overhead_to_pre_file_copies = cjob->addSleepAction("", 0);
            std::shared_ptr<Action> pre_file_copies_to_tasks = cjob->addSleepAction("", 0);
            std::shared_ptr<Action> tasks_to_post_file_copies = cjob->addSleepAction("", 0);
            std::shared_ptr<Action> tasks_post_file_copies_to_cleanup = cjob->addSleepAction("", 0);
            std::shared_ptr<Action> cleanup_to_post_overhead = cjob->addSleepAction("", 0);
            cjob->addActionDependency(pre_overhead_to_pre_file_copies,pre_file_copies_to_tasks);
            cjob->addActionDependency(pre_file_copies_to_tasks, tasks_to_post_file_copies);
            cjob->addActionDependency(tasks_to_post_file_copies, tasks_post_file_copies_to_cleanup);
            cjob->addActionDependency(tasks_post_file_copies_to_cleanup, cleanup_to_post_overhead);

            // Add all dependencies, using the dummy tasks to help
            if (pre_overhead_action != nullptr) {
                cjob->addActionDependency(pre_overhead_action, pre_overhead_to_pre_file_copies);
            }
            if (not pre_file_copy_actions.empty()) {
                for (auto const &pfca : pre_file_copy_actions) {
                    cjob->addActionDependency(pre_overhead_to_pre_file_copies, pfca);
                    cjob->addActionDependency(pfca, pre_file_copies_to_tasks);
                }
            }

            if (not task_compute_actions.empty()) {
                for (auto const &tca : task_compute_actions) {
                    WorkflowTask *task = tca.first;
                    if (not task_file_read_actions[task].empty()) {
                        for (auto const &tfra : task_file_read_actions[task]) {
                            cjob->addActionDependency(pre_file_copies_to_tasks, tfra);
                        }
                    } else {
                        cjob->addActionDependency(pre_file_copies_to_tasks, tca.second);
                    }
                    if (not task_file_write_actions[task].empty()) {
                        for (auto const &tfwa : task_file_write_actions[task]) {
                            cjob->addActionDependency(tfwa, tasks_to_post_file_copies);
                        }
                    } else {
                        cjob->addActionDependency(tca.second, tasks_to_post_file_copies);
                    }
                }
            }
            if (not post_file_copy_actions.empty()) {
                for (auto const &pfca : post_file_copy_actions) {
                    cjob->addActionDependency(tasks_to_post_file_copies, pfca);
                    cjob->addActionDependency(pfca, tasks_post_file_copies_to_cleanup);
                }
            }

            if (not cleanup_actions.empty()) {
                for (auto const &ca : cleanup_actions) {
                    cjob->addActionDependency(tasks_post_file_copies_to_cleanup, ca);
                    cjob->addActionDependency(ca, cleanup_to_post_overhead);
                }
            }

            if (post_overhead_action != nullptr) {
                cjob->addActionDependency(cleanup_to_post_overhead, post_overhead_action);
                if (scratch_cleanup != nullptr) {
                    cjob->addActionDependency(post_overhead_action, scratch_cleanup);
                }
            } else {
                if (scratch_cleanup != nullptr) {
                    cjob->addActionDependency(cleanup_to_post_overhead, scratch_cleanup);
                }
            }

            // Use the dummy tasks for "easy" dependencies and remove the dummies
            std::vector<std::shared_ptr<Action>> dummies = {pre_overhead_to_pre_file_copies, pre_file_copies_to_tasks, tasks_to_post_file_copies, tasks_post_file_copies_to_cleanup, cleanup_to_post_overhead};
            for (auto &dummy : dummies) {
                // propagate dependencies
                for (auto const &parent_action : dummy->getParents()) {
                    for (auto const &child_action : dummy->getChildren()) {
                        cjob->addActionDependency(parent_action, child_action);
                    }
                }
                // remove the dummy
                cjob->removeAction(dummy);
            }

//            cjob->printActionDependencies();


            // Tweak the service_specific_arguments
            std::map<std::string, std::string> new_args;
            if (not sjob->getTasks().empty()) {
                auto workflow = (*(sjob->getTasks().begin()))->getWorkflow();
                for (auto const &arg : service_specific_args) {
                    new_args[task_compute_actions[workflow->getTaskByID(arg.first)]->getName()] = arg.second;
                }
            }

            try {
                this->validateJobSubmission(cjob, compute_service, new_args);
            } catch (ExecutionException &e) {
                if (std::dynamic_pointer_cast<NotEnoughResources>(e.getCause())) {
                    throw ExecutionException(std::shared_ptr<NotEnoughResources>(new NotEnoughResources(job, compute_service)));
                } else {
                    throw;
                }
            } catch (std::invalid_argument &e) {
                throw;
            }

            cjob->setParentComputeService(sjob->getParentComputeService());

            // Record all this information in the sjob
            sjob->compound_job = cjob;
            sjob->pre_overhead_action = pre_overhead_action;
            sjob->post_overhead_action = post_overhead_action;
            sjob->pre_file_copy_actions = pre_file_copy_actions;
            sjob->post_file_copy_actions = post_file_copy_actions;
            sjob->cleanup_actions = cleanup_actions;
            sjob->task_file_read_actions = task_file_read_actions;
            sjob->task_compute_actions = task_compute_actions;
            sjob->task_file_write_actions = task_file_write_actions;

            // Modify task states
            sjob->state = StandardJob::PENDING;
            for (auto t : sjob->tasks) {
                t->setState(WorkflowTask::State::PENDING);
            }

            // The compound job
            this->cjob_to_sjob_map[cjob] = sjob;
            this->jobs_to_dispatch.push_back(cjob);

        } else if (auto pjob = std::dynamic_pointer_cast<PilotJob>(job)) {

            if (not compute_service->supportsPilotJobs()) {
                throw ExecutionException(std::make_shared<JobTypeNotSupported>(job, compute_service));
            }

            this->jobs_to_dispatch.push_back(job);
            try {
                this->validateJobSubmission(job, compute_service, service_specific_args);
            } catch (std::invalid_argument &e) {
                throw;
            }

        } else if (auto cjob = std::dynamic_pointer_cast<CompoundJob>(job)) {

            if (not compute_service->supportsCompoundJobs()) {
                throw ExecutionException(std::make_shared<JobTypeNotSupported>(job, compute_service));
            }

            try {
                this->validateJobSubmission(cjob, compute_service, service_specific_args);
            } catch (std::invalid_argument &e) {
                throw;
            }
            cjob->state = CompoundJob::State::SUBMITTED;
            this->jobs_to_dispatch.push_back(job);
        }

        job->already_submitted_to_job_manager = true;
        job->setServiceSpecificArguments(service_specific_args);
        job->setParentComputeService(compute_service);

        // Send a message to wake up the daemon
        try {
            S4U_Mailbox::dputMessage(this->mailbox_name,new JobManagerWakeupMessage());
        } catch (std::exception &e) {
            throw std::runtime_error("Cannot connect to job manager");
        }
    }

    /**
     * @brief Terminate a job (standard or pilot) that hasn't completed/expired/failed yet
     * @param job: the job to be terminated
     *
     * @throw ExecutionException
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    void JobManager::terminateJob(std::shared_ptr<Job> job) {
        if (job == nullptr) {
            throw std::invalid_argument("JobManager::terminateJob(): invalid argument");
        }

        if (job->getParentComputeService() == nullptr) {
            std::string err_msg = "Job cannot be terminated because it doesn't have a parent compute service";
            throw ExecutionException(std::shared_ptr<FailureCause>(new NotAllowed(nullptr, err_msg)));
        }

        try {
            job->getParentComputeService()->terminateJob(job);
        } catch (std::exception &e) {
            throw;
        }

        if (auto sjob = std::dynamic_pointer_cast<StandardJob>(job)) {
            sjob->state = StandardJob::State::TERMINATED;
            for (auto task : sjob->tasks) {
                switch (task->getInternalState()) {
                    case WorkflowTask::TASK_NOT_READY:
                        task->setState(WorkflowTask::State::NOT_READY);
                        break;
                    case WorkflowTask::TASK_READY:
                        task->setState(WorkflowTask::State::READY);
                        break;
                    case WorkflowTask::TASK_COMPLETED:
                        task->setState(WorkflowTask::State::COMPLETED);
                        break;
                    case WorkflowTask::TASK_RUNNING:
                    case WorkflowTask::TASK_FAILED:
                        task->setState(WorkflowTask::State::NOT_READY);
                        break;
                }
            }
            // Make second pass to fix NOT_READY states
            for (auto task : sjob->tasks) {
                if (task->getState() == WorkflowTask::State::NOT_READY) {
                    bool ready = true;
                    for (auto parent : task->getWorkflow()->getTaskParents(task)) {
                        if (parent->getState() != WorkflowTask::State::COMPLETED) {
                            ready = false;
                        }
                    }
                    if (ready) {
                        task->setState(WorkflowTask::State::READY);
                    }
                }
            }
        } else if (auto pjob = std::dynamic_pointer_cast<PilotJob>(job)) {
            pjob->state = PilotJob::State::TERMINATED;
        } else if (auto cjob = std::dynamic_pointer_cast<CompoundJob>(job)) {
            cjob->state = CompoundJob::State::DISCONTINUED;
        }
    }

    /**
     * @brief Get the list of currently running pilot jobs
     * @return a set of pilot jobs
     */
    unsigned long JobManager::getNumRunningPilotJobs() {
        return this->num_running_pilot_jobs;
    }


#if 0
    /**
     * @brief Forget a job (to free memory_manager_service, only once a job has completed or failed)
     *
     * @param job: a job to forget
     *
     * @throw std::invalid_argument
     * @throw ExecutionException
     */
    void JobManager::forgetJob(Job *job) {
        if (job == nullptr) {
            throw std::invalid_argument("JobManager::forgetJob(): invalid argument");
        }

        // Check the job is somewhere
        if (this->jobs.find(job) == this->jobs.end()) {
            throw std::invalid_argument("JobManager::forgetJob(): unknown job");
        }

        if (job->getType() == Job::STANDARD) {

            if ((this->pending_standard_jobs.find((std::shared_ptr<StandardJob> ) job) != this->pending_standard_jobs.end()) ||
                (this->running_standard_jobs.find((std::shared_ptr<StandardJob> ) job) != this->running_standard_jobs.end())) {
                std::string msg = "Job cannot be forgotten because it is pending or running";
                throw ExecutionException(std::shared_ptr<FailureCause>(new NotAllowed(job->getParentComputeService(), msg)));
            }
            if (this->completed_standard_jobs.find((std::shared_ptr<StandardJob> ) job) != this->completed_standard_jobs.end()) {
                this->completed_standard_jobs.erase((std::shared_ptr<StandardJob> ) job);
                this->jobs.erase(job);
                return;
            }
            if (this->failed_standard_jobs.find((std::shared_ptr<StandardJob> ) job) != this->failed_standard_jobs.end()) {
                this->failed_standard_jobs.erase((std::shared_ptr<StandardJob> ) job);
                this->jobs.erase(job);
                return;
            }
            // At this point, it's a job that was never submitted!
            if (this->jobs.find(job) != this->jobs.end()) {
                this->jobs.erase(job);
                return;
            }
            throw std::invalid_argument("JobManager::forgetJob(): unknown standard job");
        }

        if (job->getType() == Job::PILOT) {
            if ((this->pending_pilot_jobs.find((std::shared_ptr<PilotJob> ) job) != this->pending_pilot_jobs.end()) ||
                (this->running_pilot_jobs.find((std::shared_ptr<PilotJob> ) job) != this->running_pilot_jobs.end())) {
                std::string msg = "Job cannot be forgotten because it is running or pending";
                throw ExecutionException(std::shared_ptr<FailureCause>(new NotAllowed(job->getParentComputeService(), msg)));
            }
            if (this->completed_pilot_jobs.find((std::shared_ptr<PilotJob> ) job) != this->completed_pilot_jobs.end()) {
                this->jobs.erase(job);
                return;
            }
            // At this point, it's a job that was never submitted!
            if (this->jobs.find(job) != this->jobs.end()) {
                this->jobs.erase(job);
                return;
            }
            throw std::invalid_argument("JobManager::forgetJob(): unknown pilot job");
        }
    }
#endif

    /**
     * @brief Main method of the daemon that implements the JobManager
     * @return 0 on success
     */
    int JobManager::main() {
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_YELLOW);

        WRENCH_INFO("New Job Manager starting (%s)", this->mailbox_name.c_str());

        while (processNextMessage()) {
            dispatchJobs();
        }

        return 0;
    }

    /**
     * @brief Method to process an incoming message
     * @return
     */
    bool JobManager::processNextMessage() {
        std::unique_ptr<SimulationMessage> message = nullptr;
        try {
            message = S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<NetworkError> &cause) { WRENCH_INFO("Error while receiving message... ignoring");
            return true;
        }

        if (message == nullptr) { WRENCH_INFO("Got a NULL message... Likely this means we're all done. Aborting!");
            return false;
        }

        WRENCH_DEBUG("Job Manager got a %s message", message->getName().c_str());
        WRENCH_INFO("Job Manager got a %s message", message->getName().c_str());

        if (auto msg = dynamic_cast<JobManagerWakeupMessage *>(message.get())) {
            // Just wakeup
            return true;
        } else if (auto msg = dynamic_cast<ServiceStopDaemonMessage *>(message.get())) {
            // There shouldn't be any need to clean up any state
            return false;
//        } else if (auto msg = dynamic_cast<ComputeServiceStandardJobDoneMessage *>(message.get())) {
//            processStandardJobCompletion(msg->job, msg->compute_service);
//            return true;
//        } else if (auto msg = dynamic_cast<ComputeServiceStandardJobFailedMessage *>(message.get())) {
//            processStandardJobFailure(msg->job, msg->compute_service, msg->cause);
//            return true;
        } else if (auto msg = dynamic_cast<ComputeServiceCompoundJobDoneMessage *>(message.get())) {
            // Is this in fact a standard job???
            if (this->cjob_to_sjob_map.find(msg->job) != this->cjob_to_sjob_map.end()) {
                auto sjob = this->cjob_to_sjob_map[msg->job];
                this->cjob_to_sjob_map.erase(msg->job);
                processStandardJobCompletion(sjob, msg->compute_service);
            } else {
                processCompoundJobCompletion(msg->job, msg->compute_service);
            }
            return true;
        } else if (auto msg = dynamic_cast<ComputeServiceCompoundJobFailedMessage *>(message.get())) {
            if (this->cjob_to_sjob_map.find(msg->job) != this->cjob_to_sjob_map.end()) {
                auto sjob = this->cjob_to_sjob_map[msg->job];
                this->cjob_to_sjob_map.erase(msg->job);
                processStandardJobFailure(sjob, msg->compute_service);
            } else {
                processCompoundJobFailure(msg->job, msg->compute_service);
            }
            return true;
        } else if (auto msg = dynamic_cast<ComputeServicePilotJobStartedMessage *>(message.get())) {
            processPilotJobStart(msg->job, msg->compute_service);
            return true;
        } else if (auto msg = dynamic_cast<ComputeServicePilotJobExpiredMessage *>(message.get())) {
            processPilotJobExpiration(msg->job, msg->compute_service);
            return true;
        } else {
            throw std::runtime_error("JobManager::main(): Unexpected [" + message->getName() + "] message");
        }
    }

    /**
     * @brief Process a standard job completion
     * @param job: the job that completed
     * @param compute_service: the compute service on which the job was executed
     */
    void JobManager::processStandardJobCompletion(std::shared_ptr<StandardJob> job,
                                                  std::shared_ptr<ComputeService> compute_service) {

        // update job state
        job->state = StandardJob::State::COMPLETED;

        // Set the job end date
        job->end_date = Simulation::getCurrentSimulatedDate();

        // Set task information based on actions
        for (auto task : job->tasks) {

            double input_start_date = -1.0;
            double input_end_date = -1.0;
            double compute_start_date = -1.0;
            double compute_end_date = -1.0;
            double output_start_date = -1.0;
            double output_end_date = -1.0;

            // compute input start/end date
            if (not task->getInputFiles().empty()) {
                double input_start_date = DBL_MAX;
                double input_end_date = -1;
                for (auto const &file_read_action : job->task_file_read_actions[task]) {
                    input_start_date = (input_start_date > file_read_action->getStartDate()
                                        ? file_read_action->getStartDate() : input_start_date);
                    input_end_date = (input_end_date < file_read_action->getEndDate() ? file_read_action->getEndDate()
                                                                                      : input_end_date);
                }
            }

            compute_start_date = job->task_compute_actions[task]->getStartDate();
            compute_end_date = job->task_compute_actions[task]->getEndDate();

            if (not task->getOutputFiles().empty()) {
                // compute output start/end date
                double output_start_date = DBL_MAX;
                double output_end_date = -1;
                for (auto const &file_write_action : job->task_file_write_actions[task]) {
                    output_start_date = (output_start_date > file_write_action->getStartDate()
                                         ? file_write_action->getStartDate() : output_start_date);
                    output_end_date = (output_end_date < file_write_action->getEndDate()
                                       ? file_write_action->getEndDate() : output_end_date);
                }
            }

            if (input_start_date > 0) {
                task->setStartDate(input_start_date);
            } else {
                task->setStartDate(compute_start_date);
            }

            if (output_end_date > 0) {
                task->setEndDate(output_end_date);
            } else {
                task->setEndDate(compute_end_date);
            }

            task->setReadInputStartDate(input_start_date);
            task->setReadInputEndDate(input_end_date);
            task->setComputationStartDate(compute_start_date);
            task->setComputationEndDate(compute_end_date);
            task->setWriteOutputStartDate(output_start_date);
            task->setWriteOutputEndDate(output_end_date);

            this->simulation->getOutput().addTimestampTaskStart(task->getStartDate(), task);
            this->simulation->getOutput().addTimestampTaskCompletion(task->getEndDate(), task);
            // TODO: ADD OTHER STAMPS

            task->setExecutionHost(job->task_compute_actions[task]->getExecutionHistory().top().execution_host);
            task->setNumCoresAllocated(job->task_compute_actions[task]->getExecutionHistory().top().num_cores_allocated);

        }

        // Task state changes are: all completed (and deal with the children)
        std::map<WorkflowTask *, WorkflowTask::State> necessary_state_changes;
        for (auto task : job->tasks) {
            necessary_state_changes[task] = WorkflowTask::State::COMPLETED;
        }

        // remove the job from the "dispatched" list
        this->jobs_dispatched.erase(job);

        // Forward the notification along the notification chain

        std::string callback_mailbox = job->popCallbackMailbox();
        if (not callback_mailbox.empty()) {
            auto augmented_msg = new JobManagerStandardJobCompletedMessage(
                    job, compute_service, necessary_state_changes);
            S4U_Mailbox::dputMessage(callback_mailbox, augmented_msg);
        }
//        throw std::runtime_error("PROCESS STANDARD JOB COMPLETION NOT IMPLEMENTED");
    }

    /**
     * @brief Process a standard job failure
     * @param job: the job that failure
     * @param compute_service: the compute service on which the job has failed
     */
    void JobManager::processStandardJobFailure(std::shared_ptr<StandardJob> job,
                                               std::shared_ptr<ComputeService> compute_service) {
        // update job state
        job->state = StandardJob::State::FAILED;

        // Set the job end date
        job->end_date = Simulation::getCurrentSimulatedDate();

        // Determine all task state changes and failure count updates
        std::map<WorkflowTask *, WorkflowTask::State> necessary_state_changes;
        std::set<WorkflowTask *> necessary_failure_count_increments;

        std::shared_ptr<FailureCause> job_failure_cause = nullptr;

        for (auto t: job->getTasks()) {
            double failure_date = -1.0;
            double input_start_date = -1.0;
            double input_end_date = -1.0;
            double compute_start_date = -1.0;
            double compute_end_date = -1.0;
            double output_start_date = -1.0;
            double output_end_date = -1.0;

            // Look at file-read actions
            auto file_read_actions = job->task_file_read_actions[t];

            bool all_file_read_actions_completed = true;
            for (const auto &fra : file_read_actions) {
                // Set the dates
                if ((input_start_date == -1.0) or ((fra->getStartDate() < input_start_date) and (fra->getStartDate() != -1.0))) {
                    input_start_date = fra->getStartDate();
                }
                if ((input_end_date == -1.0) or ((fra->getEndDate() > input_end_date) and (fra->getEndDate() != -1.0))) {
                    input_end_date = fra->getEndDate();
                }
                if (fra->getState() != Action::State::COMPLETED) {
                    if (not job_failure_cause) {
                        job_failure_cause = fra->getFailureCause();
                    }
                    if ((failure_date == -1) or ((failure_date > fra->getEndDate()) and (fra->getEndDate() != -1.0))) {
                        failure_date = fra->getEndDate();
                    }
                    all_file_read_actions_completed = false;
                }
            }
            if (not all_file_read_actions_completed) {
                t->setStartDate(input_start_date);
                t->setReadInputStartDate(input_start_date);
                t->setFailureDate(failure_date);
                necessary_state_changes[t] = WorkflowTask::State::NOT_READY; // TODO: THIS COULD BE READY
                necessary_failure_count_increments.insert(t);

            } else {
                t->setStartDate(input_start_date);
                t->setReadInputStartDate(input_start_date);

                // Look at the compute action
                auto compute_action = job->task_compute_actions[t];
                t->setComputationStartDate(compute_action->getStartDate());  // could be -1.0
                t->setComputationEndDate(compute_action->getEndDate());      // could be -1.0

                t->setReadInputEndDate(input_end_date);

                if (compute_action->getState() != Action::State::COMPLETED) {
                    if (not job_failure_cause) job_failure_cause = compute_action->getFailureCause();
                    t->setFailureDate(compute_action->getEndDate());
                    necessary_state_changes[t] = WorkflowTask::State::NOT_READY; // TODO: THIS COULD BE READY
                } else {
                    auto file_write_actions = job->task_file_write_actions[t];

                    bool all_file_write_actions_completed = true;
                    for (const auto &fwa : file_write_actions) {
                        // Set the dates
                        if ((output_start_date == -1.0) or
                            ((fwa->getStartDate() < output_start_date) and (fwa->getStartDate() != -1.0))) {
                            output_start_date = fwa->getStartDate();
                        }
                        if ((output_end_date == -1.0) or
                            ((fwa->getEndDate() > output_end_date) and (fwa->getEndDate() != -1.0))) {
                            output_end_date = fwa->getEndDate();
                        }
                        if (fwa->getState() != Action::State::COMPLETED) {
                            if (not job_failure_cause) job_failure_cause = fwa->getFailureCause();
                            if ((failure_date == -1) or ((failure_date > fwa->getEndDate()) and (fwa->getEndDate() != -1.0))) {
                                failure_date = fwa->getEndDate();
                            }
                            all_file_write_actions_completed = false;
                        }
                    }
                    if (not all_file_write_actions_completed) {
                        t->setWriteOutputStartDate(output_start_date);
                        t->setFailureDate(failure_date);
                        necessary_state_changes[t] = WorkflowTask::State::NOT_READY; // TODO: THIS COULD BE READY
                        necessary_failure_count_increments.insert(t);
                    } else {
                        t->setWriteOutputStartDate(output_start_date);
                        t->setWriteOutputEndDate(output_end_date);
                        necessary_state_changes[t] = WorkflowTask::State::COMPLETED;
                    }
                }
            }
        }

        // remove the job from the "dispatched" list
        this->jobs_dispatched.erase(job);

        // Forward the notification along the notification chain
        auto augmented_message =
                new JobManagerStandardJobFailedMessage(job, compute_service,
                                                       necessary_state_changes,
                                                       necessary_failure_count_increments,
                                                       std::move(job_failure_cause));
        S4U_Mailbox::dputMessage(job->popCallbackMailbox(), augmented_message);
    }

    /**
     * @brief Process a pilot job starting
     * @param job: the pilot job that started
     * @param compute_service: the compute service on which it started
     */
    void JobManager::processPilotJobStart(
            std::shared_ptr<PilotJob> job,
            std::shared_ptr<ComputeService> compute_service) {
        // update job state
        job->state = PilotJob::State::RUNNING;
        this->num_running_pilot_jobs++;

        // Forward the notification to the source
        WRENCH_INFO("Forwarding to %s", job->getOriginCallbackMailbox().c_str());
        S4U_Mailbox::dputMessage(job->getOriginCallbackMailbox(),
                                 new ComputeServicePilotJobStartedMessage(job, compute_service, 0.0));
    }

    /**
     * @brief Process a pilot job expiring
     * @param job: the pilot job that expired
     * @param compute_service: the compute service on which it was running
     */
    void JobManager::processPilotJobExpiration(std::shared_ptr<PilotJob> job,
                                               std::shared_ptr<ComputeService> compute_service) {
        // update job state
        job->state = PilotJob::State::EXPIRED;
        this->num_running_pilot_jobs--;

        // Remove the job from the "dispatched" list and put it in the completed list
        this->jobs_dispatched.erase(job);

        // Forward the notification to the source
        WRENCH_INFO("Forwarding to %s", job->getOriginCallbackMailbox().c_str());
        S4U_Mailbox::dputMessage(job->getOriginCallbackMailbox(),
                                 new ComputeServicePilotJobExpiredMessage(job, compute_service, 0.0));
    }

    /**
     * @brief Create a Compound job
     * @param name: the job's name (if empty, a unique job name will be picked for you)
     * @return the job
     */
    std::shared_ptr<CompoundJob> JobManager::createCompoundJob(std::string name) {
        auto job = std::shared_ptr<CompoundJob>(new CompoundJob(name, this->getSharedPtr<JobManager>()));
        job->shared_this = job;
//        job->workflow = this->wms->getWorkflow();
        return job;
    }

    /**
     * @brief Helper method to dispatch jobs
     */
    void JobManager::dispatchJobs() {

        std::set<std::shared_ptr<Job>> dispatched;

        auto it = this->jobs_to_dispatch.begin();
        while (it != this->jobs_to_dispatch.end()) {
            auto job = *it;

            // Do pre-work
            if (auto pjob = std::dynamic_pointer_cast<PilotJob>(job)) {
                // Always ready
                pjob->state = PilotJob::State::PENDING;
            } else if (auto cjob = std::dynamic_pointer_cast<CompoundJob>(job)) {
                // Check if ready
                if (not cjob->isReady()) {
                    it++;
                    continue;
                }
            } else {
                throw std::runtime_error("UNSUPPORTED JOB");
            }

            try {
                this->dispatchJob(job);
                this->jobs_dispatched.insert(job);
                it = this->jobs_to_dispatch.erase(it);
            } catch (ExecutionException &e) {
                it = this->jobs_to_dispatch.erase(it);
                job->popCallbackMailbox();
                if (auto cjob = std::dynamic_pointer_cast<CompoundJob>(job)) {
                    cjob->setAllActionsFailed(e.getCause());
                    try {
                        auto message =
                                new JobManagerCompoundJobFailedMessage(cjob, cjob->parent_compute_service, e.getCause());
                        S4U_Mailbox::dputMessage(job->popCallbackMailbox(), message);
                    } catch (NetworkError &e) {
                    }
                } else if (auto pjob = std::dynamic_pointer_cast<PilotJob>(job)) {
                    try {
                        auto message =
                                new JobManagerPilotJobFailedMessage(pjob, pjob->parent_compute_service, e.getCause());

                        S4U_Mailbox::dputMessage(job->popCallbackMailbox(), message);
                    } catch (NetworkError &e) {
                    }
                }
            }


        }

    }


    /**
     * @brief Helper method to dispatch jobs
     */
    void JobManager::dispatchJob(std::shared_ptr<Job> job) {


        // Submit the job to the service
        try {
            job->submit_date = Simulation::getCurrentSimulatedDate();
            job->pushCallbackMailbox(this->mailbox_name);
            job->parent_compute_service->submitJob(job, job->service_specific_args);
            if (auto pjob = std::dynamic_pointer_cast<PilotJob>(job)) {
                pjob->state = PilotJob::PENDING;
            }
        } catch (ExecutionException &e) {
            job->end_date = Simulation::getCurrentSimulatedDate();
            // "Undo" everything
            if (auto cjob = std::dynamic_pointer_cast<CompoundJob>(job)) {
                cjob->state = CompoundJob::State::DISCONTINUED;
            } else if (auto pjob = std::dynamic_pointer_cast<PilotJob>(job)) {
                pjob->state = PilotJob::State::FAILED;
            }
            job->popCallbackMailbox();
            throw;

        } catch (std::invalid_argument &e) {
            throw;
        }


    }


    /**
     * @brief Method to process a compound job completion
     * @param job: the job that completed
     * @param compute_service: the compute service on which the job completed
     */
    void JobManager::processCompoundJobCompletion(std::shared_ptr<CompoundJob> job,
                                                  std::shared_ptr<ComputeService> compute_service) {
        job->state = CompoundJob::State::COMPLETED;
        // remove the job from the "dispatched" list
        this->jobs_dispatched.erase(job);

        // Forward the notification along the notification chain
        try {
            auto message =
                    new JobManagerCompoundJobCompletedMessage(job, compute_service);
            S4U_Mailbox::dputMessage(job->popCallbackMailbox(), message);
        } catch (NetworkError &e) {
        }
    }

    /**
     * @brief Method to process a compound job failure
     * @param job: the job that completed
     * @param compute_service: the compute service on which the job completed
     */
    void JobManager::processCompoundJobFailure(std::shared_ptr<CompoundJob> job,
                                               std::shared_ptr<ComputeService> compute_service) {
        job->state = CompoundJob::State::DISCONTINUED;
        // remove the job from the "dispatched" list
        this->jobs_dispatched.erase(job);

        // Forward the notification along the notification chain
        try {
            auto message =
                    new JobManagerCompoundJobFailedMessage(job, compute_service,
                                                           std::make_shared<SomeActionsHaveFailed>());
            S4U_Mailbox::dputMessage(job->popCallbackMailbox(), message);
        } catch (NetworkError &e) {
        }
    }

    /**
     * @brief Get the WMS associated to this DataManager
     * @return a WMS
     */
    std::shared_ptr<WMS> JobManager::getWMS() {
        return this->wms;
    }

}
