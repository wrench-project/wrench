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
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

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
     * @param service_specific_args: service specific arguments ({} most likely)
     *
     * @throw WorkflowExecutionException
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    void MultihostMulticoreComputeService::submitStandardJob(StandardJob *job,
                                                             std::map<std::string, std::string> &service_specific_args) {

      if (this->state == Service::DOWN) {
        throw WorkflowExecutionException(std::shared_ptr<FailureCause>(new ServiceIsDown(this)));
      }

      /* Check that the service-specific args are specific and well-formatted */
      for (auto t : job->getTasks()) {
        if (service_specific_args.find(t->getID()) == service_specific_args.end()) {
          throw std::invalid_argument("Service-specific argument map should contain an entry for task '" + t->getID() + "'");
        }
        std::string arg = service_specific_args[t->getID()];
        std::vector<std::string> tokens;
        boost::algorithm::split(tokens, arg, boost::is_any_of(":"));
        if (tokens.size() != 2) {
          throw std::invalid_argument("Invalid service-specific argument '" + arg + "' for task '" + t->getID() + "'");
        }
        std::string hostname = tokens[0];
        std::string string_num_threads = tokens[1];
        unsigned long num_threads;

        if (sscanf(string_num_threads.c_str(), "%lu", &num_threads) != 1) {
          throw std::invalid_argument("Invalid service-specific argument '" + arg + "' for task '" + t->getID() + "'");
        }

        if (this->compute_resources.find(hostname) == this->compute_resources.end()) {
          throw std::invalid_argument("Invalid service-specific argument  host '" + arg + "' for task '" + t->getID() + "': no such host");
        }

        // At this point, there may still be insufficient resources to run the task, but that will
        // be handled later (and a WorkflowExecutionError with a "not enough resources" FailureCause
        // may be generated.

      }

      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("submit_standard_job");

      //  send a "run a standard job" message to the daemon's mailbox_name
      try {
        S4U_Mailbox::putMessage(this->mailbox_name,
                                new ComputeServiceSubmitStandardJobRequestMessage(
                                        answer_mailbox, job, service_specific_args,
                                        this->getMessagePayloadValueAsDouble(
                                                ComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }

      // Get the answer
      std::unique_ptr<SimulationMessage> message = nullptr;
      try {
        message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
      } catch (std::shared_ptr<NetworkError> &cause) {
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
      this->default_property_values.clear();
    }

    /**
     * @brief Asynchronously submit a pilot job to the compute service
     *
     * @param job: a pilot job
     * @param service_specific_args: service specific arguments ({} most likely)
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    void
    MultihostMulticoreComputeService::submitPilotJob(PilotJob *job,
                                                     std::map<std::string, std::string> &service_specific_args) {

      if (this->state == Service::DOWN) {
        throw WorkflowExecutionException(std::shared_ptr<FailureCause>(new ServiceIsDown(this)));
      }

      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("submit_pilot_job");

      // Send a "run a pilot job" message to the daemon's mailbox_name
      try {
        S4U_Mailbox::putMessage(
                this->mailbox_name,
                new ComputeServiceSubmitPilotJobRequestMessage(
                        answer_mailbox, job, service_specific_args, this->getMessagePayloadValueAsDouble(
                                MultihostMulticoreComputeServiceMessagePayload::SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }

      // Wait for a reply
      std::unique_ptr<SimulationMessage> message = nullptr;

      try {
        message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
      } catch (std::shared_ptr<NetworkError> &cause) {
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
     * @param hostname: the name of the host on which the service should be started
     * @param compute_resources: a map of <num_cores, memory> tuples, indexed by hostname, which represents
     *        the compute resources available to this service.
     *          - use num_cores = ComputeService::ALL_CORES to use all cores available on the host
     *          - use memory = ComputeService::ALL_RAM to use all RAM available on the host
     * @param scratch_space_size: size (in bytes) of the compute service's scratch storage paste
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     */
    MultihostMulticoreComputeService::MultihostMulticoreComputeService(
            const std::string &hostname,
            std::map<std::string, std::tuple<unsigned long, double>> compute_resources,
            double scratch_space_size,
            std::map<std::string, std::string> property_list,
            std::map<std::string, std::string> messagepayload_list
    ) :
            ComputeService(hostname,
                           "multihost_multicore",
                           "multihost_multicore",
                           scratch_space_size) {

      initiateInstance(hostname,
                       std::move(compute_resources),
                       std::move(property_list), std::move(messagepayload_list), DBL_MAX, nullptr);
    }

    /**
     * @brief Constructor
     *
     * @param hostname: the name of the host on which the service should be started
     * @param compute_hosts:: the names of the hosts available as compute resources (the service
     *        will use all the cores and all the RAM of each host)
     * @param scratch_space_size: size (in bytes) of the compute service's scratch storage paste
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     */
    MultihostMulticoreComputeService::MultihostMulticoreComputeService(const std::string &hostname,
                                                                       const std::set<std::string> compute_hosts,
                                                                       double scratch_space_size,
                                                                       std::map<std::string, std::string> property_list,
                                                                       std::map<std::string, std::string> messagepayload_list
    ) :
            ComputeService(hostname,
                           "multihost_multicore",
                           "multihost_multicore",
                           scratch_space_size) {

      std::map<std::string, std::tuple<unsigned long, double>> compute_resources;
      for (auto h : compute_hosts) {
        compute_resources.insert(std::make_pair(h, std::make_tuple(ComputeService::ALL_CORES, ComputeService::ALL_RAM)));
      }

      initiateInstance(hostname,
                       compute_resources,
                       std::move(property_list), std::move(messagepayload_list), DBL_MAX, nullptr);
    }

    /**
     * @brief Internal constructor
     *
     * @param hostname: the name of the host on which the service should be started
     * @param compute_resources: a list of <hostname, num_cores, memory> tuples, which represent
     *        the compute resources available to this service
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     * @param ttl: the time-to-live, in seconds (DBL_MAX: infinite time-to-live)
     * @param pj: a containing PilotJob  (nullptr if none)
     * @param suffix: a string to append to the process name
     *
     * @throw std::invalid_argument
     */
    MultihostMulticoreComputeService::MultihostMulticoreComputeService(
            const std::string &hostname,
            std::map<std::string, std::tuple<unsigned long, double>> compute_resources,
            std::map<std::string, std::string> property_list,
            std::map<std::string, std::string> messagepayload_list,
            double ttl,
            PilotJob *pj,
            std::string suffix, StorageService *scratch_space) : ComputeService(hostname,
                                                                                "multihost_multicore" + suffix,
                                                                                "multihost_multicore" + suffix,
                                                                                scratch_space) {

      initiateInstance(hostname,
                       std::move(compute_resources),
                       std::move(property_list),
                       std::move(messagepayload_list),
                       ttl,
                       pj);
    }

    /**
    * @brief Internal constructor
    *
    * @param hostname: the name of the host on which the job executor should be started
    * @param compute_hosts:: a list of <hostname, num_cores, memory> tuples, which represent
    *        the compute resources available to this service
    * @param property_list: a property list ({} means "use all defaults")
    * @param messagepayload_list: a message payload list ({} means "use all defaults")
    * @param scratch_space: the scratch space for this compute service
    */
    MultihostMulticoreComputeService::MultihostMulticoreComputeService(const std::string &hostname,
                                                                       const std::set<std::string> compute_hosts,
                                                                       std::map<std::string, std::string> property_list,
                                                                       std::map<std::string, std::string> messagepayload_list,
                                                                       StorageService *scratch_space) :
            ComputeService(hostname,
                           "multihost_multicore",
                           "multihost_multicore",
                           scratch_space) {

      std::map<std::string, std::tuple<unsigned long, double>> compute_resources;
      for (auto h : compute_hosts) {
        compute_resources.insert(std::make_pair(h, std::make_tuple(ComputeService::ALL_CORES, ComputeService::ALL_RAM)));
      }

      initiateInstance(hostname,
                       compute_resources,
                       std::move(property_list), std::move(messagepayload_list), DBL_MAX, nullptr);

    }

    /**
     * @brief Internal constructor
     *
     * @param hostname: the name of the host on which the job executor should be started
     * @param compute_hosts:: a list of <hostname, num_cores, memory> tuples, which represent
     *        the compute resources available to this service
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     * @param scratch_space: the scratch space for this compute service
     */
    MultihostMulticoreComputeService::MultihostMulticoreComputeService(const std::string &hostname,
                                                                       const std::map<std::string, std::tuple<unsigned long, double>> compute_resources,
                                                                       std::map<std::string, std::string> property_list,
                                                                       std::map<std::string, std::string> messagepayload_list,
                                                                       StorageService *scratch_space) :
            ComputeService(hostname,
                           "multihost_multicore",
                           "multihost_multicore",
                           scratch_space) {

      initiateInstance(hostname,
                       compute_resources,
                       std::move(property_list), std::move(messagepayload_list), DBL_MAX, nullptr);

    }

    /**
     * @brief Helper method called by all constructors to initiate object instance
     *
     * @param hostname: the name of the host
     * @param compute_resources: compute_resources: a map of <num_cores, memory> pairs, indexed by hostname, which represent
     *        the compute resources available to this service
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     * @param ttl: the time-to-live, in seconds (DBL_MAX: infinite time-to-live)
     * @param pj: a containing PilotJob  (nullptr if none)
     *
     * @throw std::invalid_argument
     */
    void MultihostMulticoreComputeService::initiateInstance(
            const std::string &hostname,
            std::map<std::string, std::tuple<unsigned long, double>> compute_resources,
            std::map<std::string, std::string> property_list,
            std::map<std::string, std::string> messagepayload_list,
            double ttl,
            PilotJob *pj) {

      if (ttl < 0) {
        throw std::invalid_argument(
                "MultihostMulticoreComputeService::initiateInstance(): invalid TTL value (must be >0)");
      }

      // Set default and specified properties
      this->setProperties(this->default_property_values, std::move(property_list));

      // Validate that properties are correct
      this->validateProperties();

      // Set default and specified message payloads
      this->setMessagePayloads(this->default_messagepayload_values, std::move(messagepayload_list));

      // Check that there is at least one core per host and that hosts have enough cores
      for (auto host : compute_resources) {
        std::string hname = host.first;
        unsigned long requested_cores = std::get<0>(host.second);
        unsigned long available_cores;
        try {
          available_cores = S4U_Simulation::getHostNumCores(hname);
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

        double requested_ram = std::get<0>(host.second);
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

        this->compute_resources.insert(std::make_pair(hname, std::make_tuple(requested_cores, requested_ram)));
      }

      // Compute the total number of cores and set initial core (and ram) availabilities
      this->total_num_cores = 0;
      for (auto host : this->compute_resources) {
        this->total_num_cores += std::get<0>(host.second);
        this->core_and_ram_availabilities.insert(std::make_pair(
                host.first, std::make_pair(std::get<0>(host.second),
                                                  S4U_Simulation::getHostMemoryCapacity(
                                                                  host.first))));
      }

      this->ttl = ttl;
      this->has_ttl = (this->ttl != DBL_MAX);
      this->containing_pilot_job = pj;

    }

    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int MultihostMulticoreComputeService::main() {

      TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_RED);

      WRENCH_INFO("New MultihostMulticore Compute Service starting (%s) on %ld hosts with a total of %ld cores",
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

        /** Dispatch ready work units **/
        this->dispatchReadyWorkunits();
      }

      WRENCH_INFO("Multicore Job Executor on host %s terminated!", S4U_Simulation::getHostName().c_str());
      return 0;
    }

    /**
     *
     */
    void MultihostMulticoreComputeService::dispatchReadyWorkunits() {

      for (auto const &wus : this->all_workunits) {
        WorkflowJob *job = wus.first;
        for (auto const &wu: wus.second) {

          /** If the task is not ready, continue */
          if (wu->num_pending_parents != 0) {
            continue;
          }

          /** Figure out what resources it needs */
          std::string required_host;
          unsigned long required_num_cores;
          double required_ram;
          // If it contains a task, then get the submitted service-specific argument
          if (wu->task) {
            if (job->getServiceSpecificArguments().find(wu->task->getID()) == job->getServiceSpecificArguments().end()) {
              throw std::runtime_error("MultihostMulticoreComputeService::dispatchReadyWorkunits(): Invalid job-specific argument: no host found for task '" + wu->task->getID() + "'");
            }
            std::string resource_spec = job->getServiceSpecificArguments()[wu->task->getID()];
            std::vector<std::string> tokens;
            boost::algorithm::split(tokens, resource_spec, boost::is_any_of(":"));
            required_host = tokens[0];
            sscanf(tokens[1].c_str(), "%lu", &required_num_cores);
            required_ram = wu->task->getMemoryRequirement();
          } else {
            required_host = (*(this->compute_resources.begin())).first;  // some arbitrary host for non-compute intentive stuff
            required_num_cores = 1;
            required_ram = 0;
          }

          /** Dispatch it */
          std::shared_ptr<WorkunitExecutor> workunit_executor = std::shared_ptr<WorkunitExecutor>(
                  new WorkunitExecutor(this->simulation,
                                       required_host,
                                       required_num_cores,
                                       required_ram,
                                       this->mailbox_name,
                                       wu.get(),
                                       this->getScratch(),
                                       job,
                                       this->getPropertyValueAsDouble(
                                               MultihostMulticoreComputeServiceProperty::THREAD_STARTUP_OVERHEAD),
                                       false
                  ));

          workunit_executor->simulation = this->simulation;
          workunit_executor->start(workunit_executor, true);

          // Keep track of this workunit executor
          this->workunit_executors[job].insert(workunit_executor);
        }
      }
    }


    /**
     * @brief Wait for and react to any incoming message
     *
     * @return false if the daemon should terminate, true otherwise
     *
     * @throw std::runtime_error
     */
    bool MultihostMulticoreComputeService::processNextMessage() {

      S4U_Simulation::computeZeroFlop();

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
        if (this->containing_pilot_job != nullptr) {
          /*** Clean up everything in the scratch space ***/
          cleanUpScratch();
        }

        this->terminate(true);

        return false;

      } else if (auto msg = dynamic_cast<ServiceStopDaemonMessage *>(message.get())) {
        if (this->containing_pilot_job != nullptr) {
          /*** Clean up everything in the scratch space ***/
          cleanUpScratch();
        }
        this->terminate(false);

        // This is Synchronous
        try {
          S4U_Mailbox::putMessage(msg->ack_mailbox,
                                  new ServiceDaemonStoppedMessage(this->getMessagePayloadValueAsDouble(
                                          MultihostMulticoreComputeServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
          return false;
        }
        return false;

      } else if (auto msg = dynamic_cast<ComputeServiceSubmitStandardJobRequestMessage *>(message.get())) {
        processSubmitStandardJob(msg->answer_mailbox, msg->job, msg->service_specific_args);
        return true;
      } else if (auto msg = dynamic_cast<ComputeServiceSubmitPilotJobRequestMessage *>(message.get())) {
        processSubmitPilotJob(msg->answer_mailbox, msg->job, msg->service_specific_args);
        return true;
      } else if (auto *msg = dynamic_cast<ComputeServiceResourceInformationRequestMessage *>(message.get())) {
        processGetResourceInformation(msg->answer_mailbox);
        return true;

      } else if (auto *msg = dynamic_cast<ComputeServiceTerminateStandardJobRequestMessage *>(message.get())) {
        processStandardJobTerminationRequest(msg->job, msg->answer_mailbox);
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
     * @brief fail a running standard job
     * @param job: the job
     * @param cause: the failure cause
     */
    void
    MultihostMulticoreComputeService::failRunningStandardJob(StandardJob *job, std::shared_ptr<FailureCause> cause) {

      WRENCH_INFO("Failing running job %s", job->getName().c_str());

      terminateRunningStandardJob(job, MultihostMulticoreComputeService::JobTerminationCause::COMPUTE_SERVICE_KILLED);

      // Send back a job failed message (Not that it can be a partial fail)
      WRENCH_INFO("Sending job failure notification to '%s'", job->getCallbackMailbox().c_str());
      // NOTE: This is synchronous so that the process doesn't fall off the end
      try {
        S4U_Mailbox::putMessage(job->popCallbackMailbox(),
                                new ComputeServiceStandardJobFailedMessage(
                                        job, this, cause, this->getMessagePayloadValueAsDouble(
                                                MultihostMulticoreComputeServiceMessagePayload::STANDARD_JOB_FAILED_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        return;
      }
    }

    /**
    * @brief terminate a running standard job
    * @param job: the job
    */
    void MultihostMulticoreComputeService::terminateRunningStandardJob(StandardJob *job, MultihostMulticoreComputeService::JobTerminationCause termination_cause) {

      /** Kill all relevant work unit executors */
      for (auto const &wue : this->workunit_executors[job]) {
        wue->kill();
      }
      this->workunit_executors[job].clear();
      this->workunit_executors.erase(job);

      /** Remove all relevant work units */
      this->all_workunits[job].clear();

      /** Deal with task states */
      for (auto &task : job->getTasks()) {
          if (task->getInternalState() == WorkflowTask::InternalState::TASK_RUNNING) {
              if (termination_cause == MultihostMulticoreComputeService::JobTerminationCause::TERMINATED) {
                  task->setTerminationDate(S4U_Simulation::getClock());
                  this->simulation->getOutput().addTimestamp<SimulationTimestampTaskTerminated>(new SimulationTimestampTaskTerminated(task));

              } else if (termination_cause == MultihostMulticoreComputeService::JobTerminationCause::COMPUTE_SERVICE_KILLED) {
                  task->setFailureDate(S4U_Simulation::getClock());
                  this->simulation->getOutput().addTimestamp<SimulationTimestampTaskFailure>(new SimulationTimestampTaskFailure(task));
              }
          }
      }

      for (auto failed_task: job->getTasks()) {
        switch (failed_task->getInternalState()) {
          case WorkflowTask::InternalState::TASK_NOT_READY:
          case WorkflowTask::InternalState::TASK_READY:
          case WorkflowTask::InternalState::TASK_COMPLETED:
            break;

          case WorkflowTask::InternalState::TASK_RUNNING:
            throw std::runtime_error(
                    "MultihostMulticoreComputeService::terminateRunningStandardJob(): task state shouldn't be 'RUNNING'"
                    "after a StandardJobExecutor was killed!");
          case WorkflowTask::InternalState::TASK_FAILED:
            // Making failed task READY again!!!
            failed_task->setInternalState(WorkflowTask::InternalState::TASK_READY);
            break;

          default:
            throw std::runtime_error(
                    "MultihostMulticoreComputeService::terminateRunningStandardJob(): unexpected task state");

        }
      }
    }


    /**
    * @brief Declare all current jobs as failed (likely because the daemon is being terminated
    * or has timed out (because it's in fact a pilot job))
    */
    void MultihostMulticoreComputeService::failCurrentStandardJobs() {

      for (auto workflow_job : this->running_jobs) {
        if (workflow_job->getType() == WorkflowJob::STANDARD) {
          auto job = (StandardJob *) workflow_job;
          this->failRunningStandardJob(job, std::shared_ptr<FailureCause>(new JobKilled(workflow_job, this)));
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
                        job, this, this->getMessagePayloadValueAsDouble(
                                MultihostMulticoreComputeServiceMessagePayload::STANDARD_JOB_DONE_MESSAGE_PAYLOAD)));
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
                "MultihostMulticoreComputeService::processStandardJobFailure(): Received a standard job completion, but the executor is not in the executor list");
      }

      // Remove the job from the running job list
      if (this->running_jobs.find(job) == this->running_jobs.end()) {
        throw std::runtime_error(
                "MultihostMulticoreComputeService::processStandardJobFailure(): Received a standard job completion, but the job is not in the running job list");
      }
      this->running_jobs.erase(job);

      WRENCH_INFO("A standard job executor has failed to perform job %s", job->getName().c_str());

      // Fail the job
      this->failRunningStandardJob(job, std::move(cause));

    }

    /**
     * @brief Terminate the daemon, dealing with pending/running jobs
     *
     * @param notify_pilot_job_submitters:
     */
    void MultihostMulticoreComputeService::terminate(bool notify_pilot_job_submitters) {

      this->setStateToDown();

      WRENCH_INFO("Failing current standard jobs");
      this->failCurrentStandardJobs();

      // Am I myself a pilot job?
      if (notify_pilot_job_submitters && this->containing_pilot_job) {

        WRENCH_INFO("Letting the level above know that the pilot job has ended on mailbox_name %s",
                    this->containing_pilot_job->getCallbackMailbox().c_str());
        // NOTE: This is synchronous so that the process doesn't fall off the end
        try {
          S4U_Mailbox::putMessage(this->containing_pilot_job->popCallbackMailbox(),
                                  new ComputeServicePilotJobExpiredMessage(
                                          this->containing_pilot_job, this,
                                          this->getMessagePayloadValueAsDouble(
                                                  MultihostMulticoreComputeServiceMessagePayload::PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
          return;
        }
      }
    }



    /**
     * @brief Synchronously terminate a standard job previously submitted to the compute service
     *
     * @param job: a standard job
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    void MultihostMulticoreComputeService::terminateStandardJob(StandardJob *job) {

      if (this->state == Service::DOWN) {
        throw WorkflowExecutionException(std::shared_ptr<FailureCause>(new ServiceIsDown(this)));
      }

      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("terminate_standard_job");

      //  send a "terminate a standard job" message to the daemon's mailbox_name
      try {
        S4U_Mailbox::putMessage(this->mailbox_name,
                                new ComputeServiceTerminateStandardJobRequestMessage(
                                        answer_mailbox, job, this->getMessagePayloadValueAsDouble(
                                                MultihostMulticoreComputeServiceMessagePayload::TERMINATE_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }

      // Get the answer
      std::unique_ptr<SimulationMessage> message = nullptr;
      try {
        message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
      } catch (std::shared_ptr<NetworkError> &cause) {
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
                  this->getMessagePayloadValueAsDouble(
                          MultihostMulticoreComputeServiceMessagePayload::TERMINATE_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD));
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
        terminateRunningStandardJob(job, MultihostMulticoreComputeService::JobTerminationCause::TERMINATED);
        // reply
        ComputeServiceTerminateStandardJobAnswerMessage *answer_message = new ComputeServiceTerminateStandardJobAnswerMessage(
                job, this, true, nullptr,
                this->getMessagePayloadValueAsDouble(
                        MultihostMulticoreComputeServiceMessagePayload::TERMINATE_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD));
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
              this->getMessagePayloadValueAsDouble(
                      MultihostMulticoreComputeServiceMessagePayload::TERMINATE_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD));
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
                          this->getMessagePayloadValueAsDouble(
                                  ComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));
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
          unsigned long num_cores = std::get<0>(r.second);
          double ram = std::get<1>(r.second);
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
                          job, this, false, std::shared_ptr<FailureCause>(new NotEnoughResources(job, this)),
                          this->getMessagePayloadValueAsDouble(
                                  MultihostMulticoreComputeServiceMessagePayload::NOT_ENOUGH_CORES_MESSAGE_PAYLOAD)));
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
                        job, this, true, nullptr, this->getMessagePayloadValueAsDouble(
                                ComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));
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
                                                                 PilotJob *job,
                                          std::map<std::string, std::string> service_specific_args) {
      WRENCH_INFO("Asked to run a pilot job with %ld hosts and %ld cores per host for %lf seconds",
                  job->getNumHosts(), job->getNumCoresPerHost(), job->getDuration());

      if (not this->supportsPilotJobs()) {
        try {
          S4U_Mailbox::dputMessage(
                  answer_mailbox, new ComputeServiceSubmitPilotJobAnswerMessage(
                          job, this, false, std::shared_ptr<FailureCause>(new JobTypeNotSupported(job, this)),
                          this->getMessagePayloadValueAsDouble(
                                  MultihostMulticoreComputeServiceMessagePayload::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
          return;
        }
        return;
      }

      throw std::runtime_error(
              "MultihostMulticoreComputeService::processSubmitPilotJob(): We shouldn't be here! (fatal)");
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
        num_cores.push_back((double) (std::get<0>(r.second)));
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
        flop_rates.push_back(S4U_Simulation::getHostFlopRate(std::get<0>(h)));
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
              this->getMessagePayloadValueAsDouble(
                      ComputeServiceMessagePayload::RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD));
      try {
        S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
      } catch (std::shared_ptr<NetworkError> &cause) {
        return;
      }
    }

    /**
     * @brief Add the scratch files of one standardjob to the list of all the scratch files of all the standard jobs
     *        inside the pilot job
     *
     * @param scratch_files:
     */
    void MultihostMulticoreComputeService::storeFilesStoredInScratch(std::set<WorkflowFile *> scratch_files) {
      this->files_in_scratch.insert(scratch_files.begin(), scratch_files.end());
    }

    /**
     * @brief Cleans up the scratch as I am a pilot job and I need clean the files stored by the standard jobs
     *        executed inside me
     */
    void MultihostMulticoreComputeService::cleanUpScratch() {
      // First fetch all the files stored in scratch by all the workunit executors running inside a standardjob
      // Files in scratch by finished workunit executors
      for (const auto &completed_job_executor : this->completed_job_executors) {
        auto files_in_scratch_by_single_workunit = completed_job_executor->getFilesInScratch();
        this->files_in_scratch.insert(files_in_scratch_by_single_workunit.begin(),
                                      files_in_scratch_by_single_workunit.end());
      }

      for (auto scratch_cleanup_file : this->files_in_scratch) {
        try {
          getScratch()->deleteFile(scratch_cleanup_file, this->containing_pilot_job, nullptr);
        } catch (WorkflowExecutionException &e) {
          throw;
        }
      }
    }

    /**
     * @brief Method to make sure that property specs are valid
     *
     * @throw std::invalid_argument
     */
    void MultihostMulticoreComputeService::validateProperties() {

      bool success = true;

      // Thread startup overhead
      double thread_startup_overhead = 0;
      success = true;
      try {
        thread_startup_overhead = this->getPropertyValueAsDouble(MultihostMulticoreComputeServiceProperty::THREAD_STARTUP_OVERHEAD);
      } catch (std::invalid_argument &e) {
        success = false;
      }

      if ((!success) or (thread_startup_overhead < 0)) {
        throw std::invalid_argument("Invalid THREAD_STARTUP_OVERHEAD property specification: " +
                        this->getPropertyValueAsString(MultihostMulticoreComputeServiceProperty::THREAD_STARTUP_OVERHEAD));
      }

      // Job selection policy
      if (this->getPropertyValueAsString(MultihostMulticoreComputeServiceProperty::JOB_SELECTION_POLICY) != "FCFS") {
        throw std::invalid_argument("Invalid JOB_SELECTION_POLICY property specification: " +
                        this->getPropertyValueAsString(MultihostMulticoreComputeServiceProperty::JOB_SELECTION_POLICY));
      }

      // Resource allocation policy
      if (this->getPropertyValueAsString(MultihostMulticoreComputeServiceProperty::RESOURCE_ALLOCATION_POLICY) != "aggressive") {
        throw std::invalid_argument("Invalid RESOURCE_ALLOCATION_POLICY property specification: " +
                                    this->getPropertyValueAsString(MultihostMulticoreComputeServiceProperty::RESOURCE_ALLOCATION_POLICY));
      }

      // Core allocation algorithm
      if ((this->getPropertyValueAsString(MultihostMulticoreComputeServiceProperty::TASK_SCHEDULING_CORE_ALLOCATION_ALGORITHM) != "maximum") and
          (this->getPropertyValueAsString(MultihostMulticoreComputeServiceProperty::TASK_SCHEDULING_CORE_ALLOCATION_ALGORITHM) != "minimum")) {
        throw std::invalid_argument("Invalid TASK_SCHEDULING_CORE_ALLOCATION_ALGORITHM property specification: " +
                                    this->getPropertyValueAsString(MultihostMulticoreComputeServiceProperty::TASK_SCHEDULING_CORE_ALLOCATION_ALGORITHM));
      }

      // Task selection algorithm
      if ((this->getPropertyValueAsString(MultihostMulticoreComputeServiceProperty::TASK_SCHEDULING_TASK_SELECTION_ALGORITHM) != "maximum_flops") and
          (this->getPropertyValueAsString(MultihostMulticoreComputeServiceProperty::TASK_SCHEDULING_TASK_SELECTION_ALGORITHM) != "maximum_minimum_cores")) {
        throw std::invalid_argument("Invalid TASK_SCHEDULING_CORE_ALLOCATION_ALGORITHM property specification: " +
                                    this->getPropertyValueAsString(MultihostMulticoreComputeServiceProperty::TASK_SCHEDULING_TASK_SELECTION_ALGORITHM));
      }

      // Host selection algorithm
      if (this->getPropertyValueAsString(MultihostMulticoreComputeServiceProperty::TASK_SCHEDULING_HOST_SELECTION_ALGORITHM) != "best_fit") {
        throw std::invalid_argument("Invalid TASK_SCHEDULING_HOST_SELECTION_ALGORITHM property specification: " +
                                    this->getPropertyValueAsString(MultihostMulticoreComputeServiceProperty::TASK_SCHEDULING_HOST_SELECTION_ALGORITHM));
      }

      // Supporting Pilot jobs
      if (this->getPropertyValueAsBoolean(MultihostMulticoreComputeServiceProperty::SUPPORTS_PILOT_JOBS)) {
        throw std::invalid_argument("Invalid SUPPORTS_PILOT_JOBS property specification: a BareMetalService cannot support pilot jobs");
      }

      return;
    }




};
