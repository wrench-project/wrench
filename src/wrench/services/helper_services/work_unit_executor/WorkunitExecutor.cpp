/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/workflow/WorkflowFile.h>
#include <wrench/services/storage/StorageService.h>
#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>
#include <wrench/services/ServiceMessage.h>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/workflow/Workflow.h>
#include <wrench/execution_events/ExecutionEvent.h>
#include <wrench/exceptions/ExecutionException.h>
#include <wrench/simgrid_S4U_util/S4U_Simulation.h>
#include <wrench/services/helper_services/work_unit_executor/WorkunitExecutor.h>
#include <wrench/services/helper_services/standard_job_executor/StandardJobExecutorMessage.h>
#include <wrench/workflow/WorkflowTask.h>
#include <wrench/job/StandardJob.h>
#include <wrench/simulation/SimulationTimestampTypes.h>
#include <wrench/services/helper_services/work_unit_executor/Workunit.h>
#include <wrench//failure_causes/NoScratchSpace.h>
#include <wrench/services/helper_services/compute_thread/ComputeThread.h>
#include <wrench/simulation/Simulation.h>
#include <wrench/services/memory/MemoryManager.h>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/services/compute/ComputeService.h>
#include <wrench/failure_causes/FileNotFound.h>
#include <wrench/failure_causes/NetworkError.h>
#include <wrench/failure_causes/FatalFailure.h>
#include <wrench/failure_causes/ComputeThreadHasDied.h>
#include <wrench/services/memory/MemoryManager.h>


WRENCH_LOG_CATEGORY(wrench_core_workunit_executor,
                    "Log category for Multicore Workunit Executor");

