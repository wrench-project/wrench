/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <map>

#include "services/ServiceMessage.h"
#include "services/compute/ComputeServiceMessage.h"
#include "services/compute/multihost_multicore/MulticoreComputeServiceMessage.h"
#include "services/compute/standard_job_executor/StandardJobExecutorMessage.h"
#include "simgrid_S4U_util/S4U_Mailbox.h"
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

      //  send a "run a standard job" message to the daemon's mailbox
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
        message = S4U_Mailbox::getMessage(answer_mailbox);
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }

      if (auto *msg = dynamic_cast<ComputeServiceSubmitStandardJobAnswerMessage *>(message.get())) {
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

      // Send a "run a pilot job" message to the daemon's mailbox
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
        message = S4U_Mailbox::getMessage(answer_mailbox);
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }

      if (auto *msg = dynamic_cast<ComputeServiceSubmitPilotJobAnswerMessage *>(message.get())) {
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
     * @brief Synchronously ask the service for its TTL
     *
     * @return the TTL in seconds
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    double MultihostMulticoreComputeService::getTTL() {

      if (this->state == Service::DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      // send a "ttl request" message to the daemon's mailbox
      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("get_ttl");

      try {
        S4U_Mailbox::dputMessage(this->mailbox_name,
                                 new MulticoreComputeServiceTTLRequestMessage(
                                         answer_mailbox,
                                         this->getPropertyValueAsDouble(
                                                 MultihostMulticoreComputeServiceProperty::TTL_REQUEST_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> cause) {
        throw WorkflowExecutionException(cause);
      }

      // Get the reply
      std::unique_ptr<SimulationMessage> message = nullptr;

      try {
        message = S4U_Mailbox::getMessage(answer_mailbox);
      } catch (std::shared_ptr<NetworkError> cause) {
        throw WorkflowExecutionException(cause);
      }

      if (MulticoreComputeServiceTTLAnswerMessage *msg = dynamic_cast<MulticoreComputeServiceTTLAnswerMessage *>(message.get())) {
        return msg->ttl;
      } else {
        throw std::runtime_error(
                "MultihostMulticoreComputeService::getTTL(): Unexpected [" + msg->getName() + "] message");
      }
    }

    /**
     * @brief Synchronously ask the service for its per-core flop rate
     *
     * @return the rate in Flops/sec
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    double MultihostMulticoreComputeService::getCoreFlopRate() {

      if (this->state == Service::DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      // send a "floprate request" message to the daemon's mailbox
      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("get_core_flop_rate");
      try {
        S4U_Mailbox::dputMessage(this->mailbox_name,
                                 new MulticoreComputeServiceFlopRateRequestMessage(
                                         answer_mailbox,
                                         this->getPropertyValueAsDouble(
                                                 MultihostMulticoreComputeServiceProperty::FLOP_RATE_REQUEST_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }

      // Get the reply
      std::unique_ptr<SimulationMessage> message = nullptr;
      try {
        message = S4U_Mailbox::getMessage(answer_mailbox);
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }

      if (auto *msg = dynamic_cast<MulticoreComputeServiceFlopRateAnswerMessage *>(message.get())) {
        return msg->flop_rate;
      } else {
        throw std::runtime_error(
                "MultihostMulticoreComputeService::getCoreFLopRate(): unexpected [" + msg->getName() + "] message");
      }
    }


    /**
     * @brief Constructor
     *
     * @param hostname: the name of the host on which the job executor should be started
     * @param supports_standard_jobs: true if the compute service should support standard jobs
     * @param supports_pilot_jobs: true if the compute service should support pilot jobs
     * @param compute_resources: compute_resources: a list of <hostname, num_cores> pairs, which represent
     *        the compute resources available to this service. A number of cores equal to 0 means
     *        that all cores on the host are used.
     * @param default_storage_service: a storage service (or nullptr)
     * @param plist: a property list
     */
    MultihostMulticoreComputeService::MultihostMulticoreComputeService(std::string hostname,
                                                                       bool supports_standard_jobs,
                                                                       bool supports_pilot_jobs,
                                                                       std::set<std::pair<std::string, unsigned long>> compute_resources,
                                                                       StorageService *default_storage_service,
                                                                       std::map<std::string, std::string> plist) :
            MultihostMulticoreComputeService::MultihostMulticoreComputeService(hostname,
                                                                               supports_standard_jobs,
                                                                               supports_pilot_jobs,
                                                                               compute_resources,
                                                                               plist, -1, nullptr, "",
                                                                               default_storage_service) {

    }


    /**
     * @brief Constructor that starts the daemon for the service on a host,
     *        registering it with a WRENCH Simulation
     *
     * @param hostname: the name of the host
     * @param supports_standard_jobs: true if the job executor should support standard jobs
     * @param supports_pilot_jobs: true if the job executor should support pilot jobs
     * @param compute_resources: compute_resources: a list of <hostname, num_cores> pairs, which represent
     *        the compute resources available to this service
     * @param plist: a property list
     * @param ttl: the time-to-live, in seconds (-1: infinite time-to-live)
     * @param pj: a containing PilotJob  (nullptr if none)
     * @param suffix: a string to append to the process name
     * @param default_storage_service: a storage service
     *
     * @throw std::invalid_argument
     */
    MultihostMulticoreComputeService::MultihostMulticoreComputeService(
            std::string hostname,
            bool supports_standard_jobs,
            bool supports_pilot_jobs,
            std::set<std::pair<std::string, unsigned long>> compute_resources,
            std::map<std::string, std::string> plist,
            double ttl,
            PilotJob *pj,
            std::string suffix,
            StorageService *default_storage_service) :
            ComputeService("multihost_multicore" + suffix,
                           "multihost_multicore" + suffix,
                           supports_standard_jobs,
                           supports_pilot_jobs,
                           default_storage_service) {

      // Set default and specified properties
      this->setProperties(this->default_property_values, plist);

      this->hostname = hostname;

      // Check that there is at least one core per host and that hosts have enough cores
      for (auto host : compute_resources) {
        std::string hname = std::get<0>(host);
        unsigned long requested_cores = std::get<1>(host);
        unsigned long available_cores = S4U_Simulation::getNumCores(hname);
        if (requested_cores == 0) {
          requested_cores = available_cores;
        }
        if (requested_cores > available_cores) {
          throw std::invalid_argument(
                  "MultihostMulticoreComputeService::MultihostMulticoreComputeService(): host " + hname + "only has " +
                  std::to_string(available_cores) + " but " +
                  std::to_string(requested_cores) + " are requested");
        }

        this->compute_resources.insert(std::make_pair(hname, requested_cores));
      }

      // Compute the total number of cores and set initial core availabilities
      this->total_num_cores = 0;
      for (auto host : this->compute_resources) {
        this->total_num_cores += std::get<1>(host);
        this->core_availabilities.insert(std::make_pair(std::get<0>(host), std::get<1>(host)));
//        this->core_availabilities[std::get<0>(host)] = std::get<1>(host);
      }

      this->ttl = ttl;
      this->has_ttl = (ttl >= 0);
      this->containing_pilot_job = pj;

      // Start the daemon on the same host
      try {
        this->start(hostname);
      } catch (std::invalid_argument &e) {
        throw;
      }
    }

/**
 * @brief Main method of the daemon
 *
 * @return 0 on termination
 */
    int MultihostMulticoreComputeService::main() {

      TerminalOutput::setThisProcessLoggingColor(WRENCH_LOGGING_COLOR_RED);

      WRENCH_INFO("New Multicore Job Executor starting (%s) on %ld hosts with a total of %ld cores",
                  this->mailbox_name.c_str(), this->compute_resources.size(), this->total_num_cores);

      // Set an alarm for my timely death, if necessary
      if (this->has_ttl) {
        this->death_date = S4U_Simulation::getClock() + this->ttl;
        WRENCH_INFO("Will be terminating at date %lf", this->death_date);
//        std::shared_ptr<SimulationMessage> msg = std::shared_ptr<SimulationMessage>(new ServiceTTLExpiredMessage(0));
        SimulationMessage* msg = new ServiceTTLExpiredMessage(0);
        this->death_alarm = new Alarm(death_date, this->hostname, this->mailbox_name, msg, "service_string");
      } else {
        this->death_date = -1.0;
        this->death_alarm = nullptr;
      }

      /** Main loop **/
      while (this->processNextMessage()) {

        /** Dispatch jobs **/
        while (this->dispatchNextPendingJob());
      }

      WRENCH_INFO("Multicore Job Executor on host %s terminated!", S4U_Simulation::getHostName().c_str());
      return 0;
    }

/**
 * @brief Dispatch one pending job, if possible
 * @return true if a job was dispatched, false otherwise
 */
    bool MultihostMulticoreComputeService::dispatchNextPendingJob() {

      if (this->pending_jobs.size() == 0) {
        return false;
      }

      WorkflowJob *picked_job = nullptr;

      std::string job_selection_policy =
              this->getPropertyValueAsString(MultihostMulticoreComputeServiceProperty::JOB_SELECTION_POLICY);
      if (job_selection_policy == "FCFS") {
        picked_job = this->pending_jobs.front();
      } else {
        throw std::runtime_error(
                "MultihostMulticoreComputeService::dispatchNextPendingJob(): Unsupported JOB_SELECTION_POLICY '" +
                job_selection_policy + "'");
      }

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
    std::set<std::pair<std::string, unsigned long>>
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
    std::set<std::pair<std::string, unsigned long>>
    MultihostMulticoreComputeService::computeResourceAllocationAggressive(StandardJob *job) {


      WRENCH_INFO("COMPUTING RESOURCE ALLOCATION: %ld", this->core_availabilities.size());
      // Make a copy of core_availabilities
      std::map<std::string, unsigned long> tentative_availabilities;
      for (auto r : this->core_availabilities) {
        tentative_availabilities.insert(std::make_pair(r.first, r.second));
      }

      // Make a copy of the tasks
      std::set<WorkflowTask *> tasks;
      for (auto t : job->getTasks()) {
        tasks.insert(t);
      }

      // Find the task that can use the most cores somewhere, update availabilities, repeat
      bool keep_going = true;
      while (keep_going) {
        keep_going = false;

        WorkflowTask *picked_task = nullptr;
        std::string picked_picked_host;
        unsigned long picked_picked_num_cores = 0;

        for (auto t : tasks) {
//          WRENCH_INFO("LOOKING AT TASK %s", t->getId().c_str());
          std::string picked_host;
          unsigned long picked_num_cores = 0;

//          WRENCH_INFO("---> %ld", tentative_availabilities.size());
          for (auto r : tentative_availabilities) {
//            WRENCH_INFO("   LOOKING AT HOST %s", r.first.c_str());
            std::string hostname = r.first;
            unsigned long num_available_cores = r.second;

            if (num_available_cores < t->getMinNumCores()) {
//              WRENCH_INFO("      NO DICE");
              continue;
            }

            if ((picked_num_cores == 0) || (picked_num_cores < MIN(num_available_cores, t->getMaxNumCores()))) {
              picked_host = hostname;
              picked_num_cores = MIN(num_available_cores, t->getMaxNumCores());
            }
          }

          if (picked_num_cores == 0) {
//            WRENCH_INFO("NOPE");
            continue;
          }

          if (picked_num_cores > picked_picked_num_cores) {
//            WRENCH_INFO("PICKED TASK %s on HOST %s with %ld cores",
//                        t->getId().c_str(), picked_host.c_str(), picked_num_cores);
            picked_task = t;
            picked_picked_num_cores = picked_num_cores;
            picked_picked_host = picked_host;
          }
        }

        if (picked_picked_num_cores != 0) {
          // Update availabilities
          tentative_availabilities[picked_picked_host] -= picked_picked_num_cores;
          // Remove the task
          tasks.erase(picked_task);
          // We should keep trying!
          keep_going = true;
        }
      }

      // Come up with allocation based on tentative availabilities!
      std::set<std::pair<std::string, unsigned long>> allocation;
      for (auto r : tentative_availabilities) {
        std::string hostname = r.first;
        unsigned long num_cores = r.second;

        if (num_cores < this->core_availabilities[hostname]) {
//          WRENCH_INFO("ALLOCATION %s/%ld", hostname.c_str(), this->core_availabilities[hostname] - num_cores);
          allocation.insert(std::make_pair(hostname, this->core_availabilities[hostname] - num_cores));
        }
      }

      return allocation;
    }

    /**
     * @brief Try to dispatch a standard job
     * @param job: the job
     * @return true is the job was dispatched, false otherwise
     */
    bool MultihostMulticoreComputeService::dispatchStandardJob(StandardJob *job) {

      // Compute the required minimum number of cores
      unsigned long minimum_required_num_cores = 1;


//      WRENCH_INFO("IN DISPATCH");
//      for (auto r : this->core_availabilities) {
//        WRENCH_INFO("   --> %s %ld", std::get<0>(r).c_str(), std::get<1>(r));
//      }

      for (auto t : (job)->getTasks()) {
        minimum_required_num_cores = MAX(minimum_required_num_cores, t->getMinNumCores());
      }

      // Find the list of hosts with the required number of cores
      std::set<std::string> possible_hosts;
      for (auto it = this->core_availabilities.begin(); it != this->core_availabilities.end(); it++) {
        WRENCH_INFO("%s: %ld", it->first.c_str(), it->second);
        if (it->second >= minimum_required_num_cores) {
          possible_hosts.insert(it->first);
        }
      }

      // If not even one host, give up
      if (possible_hosts.size() == 0) {
//      WRENCH_INFO("*** THERE ARE NOT ENOUGH RESOURCES FOR THIS JOB!!");
        return false;
      }

//      WRENCH_INFO("*** THERE ARE POSSIBLE HOSTS FOR THIS JOB!!");

      // Compute the max num cores usable by a job task
      unsigned long maximum_num_cores = 0;
      for (auto t : job->getTasks()) {
        maximum_num_cores = MAX(maximum_num_cores, t->getMaxNumCores());
      }

      // Allocate resources for the job based on resource allocation strategies
      std::set<std::pair<std::string, unsigned long>> compute_resources;
      compute_resources = computeResourceAllocation(job);

      // Update core availabilities (and compute total number of cores for printing)
      unsigned long total_cores = 0;
      for (auto r : compute_resources) {
        this->core_availabilities[std::get<0>(r)] -= std::get<1>(r);
        total_cores += std::get<1>(r);
      }

      WRENCH_INFO("Creating a StandardJobExecutor on %ld hosts (%ld cores in total) for a standard job",
                  compute_resources.size(), total_cores);
      // Create a standard job executor
      StandardJobExecutor *executor = new StandardJobExecutor(
              this->simulation,
              this->mailbox_name,
              this->hostname,
              job,
              compute_resources,
              this->default_storage_service,
              {{StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD,   this->getPropertyValueAsString(
                      MultihostMulticoreComputeServiceProperty::THREAD_STARTUP_OVERHEAD)},
               {StandardJobExecutorProperty::CORE_ALLOCATION_ALGORITHM, this->getPropertyValueAsString(
                       MultihostMulticoreComputeServiceProperty::TASK_SCHEDULING_CORE_ALLOCATION_ALGORITHM)},
               {StandardJobExecutorProperty::TASK_SELECTION_ALGORITHM,  this->getPropertyValueAsString(
                       MultihostMulticoreComputeServiceProperty::TASK_SCHEDULING_TASK_SELECTION_ALGORITHM)},
               {StandardJobExecutorProperty::HOST_SELECTION_ALGORITHM,  this->getPropertyValueAsString(
                       MultihostMulticoreComputeServiceProperty::TASK_SCHEDULING_HOST_SELECTION_ALGORITHM)}});


      this->standard_job_executors.insert(executor);
      this->running_jobs.insert(job);

      // Tell the caller that a job was dispatched!
      return true;
    }


    /**
     * @brief Try to dispatch a pilot job
     * @param job: the job
     * @return true is the job was dispatched, false otherwise
     */
    bool MultihostMulticoreComputeService::dispatchPilotJob(PilotJob *job) {

      // Find a list of hosts with the required number of cores
      std::vector<std::string> chosen_hosts;
      for (auto it = this->core_availabilities.begin(); it != this->core_availabilities.end(); it++) {
        if (it->second >= job->getNumCoresPerHost()) {
          chosen_hosts.push_back(it->first);
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

      // Update core availabilities
      for (auto h : chosen_hosts) {
        this->core_availabilities[h] -= job->getNumCoresPerHost();
      }

      // Creates a compute service (that does not support pilot jobs!!)
      std::set<std::pair<std::string, unsigned long>> compute_resources;
      for (auto h : chosen_hosts) {
        compute_resources.insert(std::make_pair(h, job->getNumCoresPerHost()));
      }

      ComputeService *cs =
              new MultihostMulticoreComputeService(this->hostname,
                                                   true, false,
                                                   compute_resources,
                                                   this->property_list,
                                                   job->getDuration(),
                                                   job,
                                                   "_pilot",
                                                   this->default_storage_service);

      cs->setSimulation(this->simulation);
      job->setComputeService(cs);


      // Put the job in the running queue
      this->running_jobs.insert((WorkflowJob *) job);

      // Send the "Pilot job has started" callback
      // Note the getCallbackMailbox instead of the popCallbackMailbox, because
      // there will be another callback upon termination.
      try {
        S4U_Mailbox::dputMessage(job->getCallbackMailbox(),
                                 new ComputeServicePilotJobStartedMessage(job, this,
                                                                          this->getPropertyValueAsDouble(
                                                                                  MultihostMulticoreComputeServiceProperty::PILOT_JOB_STARTED_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> cause) {
        throw WorkflowExecutionException(cause);
      }

      // Push my own mailbox onto the pilot job!
      job->pushCallbackMailbox(this->mailbox_name);

      // Tell the caller that a job was dispatched!
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
      } catch (std::shared_ptr<NetworkError> cause) {
        WRENCH_INFO("Got a network error while getting some message... ignoring");
        return true;
      }

      if (message == nullptr) {
        WRENCH_INFO("Got a NULL message... Likely this means we're all done. Aborting");
        return false;
      }

      WRENCH_INFO("Got a [%s] message", message->getName().c_str());

      if (auto *msg = dynamic_cast<ServiceTTLExpiredMessage *>(message.get())) {
        WRENCH_INFO("My TTL has expired, terminating and perhaps notify a pilot job submitted");
        this->terminate(true);
        return false;

      } else if (auto *msg = dynamic_cast<ServiceStopDaemonMessage *>(message.get())) {
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

      } else if (auto *msg = dynamic_cast<ComputeServiceSubmitStandardJobRequestMessage *>(message.get())) {
        processSubmitStandardJob(msg->answer_mailbox, msg->job, msg->service_specific_args);
        return true;

      } else if (auto *msg = dynamic_cast<ComputeServiceSubmitPilotJobRequestMessage *>(message.get())) {
        WRENCH_INFO("Asked to run a pilot job with %ld hosts and %ld cores per host for %lf seconds",
                    msg->job->getNumHosts(), msg->job->getNumCoresPerHost(), msg->job->getDuration());

        bool success = true;

        if (not this->supportsPilotJobs()) {
          try {
            S4U_Mailbox::dputMessage(
                    msg->answer_mailbox, new ComputeServiceSubmitPilotJobAnswerMessage(
                            msg->job, this, false,
                            std::shared_ptr<FailureCause>(
                                    new JobTypeNotSupported(
                                            msg->job,
                                            this)),
                            this->getPropertyValueAsDouble(
                                    MultihostMulticoreComputeServiceProperty::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
          } catch (std::shared_ptr<NetworkError> &cause) {
            return true;
          }
          return true;
        }

        // count the number of hosts that have enough cores
        unsigned long num_possible_hosts = 0;
        for (auto it = this->compute_resources.begin(); it != this->compute_resources.end(); it++) {
          if (it->second >= msg->job->getNumCoresPerHost()) {
            num_possible_hosts++;
          }
        }

        // Do we have enough hosts?
        if (num_possible_hosts < msg->job->getNumHosts()) {
          try {
            S4U_Mailbox::dputMessage(
                    msg->answer_mailbox, new ComputeServiceSubmitPilotJobAnswerMessage(
                            msg->job, this, false,
                            std::shared_ptr<FailureCause>(
                                    new NotEnoughComputeResources(
                                            msg->job,
                                            this)),
                            this->getPropertyValueAsDouble(
                                    MultihostMulticoreComputeServiceProperty::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
          } catch (std::shared_ptr<NetworkError> &cause) {
            return true;
          }
          return true;
        }

        // success
        this->pending_jobs.push_front(msg->job);
        try {
          S4U_Mailbox::dputMessage(
                  msg->answer_mailbox, new ComputeServiceSubmitPilotJobAnswerMessage(
                          msg->job, this, true, nullptr,
                          this->getPropertyValueAsDouble(
                                  MultihostMulticoreComputeServiceProperty::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
          return true;
        }
        return true;

      } else if (auto *msg = dynamic_cast<ComputeServicePilotJobExpiredMessage *>(message.get())) {
        processPilotJobCompletion(msg->job);
        return true;

      } else if (auto *msg = dynamic_cast<ComputeServiceNumCoresRequestMessage *>(message.get())) {
        processGetNumCores(msg->answer_mailbox);
        return true;

      } else if (auto *msg = dynamic_cast<ComputeServiceNumIdleCoresRequestMessage *>(message.get())) {
        processGetNumIdleCores(msg->answer_mailbox);
        return true;

      } else if (auto *msg = dynamic_cast<MulticoreComputeServiceTTLRequestMessage *>(message.get())) {
        MulticoreComputeServiceTTLAnswerMessage *answer_message = new MulticoreComputeServiceTTLAnswerMessage(
                this->death_date - S4U_Simulation::getClock(),
                this->getPropertyValueAsDouble(
                        MultihostMulticoreComputeServiceProperty::TTL_ANSWER_MESSAGE_PAYLOAD));
        try {
          S4U_Mailbox::dputMessage(msg->answer_mailbox, answer_message);
        } catch (std::shared_ptr<NetworkError> &cause) {
          return true;
        }
        return true;
      } else if (auto *msg = dynamic_cast<MulticoreComputeServiceFlopRateRequestMessage *>(message.get())) {
        MulticoreComputeServiceFlopRateAnswerMessage *answer_message = new MulticoreComputeServiceFlopRateAnswerMessage(
                S4U_Simulation::getFlopRate(this->hostname),
                this->getPropertyValueAsDouble(
                        MultihostMulticoreComputeServiceProperty::FLOP_RATE_ANSWER_MESSAGE_PAYLOAD));
        try {
          S4U_Mailbox::dputMessage(msg->answer_mailbox, answer_message);
        } catch (std::shared_ptr<NetworkError> &cause) {
          return true;
        }
        return true;

      } else if (auto *msg = dynamic_cast<ComputeServiceTerminateStandardJobRequestMessage *>(message.get())) {

        processStandardJobTerminationRequest(msg->job, msg->answer_mailbox);
        return true;

      } else if (auto *msg = dynamic_cast<ComputeServiceTerminatePilotJobRequestMessage *>(message.get())) {

        processPilotJobTerminationRequest(msg->job, msg->answer_mailbox);
        return true;

      } else if (auto *msg = dynamic_cast<StandardJobExecutorDoneMessage *>(message.get())) {
        processStandardJobCompletion(msg->executor, msg->job);
        return true;

      } else if (auto *msg = dynamic_cast<StandardJobExecutorFailedMessage *>(message.get())) {
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
          PilotJob *pj = (PilotJob *) job;
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
        failed_task->setReady();
        try {
          StorageService::deleteFiles(failed_task->getOutputFiles(), job->getFileLocations(),
                                      this->default_storage_service);
        } catch (WorkflowExecutionException &e) {
          WRENCH_WARN("Warning: %s", e.getCause()->toString().c_str());
        }
      }

      // Send back a job failed message
      WRENCH_INFO("Sending job failure notification to '%s'", job->getCallbackMailbox().c_str());
      // NOTE: This is synchronous so that the process doesn't fall off the end
      try {
        S4U_Mailbox::putMessage(job->popCallbackMailbox(),
                                new ComputeServiceStandardJobFailedMessage(job, this, cause,
                                                                           this->getPropertyValueAsDouble(
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
                                new ComputeServiceStandardJobFailedMessage(job, this, cause,
                                                                           this->getPropertyValueAsDouble(
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
      for (auto e : this->standard_job_executors) {
        if (e->getJob() == job) {
          executor = e;
        }
      }
      if (executor == nullptr) {
        throw std::runtime_error(
                "MultihostMulticoreComputeService::terminateRunningStandardJob(): Cannot find standard job executor corresponding to job being terminated");
      }

      // Terminate the executor
      WRENCH_INFO("Terminating a standard job executor");
      executor->kill();

      // Set all non-COMPLETED tasks back to the READY state and wipe out output files
      // TODO: At some point we'll have to think hard about the task life cycle and make it better/coherent
      for (auto failed_task: job->getTasks()) {
//        WRENCH_INFO("====> %s %s", failed_task->getId().c_str(), WorkflowTask::stateToString(failed_task->getState()).c_str());
        switch (failed_task->getState()) {
          case WorkflowTask::NOT_READY: {
            break;
          }
          case WorkflowTask::READY: {
            break;
          }
          case WorkflowTask::PENDING: {
            failed_task->setReady();
            break;
          }
          case WorkflowTask::RUNNING: {
            failed_task->setFailed();
            failed_task->setReady();
            break;
          }
          case WorkflowTask::COMPLETED: {
            break;
          }
          case WorkflowTask::FAILED: {
            failed_task->setReady();
            break;
          }
          default: {
            throw std::runtime_error(
                    "MultihostMulticoreComputeService::terminateRunningStandardJob(): unexpected task state");
          }
        }
      }

      return;
    }

/**
 * @brief Terminate a running pilot job
 * @param job: the job
 *
 * @throw std::runtime_error
 */
    void MultihostMulticoreComputeService::terminateRunningPilotJob(PilotJob *job) {

      // Get the associated compute service
      MultihostMulticoreComputeService *compute_service = (MultihostMulticoreComputeService *) (job->getComputeService());

      if (compute_service == nullptr) {
        throw std::runtime_error(
                "MultihostMulticoreComputeService::terminateRunningPilotJob(): can't find compute service associated to pilot job");
      }

      // Stop it
      compute_service->stop();

      // Remove the job from the running list
      this->running_jobs.erase(job);

      // Update the number of available cores
      for (auto r : compute_service->compute_resources) {
        std::string hostname = std::get<0>(r);
        unsigned long num_cores = std::get<1>(r);

        this->core_availabilities[hostname] -= num_cores;
      }

    }


/**
* @brief Declare all current jobs as failed (likely because the daemon is being terminated
* or has timed out (because it's in fact a pilot job))
*/
    void MultihostMulticoreComputeService::failCurrentStandardJobs(std::shared_ptr<FailureCause> cause) {

      for (auto workflow_job : this->running_jobs) {
        if (workflow_job->getType() == WorkflowJob::STANDARD) {
          StandardJob *job = (StandardJob *) workflow_job;
          this->failRunningStandardJob(job, std::move(cause));
        }
      }

      while (not this->pending_jobs.empty()) {
        WorkflowJob *workflow_job = this->pending_jobs.front();
        this->pending_jobs.pop_back();
        if (workflow_job->getType() == WorkflowJob::STANDARD) {
          StandardJob *job = (StandardJob *) workflow_job;
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

      // Update core availabilities
      for (auto r : executor->getComputeResources()) {
        this->core_availabilities[r.first] += r.second;

      }

      // Remove the executor from the executor list
      if (this->standard_job_executors.find(executor) == this->standard_job_executors.end()) {
        throw std::runtime_error(
                "MultihostMulticoreComputeService::processStandardJobCompletion(): Received a standard job completion, but the executor is not in the executor list");
      }
      this->standard_job_executors.erase(executor);

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

      // Update core availabilities
      for (auto r : executor->getComputeResources()) {
        this->core_availabilities[r.first] += r.second;

      }

      // Remove the executor from the executor list
      if (this->standard_job_executors.find(executor) == this->standard_job_executors.end()) {
        throw std::runtime_error(
                "MultihostMulticoreComputeService::processStandardJobCompletion(): Received a standard job completion, but the executor is not in the executor list");
      }
      this->standard_job_executors.erase(executor);

      // Remove the job from the running job list
      if (this->running_jobs.find(job) == this->running_jobs.end()) {
        throw std::runtime_error(
                "MultihostMulticoreComputeService::processStandardJobCompletion(): Received a standard job completion, but the job is not in the running job list");
      }
      this->running_jobs.erase(job);

      WRENCH_INFO("A standard job executor has failed to perform job %s", job->getName().c_str());

      // Fail the job
      this->failPendingStandardJob(job, cause);

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

        WRENCH_INFO("Letting the level above know that the pilot job has ended on mailbox %s",
                    this->containing_pilot_job->getCallbackMailbox().c_str());
        // NOTE: This is synchronous so that the process doesn't fall off the end
        try {
          S4U_Mailbox::putMessage(this->containing_pilot_job->popCallbackMailbox(),
                                  new ComputeServicePilotJobExpiredMessage(this->containing_pilot_job, this,
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

      MultihostMulticoreComputeService *cs = (MultihostMulticoreComputeService *) job->getComputeService();

      // Update core availabilities
      for (auto r : cs->compute_resources) {
        std::string hostname = std::get<0>(r);
        unsigned long num_cores = std::get<1>(r);

        this->core_availabilities[hostname] += num_cores;
      }

      // Forward the notification
      try {
        S4U_Mailbox::dputMessage(job->popCallbackMailbox(),
                                 new ComputeServicePilotJobExpiredMessage(job, this,
                                                                          this->getPropertyValueAsDouble(
                                                                                  MultihostMulticoreComputeServiceProperty::PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        return;
      }

      return;
    }


/**
 * @brief Synchronously terminate a standard job previously submitted to the compute service
 *
 * @param job: the standard job
 *
 * @throw WorkflowExecutionException
 * @throw std::runtime_error
 *
 */
    void MultihostMulticoreComputeService::terminateStandardJob(StandardJob *job) {

      if (this->state == Service::DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("terminate_standard_job");

      //  send a "terminate a standard job" message to the daemon's mailbox
      try {
        S4U_Mailbox::putMessage(this->mailbox_name,
                                new ComputeServiceTerminateStandardJobRequestMessage(answer_mailbox, job,
                                                                                     this->getPropertyValueAsDouble(
                                                                                             MultihostMulticoreComputeServiceProperty::TERMINATE_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> cause) {
        throw WorkflowExecutionException(cause);
      }

      // Get the answer
      std::unique_ptr<SimulationMessage> message = nullptr;
      try {
        message = S4U_Mailbox::getMessage(answer_mailbox);
      } catch (std::shared_ptr<NetworkError> cause) {
        throw WorkflowExecutionException(cause);
      }

      if (ComputeServiceTerminateStandardJobAnswerMessage *msg = dynamic_cast<ComputeServiceTerminateStandardJobAnswerMessage *>(message.get())) {
        // If no success, throw an exception
        if (not msg->success) {
          throw WorkflowExecutionException(msg->failure_cause);
        }
      } else {
        throw std::runtime_error(
                "MultihostMulticoreComputeService::terminateStandardJob(): Received an unexpected [" +
                message->getName() +
                "] message!");
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

      // Send a "terminate a pilot job" message to the daemon's mailbox
      try {
        S4U_Mailbox::putMessage(this->mailbox_name,
                                new ComputeServiceTerminatePilotJobRequestMessage(answer_mailbox, job,
                                                                                  this->getPropertyValueAsDouble(
                                                                                          MultihostMulticoreComputeServiceProperty::TERMINATE_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> cause) {
        throw WorkflowExecutionException(cause);
      }

      // Get the answer
      std::unique_ptr<SimulationMessage> message = nullptr;

      try {
        message = S4U_Mailbox::getMessage(answer_mailbox);
      } catch (std::shared_ptr<NetworkError> cause) {
        throw WorkflowExecutionException(cause);
      }

      if (ComputeServiceTerminatePilotJobAnswerMessage *msg = dynamic_cast<ComputeServiceTerminatePilotJobAnswerMessage *>(message.get())) {
        // If no success, throw an exception
        if (not msg->success) {
          throw WorkflowExecutionException(msg->failure_cause);
        }

      } else {
        throw std::runtime_error(
                "MultihostMulticoreComputeService::terminatePilotJob(): Received an unexpected [" + message->getName() +
                "] message!");
      }
    }


/**
 * @brief Process a standard job termination request
 *
 * @param job: the job to terminate
 * @param answer_mailbox: the mailbox to which the answer message should be sent
 */
    void MultihostMulticoreComputeService::processStandardJobTerminationRequest(StandardJob *job,
                                                                                std::string answer_mailbox) {

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
          } catch (std::shared_ptr<NetworkError> cause) {
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
        } catch (std::shared_ptr<NetworkError> cause) {
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
      } catch (std::shared_ptr<NetworkError> cause) {
        return;
      }
      return;
    }

    /**
     * @brief Process a pilot job termination request
     *
     * @param job: the job to terminate
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     */
    void
    MultihostMulticoreComputeService::processPilotJobTerminationRequest(PilotJob *job, std::string answer_mailbox) {

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

    /**
     * @brief Process a get number of cores request
     *
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     *
     * @throw std::runtime_error
     */
    void MultihostMulticoreComputeService::processGetNumCores(const std::string &answer_mailbox) {
      ComputeServiceNumCoresAnswerMessage *answer_message = new ComputeServiceNumCoresAnswerMessage(
              this->total_num_cores,
              this->getPropertyValueAsDouble(
                      ComputeServiceProperty::NUM_CORES_ANSWER_MESSAGE_PAYLOAD));
      try {
        S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
      } catch (std::shared_ptr<NetworkError> &cause) {
        return;
      }
    }

    /**
     * @brief Process a get number of idle cores request
     *
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     */
    void MultihostMulticoreComputeService::processGetNumIdleCores(const std::string &answer_mailbox) {
      unsigned long num_available_cores = 0;
      for (auto r : this->core_availabilities) {
        num_available_cores += r.second;
      }
      ComputeServiceNumIdleCoresAnswerMessage *answer_message = new ComputeServiceNumIdleCoresAnswerMessage(
              num_available_cores,
              this->getPropertyValueAsDouble(
                      MultihostMulticoreComputeServiceProperty::NUM_IDLE_CORES_ANSWER_MESSAGE_PAYLOAD));
      try {
        S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
      } catch (std::shared_ptr<NetworkError> &cause) {
        return;
      }
    }

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
      unsigned long max_min_num_cores = 0;
      for (auto t : job->getTasks()) {
        max_min_num_cores = MAX(max_min_num_cores, t->getMinNumCores());
      }
      bool enough_resources = false;
      for (auto r : this->compute_resources) {
        unsigned long num_cores = r.second;
        if (num_cores >= max_min_num_cores) {
          enough_resources = true;
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
};

