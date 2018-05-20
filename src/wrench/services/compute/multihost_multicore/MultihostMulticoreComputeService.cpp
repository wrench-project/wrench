/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <map>
#include <wrench/util/PointerUtil.h>

#include "wrench/services/ServiceMessage.h"
#include "wrench/services/compute/ComputeServiceMessage.h"
#include "services/compute/standard_job_executor/StandardJobExecutorMessage.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/exceptions/WorkflowExecutionException.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/services/compute/multihost_multicore/MultihostMulticoreComputeService.h"
#include "wrench/services/storage/StorageService.h"
#include "wrench/simulation/Simulation.h"
#include "wrench/workflow/job/PilotJob.h"
#include "wrench/services/helpers/Alarm.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(multicore_compute_service, "Log category for Multicore Compute Service");

namespace wrench {

    /**
     * @brief Submit a standard job to the compute service
     * @param job: a standard job
     * @param service_specific_args: service specific arguments
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    void MultihostMulticoreComputeService::submitStandardJob(StandardJob *job,
                                                             std::map<std::string, std::string> &service_specific_args) {

      if (this->state == Service::DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("submit_standard_job");

      //  send a "run a standard job" message to the daemon's mailbox_name
      try {
        S4U_Mailbox::putMessage(this->mailbox_name,
                                new ComputeServiceSubmitStandardJobRequestMessage(
                                        answer_mailbox, job, service_specific_args,
                                        this->getPropertyValueAsDouble(
                                                ComputeServiceProperty::SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }

      // Get the answer
      std::unique_ptr<SimulationMessage> message = nullptr;
      try {
        message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      } catch (std::shared_ptr<NetworkTimeout> &cause) {
        throw WorkflowExecutionException(cause);
      }

      if (auto msg = dynamic_cast<ComputeServiceSubmitStandardJobAnswerMessage *>(message.get())) {
        // If no success, throw an exception
        if (not msg->success) {
          throw WorkflowExecutionException(msg->failure_cause);
        }
      } else {
        throw std::runtime_error(
                "ComputeService::submitStandardJob(): Received an unexpected [" + message->getName() + "] message!");
      }
    }


    /**
     * @brief Destructor
     */
    MultihostMulticoreComputeService::~MultihostMulticoreComputeService() {
//      std::cerr << "IN MHMC DESTRUCTOR\n";
      this->default_property_values.clear();
    }

    /**
     * @brief Asynchronously submit a pilot job to the compute service
     *
     * @param job: a pilot job
     * @param service_specific_args: service specific arguments
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    void
    MultihostMulticoreComputeService::submitPilotJob(PilotJob *job,
                                                     std::map<std::string, std::string> &service_specific_args) {

      if (this->state == Service::DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("submit_pilot_job");

      // Send a "run a pilot job" message to the daemon's mailbox_name
      try {
        S4U_Mailbox::putMessage(
                this->mailbox_name,
                new ComputeServiceSubmitPilotJobRequestMessage(
                        answer_mailbox, job, this->getPropertyValueAsDouble(
                                MultihostMulticoreComputeServiceProperty::SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }

      // Wait for a reply
      std::unique_ptr<SimulationMessage> message = nullptr;

      try {
        message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      } catch (std::shared_ptr<NetworkTimeout> &cause) {
        throw WorkflowExecutionException(cause);
      }

      if (auto msg = dynamic_cast<ComputeServiceSubmitPilotJobAnswerMessage *>(message.get())) {
        // If no success, throw an exception
        if (not msg->success) {
          throw WorkflowExecutionException(msg->failure_cause);
        } else {
          return;
        }

      } else {
        throw std::runtime_error(
                "MultihostMulticoreComputeService::submitPilotJob(): Received an unexpected [" + message->getName() +
                "] message!");
      }
    }


    /**
     * @brief Constructor
     *
     * @param hostname: the name of the host on which the job executor should be started
     * @param supports_standard_jobs: true if the compute service should support standard jobs
     * @param supports_pilot_jobs: true if the compute service should support pilot jobs
     * @param compute_resources: compute_resources: a list of <hostname, num_cores, memory> pairs, which represent
     *        the compute resources available to this service.
     *          - num_cores = ComputeService::ALL_CORES: use all cores available on the host
     *          - memory = ComputeService::ALL_RAM: use all RAM available on the host
     * @param plist: a property list ({} means "use all defaults")
     */
    MultihostMulticoreComputeService::MultihostMulticoreComputeService(
            const std::string &hostname,
            bool supports_standard_jobs,
            bool supports_pilot_jobs,
            std::set<std::tuple<std::string, unsigned long, double>> compute_resources,
            std::map<std::string, std::string> plist,
            double scratch_size) :
            ComputeService(hostname,
                           "multihost_multicore",
                           "multihost_multicore",
                           supports_standard_jobs,
                           supports_pilot_jobs,
                           scratch_size) {

      initiateInstance(hostname,
                       supports_standard_jobs,
                       supports_pilot_jobs,
                       std::move(compute_resources),
                       plist, -1, nullptr);
    }

    /**
     * @brief Constructor
     *
     * @param hostname: the name of the host on which the job executor should be started
     * @param supports_standard_jobs: true if the compute service should support standard jobs
     * @param supports_pilot_jobs: true if the compute service should support pilot jobs
     * @param compute_hosts:: a set of hostnames (the service
     *        will use all the cores and all the RAM of each host)
     * @param plist: a property list ({} means "use all defaults")
     */
    MultihostMulticoreComputeService::MultihostMulticoreComputeService(const std::string &hostname,
                                                                       const bool supports_standard_jobs,
                                                                       const bool supports_pilot_jobs,
                                                                       const std::set<std::string> compute_hosts,
                                                                       std::map<std::string, std::string> plist,
                                                                       double scratch_size) :
            ComputeService(hostname,
                           "multihost_multicore",
                           "multihost_multicore",
                           supports_standard_jobs,
                           supports_pilot_jobs,
                           scratch_size) {

      std::set<std::tuple<std::string, unsigned long, double>> compute_resources;
      for (auto h : compute_hosts) {
        compute_resources.insert(std::make_tuple(h, ComputeService::ALL_CORES, ComputeService::ALL_RAM));
      }

      initiateInstance(hostname,
                       supports_standard_jobs,
                       supports_pilot_jobs,
                       compute_resources,
                       std::move(plist), -1, nullptr);
    }

/**
     * @brief Internal constructor
     *
     * @param hostname: the name of the host
     * @param supports_standard_jobs: true if the job executor should support standard jobs
     * @param supports_pilot_jobs: true if the job executor should support pilot jobs
     * @param compute_resources: compute_resources: a list of <hostname, num_cores, memory> pairs, which represent
     *        the compute resources available to this service
     * @param plist: a property list
     * @param ttl: the time-to-live, in seconds (-1: infinite time-to-live)
     * @param pj: a containing PilotJob  (nullptr if none)
     * @param suffix: a string to append to the process name
     *
     * @throw std::invalid_argument
     */
    MultihostMulticoreComputeService::MultihostMulticoreComputeService(
            const std::string &hostname,
            bool supports_standard_jobs,
            bool supports_pilot_jobs,
            std::set<std::tuple<std::string, unsigned long, double>> compute_resources,
            std::map<std::string, std::string> plist,
            double ttl,
            PilotJob *pj,
            std::string suffix, StorageService* scratch_space) : ComputeService(hostname,
                                                                                "multihost_multicore" + suffix,
                                                                                "multihost_multicore" + suffix,
                                                                                supports_standard_jobs,
                                                                                supports_pilot_jobs,
                                                                                scratch_space) {

      initiateInstance(hostname,
                       supports_standard_jobs,
                       supports_pilot_jobs,
                       std::move(compute_resources),
                       std::move(plist),
                       ttl,
                       pj);
    }

