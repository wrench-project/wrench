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
#include <utility>

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
#include <wrench/services/helper_services/action_executor/ActionExecutor.h>
#include <wrench/services/helper_services/action_execution_service/ActionExecutionService.h>
#include "wrench/managers/JobManagerMessage.h"

WRENCH_LOG_CATEGORY(wrench_core_job_manager, "Log category for Job Manager");

namespace wrench {
    /**
     * @brief Constructor
     *
     * @param hostname: the name of host on which the job manager will run
     * @param creator_mailbox: the mailbox of the manager's creator
     */
    JobManager::JobManager(std::string hostname, simgrid::s4u::Mailbox *creator_mailbox) : Service(std::move(hostname), "job_manager") {
        this->creator_mailbox = creator_mailbox;
    }

    /**
     * @brief Destructor, which kills the daemon (and clears all the jobs)
     */
    JobManager::~JobManager() {
        this->jobs_to_dispatch.clear();
        this->jobs_dispatched.clear();
    }

    /**
     * @brief Kill the job manager (brutally terminate the daemon, clears all jobs)
     */
    void JobManager::kill() {
        this->killActor();
        this->jobs_to_dispatch.clear();
        this->jobs_dispatched.clear();
    }

    /**
     * @brief Stop the job manager
     *
     * @throw ExecutionException
     * @throw std::runtime_error
     */
    void JobManager::stop() {
        try {
            S4U_Mailbox::putMessage(this->mailbox,
                                    new ServiceStopDaemonMessage(nullptr, false, ComputeService::TerminationCause::TERMINATION_NONE, 0.0));
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
            const std::vector<std::shared_ptr<WorkflowTask>> &tasks,
            const std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> &file_locations,
            std::vector<std::tuple<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>>> pre_file_copies,
            std::vector<std::tuple<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>>> post_file_copies,
            std::vector<std::tuple<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>>> cleanup_file_deletions) {
        // Transform the non-vector file location map into a vector file location map
        std::map<std::shared_ptr<DataFile>, std::vector<std::shared_ptr<FileLocation>>> file_locations_vector;
        for (auto const &e: file_locations) {
            std::vector<std::shared_ptr<FileLocation>> v;
            v.push_back(e.second);
            file_locations_vector[e.first] = v;
        }

        // Call the vectorized method
        return createStandardJob(tasks,
                                 file_locations_vector,
                                 std::move(pre_file_copies),
                                 std::move(post_file_copies),
                                 std::move(cleanup_file_deletions));
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
            const std::vector<std::shared_ptr<WorkflowTask>> &tasks,
            std::map<std::shared_ptr<DataFile>, std::vector<std::shared_ptr<FileLocation>>> file_locations,
            std::vector<std::tuple<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>>> pre_file_copies,
            std::vector<std::tuple<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>>> post_file_copies,
            std::vector<std::tuple<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>>> cleanup_file_deletions) {
        // Do a sanity check of everything (looking for nullptr)
        for (const auto &t: tasks) {
            if (t == nullptr) {
                throw std::invalid_argument("JobManager::createStandardJob(): nullptr task in the task vector");
            }
        }

        for (auto const &fl: file_locations) {
            if (fl.first == nullptr) {
                throw std::invalid_argument(
                        "JobManager::createStandardJob(): nullptr workflow file in the file_locations map");
            }
            if (fl.second.empty()) {
                throw std::invalid_argument(
                        "JobManager::createStandardJob(): empty location vector in the file_locations map");
            }
            for (auto const &fl_l: fl.second) {
                if (fl_l == nullptr) {
                    throw std::invalid_argument(
                            "JobManager::createStandardJob(): nullptr file location in the file_locations map");
                }
            }
        }

        for (auto fc: pre_file_copies) {
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

        for (auto fc: post_file_copies) {
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

        for (auto fd: cleanup_file_deletions) {
            if (std::get<0>(fd) == nullptr) {
                throw std::invalid_argument(
                        "JobManager::createStandardJob(): nullptr workflow file in the cleanup_file_deletions set");
            }
            if (std::get<1>(fd) == nullptr) {
                throw std::invalid_argument(
                        "JobManager::createStandardJob(): nullptr storage service in the cleanup_file_deletions set");
            }
        }


        auto job = std::shared_ptr<StandardJob>(
                new StandardJob(this->getSharedPtr<JobManager>(), tasks, file_locations, pre_file_copies,
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
            const std::vector<std::shared_ptr<WorkflowTask>> &tasks,
            const std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> &file_locations) {
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
            const std::vector<std::shared_ptr<WorkflowTask>> &tasks,
            std::map<std::shared_ptr<DataFile>, std::vector<std::shared_ptr<FileLocation>>> file_locations) {
        if (tasks.empty()) {
            throw std::invalid_argument("JobManager::createStandardJob(): Invalid arguments (empty tasks argument!)");
        }

        return this->createStandardJob(tasks, std::move(file_locations), {}, {}, {});
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
            const std::vector<std::shared_ptr<WorkflowTask>> &tasks) {
        if (tasks.empty()) {
            throw std::invalid_argument("JobManager::createStandardJob(): Invalid arguments (empty tasks argument!)");
        }

        return this->createStandardJob(tasks, (std::map<std::shared_ptr<DataFile>, std::vector<std::shared_ptr<FileLocation>>>){},
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
            const std::shared_ptr<WorkflowTask> &task,
            const std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> &file_locations) {
        if (task == nullptr) {
            throw std::invalid_argument("JobManager::createStandardJob(): Invalid arguments");
        }

        std::vector<std::shared_ptr<WorkflowTask>> tasks;
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
            const std::shared_ptr<WorkflowTask> &task,
            std::map<std::shared_ptr<DataFile>, std::vector<std::shared_ptr<FileLocation>>> file_locations) {
        if (task == nullptr) {
            throw std::invalid_argument("JobManager::createStandardJob(): Invalid arguments");
        }

        std::vector<std::shared_ptr<WorkflowTask>> tasks;
        tasks.push_back(task);
        return this->createStandardJob(tasks, std::move(file_locations));
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
            const std::shared_ptr<WorkflowTask> &task) {
        if (task == nullptr) {
            throw std::invalid_argument("JobManager::createStandardJob(): Invalid arguments");
        }

        std::vector<std::shared_ptr<WorkflowTask>> tasks;
        tasks.push_back(task);
        return this->createStandardJob(tasks, std::map<std::shared_ptr<DataFile>, std::vector<std::shared_ptr<FileLocation>>>{});
    }

    /**
     * @brief Create a pilot job
     *
     * @return the pilot job
     *
     * @throw std::invalid_argument
     */
    std::shared_ptr<PilotJob> JobManager::createPilotJob() {
        auto job = std::shared_ptr<PilotJob>(new PilotJob(this->getSharedPtr<JobManager>()));
        return job;
    }

    /**
    * @brief Submit a standard job to a compute service
    *
    * @param job: a standard job
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
    *      - to a BatchComputeService: {{"-t":"<int>" (requested number of minutes)},{"-N":"<int>" (number of requested hosts)},{"-c":"<int>" (number of requested cores per host)}[,{"taskID":"[node_index:]num_cores"}] [,{"-u":"<string>" (username)}]}
    *      - to a VirtualizedClusterComputeService: {} (jobs should not be submitted directly to the service)}
    *      - to a CloudComputeService: {} (jobs should not be submitted directly to the service)}
    *      - to a HTCondorComputeService:
    *           - For a "grid universe" job that will be submitted to a child BatchComputeService: {{"-universe":"grid", {"-t":"<int>" (requested number of minutes)},{"-N":"<int>" (number of requested hosts)},{"-c":"<int>" (number of requested cores per host)}[,{"-service":"<string>" (BatchComputeService service name)}] [, {"taskID":"[node_index:]num_cores"}] [, {"-u":"<string>" (username)}]}
    *           - For a "non-grid universe" job that will be submitted to a child BareMetalComputeService: {}
    *
    *
    * @throw std::invalid_argument
    * @throw ExecutionException
    */
    void JobManager::submitJob(const std::shared_ptr<StandardJob> &job,
                               const std::shared_ptr<ComputeService> &compute_service,
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

        if (not compute_service->supportsStandardJobs()) {
            throw std::invalid_argument("JobManager::submitJob(): service does not support standard jobs");
        }


        // Do a sanity check on task states
        for (const auto &t: job->tasks) {
            if ((t->getState() == WorkflowTask::State::COMPLETED) or
                (t->getState() == WorkflowTask::State::PENDING)) {
                throw std::invalid_argument("JobManager()::submitJob(): task " + t->getID() +
                                            " cannot be submitted as part of a standard job because its state is " +
                                            WorkflowTask::stateToString(t->getState()));
            }
        }


        // If the job uses scratch, then do a sanity check on the CS
        if (job->usesScratch()) {
            try {
                compute_service->validateJobsUseOfScratch(service_specific_args);
            } catch (std::invalid_argument &e) {
                throw std::invalid_argument("JobManager()::submitJob(): Job's use of scratch is invalid: " + std::string(e.what()));
            }
        }

        // Create the underlying compound job
        job->createUnderlyingCompoundJob(compute_service);

        // Tweak the service_specific_arguments
        std::map<std::string, std::string> new_args;

        std::shared_ptr<Workflow> workflow = nullptr;
        if (not job->getTasks().empty()) {
            workflow = (*(job->getTasks().begin()))->getWorkflow();
        }

        for (const auto &arg: service_specific_args) {
            // Any key that doesn't start with a "-" is a task ID
            if (arg.first.rfind("-", 0) == 0) {
                new_args[arg.first] = arg.second;
            } else {
                std::shared_ptr<WorkflowTask> task;
                if (workflow == nullptr) {
                    throw std::invalid_argument("JobManager::submitJob():  invalid service-specific argument {" + arg.first + "," + arg.second + "} (unknown task ID " + arg.first + ")");
                }
                try {
                    task = workflow->getTaskByID(arg.first);
                } catch (std::invalid_argument &e) {
                    throw;
                }
                new_args[job->task_compute_actions[task]->getName()] = arg.second;
            }
        }

        try {
            compute_service->validateServiceSpecificArguments(job->compound_job, new_args);
        } catch (ExecutionException &e) {
            job->compound_job = nullptr;
            if (std::dynamic_pointer_cast<NotEnoughResources>(e.getCause())) {
                throw ExecutionException(std::shared_ptr<NotEnoughResources>(new NotEnoughResources(job, compute_service)));
            } else {
                throw;
            }
        } catch (std::invalid_argument &e) {
            throw;
        }

        // Modify task states
        job->state = StandardJob::PENDING;
        for (auto const &t: job->tasks) {
            t->setState(WorkflowTask::State::PENDING);
        }

        // The compound job
        this->cjob_to_sjob_map[job->compound_job] = job;
        job->compound_job->state = CompoundJob::State::SUBMITTED;
        this->acquireDaemonLock();
        this->jobs_to_dispatch.push_back(job->compound_job);
        this->releaseDaemonLock();


        job->already_submitted_to_job_manager = true;
        job->submit_date = Simulation::getCurrentSimulatedDate();
        job->compound_job->setServiceSpecificArguments(new_args);
        job->setParentComputeService(compute_service);
        job->compound_job->setParentComputeService(compute_service);

        // Send a message to wake up the daemon
        try {
            S4U_Mailbox::putMessage(this->mailbox, new JobManagerWakeupMessage());
        } catch (std::exception &e) {
            throw std::runtime_error("Cannot connect to job manager");
        }
    }


    /**
     * @brief Submit a compound job to a compute service
     *
     * @param job: a compound job
     * @param compute_service: a compute service
     * @param service_specific_args: arguments specific for compute services:
     *      - to a BareMetalComputeService: {{"actionID", "[hostname:][num_cores]}, ...}
     *           - If no value is provided for a task, then the service will choose a host and use as many cores as possible on that host.
     *           - If a "" value is provided for a task, then the service will choose a host and use as many cores as possible on that host.
     *           - If a "hostname" value is provided for a task, then the service will run the task on that
     *             host, using as many of its cores as possible
     *           - If a "num_cores" value is provided for a task, then the service will run that task with
     *             this many cores, but will choose the host on which to run it.
     *           - If a "hostname:num_cores" value is provided for a task, then the service will run that
     *             task with the specified number of cores on that host.
     *      - to a BatchComputeService: {{"-t":"<int>" (requested number of minutes)},{"-N":"<int>" (number of requested hosts)},{"-c":"<int>" (number of requested cores per host)}[,{"actionID":"[node_index:]num_cores"}] [,{"-u":"<string>" (username)}]}
     *      - to a VirtualizedClusterComputeService: {} (jobs should not be submitted directly to the service)}
     *      - to a CloudComputeService: {} (jobs should not be submitted directly to the service)}
     *      - to a HTCondorComputeService:
     *           - For a "grid universe" job that will be submitted to a child BatchComputeService: {{"-universe":"grid", {"-t":"<int>" (requested number of minutes)},{"-N":"<int>" (number of requested hosts)},{"-c":"<int>" (number of requested cores per host)}[,{"-service":"<string>" (BatchComputeService service name)}] [, {"actionID":"[node_index:]num_cores"}] [, {"-u":"<string>" (username)}]}
     *           - For a "non-grid universe" job that will be submitted to a child BareMetalComputeService: {}
     *
     *
     * @throw std::invalid_argument
     * @throw ExecutionException
     */
    void JobManager::submitJob(const std::shared_ptr<CompoundJob> &job,
                               const std::shared_ptr<ComputeService> &compute_service,
                               std::map<std::string, std::string> service_specific_args) {
        if ((job == nullptr) || (compute_service == nullptr)) {
            throw std::invalid_argument("JobManager::submitJob(): Invalid arguments");
        }

        if (job->already_submitted_to_job_manager) {
            throw std::invalid_argument("JobManager::submitJob(): Job was previously submitted");
        }

        if (job->actions.empty()) {
            throw std::invalid_argument("JobManager::submitJob(): Cannot submit a job that has any actions");
        }

        try {
            compute_service->assertServiceIsUp();
        } catch (ExecutionException &e) {
            throw;
        }

        if (not compute_service->supportsCompoundJobs()) {
            throw std::invalid_argument("JobManager::submitJob(): service does not support compound jobs");
        }

        try {
            compute_service->validateServiceSpecificArguments(job, service_specific_args);
        } catch (ExecutionException &e) {
            if (std::dynamic_pointer_cast<NotEnoughResources>(e.getCause())) {
                throw ExecutionException(std::shared_ptr<NotEnoughResources>(new NotEnoughResources(job, compute_service)));
            } else {
                throw;
            }
        } catch (std::invalid_argument &e) {
            throw;
        }

        // If the job uses scratch, then do a sanity check on the CS
        if (job->usesScratch()) {
            try {
                compute_service->validateJobsUseOfScratch(service_specific_args);
            } catch (std::invalid_argument &e) {
                throw std::invalid_argument("JobManager()::submitJob(): Job's use of scratch is invalid: " + std::string(e.what()));
            }
        }

        job->state = CompoundJob::State::SUBMITTED;
        this->acquireDaemonLock();
        this->jobs_to_dispatch.push_back(job);
        this->releaseDaemonLock();

        job->already_submitted_to_job_manager = true;
        job->submit_date = Simulation::getCurrentSimulatedDate();
        job->setServiceSpecificArguments(service_specific_args);
        job->setParentComputeService(compute_service);

        // Send a message to wake up the daemon
        try {
            S4U_Mailbox::putMessage(this->mailbox, new JobManagerWakeupMessage());
        } catch (std::exception &e) {
            throw std::runtime_error("Cannot connect to job manager");
        }
    }


    /**
     * @brief Submit a pilot job to a compute service
     *
     * @param job: a pilot job
     * @param compute_service: a compute service
     * @param service_specific_args: arguments specific for compute services:
     *      - to a BatchComputeService: {"-t":"<int>" (requested number of minutes)},{"-N":"<int>" (number of requested hosts)},{"-c":"<int>" (number of requested cores per host)}
     *      - to a BareMetalComputeService: {} (pilot jobs should not be submitted directly to the service)}
     *      - to a VirtualizedClusterComputeService: {} (pilot jobs should not be submitted directly to the service)}
     *      - to a CloudComputeService: {} (pilot jobs should not be submitted directly to the service)}
     *      - to a HTCondorComputeService: {} (pilot jobs should be be submitted directly to the service)
     *
     * @throw std::invalid_argument
     * @throw ExecutionException
     */
    void JobManager::submitJob(const std::shared_ptr<PilotJob> &job,
                               const std::shared_ptr<ComputeService> &compute_service,
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

        if (not compute_service->supportsPilotJobs()) {
            throw std::invalid_argument("JobManager::submitJob(): service does not support pilot jobs");
        }


        auto callback_mailbox = this->mailbox;
        std::shared_ptr<CompoundJob> cjob = this->createCompoundJob("cjob_for_" + this->getName());
        cjob->addCustomAction(
                "pilot_job_",
                0, 0,
                [callback_mailbox, job, compute_service](const std::shared_ptr<ActionExecutor>& executor) {
                    // Create a bare-metal compute service and start it
                    auto execution_service = executor->getActionExecutionService();

                    // TODO: Deal with Properties!
                    auto bm_cs = std::shared_ptr<BareMetalComputeService>(
                            new BareMetalComputeService(
                                    executor->hostname,
                                    execution_service->getComputeResources(),
                                    {},
                                    {},
                                    DBL_MAX,
                                    nullptr,
                                    "_one_shot_bm",
                                    std::dynamic_pointer_cast<ComputeService>(execution_service->getParentService())->getScratch()));

                    bm_cs->simulation = executor->getSimulation();
                    bm_cs->start(bm_cs, true, false);// Daemonized, no auto-restart
                    job->compute_service = bm_cs;

                    // Send a call back
                    S4U_Mailbox::dputMessage(
                            callback_mailbox,
                            new ComputeServicePilotJobStartedMessage(
                                    job, compute_service,
                                    compute_service->getMessagePayloadValue(
                                            BatchComputeServiceMessagePayload::PILOT_JOB_STARTED_MESSAGE_PAYLOAD)));

                    // Sleep FOREVER (will be killed by service above)
                    Simulation::sleep(DBL_MAX);
                },
                [job](const std::shared_ptr<ActionExecutor>& executor) {
                    job->compute_service->stop(true, ComputeService::TerminationCause::TERMINATION_JOB_TIMEOUT);
                });

        job->compound_job = cjob;

        try {
            compute_service->validateServiceSpecificArguments(job->compound_job, service_specific_args);
        } catch (ExecutionException &e) {
            job->compound_job = nullptr;
            if (std::dynamic_pointer_cast<NotEnoughResources>(e.getCause())) {
                throw ExecutionException(std::shared_ptr<NotEnoughResources>(new NotEnoughResources(job, compute_service)));
            } else {
                throw;
            }
        } catch (std::invalid_argument &e) {
            throw;
        }

        this->cjob_to_pjob_map[job->compound_job] = job;

        this->acquireDaemonLock();
        this->jobs_to_dispatch.push_back(job->compound_job);
        this->releaseDaemonLock();


        try {
            compute_service->validateServiceSpecificArguments(job->compound_job, service_specific_args);
        } catch (ExecutionException &e) {
            if (std::dynamic_pointer_cast<NotEnoughResources>(e.getCause())) {
                throw ExecutionException(std::shared_ptr<NotEnoughResources>(new NotEnoughResources(job, compute_service)));
            } else {
                throw;
            }
        } catch (std::invalid_argument &e) {
            throw;
        }

        job->already_submitted_to_job_manager = true;
        job->submit_date = Simulation::getCurrentSimulatedDate();
        job->compound_job->setServiceSpecificArguments(service_specific_args);
        job->compound_job->setParentComputeService(compute_service);
        job->setParentComputeService(compute_service);

        // Send a message to wake up the daemon
        try {
            S4U_Mailbox::putMessage(this->mailbox, new JobManagerWakeupMessage());
        } catch (std::exception &e) {
            throw std::runtime_error("Cannot connect to job manager");
        }
    }

    /**
     * @brief Terminate a standard job  that hasn't completed/expired/failed yet
     * @param job: the job to be terminated
     *
     * @throw ExecutionException
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    void JobManager::terminateJob(const std::shared_ptr<StandardJob> &job) {
        if (job == nullptr) {
            throw std::invalid_argument("JobManager::terminateJob(): invalid argument");
        }

        if (job->getParentComputeService() == nullptr) {
            std::string err_msg = "Job cannot be terminated because it doesn't have a parent compute service";
            throw ExecutionException(std::shared_ptr<FailureCause>(new NotAllowed(nullptr, err_msg)));
        }

        switch (job->state) {
            case StandardJob::State::COMPLETED:
            case StandardJob::State::FAILED:
            case StandardJob::State::NOT_SUBMITTED: {
                std::string err_msg = "job cannot be terminated because it's not pending/running";
                throw ExecutionException(std::shared_ptr<FailureCause>(new NotAllowed(nullptr, err_msg)));
            }
            default:
                break;
        }

        // If the job has not been dispatch, just remove it from the to-dispatch list
        this->acquireDaemonLock();
        auto it = std::find(this->jobs_to_dispatch.begin(), this->jobs_to_dispatch.end(), job->compound_job);
        if (it != this->jobs_to_dispatch.end()) {
            this->cjob_to_sjob_map.erase(*it);
            this->jobs_to_dispatch.erase(it);
            this->releaseDaemonLock();
            return;
        }
        this->releaseDaemonLock();

        try {
            job->getParentComputeService()->terminateJob(job->compound_job);
        } catch (std::exception &e) {
            throw;
        }

        job->compound_job->state = CompoundJob::State::DISCONTINUED;
        job->state = StandardJob::State::TERMINATED;

        // Update task states based on compound job
        std::map<std::shared_ptr<WorkflowTask>, WorkflowTask::State> state_changes;
        std::set<std::shared_ptr<WorkflowTask>> failure_count_increments;
        std::shared_ptr<FailureCause> job_failure_cause;
        job->processCompoundJobOutcome(state_changes, failure_count_increments, job_failure_cause, this->simulation);
        job->applyTaskUpdates(state_changes, failure_count_increments);
    }


    /**
    * @brief Terminate a compound job  that hasn't completed/expired/failed yet
    * @param job: the job to be terminated
    *
    * @throw ExecutionException
    * @throw std::invalid_argument
    * @throw std::runtime_error
    */
    void JobManager::terminateJob(const std::shared_ptr<CompoundJob> &job) {
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
        job->state = CompoundJob::State::DISCONTINUED;
    }


    /**
    * @brief Terminate a pilot jobthat hasn't completed/expired/failed yet
    * @param job: the job to be terminated
    *
    * @throw ExecutionException
    * @throw std::invalid_argument
    * @throw std::runtime_error
    */
    void JobManager::terminateJob(const std::shared_ptr<PilotJob> &job) {
        if (job == nullptr) {
            throw std::invalid_argument("JobManager::terminateJob(): invalid argument");
        }

        if (job->getParentComputeService() == nullptr) {
            std::string err_msg = "Job cannot be terminated because it doesn't have a parent compute service";
            throw ExecutionException(std::shared_ptr<FailureCause>(new NotAllowed(nullptr, err_msg)));
        }

        try {
            job->getParentComputeService()->terminateJob(job->compound_job);
        } catch (std::exception &e) {
            throw;
        }
        job->state = PilotJob::State::TERMINATED;
    }

    /**
     * @brief Get the list of currently running pilot jobs
     * @return a set of pilot jobs
     */
    unsigned long JobManager::getNumRunningPilotJobs() const {
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

        WRENCH_INFO("New Job Manager starting (%s)", this->mailbox->get_cname());

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
            message = S4U_Mailbox::getMessage(this->mailbox);
        } catch (std::shared_ptr<NetworkError> &cause) {
            WRENCH_INFO("Error while receiving message... ignoring");
            return true;
        }

        if (message == nullptr) {
            WRENCH_INFO("Got a NULL message... Likely this means we're all done. Aborting!");
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
            } else if (this->cjob_to_pjob_map.find(msg->job) != this->cjob_to_pjob_map.end()) {
                auto pjob = this->cjob_to_pjob_map[msg->job];
                auto pjob_action = *(msg->job->getActions().begin());
                if (std::dynamic_pointer_cast<JobTimeout>(pjob_action->getFailureCause())) {
                    processPilotJobExpiration(pjob, msg->compute_service);
                } else {
                    processPilotJobFailure(pjob, msg->compute_service, pjob_action->getFailureCause());
                }
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
    void JobManager::processStandardJobCompletion(const std::shared_ptr<StandardJob> &job,
                                                  std::shared_ptr<ComputeService> compute_service) {
        // update job state
        job->state = StandardJob::State::COMPLETED;

        // Set the job end date
        job->end_date = Simulation::getCurrentSimulatedDate();

        // Analyze compound job
        std::map<std::shared_ptr<WorkflowTask>, WorkflowTask::State> state_changes;
        std::set<std::shared_ptr<WorkflowTask>> failure_count_increments;
        std::shared_ptr<FailureCause> job_failure_cause;
        job->processCompoundJobOutcome(state_changes, failure_count_increments, job_failure_cause, this->simulation);

        // remove the job from the "dispatched" list
        this->jobs_dispatched.erase(job->compound_job);

        // Forward the notification along the notification chain

        auto callback_mailbox = job->popCallbackMailbox();
        if (callback_mailbox) {
            auto augmented_msg = new JobManagerStandardJobCompletedMessage(
                    job, std::move(compute_service), state_changes);
            S4U_Mailbox::dputMessage(callback_mailbox, augmented_msg);
        }
        //        throw std::runtime_error("PROCESS STANDARD JOB COMPLETION NOT IMPLEMENTED");
    }

    /**
     * @brief Process a standard job failure
     * @param job: the job that failure
     * @param compute_service: the compute service on which the job has failed
     */
    void JobManager::processStandardJobFailure(const std::shared_ptr<StandardJob>& job,
                                               std::shared_ptr<ComputeService> compute_service) {
        // update job state
        job->state = StandardJob::State::FAILED;

        // Set the job end date
        job->end_date = Simulation::getCurrentSimulatedDate();

        // Analyze compound job
        std::map<std::shared_ptr<WorkflowTask>, WorkflowTask::State> state_changes;
        std::set<std::shared_ptr<WorkflowTask>> failure_count_increments;
        std::shared_ptr<FailureCause> job_failure_cause;
        job->processCompoundJobOutcome(state_changes, failure_count_increments, job_failure_cause, this->simulation);

        // Fix the failure cause in case it's a failure cause that refers to a job (in which case it is
        // right now referring to the compound job instead of the standard job
        if (auto actual_cause = std::dynamic_pointer_cast<JobKilled>(job_failure_cause)) {
            job_failure_cause = std::make_shared<JobKilled>(job);
        } else if (auto actual_cause = std::dynamic_pointer_cast<JobTimeout>(job_failure_cause)) {
            job_failure_cause = std::make_shared<JobTimeout>(job);
        } else if (auto actual_cause = std::dynamic_pointer_cast<NotEnoughResources>(job_failure_cause)) {
            job_failure_cause = std::make_shared<NotEnoughResources>(job, actual_cause->getService());
        }

        // remove the job from the "dispatched" list
        this->jobs_dispatched.erase(job->compound_job);

        // Forward the notification along the notification chain
        auto augmented_message =
                new JobManagerStandardJobFailedMessage(job, std::move(compute_service),
                                                       state_changes,
                                                       failure_count_increments,
                                                       std::move(job_failure_cause));
        S4U_Mailbox::dputMessage(job->popCallbackMailbox(), augmented_message);
    }

    /**
     * @brief Process a pilot job starting
     * @param job: the pilot job that started
     * @param compute_service: the compute service on which it started
     */
    void JobManager::processPilotJobStart(
            const std::shared_ptr<PilotJob> &job,
            std::shared_ptr<ComputeService> compute_service) {
        // update job state
        job->state = PilotJob::State::RUNNING;
        this->num_running_pilot_jobs++;

        // Forward the notification to the source
        WRENCH_INFO("Forwarding to %s", job->getOriginCallbackMailbox()->get_cname());
        S4U_Mailbox::dputMessage(job->getOriginCallbackMailbox(),
                                 new ComputeServicePilotJobStartedMessage(job, std::move(compute_service), 0.0));
    }

    /**
     * @brief Process a pilot job expiring
     * @param job: the pilot job that expired
     * @param compute_service: the compute service on which it was running
     */
    void JobManager::processPilotJobExpiration(const std::shared_ptr<PilotJob> &job,
                                               std::shared_ptr<ComputeService> compute_service) {
        // update job state
        job->state = PilotJob::State::EXPIRED;
        this->num_running_pilot_jobs--;

        // Remove the job from the "dispatched" list and put it in the completed list
        this->jobs_dispatched.erase(job->compound_job);

        // Forward the notification to the source
        WRENCH_INFO("Forwarding to %s", job->getOriginCallbackMailbox()->get_cname());
        S4U_Mailbox::dputMessage(job->getOriginCallbackMailbox(),
                                 new ComputeServicePilotJobExpiredMessage(job, std::move(compute_service), 0.0));
    }

    /**
  * @brief Process a pilot job failing (whatever that means)
  * @param job: the pilot job that failed
  * @param compute_service: the compute service on which it was running
  * @param cause: the failure cause
  */
    void JobManager::processPilotJobFailure(const std::shared_ptr<PilotJob> &job,
                                            std::shared_ptr<ComputeService> compute_service,
                                            std::shared_ptr<FailureCause> cause) {
        // update job state
        job->state = PilotJob::State::FAILED;
        this->num_running_pilot_jobs--;

        // Remove the job from the "dispatched" list and put it in the completed list
        this->jobs_dispatched.erase(job->compound_job);

        // Forward the notification to the source
        WRENCH_INFO("Forwarding to %s", job->getOriginCallbackMailbox()->get_cname());
        S4U_Mailbox::dputMessage(job->getOriginCallbackMailbox(),
                                 new ComputeServicePilotJobFailedMessage(job, std::move(compute_service), std::move(cause), 0.0));
    }

    /**
     * @brief Create a Compound job
     * @param name: the job's name (if empty, a unique job name will be picked for you)
     * @return the job
     */
    std::shared_ptr<CompoundJob> JobManager::createCompoundJob(std::string name) {
        auto job = std::shared_ptr<CompoundJob>(new CompoundJob(std::move(name), this->getSharedPtr<JobManager>()));
        return job;
    }

    /**
     * @brief Helper method to dispatch jobs
     */
    void JobManager::dispatchJobs() {
        std::set<std::shared_ptr<Job>> dispatched;

        this->acquireDaemonLock();
        auto it = this->jobs_to_dispatch.begin();
        while (it != this->jobs_to_dispatch.end()) {
            auto job = *it;

            if (not job->isReady()) {
                it++;
                continue;
            }

            try {
                this->dispatchJob(job);
                this->jobs_dispatched.insert(job);
                it = this->jobs_to_dispatch.erase(it);
            } catch (ExecutionException &e) {
                it = this->jobs_to_dispatch.erase(it);
                //                job->popCallbackMailbox();
                if (auto cjob = std::dynamic_pointer_cast<CompoundJob>(job)) {

                    if (this->cjob_to_sjob_map.find(cjob) == this->cjob_to_sjob_map.end()) {
                        cjob->setAllActionsFailed(e.getCause());
                        try {
                            auto message =
                                    new JobManagerCompoundJobFailedMessage(cjob, cjob->parent_compute_service,
                                                                           e.getCause());
                            S4U_Mailbox::dputMessage(job->popCallbackMailbox(), message);
                        } catch (NetworkError &e) {
                        }
                    } else {
                        auto sjob = this->cjob_to_sjob_map[cjob];
                        std::map<std::shared_ptr<WorkflowTask>, WorkflowTask::State> state_changes;
                        std::set<std::shared_ptr<WorkflowTask>> failure_count_increments;
                        // Set all tasks to not-ready (will be fixed later)
                        for (auto const &t: sjob->getTasks()) {
                            state_changes[t] = WorkflowTask::State::NOT_READY;
                        }

                        this->cjob_to_sjob_map.erase(cjob);
                        try {
                            auto message =
                                    new JobManagerStandardJobFailedMessage(sjob, sjob->parent_compute_service,
                                                                           state_changes, failure_count_increments,
                                                                           e.getCause());
                            S4U_Mailbox::dputMessage(job->popCallbackMailbox(), message);
                        } catch (NetworkError &e) {
                        }
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
        this->releaseDaemonLock();
    }


    /**
     * @brief Helper method to dispatch jobs
     */
    void JobManager::dispatchJob(const std::shared_ptr<CompoundJob> &job) {

        // Submit the job to the service
        try {
            job->submit_date = Simulation::getCurrentSimulatedDate();
            job->pushCallbackMailbox(this->mailbox);
            job->parent_compute_service->submitJob(job, job->getServiceSpecificArguments());
            if (this->cjob_to_pjob_map.find(job) != this->cjob_to_pjob_map.end()) {
                this->cjob_to_pjob_map[job]->state = PilotJob::State::PENDING;
            } else if (this->cjob_to_sjob_map.find(job) != this->cjob_to_sjob_map.end()) {
                this->cjob_to_sjob_map[job]->state = StandardJob::State::PENDING;
            } else {
                job->state = CompoundJob::State::SUBMITTED;// useless likely
            }
        } catch (ExecutionException &e) {
            job->end_date = Simulation::getCurrentSimulatedDate();
            // "Undo" everything
            if (this->cjob_to_pjob_map.find(job) != this->cjob_to_pjob_map.end()) {
                this->cjob_to_pjob_map[job]->state = PilotJob::State::FAILED;
            } else if (this->cjob_to_sjob_map.find(job) != this->cjob_to_sjob_map.end()) {
                this->cjob_to_sjob_map[job]->state = StandardJob::State::FAILED;
            } else {
                job->state = CompoundJob::State::DISCONTINUED;
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
    void JobManager::processCompoundJobCompletion(const std::shared_ptr<CompoundJob> &job,
                                                  std::shared_ptr<ComputeService> compute_service) {
        job->state = CompoundJob::State::COMPLETED;
        job->end_date = Simulation::getCurrentSimulatedDate();
        // remove the job from the "dispatched" list
        this->jobs_dispatched.erase(job);

        // Forward the notification along the notification chain
        try {
            auto message =
                    new JobManagerCompoundJobCompletedMessage(job, std::move(compute_service));
            S4U_Mailbox::dputMessage(job->popCallbackMailbox(), message);
        } catch (NetworkError &e) {
        }
    }

    /**
     * @brief Method to process a compound job failure
     * @param job: the job that completed
     * @param compute_service: the compute service on which the job completed
     */
    void JobManager::processCompoundJobFailure(const std::shared_ptr<CompoundJob> &job,
                                               std::shared_ptr<ComputeService> compute_service) {
        job->state = CompoundJob::State::DISCONTINUED;
        job->end_date = Simulation::getCurrentSimulatedDate();

        // remove the job from the "dispatched" list
        this->jobs_dispatched.erase(job);

        // Forward the notification along the notification chain
        try {
            auto message =
                    new JobManagerCompoundJobFailedMessage(job, std::move(compute_service),
                                                           std::make_shared<SomeActionsHaveFailed>());
            S4U_Mailbox::dputMessage(job->popCallbackMailbox(), message);
        } catch (NetworkError &e) {
        }
    }

    /**
     * @brief Return the mailbox of the job manager's creator
     *
     * @return a mailbox
     */
    simgrid::s4u::Mailbox *JobManager::getCreatorMailbox() {
        return this->creator_mailbox;
    }

}// namespace wrench
