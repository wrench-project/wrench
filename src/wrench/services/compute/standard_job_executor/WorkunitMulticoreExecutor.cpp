/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <wrench/services/storage/StorageService.h>
#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>
#include <wrench/services/ServiceMessage.h>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/exceptions/WorkflowExecutionException.h>
#include <wrench/simgrid_S4U_util/S4U_Simulation.h>
#include "wrench/services/compute/standard_job_executor/WorkunitMulticoreExecutor.h"
#include "StandardJobExecutorMessage.h"
#include <wrench/workflow/WorkflowTask.h>
#include <wrench/workflow/job/StandardJob.h>
#include <wrench/simulation/SimulationTimestampTypes.h>
#include "wrench/services/compute/standard_job_executor/Workunit.h"
#include "ComputeThread.h"
#include "wrench/simulation/Simulation.h"

#include <xbt.h>
#include <wrench/services/compute/ComputeService.h>


XBT_LOG_NEW_DEFAULT_CATEGORY(workunit_multicore_executor, "Log category for Multicore Workunit Executor");

//#define S4U_KILL_JOIN_WORKS

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param simulation: a pointer to the simulation object
     * @param hostname: the name of the host on which the service will run
     * @param num_cores: the number of cores available to the service
     * @param ram_utilization: the number of bytes of RAM used by the service
     * @param callback_mailbox: the callback mailbox to which a "work done" or "work failed" message will be sent
     * @param workunit: the work unit to perform
     * @param scratch_space: the service's scratch storage service (nullptr if none)
     * @param thread_startup_overhead: the thread_startup overhead, in seconds
     * @param simulate_computation_as_sleep: simulate computation as a sleep instead of an actual compute thread (for simulation scalability reasons)
     */
    WorkunitMulticoreExecutor::WorkunitMulticoreExecutor(
            Simulation *simulation,
            std::string hostname,
            unsigned long num_cores,
            double ram_utilization,
            std::string callback_mailbox,
            Workunit *workunit,
            StorageService *scratch_space,
            WorkflowJob* job,
            double thread_startup_overhead,
            bool simulate_computation_as_sleep) :
            Service(hostname, "workunit_multicore_executor", "workunit_multicore_executor") {

      if (thread_startup_overhead < 0) {
        throw std::invalid_argument("WorkunitMulticoreExecutor::WorkunitMulticoreExecutor(): thread_startup_overhead must be >= 0");
      }
      if (num_cores < 1) {
        throw std::invalid_argument("WorkunitMulticoreExecutor::WorkunitMulticoreExecutor(): num_cores must be >= 1");
      }

      this->simulation = simulation;
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
     * @brief Kill the worker thread
     */
    void WorkunitMulticoreExecutor::kill() {

      this->acquireDaemonLock();

      // Then kill all compute threads, if any
      WRENCH_INFO("Killing %ld compute threads", this->compute_threads.size());
      for (auto const &compute_thread : this->compute_threads) {
        WRENCH_INFO("Killing compute thread [%s]", compute_thread->getName().c_str());
        compute_thread->kill();
      }

      this->killActor();

      this->releaseDaemonLock();

    }



    /**
    * @brief Main method of the worker thread daemon
    *
    * @return 0 on termination
    *
    * @throw std::runtime_error
    */
    int WorkunitMulticoreExecutor::main() {


      TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_BLUE);

      WRENCH_INFO("New WorkunitExecutor starting (%s) to do: %ld pre file copies, %ld tasks, %ld post file copies",
                  this->mailbox_name.c_str(),
                  this->workunit->pre_file_copies.size(),
                  this->workunit->tasks.size(),
                  this->workunit->post_file_copies.size());

      SimulationMessage *msg_to_send_back = nullptr;
      bool success;

      try {
        S4U_Simulation::computeZeroFlop();

        performWork(this->workunit);

        // build "success!" message
        success = true;
        msg_to_send_back = new WorkunitExecutorDoneMessage(
                this,
                this->workunit,
                0.0);

      } catch (WorkflowExecutionException &e) {

        // build "failed!" message
        WRENCH_INFO("Got an exception while performing work: %s", e.getCause()->toString().c_str());
        success = false;
        msg_to_send_back = new WorkunitExecutorFailedMessage(
                this,
                this->workunit,
                e.getCause(),
                0.0);
      }

      // Send the callback
      if (success) {
        WRENCH_INFO("Notifying mailbox_name %s that work has completed",
                    this->callback_mailbox.c_str());
      } else {
        WRENCH_INFO("Notifying mailbox_name %s that work has failed",
                    this->callback_mailbox.c_str());
      }


      try {
        S4U_Mailbox::putMessage(this->callback_mailbox, msg_to_send_back);
      } catch (std::shared_ptr<NetworkError> &cause) {
        WRENCH_INFO("Work unit executor on can't report back due to network error.. aborting!");
        this->workunit = nullptr; // To decrease the ref count
        return 0;
      }

      WRENCH_INFO("Work unit executor on host %s terminating!", S4U_Simulation::getHostName().c_str());
      return 0;
    }


    /**
     * @brief Simulate work execution
     *
     * @param work: the work to perform
     */
    void
    WorkunitMulticoreExecutor::performWork(Workunit *work) {

//      std::set<WorkflowFile* > files_stored_in_scratch = {};

      /** Perform all pre file copies operations */
      for (auto file_copy : work->pre_file_copies) {
        WorkflowFile *file = std::get<0>(file_copy);
        //Even in the pre-file copies, the src can be the scratch itself???
        StorageService *src = std::get<1>(file_copy);
        if (src == ComputeService::SCRATCH) {
          if (this->scratch_space == nullptr) {
            throw WorkflowExecutionException(std::shared_ptr<FailureCause>(new NoScratchSpace("WorkunitMulticoreExecutor::performWork(): Scratch Space was asked to be used as source but is null")));
          }
          src = this->scratch_space;
        }
        StorageService *dst = std::get<2>(file_copy);
        if (dst == ComputeService::SCRATCH) {
          if (this->scratch_space == nullptr) {
            throw WorkflowExecutionException(std::shared_ptr<FailureCause>(new NoScratchSpace("WorkunitMulticoreExecutor::performWork(): Scratch Space was asked to be used as destination but is null")));
          } else {
            dst = this->scratch_space;
          }
        }

        if ((file == nullptr) || (src == nullptr) || (dst == nullptr)) {
          throw std::runtime_error("WorkunitMulticoreExecutor::performWork(): internal error: malformed workunit");
        }

        try {
          WRENCH_INFO("Copying file %s from %s to %s",
                      file->getID().c_str(),
                      src->getName().c_str(),
                      dst->getName().c_str());

          S4U_Simulation::sleep(this->thread_startup_overhead);
          if (dst == this->scratch_space) {
            dst->copyFile(file, src, nullptr, job);
          } else {
            dst->copyFile(file, src, nullptr, nullptr); // if there is no scratch space, then there is no notion of job's partition, it is always to / partition in such case
          }
        } catch (WorkflowExecutionException &e) {
          throw;
        }

        if (dst == this->scratch_space) {
          files_stored_in_scratch.insert(file);
        }
      }

      /** Perform all tasks **/
      for (auto task : work->tasks) {

        task->setInternalState(WorkflowTask::InternalState::TASK_RUNNING);
        task->setStartDate(S4U_Simulation::getClock());

        // Read  all input files
        WRENCH_INFO("Reading the %ld input files for task %s", task->getInputFiles().size(), task->getID().c_str());
        try {
            StorageService::readFiles(task->getInputFiles(),
                                      work->file_locations,
                                      this->scratch_space, files_stored_in_scratch, job);
        } catch (WorkflowExecutionException &e) {
          task->setInternalState(WorkflowTask::InternalState::TASK_FAILED);
          throw;
        }

        // Run the task's computation (which can be multicore)
        WRENCH_INFO("Executing task %s (%lf flops) on %ld cores (%s)", task->getID().c_str(), task->getFlops(), this->num_cores, S4U_Simulation::getHostName().c_str());

        try {
          runMulticoreComputation(task->getFlops(), task->getParallelEfficiency(), this->simulate_computation_as_sleep );
        } catch (WorkflowExecutionEvent &e) {
          task->setInternalState(WorkflowTask::InternalState::TASK_FAILED);
          throw;
        }

        WRENCH_INFO("Writing the %ld output files for task %s", task->getOutputFiles().size(), task->getID().c_str());

        // Write all output files
        try {
            StorageService::writeFiles(task->getOutputFiles(), work->file_locations, this->scratch_space,
                                       files_stored_in_scratch, job);
        } catch (WorkflowExecutionException &e) {
          task->setInternalState(WorkflowTask::InternalState::TASK_FAILED);
          throw;
        }

        WRENCH_INFO("Setting the internal state of %s to TASK_COMPLETED", task->getID().c_str());
        task->setInternalState(WorkflowTask::InternalState::TASK_COMPLETED);

        task->setEndDate(S4U_Simulation::getClock());
        task->setExecutionHost(this->hostname);

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

      WRENCH_INFO("Done with all tasks");


      /** Perform all post file copies operations */
      // TODO: This is sequential right now, but probably it should be concurrent in some fashion
      for (auto file_copy : work->post_file_copies) {
        WorkflowFile *file = std::get<0>(file_copy);
        StorageService *src = std::get<1>(file_copy);
        if (src == ComputeService::SCRATCH) {
          src = this->scratch_space;
        }
        StorageService *dst = std::get<2>(file_copy);
        if (dst == ComputeService::SCRATCH) {
          dst = this->scratch_space;
          files_stored_in_scratch.insert(file);
          WRENCH_WARN(
                  "WARNING: WorkunitMulticoreExecutor::performWork(): Post copying files to the sratch space: Can cause implicit deletion afterwards"
          );
        }

        try {
          S4U_Simulation::sleep(this->thread_startup_overhead);
          if (src == this->scratch_space) {
            dst->copyFile(file, src, job, nullptr);
          } else {
            dst->copyFile(file, src, nullptr, nullptr);
          }

        } catch (WorkflowExecutionException &e) {
          throw;
        }
      }

      /** Perform all cleanup file deletions */
      for (auto cleanup : work->cleanup_file_deletions) {
        WorkflowFile *file = std::get<0>(cleanup);
        StorageService *storage_service = std::get<1>(cleanup);
        try {
          S4U_Simulation::sleep(this->thread_startup_overhead);
          if (storage_service == this->scratch_space) {
            storage_service->deleteFile(file, job, nullptr);
          } else {
            storage_service->deleteFile(file, nullptr, nullptr);
          }
        } catch (WorkflowExecutionException &e) {
          throw;
        }
      }

      WRENCH_INFO("Done with my work");


    }


    /**
     * @brief Simulate the execution of a multicore computation
     * @param flops: the number of flops
     * @param parallel_efficiency: the parallel efficiency
     */
    void WorkunitMulticoreExecutor::runMulticoreComputation(double flops, double parallel_efficiency, bool simulate_computation_as_sleep) {
      double effective_flops = (flops / (this->num_cores * parallel_efficiency));

      std::string tmp_mailbox = S4U_Mailbox::generateUniqueMailboxName("workunit_executor");

      // Nobody kills me while I am starting compute threads!
      this->acquireDaemonLock();

      if (simulate_computation_as_sleep) {

        /** Simulate computation as sleep **/

        // Still sleep for the thread startup overhead
        S4U_Simulation::sleep(this->num_cores * this->thread_startup_overhead);

        // Then sleep for the computation duration
        double sleep_time = flops / Simulation::getFlopRate();
        Simulation::sleep(sleep_time);

      } else {
        /** Simulate computation with actual compute threads **/

        WRENCH_INFO("Simulating %ld compute threads", this->num_cores);

        // Create a compute thread to run the computation on each core
        bool success = true;
        for (unsigned long i = 0; i < this->num_cores; i++) {
//        WRENCH_INFO("Creating compute thread %ld", i);
          try {
            S4U_Simulation::sleep(this->thread_startup_overhead);
          } catch (std::exception &e) {
            WRENCH_INFO("Got an exception while sleeping... perhaps I am being killed?");
            this->releaseDaemonLock();
            throw WorkflowExecutionException(std::shared_ptr<FailureCause>(new FatalFailure()));
          }
          std::shared_ptr<ComputeThread> compute_thread;
          try {
            compute_thread = std::shared_ptr<ComputeThread>(
                    new ComputeThread(this->simulation, S4U_Simulation::getHostName(), effective_flops, tmp_mailbox));
            compute_thread->start(compute_thread, true);
          } catch (std::exception &e) {
            // Some internal SimGrid exceptions...????
            WRENCH_INFO("Could not create compute thread... perhaps I am being killed?");
            success = false;
            break;
          }
          WRENCH_INFO("Launched compute thread [%s]", compute_thread->getName().c_str());
          this->compute_threads.push_back(compute_thread);
        }


        if (!success) {
          WRENCH_INFO("Failed to create some compute threads...");
          // TODO: Probably dangerous to kill these now...
//        for (auto ct : this->compute_threads) {
//          ct->kill();
//        }
          this->releaseDaemonLock();
          throw WorkflowExecutionException(std::shared_ptr<FailureCause>(new ComputeThreadHasDied()));
        }
        WRENCH_INFO("Waiting for completion of all compute threads");

        this->releaseDaemonLock();  // People can kill me now

        success = true;
        // Wait for all actors to complete
        #ifndef S4U_KILL_JOIN_WORKS
        for (unsigned long i = 0; i < this->compute_threads.size(); i++) {
          try {
            S4U_Mailbox::getMessage(tmp_mailbox);
          } catch (std::shared_ptr<NetworkError> &e) {
            WRENCH_INFO("Got a network error when trying to get completion message from compute thread");
            // Do nothing, perhaps the child has died
            success = false;
            continue;
          }
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

        if (!success) {
          throw WorkflowExecutionException(std::shared_ptr<FailureCause>(new ComputeThreadHasDied()));
        }
      }
    }

    /**
     * @brief Returns the number of cores the service has been allocated
     * @return a number of cores
     */
    unsigned long WorkunitMulticoreExecutor::getNumCores() {
      return this->num_cores;
    }

    /**
     * @brief Returns the RAM the service has been allocated
     * @return a number of bytes
     */
    double WorkunitMulticoreExecutor::getMemoryUtilization() {
      return this->ram_utilization;
    }

    /**
     * @brief Retrieve the list of files stored in scratch space storage
     * @return  a list of files
     */
    std::set<WorkflowFile *> WorkunitMulticoreExecutor::getFilesStoredInScratch() {
      return this->files_stored_in_scratch;
    }

};
