/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <map>
#include <memory>
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
#include "wrench/workflow/job/StandardJob.h"
#include "wrench/workflow/job/PilotJob.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(multicore_compute_service, "Log category for Multicore Compute Service");

namespace wrench {

    /**
     * @brief Destructor
     */
    MultihostMulticoreComputeService::~MultihostMulticoreComputeService() {
      this->default_property_values.clear();
    }


    /**
     * @brief Helper static function to parse resource specifications to the <cores,ram> format
     * @param spec: specification string
     * @return a <cores, ram> tuple
     * @throw std::invalid_argument
     */
    static std::tuple<std::string, unsigned long> parseResourceSpec(std::string spec) {
      std::vector<std::string> tokens;
      boost::algorithm::split(tokens, spec, boost::is_any_of(":"));
      switch (tokens.size()) {
        case 1: // "num_cores"
        {
          unsigned long num_threads;
          if (sscanf(tokens[1].c_str(), "%lu", &num_threads) != 1) {
            throw std::invalid_argument("Invalid service-specific argument '" + spec + "'");
          }
          return std::make_tuple(std::string(""), num_threads);
        }
        case 2: // "hostname:num_cores"
        {
          unsigned long num_threads;
          if (sscanf(tokens[1].c_str(), "%lu", &num_threads) != 1) {
            throw std::invalid_argument("Invalid service-specific argument '" + spec + "'");
          }
          return std::make_tuple(tokens[0], num_threads);
        }
        default:
        {
          throw std::invalid_argument("Invalid service-specific argument '" + spec +"'");
        }
      }
    }


    /**
     * @brief Submit a standard job to the compute service
     * @param job: a standard job
     * @param service_specific_args: optional service specific arguments
     *
     *    These arguments are provided as a map of strings, indexed by task IDs. These
     *    strings are formatted as "[hostname:]num_cores" (e.g., "somehost:12", "6"). If this
     *    argument is provided  for a task, this will enforce the execution of that task on that host with that
     *    number of cores.  If the hostname is not provided, then the host will be chosen
     *    arbitrarily by the compute service. If the specified number of cores is larger than
     *    the maximum number of cores available on the host, then it will be capped to that number.
     *    If the specified number of cores is larger than the maximum number of cores usable by the
     *    task, then it will be capped to that number. If this argument is not provided at all for
     *    a task, then that task will execute on an arbitrarily chosen host with as many
     *    cores as possible on that host.
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

      /* parse and update service-specific args are provided and well-formatted */
      std::map<std::string, std::string> finalized_service_specific_arguments;

      std::vector<std::string> available_hosts;
      for (auto r: this->compute_resources) {
        available_hosts.push_back(r.first);
      }

      size_t current_picked_host = 0;
      for (auto t : job->getTasks()) {
        std::string target_host;
        unsigned long target_num_cores;

        if (service_specific_args.find(t->getID()) == service_specific_args.end()) {
          // Pick a host
          target_host = available_hosts.at(current_picked_host++);
          // Pick a number of cores
          target_num_cores =
                  std::min<unsigned long>(t->getMaxNumCores(),
                                          std::get<0>(this->compute_resources[target_host]));

        } else {
          std::tuple<std::string, unsigned long> parsed_spec;

          try {
            parsed_spec = parseResourceSpec(service_specific_args[t->getID()]);
          } catch (std::invalid_argument &e) {
            throw;
          }

          target_host = std::get<0>(parsed_spec);
          if (target_host.empty()) {
            target_host = available_hosts.at(current_picked_host++);
          }

          if (this->compute_resources.find(target_host) == this->compute_resources.end()) {
            throw std::invalid_argument(
                    "Invalid service-specific argument '" + service_specific_args[t->getID()] + "' for task '" +
                    t->getID() + "': no such host");
          }
          target_num_cores = std::get<1>(parsed_spec);
        }
        finalized_service_specific_arguments.insert(std::make_pair(t->getID(),
                                                    target_host + ":" + std::to_string(target_num_cores)));
      }