//#define S4U_KILL_JOIN_WORKS

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param hostname: the name of the host on which the workunit execution will run
     * @param num_cores: the number of cores available to the workunit executor
     * @param ram_utilization: the number of bytes of RAM used by the service
     * @param callback_mailbox: the callback mailbox to which a "work done" or "work failed" message will be sent
     * @param workunit: the work unit to perform
     * @param scratch_space: the service's scratch storage service (nullptr if none)
     * @param job: the SandardJob the workunit corresponds to
     * @param thread_startup_overhead: the thread_startup overhead, in seconds
     * @param simulate_computation_as_sleep: simulate computation as a sleep instead of an actual compute thread (for simulation scalability reasons)
     */
    WorkunitExecutor::WorkunitExecutor(
            std::string hostname,
            unsigned long num_cores,
            double ram_utilization,
            std::string callback_mailbox,
            std::shared_ptr <Workunit> workunit,
            std::shared_ptr <StorageService> scratch_space,
            std::shared_ptr <StandardJob> job,
            double thread_startup_overhead,
            bool simulate_computation_as_sleep) :
            Service(hostname, "workunit_executor", "workunit_executor") {
        if (num_cores < 1) {
            throw std::invalid_argument("WorkunitExecutor::WorkunitExecutor(): num_cores must be >= 1");
        }
        if (ram_utilization < 0) {
            throw std::invalid_argument("WorkunitExecutor::WorkunitExecutor(): ram_utilization must be >= 0");
        }
        if (workunit == nullptr) {
            throw std::invalid_argument("WorkunitExecutor::WorkunitExecutor(): workunit cannot be nullptr");
        }
        if (thread_startup_overhead < 0) {
            throw std::invalid_argument("WorkunitExecutor::WorkunitExecutor(): thread_startup_overhead must be >= 0");
        }

        this->callback_mailbox = callback_mailbox;
        this->workunit = workunit;
        this->thread_startup_overhead = thread_startup_overhead;
        this->simulate_computation_as_sleep = simulate_computation_as_sleep;
        this->num_cores = num_cores;
        this->ram_utilization = ram_utilization;
        this->scratch_space = scratch_space;
        this->files_stored_in_scratch = {};
        this->job = job;
    }

    /**
     * @brief Cleanup method
     *
     * @param has_returned_from_main: whether main() returned
     * @param return_value: the return value (if main() returned)
     */
    void WorkunitExecutor::cleanup(bool has_returned_from_main, int return_value) {
        WRENCH_DEBUG(
                "In on_exit.cleanup(): WorkunitExecutor: %s has_returned_from_main = %d (return_value = %d, job forcefully terminated = %d)",
                this->getName().c_str(), has_returned_from_main, return_value,
                this->terminated_due_job_being_forcefully_terminated);

        // Check if it's a failure!
        if ((not has_returned_from_main) and (this->task_start_timestamp_has_been_inserted) and
            (not this->task_failure_time_stamp_has_already_been_generated)) {
            if (this->workunit->task != nullptr) {
                WorkflowTask *task = this->workunit->task;
                task->setInternalState(WorkflowTask::InternalState::TASK_FAILED);
                if (not this->terminated_due_job_being_forcefully_terminated) {
                    task->setFailureDate(S4U_Simulation::getClock());
                    this->simulation->getOutput().addTimestampTaskFailure(Simulation::getCurrentSimulatedDate(), task);
                } else {
                    task->setTerminationDate(S4U_Simulation::getClock());
                    this->simulation->getOutput().addTimestampTaskTermination(Simulation::getCurrentSimulatedDate(), task);
                }
            }
        } else if ((this->workunit->task != nullptr) and this->task_completion_timestamp_should_be_generated) {
            this->simulation->getOutput().addTimestampTaskCompletion(Simulation::getCurrentSimulatedDate(), this->workunit->task);
        }
    }

    /**
     * @brief Kill the worker thread
     *
     * @param job_termination: if the reason for being killed is that the job was terminated by the submitter
     * (as opposed to being terminated because the above service was also terminated).
     */
    void WorkunitExecutor::kill(bool job_termination) {
        this->acquireDaemonLock();

        // Then kill all compute threads, if any
        WRENCH_INFO("Killing %ld compute threads", this->compute_threads.size());
        for (auto const &compute_thread : this->compute_threads) {
            WRENCH_INFO("Killing compute thread [%s]", compute_thread->getName().c_str());
            compute_thread->kill();
        }
        this->compute_threads.clear();

        this->terminated_due_job_being_forcefully_terminated = job_termination;
        this->killActor();

        this->releaseDaemonLock();
    }

    /**
     * @brief Helper method determine whether scratch-space use is correct
     * @return true if OK, false if not
     */
    bool WorkunitExecutor::isUseOfScratchSpaceOK() {
        if (this->scratch_space == nullptr) {
            for (auto const &pfc : workunit->pre_file_copies) {
                auto src = std::get<1>(pfc);
                auto dst = std::get<2>(pfc);

                if ((src == FileLocation::SCRATCH) ||
                    (dst == FileLocation::SCRATCH)) {
                    return false;
                }
            }
            for (auto const &fl : workunit->file_locations) {
                for (auto const &fl_l : fl.second) {
                    if (fl_l == FileLocation::SCRATCH) {
                        return false;
                    }
                }
            }
            for (auto const &pfc : workunit->post_file_copies) {
                auto src = std::get<1>(pfc);
                auto dst = std::get<2>(pfc);
                if ((src == FileLocation::SCRATCH) ||
                    (dst == FileLocation::SCRATCH)) {
                    return false;
                }
            }
            for (auto const &cd : workunit->cleanup_file_deletions) {
                auto location = std::get<1>(cd);
                if (location == FileLocation::SCRATCH) {
                    return false;
                }
            }
        }
        return true;
    }

    /**
     * @brief Helper method to determine whether file locations are OK
     * @param offending_file: set to the (first) offending file
     * @return true if OK, false otherwise
     */
    bool WorkunitExecutor::areFileLocationsOK(WorkflowFile **offending_file) {
        for (const auto &fl : workunit->file_locations) {
            if (fl.second.empty()) {
                *offending_file = fl.first;
                return false;
                break;
            }
            if (fl.second.size() > 1) {
                bool found_a_storage_service = false;
                for (auto const &fl_l : fl.second) {
                    if (fl_l->getStorageService()->lookupFile(fl.first, fl_l)) {
                        found_a_storage_service = true;
                        workunit->file_locations[fl.first].clear();
                        workunit->file_locations[fl.first].emplace_back(fl_l);
                        break;
                    }
                }
                if (not found_a_storage_service) {
                    *offending_file = fl.first;
                    return false;
                }
            }
        }
        *offending_file = nullptr;
        return true;
    }

    /**
    * @brief Main method of the worker thread daemon
    *
    * @return 1 if a task1 failure timestamp should be generated, 0 otherwise
    *
    * @throw std::runtime_error
    */
    int WorkunitExecutor::main() {
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_BLUE);
        WRENCH_INFO(
                "New work_unit_executor starting (%s) to do: %ld pre file copies, %d tasks, %ld post file copies, %ld file deletions",
                this->mailbox_name.c_str(),
                this->workunit->pre_file_copies.size(),
                (this->workunit->task != nullptr) ? 1 : 0,
                this->workunit->post_file_copies.size(),
                this->workunit->cleanup_file_deletions.size());

        SimulationMessage *msg_to_send_back = nullptr;
        bool success;

        // Check that there is no Scratch Space weirdness
        if (not isUseOfScratchSpaceOK()) {
            success = false;
            msg_to_send_back = new WorkunitExecutorFailedMessage(
                    this->getSharedPtr<WorkunitExecutor>(),
                    this->workunit,
                    std::shared_ptr<NoScratchSpace>(new NoScratchSpace("No scratch space on compute service")),
                    0.0);

        }

        WorkflowFile *offending_file = nullptr;
        if (not areFileLocationsOK(&offending_file)) {
            success = false;
            msg_to_send_back = new WorkunitExecutorFailedMessage(
                    this->getSharedPtr<WorkunitExecutor>(),
                    this->workunit,
                    std::shared_ptr<FileNotFound>(new FileNotFound(offending_file, nullptr)),
                    0.0);
        }

        if (msg_to_send_back == nullptr) {

            try {
                S4U_Simulation::computeZeroFlop();

                performWork(this->workunit.get());

                // build "success!" message
                success = true;
                msg_to_send_back = new WorkunitExecutorDoneMessage(
                        this->getSharedPtr<WorkunitExecutor>(),
                        this->workunit,
                        0.0);

            } catch (ExecutionException &e) {
                // build "failed!" message
                WRENCH_DEBUG("Got an exception while performing work: %s", e.getCause()->toString().c_str());
                success = false;
                msg_to_send_back = new WorkunitExecutorFailedMessage(
                        this->getSharedPtr<WorkunitExecutor>(),
                        this->workunit,
                        e.getCause(),
                        0.0);
            }
        }

        WRENCH_INFO("Work unit executor on host %s terminating!", S4U_Simulation::getHostName().c_str());
        if ((not this->task_failure_time_stamp_has_already_been_generated) and
            this->failure_timestamp_should_be_generated) {
            if (this->workunit->task != nullptr) {
                WorkflowTask *task = this->workunit->task;
                task->setInternalState(WorkflowTask::InternalState::TASK_FAILED);
                task->setFailureDate(S4U_Simulation::getClock());
                this->simulation->getOutput().addTimestampTaskFailure(Simulation::getCurrentSimulatedDate(), task);
                this->task_failure_time_stamp_has_already_been_generated = true;
            }
        }

        // Send the callback
        if (success) {
            WRENCH_INFO("Notifying mailbox_name %s that work has completed", this->callback_mailbox.c_str());
        } else {
            WRENCH_INFO("Notifying mailbox_name %s that work has failed", this->callback_mailbox.c_str());
        }

        try {
            S4U_Mailbox::putMessage(this->callback_mailbox, msg_to_send_back);
        } catch (std::shared_ptr <NetworkError> &cause) {
            WRENCH_INFO("Work unit executor on can't report back due to network error.. oh well!");
        }

        return 0;
    }

    /**
     * @brief Simulate work execution
     *
     * @param work: the work to perform
     *
     */
    void WorkunitExecutor::performWork(Workunit *work) {
        double mem_req = 0;

        /** Sleep for the sleep time */
        if (work->sleep_time > 0.0) {
            S4U_Simulation::sleep(work->sleep_time);
        }

        /** Perform all pre file copies operations */
        for (auto file_copy : work->pre_file_copies) {
            WorkflowFile *file = std::get<0>(file_copy);
            auto src_location = std::get<1>(file_copy);
            auto dst_location = std::get<2>(file_copy);

            try {
                WRENCH_INFO("Copying file %s from %s to %s",
                            file->getID().c_str(),
                            src_location->toString().c_str(),
                            dst_location->toString().c_str());

                S4U_Simulation::sleep(this->thread_startup_overhead);
                if (dst_location == FileLocation::SCRATCH) {
                    // Always use the job's name as directory if necessary
                    this->scratch_space->getMountPoint();
                    auto augmented_dst_location = FileLocation::LOCATION(
                            this->scratch_space,
                            this->scratch_space->getMountPoint() + "/" +
                            job->getName());
                    StorageService::copyFile(file, src_location, augmented_dst_location);
                    files_stored_in_scratch.insert(file);
                } else {
                    StorageService::copyFile(file, src_location, dst_location);
                }
            } catch (ExecutionException &e) {
                throw;
            }
        }

        /** Perform the computational task1 if any **/
        if (this->workunit->task != nullptr) {
            auto task = this->workunit->task;

            task->setInternalState(WorkflowTask::InternalState::TASK_RUNNING);

            task->setStartDate(S4U_Simulation::getClock());
            task->setExecutionHost(this->hostname);
            task->setNumCoresAllocated(this->num_cores);

            this->simulation->getOutput().addTimestampTaskStart(Simulation::getCurrentSimulatedDate(), task);
//            this->simulation->getOutput().addTimestamp<SimulationTimestampTaskStart>(
//                    new SimulationTimestampTaskStart(task1));
            this->task_start_timestamp_has_been_inserted = true;

            // Read  all input files
            if (not task->getInputFiles().empty()) {
                WRENCH_INFO("Reading the %ld input files for task1 %s",
                            task->getInputFiles().size(), task->getID().c_str());
            }
            try {
                task->setReadInputStartDate(S4U_Simulation::getClock());
//                std::map<WorkflowFile *, std::shared_ptr<FileLocation>> files_to_read;
                std::vector < std::pair < WorkflowFile * , std::shared_ptr < FileLocation>>> files_to_read;
                for (auto const &f : task->getInputFiles()) {
                    if (work->file_locations.find(f) != work->file_locations.end()) {
                        if (work->file_locations[f].size() == 1) {
                            files_to_read.push_back(std::make_pair(f, work->file_locations[f].at(0)));
                        } else {
                            throw std::runtime_error("WorkunitExecutor::PerformWork(): At this stage, there should be a single file location for each file");
                        }
                    } else {
                        if (this->scratch_space == nullptr) { // File should be in scratch, but there is no scratch
                            throw ExecutionException(
                                    std::make_shared<FileNotFound>(f, FileLocation::SCRATCH));
                        }
                        files_to_read.push_back(std::make_pair(
                                f,
                                FileLocation::LOCATION(this->scratch_space,
                                                       this->scratch_space->getMountPoint() + "/" +
                                                       job->getName())));
                        this->files_stored_in_scratch.insert(f);
                    }
                }
                for (auto const &p : files_to_read) {
                    WorkflowFile *f = p.first;
                    std::shared_ptr <FileLocation> l = p.second;

                    if (Simulation::isPageCachingEnabled()) {
                        mem_req += f->getSize();
                    }

                    bool isFileRead = false;
                    try {
                        this->simulation->getOutput().addTimestampFileReadStart(Simulation::getCurrentSimulatedDate(), f, l.get(),
                                                                                l->getStorageService().get(), task);
                        if (Simulation::isPageCachingEnabled() && l->getServerStorageService() != nullptr) {
                            MemoryManager *mm = simulation->getMemoryManagerByHost(S4U_Simulation::getHostName());
                            if (f->getSize() > mm->getTotalMemory()) {
                                throw std::runtime_error("WorkunitExecutor::performWork(): Size of file " + f->getID() +
                                                         " is larger than memory manager's cache size. This is not yet supported");
                            }
                            if (mm->getCachedAmount(f->getID().c_str()) < f->getSize()) {
                                StorageService::copyFile(f, FileLocation::LOCATION(l->getServerStorageService()), l);
                                isFileRead = true;
                            }
                        }
                        if (not isFileRead) {
                            StorageService::readFile(f, l);
                        }
//                        this->simulation->getOutput().addTimestampFileReadStart(f, l.get(), l->getStorageService().get(), task1);
//                        StorageService::readFile(f, l);

                    } catch (ExecutionException &e) {
                        this->simulation->getOutput().addTimestampFileReadFailure(Simulation::getCurrentSimulatedDate(), f, l.get(),
                                                                                  l->getStorageService().get(), task);
                        throw;
                    }
                    this->simulation->getOutput().addTimestampFileReadCompletion(Simulation::getCurrentSimulatedDate(), f, l.get(),
                                                                                 l->getStorageService().get(), task);
                }
                task->setReadInputEndDate(S4U_Simulation::getClock());
            } catch (ExecutionException &e) {
                this->failure_timestamp_should_be_generated = true;
                throw;
            }
            WRENCH_INFO("Reading done")

            // Run the task1's computation (which can be multicore)
            WRENCH_INFO("Executing task1 %s (%lf flops) on %ld cores (%s)", task->getID().c_str(), task->getFlops(),
                        this->num_cores, S4U_Simulation::getHostName().c_str());

            try {
                task->setComputationStartDate(S4U_Simulation::getClock());
                runMulticoreComputationForTask(task, this->simulate_computation_as_sleep);
                task->setComputationEndDate(S4U_Simulation::getClock());
            } catch (ExecutionEvent &e) {
                this->failure_timestamp_should_be_generated = true;
                throw;
            }

            if (not task->getOutputFiles().empty()) {
                WRENCH_INFO("Writing the %ld output files for task1 %s",
                            task->getOutputFiles().size(),
                            task->getID().c_str());
            }

            // Write all output files
            try {
                task->setWriteOutputStartDate(S4U_Simulation::getClock());
//                std::map<WorkflowFile *, std::shared_ptr<FileLocation>> files_to_write;
                std::vector < std::pair < WorkflowFile * , std::shared_ptr < FileLocation>>> files_to_write;
                for (auto const &f : task->getOutputFiles()) {
                    if (Simulation::isPageCachingEnabled()) {
                        MemoryManager *mm = simulation->getMemoryManagerByHost(S4U_Simulation::getHostName());
                        if (f->getSize() > mm->getTotalMemory()) {
                            throw std::runtime_error("WorkunitExecutor::performWork(): Size of file " + f->getID() +
                                                     " is larger than memory manager's cache size. This is not yet supported");
                        }
                    }
                    if (work->file_locations.find(f) != work->file_locations.end()) {
                        if (work->file_locations[f].size() == 1) {
                            files_to_write.push_back(std::make_pair(f, work->file_locations[f].at(0)));
                        } else {
                            throw std::runtime_error("WorkunitExecutor::PerformWork(): At the stage, there should be a single file location for each file");
                        }
                    } else {
                        files_to_write.push_back(std::make_pair(f, FileLocation::LOCATION(
                                this->scratch_space,
                                this->scratch_space->getMountPoint() +
                                "/" + job->getName())));
                        this->files_stored_in_scratch.insert(f);
                    }
                }

                for (auto const &p : files_to_write) {
                    WorkflowFile *f = p.first;
                    std::shared_ptr <FileLocation> l = p.second;

                    try {
                        this->simulation->getOutput().addTimestampFileWriteStart(Simulation::getCurrentSimulatedDate(), f, l.get(),
                                                                                 l->getStorageService().get(), task);
                        StorageService::writeFile(f, l);
                    } catch (ExecutionException &e) {
                        this->simulation->getOutput().addTimestampFileWriteFailure(Simulation::getCurrentSimulatedDate(), f, l.get(),
                                                                                   l->getStorageService().get(), task);
                        throw;
                    }
                    this->simulation->getOutput().addTimestampFileWriteCompletion(Simulation::getCurrentSimulatedDate(), f, l.get(),
                                                                                  l->getStorageService().get(), task);
                }
                task->setWriteOutputEndDate(S4U_Simulation::getClock());
            } catch (ExecutionException &e) {
                this->failure_timestamp_should_be_generated = true;
                throw;
            }
            WRENCH_INFO("Writing done")

            WRENCH_DEBUG("Setting the internal state of %s to TASK_COMPLETED", task->getID().c_str());
            task->setInternalState(WorkflowTask::InternalState::TASK_COMPLETED);

            this->task_completion_timestamp_should_be_generated = true;
//            this->simulation->getOutput().addTimestamp<SimulationTimestampTaskCompletion>(
//                    new SimulationTimestampTaskCompletion(task1));
            task->setEndDate(S4U_Simulation::getClock());

            // Deal with Children
            for (auto child : task->getWorkflow()->getTaskChildren(task)) {
                bool all_parents_completed = true;
                for (auto parent : child->getWorkflow()->getTaskParents(child)) {
                    if (parent->getInternalState() != WorkflowTask::InternalState::TASK_COMPLETED) {
                        all_parents_completed = false;
                        break;
                    }
                }
                if (all_parents_completed) {
                    child->setInternalState(WorkflowTask::InternalState::TASK_READY);
                }
            }
        }

        WRENCH_INFO("Done with the task1's computation");

        /** Perform all post file copies operations */
        // TODO: This is sequential right now, but probably it should be concurrent in some fashion
        for (auto fc : work->post_file_copies) {
            auto file = std::get<0>(fc);
            auto src_location = std::get<1>(fc);
            auto dst_location = std::get<2>(fc);

            if (dst_location == FileLocation::SCRATCH) {
                files_stored_in_scratch.insert(file);
                WRENCH_WARN("WARNING: WorkunitExecutor::performWork(): Post copying files to the "
                            "scratch space: Can cause implicit deletion afterwards");
            }

            StorageService::copyFile(file, src_location, dst_location);
        }

        /** Perform all cleanup file deletions */
        for (auto cleanup : work->cleanup_file_deletions) {
            auto file = std::get<0>(cleanup);
            auto location = std::get<1>(cleanup);
            try {
                StorageService::deleteFile(file, location);
            } catch (ExecutionException &e) {
                if (std::dynamic_pointer_cast<FileNotFound>(e.getCause())) {
                    // Ignore (maybe it was already deleted during a previous attempt)
                } else {
                    throw;
                }
            }
        }

        if (Simulation::isPageCachingEnabled()) {
            MemoryManager *mem_mng = simulation->getMemoryManagerByHost(this->getHostname());
            mem_mng->releaseMemory(mem_req);
        }
    }

    /**
     * @brief Simulate the execution of a multicore computation
     * @param flops: the number of flops
     * @param multicore_performance_spec: the parallel efficiency
     */
    void WorkunitExecutor::runMulticoreComputationForTask(
            WorkflowTask *task, bool simulate_computation_as_sleep) {
        std::vector<double> work_per_thread = task->getParallelModel()->getWorkPerThread(
                task->getFlops(), this->num_cores);
        double max_work_per_thread = *(std::max_element(work_per_thread.begin(), work_per_thread.end()));

        std::string tmp_mailbox = S4U_Mailbox::generateUniqueMailboxName("workunit_executor");

        if (simulate_computation_as_sleep) {
            /** Simulate computation as sleep **/
            // Sleep for the thread startup overhead
            S4U_Simulation::sleep(this->num_cores * this->thread_startup_overhead);

            // Then sleep for the computation duration
            double sleep_time = (max_work_per_thread) / Simulation::getFlopRate();
            Simulation::sleep(sleep_time);

        } else {
            /** Simulate computation with actual compute threads **/
            // Nobody kills me while I am starting compute threads!
            this->acquireDaemonLock();

            WRENCH_INFO("Launching %ld compute threads", this->num_cores);

            // Create a compute thread to run the computation on each core
            bool success = true;
            for (unsigned long i = 0; i < this->num_cores; i++) {
//        WRENCH_INFO("Creating compute thread %ld", i);
                try {
                    S4U_Simulation::sleep(this->thread_startup_overhead);
                } catch (std::exception &e) {
                    WRENCH_INFO("Got an exception while sleeping... perhaps I am being killed?");
                    this->releaseDaemonLock();
                    throw ExecutionException(std::shared_ptr<FailureCause>(new FatalFailure("")));
                }
                std::shared_ptr <ComputeThread> compute_thread;
                try {
                    compute_thread = std::shared_ptr<ComputeThread>(
                            new ComputeThread(S4U_Simulation::getHostName(), work_per_thread.at(i), tmp_mailbox));
                    compute_thread->setSimulation(this->simulation);
                    compute_thread->start(compute_thread, true, false); // Daemonized, no auto-restart
                } catch (std::exception &e) {
                    // Some internal SimGrid exceptions...????
                    WRENCH_INFO("Could not create compute thread... perhaps I am being killed?");
                    success = false;
                    break;
                }
//                WRENCH_INFO("Launched compute thread [%s]", compute_thread->getName().c_str());
                this->compute_threads.push_back(compute_thread);
            }

            if (!success) {
                WRENCH_INFO("Failed to create some compute threads...");
                // TODO: Dangerous to kill these now?? (this was commented out before, but seems legit, so Henri uncommented them)
                for (auto const &ct : this->compute_threads) {
                    ct->kill();
                }
                this->releaseDaemonLock();
                throw ExecutionException(std::shared_ptr<FailureCause>(new ComputeThreadHasDied()));
            }

            this->releaseDaemonLock();  // People can kill me now

            success = true;
            // Wait for all actors to complete
#ifndef S4U_KILL_JOIN_WORKS
            for (unsigned long i = 0; i < this->compute_threads.size(); i++) {
                WRENCH_INFO("Waiting for message from a compute threads...");
                try {
                    S4U_Mailbox::getMessage(tmp_mailbox);
                } catch (std::shared_ptr <NetworkError> &e) {
                    WRENCH_INFO("Got a network error when trying to get completion message from compute thread");
                    // Do nothing, perhaps the child has died
                    success = false;
                    continue;
                }
                WRENCH_INFO("Got it...");
            }
#else
            for (unsigned long i=0; i < this->compute_threads.size(); i++) {
            WRENCH_INFO("JOINING WITH A COMPUTE THREAD %s", this->compute_threads[i]->process_name.c_str());
          try {
            this->compute_threads[i]->join();
          } catch (std::shared_ptr<FatalFailure> &e) {
            WRENCH_INFO("EXCEPTION WHILE JOINED");
            // Do nothing, perhaps the child has died...
            continue;
          }
          WRENCH_INFO("JOINED with COMPUTE THREAD %s", this->compute_threads[i]->process_name.c_str());

        }
#endif
            WRENCH_INFO("All compute threads have completed");
            this->compute_threads.clear();

            if (!success) {
                throw ExecutionException(std::shared_ptr<FailureCause>(new ComputeThreadHasDied()));
            }
        }
    }

    /**
     * @brief Returns the number of cores the service has been allocated
     * @return a number of cores
     */
    unsigned long WorkunitExecutor::getNumCores() {
        return this->num_cores;
    }

    /**
     * @brief Returns the RAM the service has been allocated
     * @return a number of bytes
     */
    double WorkunitExecutor::getMemoryUtilization() {
        return this->ram_utilization;
    }

    /**
     * @brief Retrieve the list of files stored in scratch space storage
     * @return  a list of files
     */
    std::set<WorkflowFile *> WorkunitExecutor::getFilesStoredInScratch() {
        return this->files_stored_in_scratch;
    }

    /**
     * @brief Retrieve the job the WorkunitExecutor is working for
     * @return a job
     */
    std::shared_ptr <StandardJob> WorkunitExecutor::getJob() {
        return this->job;
    }

}