    /**
     * @brief Internal constructor
     *
     * @param hostname: the name of the host on which the job executor should be started
     * @param supports_standard_jobs: true if the compute service should support standard jobs
     * @param supports_pilot_jobs: true if the compute service should support pilot jobs
     * @param compute_hosts:: a set of hostnames (the service
     *        will use all the cores and all the RAM of each host)
     * @param plist: a property list ({} means "use all defaults")
     * @param scratch_space: the scratch space for this compute service
     */
    MultihostMulticoreComputeService::MultihostMulticoreComputeService(const std::string &hostname,
                                                                       const bool supports_standard_jobs,
                                                                       const bool supports_pilot_jobs,
                                                                       const std::set<std::tuple<std::string, unsigned long, double>> compute_resources,
                                                                       std::map<std::string, std::string> plist,
                                                                       StorageService *scratch_space):
            ComputeService(hostname,
                           "multihost_multicore",
                           "multihost_multicore",
                           supports_standard_jobs,
                           supports_pilot_jobs,
                           scratch_space) {

      initiateInstance(hostname,
                       supports_standard_jobs,
                       supports_pilot_jobs,
                       compute_resources,
                       std::move(plist), -1, nullptr);

    }

/**
 * @brief Internal method called by all constructors to initiate instance
 *
 * @param hostname: the name of the host
 * @param supports_standard_jobs: true if the job executor should support standard jobs
 * @param supports_pilot_jobs: true if the job executor should support pilot jobs
 * @param compute_resources: compute_resources: a list of <hostname, num_cores, memory> pairs, which represent
 *        the compute resources available to this service
 * @param plist: a property list
 * @param ttl: the time-to-live, in seconds (-1: infinite time-to-live)
 * @param pj: a containing PilotJob  (nullptr if none)
 *
 * @throw std::invalid_argument
 */
    void MultihostMulticoreComputeService::initiateInstance(
            const std::string &hostname,
            bool supports_standard_jobs,
            bool supports_pilot_jobs,
            std::set<std::tuple<std::string, unsigned long, double>> compute_resources,
            std::map<std::string, std::string> plist,
            double ttl,
            PilotJob *pj) {

      // Set default and specified properties
      this->setProperties(this->default_property_values, plist);

      this->supports_pilot_jobs = supports_pilot_jobs;
      this->supports_standard_jobs = supports_standard_jobs;


      // Check that there is at least one core per host and that hosts have enough cores
      for (auto host : compute_resources) {
        std::string hname = std::get<0>(host);
        unsigned long requested_cores = std::get<1>(host);
        unsigned long available_cores;
        try {
          available_cores = S4U_Simulation::getNumCores(hname);
        } catch (std::runtime_error &e) {
          throw std::invalid_argument(
                  "MultihostMulticoreComputeService::initiateInstance(): Host '" + hname + "' does not exist");
        }
        if (requested_cores == ComputeService::ALL_CORES) {
          requested_cores = available_cores;
        }
        if (requested_cores == 0) {
          throw std::invalid_argument(
                  "MultihostMulticoreComputeService::MultihostMulticoreComputeService(): at least 1 core should be requested");
        }
        if (requested_cores > available_cores) {
          throw std::invalid_argument(
                  "MultihostMulticoreComputeService::MultihostMulticoreComputeService(): host " + hname + "only has " +
                  std::to_string(available_cores) + " cores but " +
                  std::to_string(requested_cores) + " are requested");
        }


        double requested_ram = std::get<2>(host);
        double available_ram = S4U_Simulation::getHostMemoryCapacity(hname);
        if (requested_ram < 0) {
          throw std::invalid_argument(
                  "MultihostMulticoreComputeService::MultihostMulticoreComputeService(): requested ram should be non-negative");
        }

        if (requested_ram == ComputeService::ALL_RAM) {
          requested_ram = available_ram;
        }

        if (requested_ram > available_ram) {
          throw std::invalid_argument(
                  "MultihostMulticoreComputeService::MultihostMulticoreComputeService(): host " + hname + "only has " +
                  std::to_string(available_ram) + " bytes of RAM but " +
                  std::to_string(requested_ram) + " are requested");
        }

        this->compute_resources.insert(std::make_tuple(hname, requested_cores, requested_ram));
      }

      // Compute the total number of cores and set initial core (and ram) availabilities
      this->total_num_cores = 0;
      for (auto host : this->compute_resources) {
        this->total_num_cores += std::get<1>(host);
        this->core_and_ram_availabilities.insert(std::make_pair(std::get<0>(host), std::make_pair(std::get<1>(host),
                                                                                                  S4U_Simulation::getHostMemoryCapacity(
                                                                                                          std::get<0>(
                                                                                                                  host)))));
      }

      this->ttl = ttl;
      this->has_ttl = (ttl >= 0);
      this->containing_pilot_job = pj;

    }

/**
 * @brief Main method of the daemon
 *
 * @return 0 on termination
 */
    int MultihostMulticoreComputeService::main() {

      TerminalOutput::setThisProcessLoggingColor(COLOR_RED);

      WRENCH_INFO("New Multicore Job Executor starting (%s) on %ld hosts with a total of %ld cores",
                  this->mailbox_name.c_str(), this->compute_resources.size(), this->total_num_cores);

      // Set an alarm for my timely death, if necessary
      if (this->has_ttl) {
        this->death_date = S4U_Simulation::getClock() + this->ttl;
        WRENCH_INFO("Will be terminating at date %lf", this->death_date);
//        std::shared_ptr<SimulationMessage> msg = std::shared_ptr<SimulationMessage>(new ServiceTTLExpiredMessage(0));
        SimulationMessage *msg = new ServiceTTLExpiredMessage(0);
        this->death_alarm = Alarm::createAndStartAlarm(this->simulation, death_date, this->hostname, this->mailbox_name,
                                                       msg, "service_string");
      } else {
        this->death_date = -1.0;
        this->death_alarm = nullptr;
      }

      /** Main loop **/
      while (this->processNextMessage()) {

        /** Dispatch jobs **/
        while (this->dispatchNextPendingJob());
      }

      if (this->containing_pilot_job != nullptr) {
        /*** Clean up everything in the scratch space ***/
        WRENCH_INFO("CLEANING UP SCRATCH IN MULTIHOSTMULTICORECOMPUTE SERVICE");
        cleanUpScratch();
      }

      WRENCH_INFO("Multicore Job Executor on host %s terminated!", S4U_Simulation::getHostName().c_str());
      return 0;
    }

/**
 * @brief Dispatch one pending job, if possible
 * @return true if a job was dispatched, false otherwise
 */
    bool MultihostMulticoreComputeService::dispatchNextPendingJob() {

      if (this->pending_jobs.empty()) {
        return false;
      }

      std::string job_selection_policy =
              this->getPropertyValueAsString(MultihostMulticoreComputeServiceProperty::JOB_SELECTION_POLICY);
      if (job_selection_policy != "FCFS") {
        throw std::runtime_error(
                "MultihostMulticoreComputeService::dispatchNextPendingJob(): Unsupported JOB_SELECTION_POLICY '" +
                job_selection_policy + "'");
      }

      WorkflowJob *picked_job = nullptr;
      picked_job = this->pending_jobs.back();

      bool dispatched = false;
      switch (picked_job->getType()) {
        case WorkflowJob::STANDARD: {
          dispatched = dispatchStandardJob((StandardJob *) picked_job);
          break;
        }
        case WorkflowJob::PILOT: {
          dispatched = dispatchPilotJob((PilotJob *) picked_job);
          break;
        }
      }

      // If we dispatched, take the job out of the pending job list
      if (dispatched) {
        this->pending_jobs.pop_back();
      }
      return dispatched;
    }

/**
 * @brief Compute a resource allocation for a standard job
 * @param job: the job
 * @return the resource allocation
 */
    std::set<std::tuple<std::string, unsigned long, double>>
    MultihostMulticoreComputeService::computeResourceAllocation(StandardJob *job) {

      std::string resource_allocation_policy =
              this->getPropertyValueAsString(MultihostMulticoreComputeServiceProperty::RESOURCE_ALLOCATION_POLICY);

      if (resource_allocation_policy == "aggressive") {
        return computeResourceAllocationAggressive(job);
      } else {
        throw std::runtime_error("MultihostMulticoreComputeService::computeResourceAllocation():"
                                         " Unsupported resource allocation policy '" +
                                 resource_allocation_policy + "'");
      }
    }

/**
 * @brief Compute a resource allocation for a standard job using the "aggressive" policy
 * @param job: the job
 * @return the resource allocation
 */
    std::set<std::tuple<std::string, unsigned long, double>>
    MultihostMulticoreComputeService::computeResourceAllocationAggressive(StandardJob *job) {

//      WRENCH_INFO("COMPUTING RESOURCE ALLOCATION");
      // Make a copy of core_and_ram_availabilities
      std::map<std::string, std::pair<unsigned long, double>> tentative_core_and_ram_availabilities;
      for (auto r : this->core_and_ram_availabilities) {
        tentative_core_and_ram_availabilities.insert(
                std::make_pair(r.first, std::make_pair(std::get<0>(r.second), std::get<1>(r.second))));
      }

      // Make a copy of the tasks
      std::set<WorkflowTask *> tasks;
      for (auto t : job->getTasks()) {
        tasks.insert(t);
      }

      std::map<std::string, std::tuple<unsigned long, double>> allocation;


      // Find the task that can use the most cores somewhere, update availabilities, repeat
      bool keep_going = true;
      while (keep_going) {

        WorkflowTask *picked_task = nullptr;
        std::string picked_picked_host;
        unsigned long picked_picked_num_cores = 0;
        double picked_picked_ram = 0.0;

        for (auto t : tasks) {
//          WRENCH_INFO("LOOKING AT TASK %s", t->getId().c_str());
          std::string picked_host;
          unsigned long picked_num_cores = 0;
          double picked_ram = 0.0;

//          WRENCH_INFO("---> %ld", tentative_availabilities.size());
          for (auto r : tentative_core_and_ram_availabilities) {
//            WRENCH_INFO("   LOOKING AT HOST %s", r.first.c_str());
            std::string hostname = r.first;
            unsigned long num_available_cores = std::get<0>(r.second);
            double available_ram = std::get<1>(r.second);

            if ((num_available_cores < t->getMinNumCores()) or (available_ram < t->getMemoryRequirement())) {
//              WRENCH_INFO("      NO DICE");
              continue;
            }

            unsigned long desired_num_cores;
            if (this->getPropertyValueAsString(
                    MultihostMulticoreComputeServiceProperty::TASK_SCHEDULING_CORE_ALLOCATION_ALGORITHM) == "maximum") {
              desired_num_cores = t->getMaxNumCores();
            } else {
              desired_num_cores = t->getMinNumCores();
            }

            if ((picked_num_cores == 0) || (picked_num_cores < MIN(num_available_cores, desired_num_cores))) {
              picked_host = hostname;
              picked_num_cores = MIN(num_available_cores, desired_num_cores);
              picked_ram = t->getMemoryRequirement();
            }
          }

          if (picked_num_cores == 0) {
            continue;
          }

          if (picked_num_cores > picked_picked_num_cores) {
            picked_task = t;
            picked_picked_num_cores = picked_num_cores;
            picked_picked_ram = picked_ram;
            picked_picked_host = picked_host;
          }
        }


        if (picked_picked_num_cores > 0) {
//          WRENCH_INFO("ALLOCATION %s/%ld-%.2lf for task %s", picked_picked_host.c_str(),
//                      picked_picked_num_cores, picked_picked_ram, picked_task->getId().c_str());

          if (allocation.find(picked_picked_host) != allocation.end()) {
            std::get<0>(allocation[picked_picked_host]) += picked_picked_num_cores;
            std::get<1>(allocation[picked_picked_host]) += picked_picked_ram;
          } else {
            allocation.insert(
                    std::make_pair(picked_picked_host,
                                   std::make_tuple(picked_picked_num_cores,
                                                   picked_picked_ram)));
          }

          // Update availabilities
          std::get<0>(tentative_core_and_ram_availabilities[picked_picked_host]) -= picked_picked_num_cores;
          std::get<1>(tentative_core_and_ram_availabilities[picked_picked_host]) -= picked_picked_ram;

          // Remove the task
          tasks.erase(picked_task);

          // We should keep trying!
          keep_going = not tasks.empty();
        } else {
          keep_going = false;
        }
      }


      // Convert back to a set, which is lame
      std::set<std::tuple<std::string, unsigned long, double>> to_return;
      for (auto h : allocation) {
        to_return.insert(std::make_tuple(h.first, std::get<0>(h.second), std::get<1>(h.second)));
      }
      return to_return;
    }

/**
 * @brief Try to dispatch a standard job
 * @param job: the job
 * @return true is the job was dispatched, false otherwise
 */
    bool MultihostMulticoreComputeService::dispatchStandardJob(StandardJob *job) {


      // Compute the required minimum number of cores
      unsigned long max_min_required_num_cores = 1;
      for (auto t : (job)->getTasks()) {
        max_min_required_num_cores = MAX(max_min_required_num_cores, t->getMinNumCores());
      }

      // Compute the required minimum ram
      double max_min_required_ram = 0.0;
      for (auto t : (job)->getTasks()) {
        max_min_required_ram = MAX(max_min_required_ram, t->getMemoryRequirement());
      }

      // Find the list of hosts with the required number of cores AND the required RAM
      std::set<std::string> possible_hosts;
      for (auto it = this->core_and_ram_availabilities.begin(); it != this->core_and_ram_availabilities.end(); it++) {
//        WRENCH_INFO("%s: %ld %.2lf", it->first.c_str(), std::get<0>(it->second), std::get<1>(it->second));
        if ((std::get<0>(it->second) >= max_min_required_num_cores) and
            (std::get<1>(it->second) >= max_min_required_ram)) {
          possible_hosts.insert(it->first);
        }
      }

      // If not even one host, give up
      if (possible_hosts.empty()) {
//      WRENCH_INFO("*** THERE ARE NOT ENOUGH RESOURCES FOR THIS JOB!!");
        return false;
      }

//      WRENCH_INFO("*** THERE ARE POSSIBLE HOSTS FOR THIS JOB!!");

//      // Compute the max num cores usable by a job task
//      unsigned long maximum_num_cores = 0;
//      for (auto t : job->getTasks()) {
//        WRENCH_INFO("===> %s", this->getPropertyValueAsString(MultihostMulticoreComputeServiceProperty::TASK_SCHEDULING_CORE_ALLOCATION_ALGORITHM).c_str()
//        );
//        if (this->getPropertyValueAsString(MultihostMulticoreComputeServiceProperty::TASK_SCHEDULING_CORE_ALLOCATION_ALGORITHM) == "minimum") {
//          maximum_num_cores = MAX(maximum_num_cores, t->getMinNumCores());
//        } else {
//          maximum_num_cores = MAX(maximum_num_cores, t->getMaxNumCores());
//        }
//      }
//
//      WRENCH_INFO("MAXIMUM NUMBER OF CORES = %ld", maximum_num_cores);

      // Allocate resources for the job based on resource allocation strategies
      std::set<std::tuple<std::string, unsigned long, double>> compute_resources;
      compute_resources = computeResourceAllocation(job);



      // Update core availabilities (and compute total number of cores for printing)
      unsigned long total_cores = 0;
      double total_ram = 0.0;
      for (auto r : compute_resources) {
        std::get<0>(this->core_and_ram_availabilities[std::get<0>(r)]) -= std::get<1>(r);
        std::get<1>(this->core_and_ram_availabilities[std::get<0>(r)]) -= std::get<2>(r);
        total_cores += std::get<1>(r);
        total_ram += std::get<2>(r);
      }


      WRENCH_INFO(
              "Creating a StandardJobExecutor on %ld hosts (total of %ld cores and %.2ef bytes of RAM) for a standard job",
              compute_resources.size(), total_cores, total_ram);

      // Create and start a standard job executor
      // If this is itself NOT a pilot job
      bool part_of_pilot_job = false;
      if (this->containing_pilot_job != nullptr) {
        part_of_pilot_job = true;
      }
      std::shared_ptr<StandardJobExecutor> executor = std::shared_ptr<StandardJobExecutor>(new StandardJobExecutor(
              this->simulation,
              this->mailbox_name,
              this->hostname,
              job,
              compute_resources,
              getScratch(),
              part_of_pilot_job,
              {{StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD,   this->getPropertyValueAsString(
                      MultihostMulticoreComputeServiceProperty::THREAD_STARTUP_OVERHEAD)},
               {StandardJobExecutorProperty::CORE_ALLOCATION_ALGORITHM, this->getPropertyValueAsString(
                       MultihostMulticoreComputeServiceProperty::TASK_SCHEDULING_CORE_ALLOCATION_ALGORITHM)},
               {StandardJobExecutorProperty::TASK_SELECTION_ALGORITHM,  this->getPropertyValueAsString(
                       MultihostMulticoreComputeServiceProperty::TASK_SCHEDULING_TASK_SELECTION_ALGORITHM)},
               {StandardJobExecutorProperty::HOST_SELECTION_ALGORITHM,  this->getPropertyValueAsString(
                       MultihostMulticoreComputeServiceProperty::TASK_SCHEDULING_HOST_SELECTION_ALGORITHM)}}));

      executor->start(executor, true);
      this->standard_job_executors.insert(executor);
      this->running_jobs.insert(job);

      // Tell the caller that a job was dispatched!
      return true;
    }

/**
 * @brief Try to dispatch a pilot job
 * @param job: the job
 * @return true is the job was dispatched, false otherwise
 *
 * @throw std::runtime_error
 */
    bool MultihostMulticoreComputeService::dispatchPilotJob(PilotJob *job) {

      // Find a list of hosts with the required number of cores and ram
      std::vector<std::string> chosen_hosts;
      for (auto &core_availability : this->core_and_ram_availabilities) {
        if ((std::get<0>(core_availability.second) >= job->getNumCoresPerHost()) and
            (std::get<1>(core_availability.second) >= job->getMemoryPerHost())) {
          chosen_hosts.push_back(core_availability.first);
          if (chosen_hosts.size() == job->getNumHosts()) {
            break;
          }
        }
      }

      // If we didn't find enough, give up
      if (chosen_hosts.size() < job->getNumHosts()) {
        return false;
      }

      /* Allocate resources to the job */
      WRENCH_INFO("Allocating %ld/%ld hosts/cores to a pilot job", job->getNumHosts(), job->getNumCoresPerHost());

      // Update core and ram availabilities
      for (auto const &h : chosen_hosts) {
        std::get<0>(this->core_and_ram_availabilities[h]) -= job->getNumCoresPerHost();
        std::get<1>(this->core_and_ram_availabilities[h]) -= job->getMemoryPerHost();
      }

      // Creates a compute service (that does not support pilot jobs!!)
      std::set<std::tuple<std::string, unsigned long, double>> compute_resources;
      for (auto const &h : chosen_hosts) {
        compute_resources.insert(std::make_tuple(h, job->getNumCoresPerHost(), job->getMemoryPerHost()));
      }

      std::shared_ptr<ComputeService> cs = std::shared_ptr<ComputeService>(
              new MultihostMulticoreComputeService(this->hostname,
                                                   true, false,
                                                   compute_resources,
                                                   this->property_list,
                                                   job->getDuration(),
                                                   job,
                                                   "_pilot_job",
                                                   getScratch()));
      cs->simulation = this->simulation;
      job->setComputeService(cs);

      // Start the compute service
      try {
        cs->start(cs, true);
      } catch (std::runtime_error &e) {
        throw;
      }

      // Put the job in the running queue
      this->running_jobs.insert((WorkflowJob *) job);

      // Send the "Pilot job has started" callback
      // Note the getCallbackMailbox instead of the popCallbackMailbox, because
      // there will be another callback upon termination.
      try {
        S4U_Mailbox::dputMessage(job->getCallbackMailbox(),
                                 new ComputeServicePilotJobStartedMessage(
                                         job, this, this->getPropertyValueAsDouble(
                                                 MultihostMulticoreComputeServiceProperty::PILOT_JOB_STARTED_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }


      // Push my own mailbox_name onto the pilot job!
      job->pushCallbackMailbox(this->mailbox_name);

      return true;
    }

/**
 * @brief Wait for and react to any incoming message
 *
 * @return false if the daemon should terminate, true otherwise
 *
 * @throw std::runtime_error
 */
    bool MultihostMulticoreComputeService::processNextMessage() {

      // Wait for a message
      std::unique_ptr<SimulationMessage> message;

      try {
        message = S4U_Mailbox::getMessage(this->mailbox_name);
      } catch (std::shared_ptr<NetworkError> &cause) {
        WRENCH_INFO("Got a network error while getting some message... ignoring");
        return true;
      }

      if (message == nullptr) {
        WRENCH_INFO("Got a NULL message... Likely this means we're all done. Aborting");
        return false;
      }

      WRENCH_INFO("Got a [%s] message", message->getName().c_str());

      if (auto msg = dynamic_cast<ServiceTTLExpiredMessage *>(message.get())) {
        WRENCH_INFO("My TTL has expired, terminating and perhaps notify a pilot job submitted");
        this->terminate(true);
        return false;

      } else if (auto msg = dynamic_cast<ServiceStopDaemonMessage *>(message.get())) {
        this->terminate(false);
        // This is Synchronous
        try {
          S4U_Mailbox::putMessage(msg->ack_mailbox,
                                  new ServiceDaemonStoppedMessage(this->getPropertyValueAsDouble(
                                          MultihostMulticoreComputeServiceProperty::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
          return false;
        }
        return false;

      } else if (auto msg = dynamic_cast<ComputeServiceSubmitStandardJobRequestMessage *>(message.get())) {
        processSubmitStandardJob(msg->answer_mailbox, msg->job, msg->service_specific_args);
        return true;

      } else if (auto msg = dynamic_cast<ComputeServiceSubmitPilotJobRequestMessage *>(message.get())) {
        processSubmitPilotJob(msg->answer_mailbox, msg->job);
        return true;

      } else if (auto msg = dynamic_cast<ComputeServicePilotJobExpiredMessage *>(message.get())) {
        processPilotJobCompletion(msg->job);
        return true;

      } else if (auto *msg = dynamic_cast<ComputeServiceResourceInformationRequestMessage *>(message.get())) {
        processGetResourceInformation(msg->answer_mailbox);
        return true;

      } else if (auto *msg = dynamic_cast<ComputeServiceTerminateStandardJobRequestMessage *>(message.get())) {
        processStandardJobTerminationRequest(msg->job, msg->answer_mailbox);
        return true;

      } else if (auto *msg = dynamic_cast<ComputeServiceTerminatePilotJobRequestMessage *>(message.get())) {
        processPilotJobTerminationRequest(msg->job, msg->answer_mailbox);
        return true;

      } else if (auto msg = dynamic_cast<StandardJobExecutorDoneMessage *>(message.get())) {
        processStandardJobCompletion(msg->executor, msg->job);
        return true;

      } else if (auto msg = dynamic_cast<StandardJobExecutorFailedMessage *>(message.get())) {
        processStandardJobFailure(msg->executor, msg->job, msg->cause);
        return true;

      } else {
        throw std::runtime_error("Unexpected [" + message->getName() + "] message");
      }
    }

/**
 * @brief Terminate all pilot job compute services
 */
    void MultihostMulticoreComputeService::terminateAllPilotJobs() {
      for (auto job : this->running_jobs) {
        if (job->getType() == WorkflowJob::PILOT) {
          auto *pj = (PilotJob *) job;
          WRENCH_INFO("Terminating pilot job %s", job->getName().c_str());
          pj->getComputeService()->stop();
        }
      }
    }

/**
 * @brief fail a pending standard job
 * @param job: the job
 * @param cause: the failure cause
 */
    void
    MultihostMulticoreComputeService::failPendingStandardJob(StandardJob *job, std::shared_ptr<FailureCause> cause) {
      WRENCH_INFO("Failing pending job %s", job->getName().c_str());
      // Set all tasks back to the READY state and wipe out output files
      for (auto failed_task: job->getTasks()) {
        failed_task->setInternalState(WorkflowTask::InternalState::TASK_READY);
        try {
          StorageService::deleteFiles(failed_task->getOutputFiles(), job->getFileLocations(),
                                      getScratch());
        } catch (WorkflowExecutionException &e) {
          WRENCH_WARN("Warning: %s", e.getCause()->toString().c_str());
        }
      }

      // Send back a job failed message
      WRENCH_INFO("Sending job failure notification to '%s'", job->getCallbackMailbox().c_str());
      // NOTE: This is synchronous so that the process doesn't fall off the end
      try {
        S4U_Mailbox::putMessage(job->popCallbackMailbox(),
                                new ComputeServiceStandardJobFailedMessage(
                                        job, this, cause, this->getPropertyValueAsDouble(
                                                MultihostMulticoreComputeServiceProperty::STANDARD_JOB_FAILED_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        return;
      }
    }

/**
 * @brief fail a running standard job
 * @param job: the job
 * @param cause: the failure cause
 */
    void
    MultihostMulticoreComputeService::failRunningStandardJob(StandardJob *job, std::shared_ptr<FailureCause> cause) {

      WRENCH_INFO("Failing running job %s", job->getName().c_str());

      terminateRunningStandardJob(job);

      // Send back a job failed message (Not that it can be a partial fail)
      WRENCH_INFO("Sending job failure notification to '%s'", job->getCallbackMailbox().c_str());
      // NOTE: This is synchronous so that the process doesn't fall off the end
      try {
        S4U_Mailbox::putMessage(job->popCallbackMailbox(),
                                new ComputeServiceStandardJobFailedMessage(
                                        job, this, cause, this->getPropertyValueAsDouble(
                                                MultihostMulticoreComputeServiceProperty::STANDARD_JOB_FAILED_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        return;
      }
    }

/**
* @brief terminate a running standard job
* @param job: the job
*/
    void MultihostMulticoreComputeService::terminateRunningStandardJob(StandardJob *job) {

      StandardJobExecutor *executor = nullptr;
      for (const auto &standard_job_executor : this->standard_job_executors) {
        if (standard_job_executor->getJob() == job) {
          executor = standard_job_executor.get();
        }
      }

      if (executor == nullptr) {
        throw std::runtime_error(
                "MultihostMulticoreComputeService::terminateRunningStandardJob(): Cannot find standard job executor "
                        "corresponding to job being terminated");
      }

      // Terminate the executor
      WRENCH_INFO("Terminating a standard job executor");
      executor->kill();

      for (auto failed_task: job->getTasks()) {
        switch (failed_task->getInternalState()) {
          case WorkflowTask::InternalState::TASK_NOT_READY:
          case WorkflowTask::InternalState::TASK_READY:
          case WorkflowTask::InternalState::TASK_COMPLETED:
            break;

          case WorkflowTask::InternalState::TASK_RUNNING:
            throw std::runtime_error("MultihostMulticoreComputeService::terminateRunningStandardJob(): task state shouldn't be 'RUNNING'"
                                             "after a StandardJobExecutor was killed!");
          case WorkflowTask::InternalState::TASK_FAILED:
            // Making failed task READY again
            failed_task->setInternalState(WorkflowTask::InternalState::TASK_READY);
            break;

          default:
            throw std::runtime_error(
                    "MultihostMulticoreComputeService::terminateRunningStandardJob(): unexpected task state");

        }
      }
    }

/**
 * @brief Terminate a running pilot job
 * @param job: the job
 *
 * @throw std::runtime_error
 */
    void MultihostMulticoreComputeService::terminateRunningPilotJob(PilotJob *job) {

      // Get the associated compute service
      auto compute_service = (MultihostMulticoreComputeService *) (job->getComputeService());

      if (compute_service == nullptr) {
        throw std::runtime_error(
                "MultihostMulticoreComputeService::terminateRunningPilotJob(): can't find compute service associated to pilot job");
      }

      // Stop it
      compute_service->stop();

      // Remove the job from the running list
      this->running_jobs.erase(job);

      // Update the number of available cores
      for (auto const &r : compute_service->compute_resources) {
        std::string hostname = std::get<0>(r);
        unsigned long num_cores = std::get<1>(r);

        std::get<0>(this->core_and_ram_availabilities[hostname]) -= num_cores;
      }
    }

/**
* @brief Declare all current jobs as failed (likely because the daemon is being terminated
* or has timed out (because it's in fact a pilot job))
*/
    void MultihostMulticoreComputeService::failCurrentStandardJobs(std::shared_ptr<FailureCause> cause) {

      for (auto workflow_job : this->running_jobs) {
        if (workflow_job->getType() == WorkflowJob::STANDARD) {
          auto job = (StandardJob *) workflow_job;
          this->failRunningStandardJob(job, cause);
        }
      }


      while (not this->pending_jobs.empty()) {
        WorkflowJob *workflow_job = this->pending_jobs.front();
        this->pending_jobs.pop_back();
        if (workflow_job->getType() == WorkflowJob::STANDARD) {
          auto *job = (StandardJob *) workflow_job;
          this->failPendingStandardJob(job, cause);
        }
      }
    }

/**
 * @brief Process a standard job completion
 * @param executor: the standard job executor
 * @param job: the job
 *
 * @throw std::runtime_error
 */
    void
    MultihostMulticoreComputeService::processStandardJobCompletion(StandardJobExecutor *executor, StandardJob *job) {

      // Update core and ram availabilities
      for (auto r : executor->getComputeResources()) {
        std::string hostname = std::get<0>(r);
        unsigned long num_cores = std::get<1>(r);
        double ram = std::get<2>(r);
        std::get<0>(this->core_and_ram_availabilities[hostname]) += num_cores;
        std::get<1>(this->core_and_ram_availabilities[hostname]) += ram;

      }

      // Remove the executor from the executor list
      bool found_it = false;
      for (auto it = this->standard_job_executors.begin();
           it != this->standard_job_executors.end(); it++) {
        if ((*it).get() == executor) {
          PointerUtil::moveSharedPtrFromSetToSet(it, &(this->standard_job_executors), &(this->completed_job_executors));
          found_it = true;
          break;
        }
      }

      if (!found_it) {
        throw std::runtime_error(
                "MultihostMulticoreComputeService::processStandardJobCompletion(): Received a standard job completion, but the executor is not in the executor list");
      }

      // Remove the job from the running job list
      if (this->running_jobs.find(job) == this->running_jobs.end()) {
        throw std::runtime_error(
                "MultihostMulticoreComputeService::processStandardJobCompletion(): Received a standard job completion, but the job is not in the running job list");
      }
      this->running_jobs.erase(job);

      WRENCH_INFO("A standard job executor has completed job %s", job->getName().c_str());


      // Send the callback to the originator
      try {
        S4U_Mailbox::dputMessage(
                job->popCallbackMailbox(), new ComputeServiceStandardJobDoneMessage(
                        job, this, this->getPropertyValueAsDouble(
                                MultihostMulticoreComputeServiceProperty::STANDARD_JOB_DONE_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        return;
      }
    }


/**
 * @brief Process a work failure
 * @param worker_thread: the worker thread that did the work
 * @param work: the work
 * @param cause: the cause of the failure
 */
    void MultihostMulticoreComputeService::processStandardJobFailure(StandardJobExecutor *executor,
                                                                     StandardJob *job,
                                                                     std::shared_ptr<FailureCause> cause) {

      // Update core and ram availabilities
      for (auto r : executor->getComputeResources()) {
        std::string hostname = std::get<0>(r);
        unsigned long num_cores = std::get<1>(r);
        double ram = std::get<2>(r);
        std::get<0>(this->core_and_ram_availabilities[hostname]) += num_cores;
        std::get<1>(this->core_and_ram_availabilities[hostname]) += ram;

      }


      // Remove the executor from the executor list
      bool found_it = false;
      for (auto it = this->standard_job_executors.begin();
           it != this->standard_job_executors.end(); it++) {
        if ((*it).get() == executor) {
          PointerUtil::moveSharedPtrFromSetToSet(it, &(this->standard_job_executors), &(this->completed_job_executors));
          found_it = true;
          break;
        }
      }

      // Remove the executor from the executor list
      if (!found_it) {
        throw std::runtime_error(
                "MultihostMulticoreComputeService::processStandardJobCompletion(): Received a standard job completion, but the executor is not in the executor list");
      }

      // Remove the job from the running job list
      if (this->running_jobs.find(job) == this->running_jobs.end()) {
        throw std::runtime_error(
                "MultihostMulticoreComputeService::processStandardJobCompletion(): Received a standard job completion, but the job is not in the running job list");
      }
      this->running_jobs.erase(job);

      WRENCH_INFO("A standard job executor has failed to perform job %s", job->getName().c_str());

      // Fail the job
      this->failPendingStandardJob(job, std::move(cause));

    }

/**
 * @brief Terminate the daemon, dealing with pending/running jobs
 *
 * @param notify_pilot_job_submitters:
 */
    void MultihostMulticoreComputeService::terminate(bool notify_pilot_job_submitters) {

      this->setStateToDown();

      WRENCH_INFO("Failing current standard jobs");
      this->failCurrentStandardJobs(std::shared_ptr<FailureCause>(new ServiceIsDown(this)));

      WRENCH_INFO("Terminate all pilot jobs");
      this->terminateAllPilotJobs();

      // Am I myself a pilot job?
      if (notify_pilot_job_submitters && this->containing_pilot_job) {

        WRENCH_INFO("Letting the level above know that the pilot job has ended on mailbox_name %s",
                    this->containing_pilot_job->getCallbackMailbox().c_str());
        // NOTE: This is synchronous so that the process doesn't fall off the end
        try {
          S4U_Mailbox::putMessage(this->containing_pilot_job->popCallbackMailbox(),
                                  new ComputeServicePilotJobExpiredMessage(
                                          this->containing_pilot_job, this,
                                          this->getPropertyValueAsDouble(
                                                  MultihostMulticoreComputeServiceProperty::PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
          return;
        }
      }
    }

/**
 * @brief Process a pilot job completion
 *
 * @param job: the pilot job
 */
    void MultihostMulticoreComputeService::processPilotJobCompletion(PilotJob *job) {

      // Remove the job from the running list
      this->running_jobs.erase(job);

      auto *cs = (MultihostMulticoreComputeService *) job->getComputeService();

      // Update core and ram availabilities
      for (auto const &r : cs->compute_resources) {
        std::string hostname = std::get<0>(r);
        unsigned long num_cores = job->getNumCoresPerHost();
        double ram = job->getMemoryPerHost();

        std::get<0>(this->core_and_ram_availabilities[hostname]) += num_cores;
        std::get<1>(this->core_and_ram_availabilities[hostname]) += ram;
      }

      // Forward the notification
      try {
        S4U_Mailbox::dputMessage(job->popCallbackMailbox(), new ComputeServicePilotJobExpiredMessage(
                job, this, this->getPropertyValueAsDouble(
                        MultihostMulticoreComputeServiceProperty::PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        return;
      }
    }

/**
 * @brief Synchronously terminate a standard job previously submitted to the compute service
 *
 * @param job: the standard job
 *
 * @throw WorkflowExecutionException
 * @throw std::runtime_error
 */
    void MultihostMulticoreComputeService::terminateStandardJob(StandardJob *job) {

      if (this->state == Service::DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("terminate_standard_job");

      //  send a "terminate a standard job" message to the daemon's mailbox_name
      try {
        S4U_Mailbox::putMessage(this->mailbox_name,
                                new ComputeServiceTerminateStandardJobRequestMessage(
                                        answer_mailbox, job, this->getPropertyValueAsDouble(
                                                MultihostMulticoreComputeServiceProperty::TERMINATE_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }

      // Get the answer
      std::unique_ptr<SimulationMessage> message = nullptr;
      try {
        message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      } catch (std::shared_ptr<NetworkTimeout> &cause) {
        throw WorkflowExecutionException(cause);
      }

      if (auto msg = dynamic_cast<ComputeServiceTerminateStandardJobAnswerMessage *>(message.get())) {
        // If no success, throw an exception
        if (not msg->success) {
          throw WorkflowExecutionException(msg->failure_cause);
        }
      } else {
        throw std::runtime_error(
                "MultihostMulticoreComputeService::terminateStandardJob(): Received an unexpected [" +
                message->getName() + "] message!");
      }
    }

/**
 * @brief Synchronously terminate a pilot job to the compute service
 *
 * @param job: a pilot job
 *
 * @throw WorkflowExecutionException
 * @throw std::runtime_error
 */
    void MultihostMulticoreComputeService::terminatePilotJob(PilotJob *job) {

      if (this->state == Service::DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("terminate_pilot_job");

      // Send a "terminate a pilot job" message to the daemon's mailbox_name
      try {
        S4U_Mailbox::putMessage(this->mailbox_name,
                                new ComputeServiceTerminatePilotJobRequestMessage(
                                        answer_mailbox, job, this->getPropertyValueAsDouble(
                                                MultihostMulticoreComputeServiceProperty::TERMINATE_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }

      // Get the answer
      std::unique_ptr<SimulationMessage> message = nullptr;

      try {
        message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      } catch (std::shared_ptr<NetworkTimeout> &cause) {
        throw WorkflowExecutionException(cause);
      }

      if (auto msg = dynamic_cast<ComputeServiceTerminatePilotJobAnswerMessage *>(message.get())) {
        // If no success, throw an exception
        if (not msg->success) {
          throw WorkflowExecutionException(msg->failure_cause);
        }

      } else {
        throw std::runtime_error(
                "MultihostMulticoreComputeService::terminatePilotJob(): Received an unexpected ["
                + message->getName() + "] message!");
      }
    }

/**
 * @brief Process a standard job termination request
 *
 * @param job: the job to terminate
 * @param answer_mailbox: the mailbox to which the answer message should be sent
 */
    void MultihostMulticoreComputeService::processStandardJobTerminationRequest(StandardJob *job,
                                                                                const std::string &answer_mailbox) {

      // Check whether job is pending
      for (auto it = this->pending_jobs.begin(); it < this->pending_jobs.end(); it++) {
        if (*it == job) {
          this->pending_jobs.erase(it);
          ComputeServiceTerminateStandardJobAnswerMessage *answer_message = new ComputeServiceTerminateStandardJobAnswerMessage(
                  job, this, true, nullptr,
                  this->getPropertyValueAsDouble(
                          MultihostMulticoreComputeServiceProperty::TERMINATE_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD));
          try {
            S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
          } catch (std::shared_ptr<NetworkError> &cause) {
            return;
          }
          return;
        }
      }

      // Check whether the job is running
      if (this->running_jobs.find(job) != this->running_jobs.end()) {
        // Remove the job from the list of running jobs
        this->running_jobs.erase(job);
        // terminate it
        terminateRunningStandardJob(job);
        // reply
        ComputeServiceTerminateStandardJobAnswerMessage *answer_message = new ComputeServiceTerminateStandardJobAnswerMessage(
                job, this, true, nullptr,
                this->getPropertyValueAsDouble(
                        MultihostMulticoreComputeServiceProperty::TERMINATE_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD));
        try {
          S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
        } catch (std::shared_ptr<NetworkError> &cause) {
          return;
        }
        return;
      }

      // If we got here, we're in trouble
      WRENCH_INFO("Trying to terminate a standard job that's neither pending nor running!");
      ComputeServiceTerminateStandardJobAnswerMessage *answer_message = new ComputeServiceTerminateStandardJobAnswerMessage(
              job, this, false, std::shared_ptr<FailureCause>(new JobCannotBeTerminated(job)),
              this->getPropertyValueAsDouble(
                      MultihostMulticoreComputeServiceProperty::TERMINATE_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD));
      try {
        S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
      } catch (std::shared_ptr<NetworkError> &cause) {
        return;
      }
    }

/**
 * @brief Process a pilot job termination request
 *
 * @param job: the job to terminate
 * @param answer_mailbox: the mailbox to which the answer message should be sent
 */
    void
    MultihostMulticoreComputeService::processPilotJobTerminationRequest(PilotJob *job,
                                                                        const std::string &answer_mailbox) {

      // Check whether job is pending
      for (auto it = this->pending_jobs.begin(); it < this->pending_jobs.end(); it++) {
        if (*it == job) {
          this->pending_jobs.erase(it);
          ComputeServiceTerminatePilotJobAnswerMessage *answer_message = new ComputeServiceTerminatePilotJobAnswerMessage(
                  job, this, true, nullptr,
                  this->getPropertyValueAsDouble(
                          MultihostMulticoreComputeServiceProperty::TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD));
          try {
            S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
          } catch (std::shared_ptr<NetworkError> &cause) {
            return;
          }
          return;
        }
      }

      // Check whether the job is running
      if (this->running_jobs.find(job) != this->running_jobs.end()) {
        this->running_jobs.erase(job);
        terminateRunningPilotJob(job);
        ComputeServiceTerminatePilotJobAnswerMessage *answer_message = new ComputeServiceTerminatePilotJobAnswerMessage(
                job, this, true, nullptr,
                this->getPropertyValueAsDouble(
                        MultihostMulticoreComputeServiceProperty::TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD));
        try {
          S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
        } catch (std::shared_ptr<NetworkError> &cause) {
          return;
        }
        return;
      }

      // If we got here, we're in trouble
      WRENCH_INFO("Trying to terminate a pilot job that's neither pending nor running!");
      ComputeServiceTerminatePilotJobAnswerMessage *answer_message = new ComputeServiceTerminatePilotJobAnswerMessage(
              job, this, false, std::shared_ptr<FailureCause>(new JobCannotBeTerminated(job)),
              this->getPropertyValueAsDouble(
                      MultihostMulticoreComputeServiceProperty::TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD));
      try {
        S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
      } catch (std::shared_ptr<NetworkError> &cause) {
        return;
      }
    }

//    /*

/**
 * @brief Process a submit standard job request
 *
 * @param answer_mailbox: the mailbox to which the answer message should be sent
 * @param job: the job
 * @param service_specific_args: service specific arguments
 *
 * @throw std::runtime_error
 */
    void MultihostMulticoreComputeService::processSubmitStandardJob(
            const std::string &answer_mailbox, StandardJob *job,
            std::map<std::string, std::string> &service_specific_arguments) {
      WRENCH_INFO("Asked to run a standard job with %ld tasks", job->getNumTasks());

      // Do we support standard jobs?
      if (not this->supportsStandardJobs()) {
        try {
          S4U_Mailbox::dputMessage(
                  answer_mailbox,
                  new ComputeServiceSubmitStandardJobAnswerMessage(
                          job, this, false, std::shared_ptr<FailureCause>(new JobTypeNotSupported(job, this)),
                          this->getPropertyValueAsDouble(
                                  ComputeServiceProperty::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
          return;
        }
        return;
      }


      // Can we run this job assuming the whole set of resources is available?
      // Let's check for each task
      bool enough_resources = false;
      for (auto t : job->getTasks()) {
        unsigned long required_num_cores = t->getMinNumCores();
        double required_ram = t->getMemoryRequirement();

        for (auto r : this->compute_resources) {
          unsigned long num_cores = std::get<1>(r);
          double ram = std::get<2>(r);
          if ((num_cores >= required_num_cores) and (ram >= required_ram)) {
            enough_resources = true;
          }
        }
      }

      if (!enough_resources) {
        try {
          S4U_Mailbox::dputMessage(
                  answer_mailbox,
                  new ComputeServiceSubmitStandardJobAnswerMessage(
                          job, this, false, std::shared_ptr<FailureCause>(new NotEnoughComputeResources(job, this)),
                          this->getPropertyValueAsDouble(
                                  MultihostMulticoreComputeServiceProperty::NOT_ENOUGH_CORES_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
          return;
        }
        return;
      }


      // Since we can run, add the job to the list of pending jobs
      this->pending_jobs.push_front((WorkflowJob *) job);

      try {
        S4U_Mailbox::dputMessage(
                answer_mailbox,
                new ComputeServiceSubmitStandardJobAnswerMessage(
                        job, this, true, nullptr, this->getPropertyValueAsDouble(
                                ComputeServiceProperty::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        return;
      }
    }

/**
 * @brief Process a submit pilot job request
 *
 * @param answer_mailbox: the mailbox to which the answer message should be sent
 * @param job: the job
 *
 * @throw std::runtime_error
 */
    void MultihostMulticoreComputeService::processSubmitPilotJob(const std::string &answer_mailbox,
                                                                 PilotJob *job) {
      WRENCH_INFO("Asked to run a pilot job with %ld hosts and %ld cores per host for %lf seconds",
                  job->getNumHosts(), job->getNumCoresPerHost(), job->getDuration());

      if (not this->supportsPilotJobs()) {
        try {
          S4U_Mailbox::dputMessage(
                  answer_mailbox, new ComputeServiceSubmitPilotJobAnswerMessage(
                          job, this, false, std::shared_ptr<FailureCause>(new JobTypeNotSupported(job, this)),
                          this->getPropertyValueAsDouble(
                                  MultihostMulticoreComputeServiceProperty::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
          return;
        }
        return;
      }

      // count the number of hosts that have enough cores
      unsigned long num_possible_hosts = 0;
      for (const auto &compute_resource : this->compute_resources) {
        if (std::get<1>(compute_resource) >= job->getNumCoresPerHost()) {
          num_possible_hosts++;
        }
      }

      // Do we have enough hosts?
      if (num_possible_hosts < job->getNumHosts()) {
        try {
          S4U_Mailbox::dputMessage(
                  answer_mailbox, new ComputeServiceSubmitPilotJobAnswerMessage(
                          job, this, false, std::shared_ptr<FailureCause>(new NotEnoughComputeResources(job, this)),
                          this->getPropertyValueAsDouble(
                                  MultihostMulticoreComputeServiceProperty::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
          return;
        }
        return;
      }

      // success
      this->pending_jobs.push_front((WorkflowJob *) job);
      try {
        S4U_Mailbox::dputMessage(
                answer_mailbox, new ComputeServiceSubmitPilotJobAnswerMessage(
                        job, this, true, nullptr,
                        this->getPropertyValueAsDouble(
                                MultihostMulticoreComputeServiceProperty::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        return;
      }
    }

/**
 * @brief Process a "get resource description message"
 * @param answer_mailbox: the mailbox to which the description message should be sent
 */
    void MultihostMulticoreComputeService::processGetResourceInformation(const std::string &answer_mailbox) {
      // Build a dictionary
      std::map<std::string, std::vector<double>> dict;

      // Num hosts
      std::vector<double> num_hosts;
      num_hosts.push_back((double) (this->compute_resources.size()));
      dict.insert(std::make_pair("num_hosts", num_hosts));

      // Num cores per hosts
      std::vector<double> num_cores;
      for (auto r : this->compute_resources) {
        num_cores.push_back((double) (std::get<1>(r)));
      }
      dict.insert(std::make_pair("num_cores", num_cores));

      // Num idle cores per hosts
      std::vector<double> num_idle_cores;
      for (auto r : this->core_and_ram_availabilities) {
        num_idle_cores.push_back(std::get<0>(r.second));
      }
      dict.insert(std::make_pair("num_idle_cores", num_idle_cores));

      // Flop rate per host
      std::vector<double> flop_rates;
      for (auto h : this->compute_resources) {
        flop_rates.push_back(S4U_Simulation::getFlopRate(std::get<0>(h)));
      }
      dict.insert(std::make_pair("flop_rates", flop_rates));

      // RAM capacity per host
      std::vector<double> ram_capacities;
      for (auto h : this->compute_resources) {
        ram_capacities.push_back(S4U_Simulation::getHostMemoryCapacity(std::get<0>(h)));
      }
      dict.insert(std::make_pair("ram_capacities", ram_capacities));

      // RAM availability per host
      std::vector<double> ram_availabilities;
      for (auto r : this->core_and_ram_availabilities) {
        ram_availabilities.push_back(std::get<1>(r.second));
      }
      dict.insert(std::make_pair("ram_availabilities", ram_availabilities));

      std::vector<double> ttl;
      if (this->has_ttl) {
        ttl.push_back(this->death_date - S4U_Simulation::getClock());
      } else {
        ttl.push_back(ComputeService::ALL_RAM);
      }
      dict.insert(std::make_pair("ttl", ttl));


      // Send the reply
      ComputeServiceResourceInformationAnswerMessage *answer_message = new ComputeServiceResourceInformationAnswerMessage(
              dict,
              this->getPropertyValueAsDouble(
                      ComputeServiceProperty::RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD));
      try {
        S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
      } catch (std::shared_ptr<NetworkError> &cause) {
        return;
      }
    }

    /**
     * @brief Add the scratch files of one standardjob to the list of all the scratch files of all the standard jobs inside the pilot job
     * @param scratch_files:
     */
    void MultihostMulticoreComputeService::storeFilesStoredInScratch(std::set<WorkflowFile*> scratch_files) {
      this->files_in_scratch.insert(scratch_files.begin(),scratch_files.end());
    }

    /**
     * @brief Cleans up the scratch as I am a pilot job and I need clean the files stored by the standard jobs executed inside me
     */
    void MultihostMulticoreComputeService::cleanUpScratch() {
      // First fetch all the files stored in scratch by all the workunit executors running inside a standardjob
      // Files in scratch by finished workunit executors
      for (auto it = this->completed_job_executors.begin(); it != this->completed_job_executors.end(); it++) {
        std::set<WorkflowFile*> files_in_scratch_by_single_workunit = (*it)->getFilesInScratch();
        this->files_in_scratch.insert(files_in_scratch_by_single_workunit.begin(),
                                      files_in_scratch_by_single_workunit.end());
      }

      for (auto scratch_cleanup_file : this->files_in_scratch) {
        try {
          getScratch()->deleteFile(scratch_cleanup_file);
        } catch (WorkflowExecutionException &e) {
          throw;
        }
      }
    }


};