      // At this point, there may still be insufficient resources to run the task, but that will
      // be handled later (and a WorkflowExecutionError with a "not enough resources" FailureCause
      // may be generated.

      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("submit_standard_job");

      //  send a "run a standard job" message to the daemon's mailbox_name
      try {
        S4U_Mailbox::putMessage(this->mailbox_name,
                                new ComputeServiceSubmitStandardJobRequestMessage(
                                        answer_mailbox, job, finalized_service_specific_arguments,
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

      // Compute the total number of cores and set initial ram availabilities
      this->total_num_cores = 0;
      for (auto host : this->compute_resources) {
        this->total_num_cores += std::get<0>(host.second);
        this->ram_availabilities.insert(std::make_pair(host.first, S4U_Simulation::getHostMemoryCapacity(host.first)));
        this->running_thread_counts.insert(std::make_pair(host.first, 0));
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
     * @brief: Dispatch ready work units
     */
    void MultihostMulticoreComputeService::dispatchReadyWorkunits() {

      // Don't kill me while I am doing this
      this->acquireDaemonLock();

      // TODO: TO IMPLEMENT
      for (auto const &j : this->ready_workunits) {
        StandardJob *job = j.first;
        std::set<Workunit*> dispatched_wus_for_job;
        for (auto const &wu : j.second) {
          std::string required_host;
          unsigned long required_num_cores;
          double required_ram;
          if (wu->task == nullptr) {
            required_host = (*(this->compute_resources.begin())).first;
            required_num_cores = 1;
            required_ram = 0.0;
          } else {
            required_host = std::get<0>(this->job_run_specs[job][wu->task]);
            required_num_cores = std::get<1>(this->job_run_specs[job][wu->task]);
            required_ram = wu->task->getMaxNumCores();
          }
          // Check that RAM is ok
          if (this->ram_availabilities[required_host] < required_ram) {
            continue;
          }

          /** Dispatch it */
          std::shared_ptr<WorkunitExecutor> workunit_executor = std::shared_ptr<WorkunitExecutor>(
                  new WorkunitExecutor(this->simulation,
                                       required_host,
                                       required_num_cores,
                                       required_ram,
                                       this->mailbox_name,
                                       wu,
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

          // Update core and RAM availability
          this->ram_availabilities[required_host] -= required_ram;
          this->running_thread_counts[required_host] += required_num_cores;

          dispatched_wus_for_job.insert(wu);
        }

        // Remove the WUs from the ready queue
        for (auto const &wu : dispatched_wus_for_job) {
          this->ready_workunits[job].erase(wu);
        }
      }

      this->releaseDaemonLock();
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

      } else if (auto msg = dynamic_cast<WorkunitExecutorDoneMessage *>(message.get())) {
        processWorkunitExecutorCompletion(msg->workunit_executor, msg->workunit);
        return true;

      } else if (auto msg = dynamic_cast<WorkunitExecutorFailedMessage *>(message.get())) {
        processWorkunitExecutorFailure(msg->workunit_executor, msg->workunit, msg->cause);
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
        for (auto const &f : wue->getFilesStoredInScratch()) {
          this->files_in_scratch.insert(f);
        }
        if (wue->workunit->task) {
          this->ram_availabilities[wue->getHostname()] += wue->workunit->task->getMemoryRequirement();
          this->running_thread_counts[wue->getHostname()] -= wue->getNumCores();
        }
        wue->kill();
      }
      this->workunit_executors[job].clear();
      this->workunit_executors.erase(job);

      /** Remove all relevant work units */
      this->ready_workunits[job].clear();
      this->ready_workunits.erase(job);
      this->running_workunits[job].clear();
      this->running_workunits.erase(job);
      this->completed_workunits[job].clear();
      this->completed_workunits.erase(job);
      this->all_workunits[job].clear();
      this->all_workunits.erase(job);

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

      for (auto job : this->running_jobs) {
        this->failRunningStandardJob(job, std::shared_ptr<FailureCause>(new JobKilled(job, this)));
      }
    }



//    /**
//     * @brief Process a work failure
//     * @param worker_thread: the worker thread that did the work
//     * @param work: the work
//     * @param cause: the cause of the failure
//     */
//    void MultihostMulticoreComputeService::processStandardJobFailure(StandardJobExecutor *executor,
//                                                                     StandardJob *job,
//                                                                     std::shared_ptr<FailureCause> cause) {
//
//      // Update core and ram availabilities
//      for (auto r : executor->getComputeResources()) {
//        std::string hostname = std::get<0>(r);
//        unsigned long num_cores = std::get<1>(r);
//        double ram = std::get<2>(r);
//        std::get<0>(this->core_and_ram_availabilities[hostname]) += num_cores;
//        std::get<1>(this->core_and_ram_availabilities[hostname]) += ram;
//
//      }
//
//      // Remove the executor from the executor list
//      bool found_it = false;
//      for (auto it = this->standard_job_executors.begin();
//           it != this->standard_job_executors.end(); it++) {
//        if ((*it).get() == executor) {
//          PointerUtil::moveSharedPtrFromSetToSet(it, &(this->standard_job_executors), &(this->completed_job_executors));
//          found_it = true;
//          break;
//        }
//      }
//
//      // Remove the executor from the executor list
//      if (!found_it) {
//        throw std::runtime_error(
//                "MultihostMulticoreComputeService::processStandardJobFailure(): Received a standard job completion, but the executor is not in the executor list");
//      }
//
//      // Remove the job from the running job list
//      if (this->running_jobs.find(job) == this->running_jobs.end()) {
//        throw std::runtime_error(
//                "MultihostMulticoreComputeService::processStandardJobFailure(): Received a standard job completion, but the job is not in the running job list");
//      }
//      this->running_jobs.erase(job);
//
//      WRENCH_INFO("A standard job executor has failed to perform job %s", job->getName().c_str());
//
//      // Fail the job
//      this->failRunningStandardJob(job, std::move(cause));
//
//    }

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
     * @brief Process a workunit executor completion
     * @param workunit_executor: the workunit executor
     * @param workunit: the workunit
     */

    void MultihostMulticoreComputeService::processWorkunitExecutorCompletion(WorkunitExecutor *workunit_executor,
                                                                             Workunit *workunit) {
      StandardJob *job = workunit_executor->getJob();

      // Get the scratch files that executor may have generated
      for (auto &f : workunit_executor->getFilesStoredInScratch()) {
        this->files_in_scratch.insert(f);
      }
      // Update RAM availabilities and running thread counts
      if (workunit->task) {
        this->ram_availabilities[workunit_executor->getHostname()] -= workunit->task->getMemoryRequirement();
        this->running_thread_counts[workunit_executor->getHostname()] -= workunit_executor->getNumCores();
      }
      // Forget the workunit executor
      forgetWorkunitExecutor(workunit_executor);

      // Don't kill me while I am doing this
      this->acquireDaemonLock();

      // Process task completions, if any
      if (workunit->task != nullptr) {
        WRENCH_INFO("A workunit executor completed task %s (and its state is: %s)", workunit->task->getID().c_str(),
                    WorkflowTask::stateToString(workunit->task->getInternalState()).c_str());

        // Increase the "completed tasks" count of the job
        job->incrementNumCompletedTasks();
      }

      // Move the workunit from "running" to "completed"
      this->running_workunits[job].erase(workunit);
      this->completed_workunits[job].insert(workunit);

      // Update workunit dependencies if any
      for (auto child : workunit->children) {
        child->num_pending_parents--;
        if (child->num_pending_parents == 0) {
          // Make sure the child's tasks ready (paranoid)
          if (child->task != nullptr) {
            if (child->task->getInternalState() != WorkflowTask::InternalState::TASK_READY) {
              throw std::runtime_error("StandardJobExecutor::processWorkunitExecutorCompletion(): Weird task state " +
                                       std::to_string(child->task->getInternalState()) + " for task " +
                                       child->task->getID());
            }
          }
          // Move the workunit to ready
          this->ready_workunits[job].insert(child);
        }
      }

      this->releaseDaemonLock();

      // If the job not done, just return
      if (this->completed_workunits[job].size() != this->all_workunits[job].size()) {
        return;
      }

      // At this point, the job is done and we can get rid of workunits
      this->completed_workunits[job].clear();
      this->ready_workunits.erase(job);
      this->running_workunits.erase(job);
      this->completed_workunits.erase(job);
      this->all_workunits[job].clear();
      this->all_workunits.erase(job);

      this->job_run_specs.erase(job);

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
    * @brief Process a workunit executor failure
    * @param workunit_executor: the workunit executor
    * @param workunit: the workunit
    * @param cause: the failure cause
    */

    void MultihostMulticoreComputeService::processWorkunitExecutorFailure(WorkunitExecutor *workunit_executor,
                                                                          Workunit *workunit,
                                                                          std::shared_ptr<FailureCause> cause) {
      StandardJob *job = workunit_executor->getJob();

      // Get the scratch files that executor may have generated
      for (auto &f : workunit_executor->getFilesStoredInScratch()) {
        this->files_in_scratch.insert(f);
      }
      // Update RAM availabilities and running thread counts
      if (workunit->task) {
        this->ram_availabilities[workunit_executor->getHostname()] -= workunit->task->getMemoryRequirement();
        this->running_thread_counts[workunit_executor->getHostname()] -= workunit_executor->getNumCores();
      }
      // Forget the workunit executor
      forgetWorkunitExecutor(workunit_executor);

      // Fail the job
      this->failRunningStandardJob(job, std::move(cause));

      this->job_run_specs.erase(job);
    }


    /**
     * @brief Helper function to "forget" a workunit executor (and free memory)
     * @param workunit_executor: the workunit executor
     */
    void MultihostMulticoreComputeService::forgetWorkunitExecutor(WorkunitExecutor *workunit_executor) {

      StandardJob *job = workunit_executor->getJob();
      std::shared_ptr<WorkunitExecutor> found_it;
      for (auto const wue : this->workunit_executors[job]) {
        if (wue.get() == workunit_executor) {
          found_it = wue;
        }
      }
      if (found_it == nullptr) {
        throw std::runtime_error("MultihostMulticoreComputeService::processWorkunitExecutorCompletion(): Couldn't find workunit executor");
      }
      this->workunit_executors[job].erase(found_it);

    }



    /**
     * @brief Process a standard job termination request
     *
     * @param job: the job to terminate
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     */
    void MultihostMulticoreComputeService::processStandardJobTerminationRequest(StandardJob *job,
                                                                                const std::string &answer_mailbox) {

      // If the job doesn't exit, we reply right away
      if (this->all_workunits.find(job) == this->all_workunits.end()) {
        WRENCH_INFO("Trying to terminate a standard job that's not (no longer?) running!");
        ComputeServiceTerminateStandardJobAnswerMessage *answer_message = new ComputeServiceTerminateStandardJobAnswerMessage(
                job, this, false, std::shared_ptr<FailureCause>(new JobCannotBeTerminated(job)),
                this->getMessagePayloadValueAsDouble(
                        MultihostMulticoreComputeServiceMessagePayload::TERMINATE_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD));
        try {
          S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
        } catch (std::shared_ptr<NetworkError> &cause) {
          return;
        }
        return;
      }

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


      // Can we run each task in this job?
      std::map<WorkflowTask *, std::tuple<std::string, unsigned long>> task_run_specs;
      bool enough_resources = true;
      for (auto t : job->getTasks()) {
        std::string spec = service_specific_arguments[t->getID()];
        std::tuple<std::string, unsigned long> parsed_spec = parseResourceSpec(spec);

        std::string required_host = std::get<0>(parsed_spec);
        unsigned long required_num_cores = std::get<1>(parsed_spec);
        double required_ram = t->getMemoryRequirement();

        task_run_specs.insert(std::make_pair(t, std::make_tuple(required_host, required_num_cores)));
        if (std::get<0>(this->compute_resources[required_host]) < required_num_cores) {
          enough_resources = false;
        }
        if (std::get<1>(this->compute_resources[required_host]) < required_ram) {
          enough_resources = false;
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

      // We can now admit the job!
      this->all_workunits.insert(std::make_pair(job, Workunit::createWorkunits(job)));
      this->job_run_specs.insert(std::make_pair(job, task_run_specs));

      // Add the ready ones to the ready list
      std::set<Workunit*> ready;
      for (auto const &wu: this->all_workunits[job]) {
        if (wu->num_pending_parents == 0) {
          ready.insert(wu.get());
        }
      }
      this->ready_workunits.insert(std::make_pair(job, ready));

      // And send a reply!
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
      std::map<std::string, std::map<std::string, double>> dict;

      // Num hosts
      std::map<std::string, double> num_hosts;
      num_hosts.insert(std::make_pair(this->getName(), this->compute_resources.size()));
      dict.insert(std::make_pair("num_hosts", num_hosts));

      // Num cores per hosts
      std::map<std::string, double> num_cores;
      for (auto r : this->compute_resources) {
        num_cores.insert(std::make_pair(r.first, (double) (std::get<0>(r.second))));
      }
      dict.insert(std::make_pair("num_cores", num_cores));

      // Num idle cores per hosts
      std::map<std::string, double> num_idle_cores;
      for (auto r : this->running_thread_counts) {
        unsigned long cores = std::get<0>(this->compute_resources[r.first]);
        unsigned long running_threads = r.second;
        num_idle_cores.insert(std::make_pair(r.first, (double) (std::max<unsigned long>(cores - running_threads, 0))));
      }
      dict.insert(std::make_pair("num_idle_cores", num_idle_cores));

      // Flop rate per host
      std::map<std::string, double> flop_rates;
      for (auto h : this->compute_resources) {
        flop_rates.insert(std::make_pair(h.first, S4U_Simulation::getHostFlopRate(std::get<0>(h))));
      }
      dict.insert(std::make_pair("flop_rates", flop_rates));

      // RAM capacity per host
      std::map<std::string, double> ram_capacities;
      for (auto h : this->compute_resources) {
        ram_capacities.insert(std::make_pair(h.first, S4U_Simulation::getHostMemoryCapacity(std::get<0>(h))));
      }
      dict.insert(std::make_pair("ram_capacities", ram_capacities));

      // RAM availability per host
      std::map<std::string, double> ram_availabilities;
      for (auto r : this->ram_availabilities) {
        ram_availabilities.insert(std::make_pair(r.first, r.second));
      }
      dict.insert(std::make_pair("ram_availabilities", ram_availabilities));

      std::map<std::string, double> ttl;
      if (this->has_ttl) {
        ttl.insert(std::make_pair(this->getName(), this->death_date - S4U_Simulation::getClock()));
      } else {
        ttl.insert(std::make_pair(this->getName(), DBL_MAX));
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
 * @brief Cleans up the scratch as I am a pilot job and I to need clean the files stored by the standard jobs
 *        executed inside me
 */
    void MultihostMulticoreComputeService::cleanUpScratch() {

      for (auto &f : this->files_in_scratch) {
        try {
          getScratch()->deleteFile(f, this->containing_pilot_job, nullptr);
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

      // Supporting Pilot jobs
      if (this->getPropertyValueAsBoolean(MultihostMulticoreComputeServiceProperty::SUPPORTS_PILOT_JOBS)) {
        throw std::invalid_argument("Invalid SUPPORTS_PILOT_JOBS property specification: a BareMetalService cannot support pilot jobs");
      }

    }

    /**
     * @brief non-implemented
     * @param job: a pilot job to (supposedly) terminate
     */
    void MultihostMulticoreComputeService::terminatePilotJob(PilotJob *job) {
      throw std::runtime_error("MultihostMulticoreComputeService::terminatePilotJob(): not implemented because MultihostMulticoreComputeService never supports pilot jobs");
    }


};
