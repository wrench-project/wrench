/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <string>
#include <wrench/wms/WMS.h>

#include "wrench/exceptions/WorkflowExecutionException.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/managers/JobManager.h"
#include "wrench/services/compute/ComputeService.h"
#include "wrench/services/ServiceMessage.h"
#include "wrench/services/compute/ComputeServiceMessage.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
#include "wrench/simulation/SimulationMessage.h"
#include "wrench/workflow/WorkflowTask.h"
#include "wrench/workflow/job/StandardJob.h"
#include "wrench/workflow/job/PilotJob.h"
#include "wrench/wms/WMS.h"
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
        this->pending_standard_jobs.clear();
        this->running_standard_jobs.clear();
        this->completed_standard_jobs.clear();
        this->failed_standard_jobs.clear();

        this->pending_pilot_jobs.clear();
        this->running_pilot_jobs.clear();
        this->completed_pilot_jobs.clear();
    }

    /**
     * @brief Kill the job manager (brutally terminate the daemon, clears all jobs)
     */
    void JobManager::kill() {
        this->killActor();
        this->pending_standard_jobs.clear();
        this->running_standard_jobs.clear();
        this->completed_standard_jobs.clear();
        this->failed_standard_jobs.clear();

        this->pending_pilot_jobs.clear();
        this->running_pilot_jobs.clear();
        this->completed_pilot_jobs.clear();
    }

    /**
     * @brief Stop the job manager
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    void JobManager::stop() {
        try {
            S4U_Mailbox::putMessage(this->mailbox_name, new ServiceStopDaemonMessage("", 0.0));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
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
            std::map<WorkflowFile *, std::shared_ptr<FileLocation> > file_locations,
            std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>  >> pre_file_copies,
            std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>  >> post_file_copies,
            std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>  >> cleanup_file_deletions) {

        // Do a sanity check of everything (looking for nullptr)
        for (auto t : tasks) {
            if (t == nullptr) {
                throw std::invalid_argument("JobManager::createStandardJob(): nullptr task in the task vector");
            }
        }

        for (auto fl : file_locations) {
            if (fl.first == nullptr) {
                throw std::invalid_argument(
                        "JobManager::createStandardJob(): nullptr workflow file in the file_locations map");
            }
            if (fl.second == nullptr) {
                throw std::invalid_argument(
                        "JobManager::createStandardJob(): nullptr storage service in the file_locations map");
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

        auto job = std::shared_ptr<StandardJob>(
                new StandardJob(this->wms->getWorkflow(), tasks, file_locations, pre_file_copies,
                                post_file_copies, cleanup_file_deletions));

        this->new_standard_jobs.insert(job);
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
     * @brief Create a pilot job
     *
     * @return the pilot job
     *
     * @throw std::invalid_argument
     */
    std::shared_ptr<PilotJob> JobManager::createPilotJob() {
        auto job = std::shared_ptr<PilotJob>(new PilotJob(this->wms->workflow));
        this->new_pilot_jobs.insert(job);
        return job;
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
     * @throw WorkflowExecutionException
     */
    void JobManager::submitJob(std::shared_ptr<WorkflowJob> job,
                               std::shared_ptr<ComputeService> compute_service,
                               std::map<std::string, std::string> service_specific_args) {
        if ((job == nullptr) || (compute_service == nullptr)) {
            throw std::invalid_argument("JobManager::submitJob(): Invalid arguments");
        }

        // Push back the mailbox_name of the manager,
        // so that it will getMessage the initial callback
        job->pushCallbackMailbox(this->mailbox_name);

        std::map<WorkflowTask *, WorkflowTask::State> original_states;

        // Update the job state and insert it into the pending list
        if (auto sjob = std::dynamic_pointer_cast<StandardJob>(job)) {
            // Do a sanity check on task states
            for (auto t : sjob->tasks) {
                if ((t->getState() == WorkflowTask::State::COMPLETED) or
                    (t->getState() == WorkflowTask::State::PENDING)) {
                    throw std::invalid_argument("JobManager()::submitJob(): task " + t->getID() +
                                                " cannot be submitted as part of a standard job because its state is " +
                                                WorkflowTask::stateToString(t->getState()));
                }
            }

            // Modify task states
            sjob->state = StandardJob::PENDING;
            for (auto t : sjob->tasks) {
                original_states.insert(std::make_pair(t, t->getState()));
                t->setState(WorkflowTask::State::PENDING);
            }

            // Do a sanity check on use of scratch space, and replace scratch space by the compute
            // Service's scratch space
            for (auto fl : sjob->file_locations) {
                if ((fl.second == FileLocation::SCRATCH) and (not compute_service->hasScratch())) {
                    throw std::invalid_argument("JobManager():submitJob(): file location for file " +
                                                fl.first->getID() +
                                                " is scratch  space, but the compute service to which this " +
                                                "job is being submitted to doesn't have any!");
                }
                if (fl.second == FileLocation::SCRATCH) {
                    sjob->file_locations[fl.first] = FileLocation::LOCATION(compute_service->getScratch());
                }
            }

            this->new_standard_jobs.erase(sjob);
            this->pending_standard_jobs.insert(sjob);

        } else if (auto pjob = std::dynamic_pointer_cast<PilotJob>(job)) {
            pjob->state = PilotJob::PENDING;
            this->new_pilot_jobs.erase(pjob);
            this->pending_pilot_jobs.insert(pjob);
        }

        // Submit the job to the service
        try {
            job->submit_date = Simulation::getCurrentSimulatedDate();
            job->service_specific_args = service_specific_args;
            compute_service->submitJob(job, service_specific_args);
            job->setParentComputeService(compute_service);

        } catch (WorkflowExecutionException &e) {

            // "Undo" everything
            job->popCallbackMailbox();
            if (auto sjob = std::dynamic_pointer_cast<StandardJob>(job)) {
                sjob->state = StandardJob::NOT_SUBMITTED;
                for (auto t : sjob->tasks) {
                    t->setState(original_states[t]);
                }
                this->pending_standard_jobs.erase(sjob);
            } else if (auto pjob = std::dynamic_pointer_cast<PilotJob>(job)) {
                pjob->state = PilotJob::NOT_SUBMITTED;
                this->pending_pilot_jobs.erase(pjob);
            }
            throw;
        } catch (std::invalid_argument &e) {
            // "Undo" everything
            job->popCallbackMailbox();
            if (auto sjob = std::dynamic_pointer_cast<StandardJob>(job)) {
                sjob->state = StandardJob::NOT_SUBMITTED;
                for (auto t : sjob->tasks) {
                    t->setState(original_states[t]);
                }
                this->pending_standard_jobs.erase(sjob);
            } else if (auto pjob = std::dynamic_pointer_cast<PilotJob>(job)) {
                pjob->state = PilotJob::NOT_SUBMITTED;
                this->pending_pilot_jobs.erase(pjob);
            }
            throw;
        }
    }

    /**
     * @brief Terminate a job (standard or pilot) that hasn't completed/expired/failed yet
     * @param job: the job to be terminated
     *
     * @throw WorkflowExecutionException
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    void JobManager::terminateJob(std::shared_ptr<WorkflowJob> job) {
        if (job == nullptr) {
            throw std::invalid_argument("JobManager::terminateJob(): invalid argument");
        }

        if (job->getParentComputeService() == nullptr) {
            std::string err_msg = "Job cannot be terminated because it doesn't  have a parent compute service";
            throw WorkflowExecutionException(std::shared_ptr<FailureCause>(new NotAllowed(nullptr, err_msg)));
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
        }
    }

    /**
     * @brief Get the list of currently running pilot jobs
     * @return a set of pilot jobs
     */
    std::set<std::shared_ptr<PilotJob>> JobManager::getRunningPilotJobs() {
        return this->running_pilot_jobs;
    }

    /**
     * @brief Get the list of currently pending pilot jobs
     * @return a set of pilot jobs
     */
    std::set<std::shared_ptr<PilotJob> > JobManager::getPendingPilotJobs() {
        return this->pending_pilot_jobs;
    }

#if 0
    /**
     * @brief Forget a job (to free memory_manager_service, only once a job has completed or failed)
     *
     * @param job: a job to forget
     *
     * @throw std::invalid_argument
     * @throw WorkflowExecutionException
     */
    void JobManager::forgetJob(WorkflowJob *job) {
        if (job == nullptr) {
            throw std::invalid_argument("JobManager::forgetJob(): invalid argument");
        }

        // Check the job is somewhere
        if (this->jobs.find(job) == this->jobs.end()) {
            throw std::invalid_argument("JobManager::forgetJob(): unknown job");
        }

        if (job->getType() == WorkflowJob::STANDARD) {

            if ((this->pending_standard_jobs.find((std::shared_ptr<StandardJob> ) job) != this->pending_standard_jobs.end()) ||
                (this->running_standard_jobs.find((std::shared_ptr<StandardJob> ) job) != this->running_standard_jobs.end())) {
                std::string msg = "Job cannot be forgotten because it is pending or running";
                throw WorkflowExecutionException(std::shared_ptr<FailureCause>(new NotAllowed(job->getParentComputeService(), msg)));
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

        if (job->getType() == WorkflowJob::PILOT) {
            if ((this->pending_pilot_jobs.find((std::shared_ptr<PilotJob> ) job) != this->pending_pilot_jobs.end()) ||
                (this->running_pilot_jobs.find((std::shared_ptr<PilotJob> ) job) != this->running_pilot_jobs.end())) {
                std::string msg = "Job cannot be forgotten because it is running or pending";
                throw WorkflowExecutionException(std::shared_ptr<FailureCause>(new NotAllowed(job->getParentComputeService(), msg)));
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
            {
                // Clean up completed standard jobs if need be
                std::vector<std::shared_ptr<StandardJob>> to_clean_up;
                for (auto const &j : this->completed_standard_jobs) {
                    if (j.use_count() == 1) {
                        to_clean_up.push_back(j);
                    }
                }
                for (auto const &j : to_clean_up) {
                    this->completed_standard_jobs.erase(j);
                }
            }

            {
                // Clean up completed pilot jobs if need be
                std::vector<std::shared_ptr<PilotJob>> to_clean_up;
                for (auto const &j : this->completed_pilot_jobs) {
                    if (j.use_count() == 1) {
                        to_clean_up.push_back(j);
                    }
                }
                for (auto const &j : to_clean_up) {
                    this->completed_pilot_jobs.erase(j);
                }
            }
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

        WRENCH_INFO("Job Manager got a %s message", message->getName().c_str());

        if (auto msg = dynamic_cast<ServiceStopDaemonMessage *>(message.get())) {
            // There shouldn't be any need to clean up any state
            return false;
        } else if (auto msg = dynamic_cast<ComputeServiceStandardJobDoneMessage *>(message.get())) {
            processStandardJobCompletion(msg->job, msg->compute_service);
            return true;
        } else if (auto msg = dynamic_cast<ComputeServiceStandardJobFailedMessage *>(message.get())) {
            processStandardJobFailure(msg->job, msg->compute_service, msg->cause);
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

        // Determine all task state changes
        std::map<WorkflowTask *, WorkflowTask::State> necessary_state_changes;
        for (auto task : job->tasks) {

            if (task->getInternalState() == WorkflowTask::InternalState::TASK_COMPLETED) {
                if (task->getUpcomingState() != WorkflowTask::State::COMPLETED) {
                    if (necessary_state_changes.find(task) == necessary_state_changes.end()) {
                        necessary_state_changes.insert(std::make_pair(task, WorkflowTask::State::COMPLETED));
                    } else {
                        necessary_state_changes[task] = WorkflowTask::State::COMPLETED;
                    }
                }
            } else {
                throw std::runtime_error("JobManager::main(): got a 'job done' message, but task " +
                                         task->getID() + " does not have a TASK_COMPLETED internal state (" +
                                         WorkflowTask::stateToString(task->getInternalState()) + ")");
            }
            auto children = task->getWorkflow()->getTaskChildren(task);
            for (auto child : children) {
                switch (child->getInternalState()) {
                    case WorkflowTask::InternalState::TASK_NOT_READY:
                        if (child->getState() != WorkflowTask::State::NOT_READY) {
                            throw std::runtime_error(
                                    "JobManager::main(): Child's internal state if NOT READY, but child's visible "
                                    "state is " + WorkflowTask::stateToString(child->getState()));
                        }
                    case WorkflowTask::InternalState::TASK_COMPLETED:
                        break;
                    case WorkflowTask::InternalState::TASK_FAILED:
                    case WorkflowTask::InternalState::TASK_RUNNING:
                        // no nothing
                        throw std::runtime_error("JobManager::main(): should never happen: " +
                                                 WorkflowTask::stateToString(child->getInternalState()));
                        break;
                    case WorkflowTask::InternalState::TASK_READY:
                        if (child->getState() == WorkflowTask::State::NOT_READY) {
                            bool all_parents_visibly_completed = true;
                            for (auto parent : child->getWorkflow()->getTaskParents(child)) {
                                if (parent->getState() == WorkflowTask::State::COMPLETED) {
                                    continue; // COMPLETED FROM BEFORE
                                }
                                if (parent->getUpcomingState() == WorkflowTask::State::COMPLETED) {
                                    continue; // COMPLETED FROM BEFORE, BUT NOT YET SEEN BY WMS
                                }
                                if ((necessary_state_changes.find(parent) != necessary_state_changes.end()) &&
                                    (necessary_state_changes[parent] == WorkflowTask::State::COMPLETED)) {
                                    continue; // ABOUT TO BECOME COMPLETED
                                }
                                all_parents_visibly_completed = false;
                                break;
                            }
                            if (all_parents_visibly_completed) {
                                if (necessary_state_changes.find(child) == necessary_state_changes.end()) {
                                    necessary_state_changes.insert(
                                            std::make_pair(child, WorkflowTask::State::READY));
                                } else {
                                    necessary_state_changes[child] = WorkflowTask::State::READY;
                                }
                            }
                        }
                        break;
                }
            }
        }

        // move the job from the "pending" list to the "completed" list
        this->pending_standard_jobs.erase(job);
        this->completed_standard_jobs.insert(job);

        /*
        WRENCH_INFO("HERE ARE NECESSARY STATE CHANGES");
        for (auto s : necessary_state_changes) {
          WRENCH_INFO("  STATE(%s) = %s", s.first->getID().c_str(),
          WorkflowTask::stateToString(s.second).c_str());
        }
        */

        for (auto s : necessary_state_changes) {
            s.first->setUpcomingState(s.second);
        }

        // Forward the notification along the notification chain
        std::string callback_mailbox = job->popCallbackMailbox();
        if (not callback_mailbox.empty()) {
            auto augmented_msg = new JobManagerStandardJobDoneMessage(
                    job, compute_service, necessary_state_changes);
            S4U_Mailbox::dputMessage(callback_mailbox, augmented_msg);
        }
    }

    /**
     * @brief Process a standard job failure
     * @param job: the job that failure
     * @param compute_service: the compute service on which the job has failed
     * @param failure_cause: the cause of the failure
     */
    void JobManager::processStandardJobFailure(std::shared_ptr<StandardJob> job,
                                               std::shared_ptr<ComputeService> compute_service,
                                               std::shared_ptr<FailureCause> cause) {
        // update job state
        job->state = StandardJob::State::FAILED;

        // Set the job end date
        job->end_date = Simulation::getCurrentSimulatedDate();

        // Determine all task state changes and failure count updates
        std::map<WorkflowTask *, WorkflowTask::State> necessary_state_changes;
        std::set<WorkflowTask *> necessary_failure_count_increments;

        for (auto t: job->getTasks()) {
            if (t->getInternalState() == WorkflowTask::InternalState::TASK_COMPLETED) {
                if (necessary_state_changes.find(t) == necessary_state_changes.end()) {
                    necessary_state_changes.insert(std::make_pair(t, WorkflowTask::State::COMPLETED));
                } else {
                    necessary_state_changes[t] = WorkflowTask::State::COMPLETED;
                }
                for (auto child : this->wms->getWorkflow()->getTaskChildren(t)) {
                    switch (child->getInternalState()) {
                        case WorkflowTask::InternalState::TASK_NOT_READY:
                        case WorkflowTask::InternalState::TASK_RUNNING:
                        case WorkflowTask::InternalState::TASK_FAILED:
                        case WorkflowTask::InternalState::TASK_COMPLETED:
                            // no nothing
                            break;
                        case WorkflowTask::InternalState::TASK_READY:
                            if (necessary_state_changes.find(child) == necessary_state_changes.end()) {
                                necessary_state_changes.insert(
                                        std::make_pair(child, WorkflowTask::State::READY));
                            } else {
                                necessary_state_changes[child] = WorkflowTask::State::READY;
                            }
                            break;
                    }
                }

            } else if (t->getInternalState() == WorkflowTask::InternalState::TASK_READY) {
                if (necessary_state_changes.find(t) == necessary_state_changes.end()) {
                    necessary_state_changes.insert(std::make_pair(t, WorkflowTask::State::READY));
                } else {
                    necessary_state_changes[t] = WorkflowTask::State::READY;
                }
                necessary_failure_count_increments.insert(t);

            } else if (t->getInternalState() == WorkflowTask::InternalState::TASK_FAILED) {
                bool ready = true;
                for (auto parent : this->wms->getWorkflow()->getTaskParents(t)) {
                    if ((parent->getInternalState() != WorkflowTask::InternalState::TASK_COMPLETED) and
                        (parent->getUpcomingState() != WorkflowTask::State::COMPLETED)) {
                        ready = false;
                    }
                }
                if (ready) {
                    t->setState(WorkflowTask::State::READY);
                    if (necessary_state_changes.find(t) == necessary_state_changes.end()) {
                        necessary_state_changes.insert(std::make_pair(t, WorkflowTask::State::READY));
                    } else {
                        necessary_state_changes[t] = WorkflowTask::State::READY;
                    }
                } else {
                    t->setState(WorkflowTask::State::NOT_READY);
                    if (necessary_state_changes.find(t) == necessary_state_changes.end()) {
                        necessary_state_changes.insert(std::make_pair(t, WorkflowTask::State::NOT_READY));
                    } else {
                        necessary_state_changes[t] = WorkflowTask::State::NOT_READY;
                    }
                }
            }
        }

        // remove the job from the "pending" list
        this->pending_standard_jobs.erase(job);
        // put it in the "failed" list
        this->failed_standard_jobs.insert(job);

        // Forward the notification along the notification chain
        JobManagerStandardJobFailedMessage *augmented_message =
                new JobManagerStandardJobFailedMessage(job, compute_service,
                                                       necessary_state_changes,
                                                       necessary_failure_count_increments,
                                                       std::move(cause));
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

        // move the job from the "pending" list to the "running" list
        this->pending_pilot_jobs.erase(job);
        this->running_pilot_jobs.insert(job);

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

        // Remove the job from the "running" list and put it in the completed list
        this->running_pilot_jobs.erase(job);
        this->completed_pilot_jobs.insert(job);

        // Forward the notification to the source
        WRENCH_INFO("Forwarding to %s", job->getOriginCallbackMailbox().c_str());
        S4U_Mailbox::dputMessage(job->getOriginCallbackMailbox(),
                                 new ComputeServicePilotJobExpiredMessage(job, compute_service, 0.0));
    }

}
