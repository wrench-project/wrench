/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include "wrench/logging/TerminalOutput.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
#include "wrench/services/compute/batch/BatchService.h"
#include <simgrid_S4U_util/S4U_Mailbox.h>
#include <services/ServiceMessage.h>
#include <wrench/services/compute/multihost_multicore/MultihostMulticoreComputeService.h>
#include <wrench/util/PointerUtil.h>
#include "wrench/services/helpers/Alarm.h"
#include "wrench/exceptions/WorkflowExecutionException.h"
#include "services/compute/ComputeServiceMessage.h"
#include "services/compute/standard_job_executor/StandardJobExecutorMessage.h"
#include "wrench/simulation/Simulation.h"
#include "wrench/services/storage/StorageService.h"
#include "wrench/workflow/job/PilotJob.h"
#include "wrench/services/compute/batch/BatchServiceMessage.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(batch_service, "Log category for Batch Service");

namespace wrench {

    BatchService::~BatchService() {
        WRENCH_INFO("In the Batch Service destructor\n");
        this->default_property_values.clear();
    }

    /**
     * @brief Constructor
     * @param hostname: the hostname on which to start the service
     * @param compute_nodes: the hosts available for running jobs submitted to the service
     * @param supports_standard_jobs: true if the compute service should support standard jobs
     * @param supports_pilot_jobs: true if the compute service should support pilot jobs
     * @param plist: a property list ({} means "use all defaults")
     */
    BatchService::BatchService(std::string hostname,
                               std::vector<std::string> compute_nodes,
                               StorageService *default_storage_service,
                               bool supports_standard_jobs,
                               bool supports_pilot_jobs,
                               std::map<std::string, std::string> plist) :
            BatchService(std::move(hostname),std::move(compute_nodes), default_storage_service, supports_standard_jobs,
                         supports_pilot_jobs, 0, std::move(plist), "") {

    }

    /**
     * @brief Constructor
     * @param hostname: the hostname on which to start the service
     * @param nodes_in_network: the hosts running in the network
     * @param supports_standard_jobs: true if the compute service should support standard jobs
     * @param supports_pilot_jobs: true if the compute service should support pilot jobs
     * @param plist: a property list ({} means "use all defaults")
     * @param suffix: suffix to append to the service name and mailbox
     */
    BatchService::BatchService(std::string hostname,
                               std::vector<std::string> nodes_in_network,
                               StorageService *default_storage_service,
                               bool supports_standard_jobs,
                               bool supports_pilot_jobs,
                               unsigned long reduced_cores,
                               std::map<std::string, std::string> plist, std::string suffix) :
            ComputeService("batch" + suffix,
                           "batch" + suffix,
                           supports_standard_jobs,
                           supports_pilot_jobs,
                           default_storage_service) {

      // Set default and specified properties
      this->setProperties(this->default_property_values, std::move(plist));

      //create a map for host to cores
      std::vector<std::string>::iterator it;
      for (it = nodes_in_network.begin(); it != nodes_in_network.end(); it++) {
        if (reduced_cores > 0 && not this->supports_pilot_jobs) {
          this->nodes_to_cores_map.insert({*it, reduced_cores});
          this->available_nodes_to_cores.insert({*it, reduced_cores});
        } else {
          this->nodes_to_cores_map.insert({*it, S4U_Simulation::getNumCores(*it)});
          this->available_nodes_to_cores.insert({*it, S4U_Simulation::getNumCores(*it)});
        }
      }

      this->total_num_of_nodes = nodes_in_network.size();
      this->hostname = hostname;

      this->generateUniqueJobId();

      // Start the daemon on the same host
      try {
        this->start(hostname);
      } catch (std::invalid_argument e) {
        throw e;
      }
    }


    /**
     * @brief Synchronously submit a standard job to the batch service
     *
     * @param job: a standard job
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     *
     */
    void BatchService::submitStandardJob(StandardJob *job, std::map<std::string, std::string> &batch_job_args) {

      if (this->state == Service::DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      //check if there are enough hosts
      unsigned long nodes_asked_for = 0;
      std::map<std::string, std::string>::iterator it;
      it = batch_job_args.find("-N");
      if (it != batch_job_args.end()) {
        if (sscanf((*it).second.c_str(), "%lu", &nodes_asked_for) != 1) {
          throw std::invalid_argument(
                  "BatchService::submitStandardJob(): Invalid -N value '" + (*it).second + "'");
        }
        if (nodes_asked_for > this->total_num_of_nodes) {
          throw std::runtime_error(
                  "BatchService::submitStandardJob(): There are not enough hosts");
        }
      } else {
        throw std::invalid_argument(
                "BatchService::submitStandardJob(): Batch Service requires -N(number of nodes) to be specified "
        );
      }

      //check if there are enough cores per node
      unsigned long nodes_with_c_cores = 0;
      it = batch_job_args.find("-c");
      if (it != batch_job_args.end()) {
        std::map<std::string, unsigned long>::iterator nodes_to_core_iterator;
        for (nodes_to_core_iterator = this->nodes_to_cores_map.begin();
             nodes_to_core_iterator != this->nodes_to_cores_map.end(); nodes_to_core_iterator++) {
          unsigned long num;
          if (sscanf((*it).second.c_str(), "%lu", &num) != 1) {
            throw std::invalid_argument(
                    "BatchService::submitStandardJob(): Invalid -c value '" + (*it).second +
                    "'");
          }
          if (num <= (*nodes_to_core_iterator).second) {
            nodes_with_c_cores++;
          }
        }
      } else {
        throw std::invalid_argument(
                "BatchService::submitStandardJob(): Batch Service requires -c(cores per node) to be specified "
        );
      }
      if (nodes_asked_for > nodes_with_c_cores) {
        throw std::runtime_error(
                "BatchService::submitStandardJob(): There are not enough hosts with those amount of cores per host");
      }
      unsigned long cores_asked_for;
      if (sscanf((*it).second.c_str(), "%lu", &cores_asked_for) != 1) {
        throw std::invalid_argument(
                "BatchService::submitStandardJob(): Invalid -c value '" + (*it).second + "'");
      }


      //get job time
      unsigned long time_asked_for = 0;
      it = batch_job_args.find("-t");
      if (it != batch_job_args.end()) {
        if (sscanf((*it).second.c_str(), "%lu", &time_asked_for) != 1) {
          throw std::invalid_argument(
                  "BatchService::submitStandardJob(): Invalid -t value '" + (*it).second + "'");
        }
      } else {
        throw std::invalid_argument(
                "BatchService::submitStandardJob(): Batch Service requires -t to be specified "
        );
      }

      //Create a Batch Job
      unsigned long jobid = this->generateUniqueJobId();
      std::unique_ptr<BatchJob> batch_job = std::unique_ptr<BatchJob>(new BatchJob(job, jobid, time_asked_for, nodes_asked_for, cores_asked_for, -1));

      //  send a "run a batch job" message to the daemon's mailbox
      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("batch_standard_job_mailbox");
      try {
        S4U_Mailbox::putMessage(this->mailbox_name,
                                new BatchServiceJobRequestMessage(
                                        answer_mailbox,
                                                                  std::move(batch_job),
                                                                  this->getPropertyValueAsDouble(
                                                                          BatchServiceProperty::SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD)));
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

      if (ComputeServiceSubmitStandardJobAnswerMessage *msg = dynamic_cast<ComputeServiceSubmitStandardJobAnswerMessage *>(message.get())) {
        // If no success, throw an exception
        if (not msg->success) {
          throw WorkflowExecutionException(msg->failure_cause);
        }

      } else {
        throw std::runtime_error(
                "BatchService::submitStandardJob(): Received an unexpected [" + message->getName() +
                "] message!");
      }

    }


    /**
     * @brief Synchronously submit a pilot job to the compute service
     *
     * @param job: a pilot job
     * @param batch_job_args: arguments to the batch job
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    void BatchService::submitPilotJob(PilotJob *job, std::map<std::string, std::string> &batch_job_args) {

      if (this->state == Service::DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      //check if there are enough hosts
      unsigned long nodes_asked_for = 0;
      std::map<std::string, std::string>::iterator it;
      it = batch_job_args.find("-N");
      if (it != batch_job_args.end()) {
        if (sscanf((*it).second.c_str(), "%lu", &nodes_asked_for) != 1) {
          throw std::runtime_error(
                  "BatchService::submitPilotJob(): Invalid -N value '" + (*it).second + "'");
        }
        if (nodes_asked_for > this->total_num_of_nodes) {
          throw std::runtime_error(
                  "BatchService::submitPilotJob(): There are not enough hosts");
        }
      } else {
        throw std::invalid_argument(
                "BatchService::submitPilotJob(): Batch Service requires -N(number of nodes) to be specified "
        );
      }

      //check if there are enough cores per node
      unsigned long nodes_with_c_cores = 0;
      unsigned long cores_asked_for = 0;
      it = batch_job_args.find("-c");
      if (it != batch_job_args.end()) {
        std::map<std::string, unsigned long>::iterator nodes_to_core_iterator;
        if (sscanf((*it).second.c_str(), "%lu", &cores_asked_for) != 1) {
          throw std::invalid_argument(
                  "BatchService::submitPilotJob(): Invalid -c value '" + (*it).second + "'");
        }
        for (nodes_to_core_iterator = this->nodes_to_cores_map.begin();
             nodes_to_core_iterator != this->nodes_to_cores_map.end(); nodes_to_core_iterator++) {
          if (cores_asked_for <= (*nodes_to_core_iterator).second) {
            nodes_with_c_cores++;
          }
        }
      } else {
        throw std::invalid_argument(
                "BatchService::submitPilotJob(): Batch Service requires -c(cores per node) to be specified "
        );
      }
      if (nodes_asked_for > nodes_with_c_cores) {
        throw std::runtime_error(
                "BatchService::submitPilotJob(): There are not enough hosts with those amount of cores per host");
      }

      //get job time
      unsigned long time_asked_for = 0;
      it = batch_job_args.find("-t");
      if (it != batch_job_args.end()) {
        if (sscanf((*it).second.c_str(), "%lu", &time_asked_for) != 1) {
          throw std::invalid_argument(
                  "BatchService::submitPilotJob(): Invalid -t argument '" + (*it).second + "'");
        }
      } else {
        throw std::invalid_argument(
                "BatchService::submitPilotJob(): Batch Service requires -t to be specified "
        );
      }

      //Create a Batch Job
      unsigned long jobid = this->generateUniqueJobId();
        std::unique_ptr<BatchJob> batch_job = std::unique_ptr<BatchJob>(new BatchJob(job, jobid, time_asked_for, nodes_asked_for, cores_asked_for, -1));

      //  send a "run a batch job" message to the daemon's mailbox
      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("batch_pilot_job_mailbox");
      try {
        S4U_Mailbox::putMessage(this->mailbox_name,
                                new BatchServiceJobRequestMessage(answer_mailbox, std::move(batch_job),
                                                                  this->getPropertyValueAsDouble(
                                                                          BatchServiceProperty::SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD)));
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

      if (ComputeServiceSubmitPilotJobAnswerMessage *msg = dynamic_cast<ComputeServiceSubmitPilotJobAnswerMessage *>(message.get())) {
        // If no success, throw an exception
        if (not msg->success) {
          throw WorkflowExecutionException(msg->failure_cause);
        }

      } else {
        throw std::runtime_error(
                "BatchService::submitPilotJob(): Received an unexpected [" + message->getName() +
                "] message!");
      }
      return;
    }

    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int BatchService::main() {

      TerminalOutput::setThisProcessLoggingColor(WRENCH_LOGGING_COLOR_MAGENTA);

      WRENCH_INFO("Batch Service starting on host %s!", S4U_Simulation::getHostName().c_str());


      /** Main loop **/
      bool life = true;
      while (life) {
        life = processNextMessage();
        if (this->running_jobs.empty()) {
          std::set<std::unique_ptr<BatchJob>>::iterator it;
          for (it = this->running_jobs.begin(); it != this->running_jobs.end();) {
            if ((*it)->getEndingTimeStamp() <= S4U_Simulation::getClock()) {
              if ((*it)->getWorkflowJob()->getType() == WorkflowJob::STANDARD) {
                this->processStandardJobTimeout(
                        (StandardJob *) (*it)->getWorkflowJob());
                this->updateResources((*it)->getResourcesAllocated());
                it = this->running_jobs.erase(it);
              } else if ((*it)->getWorkflowJob()->getType() == WorkflowJob::PILOT) {
              } else {
                throw std::runtime_error(
                        "BatchService::main(): Received a JOB type other than Standard and Pilot jobs"
                );
              }
            } else {
              ++it;
            }

          }
        }
        if (life) {
          while (this->dispatchNextPendingJob());
        }
      }

      WRENCH_INFO("Batch Service on host %s terminated!", S4U_Simulation::getHostName().c_str());
      return 0;
    }


    std::set<std::pair<std::string, unsigned long>> BatchService::scheduleOnHosts(std::string host_selection_algorithm,
                                                                                  unsigned long num_nodes,
                                                                                  unsigned long cores_per_node) {
      std::set<std::pair<std::string, unsigned long>> resources = {};
      std::vector<std::string> hosts_assigned = {};
      if (host_selection_algorithm == "FIRSTFIT") {
        std::map<std::string, unsigned long>::iterator it;
        unsigned long host_count = 0;
        for (it = this->available_nodes_to_cores.begin();
             it != this->available_nodes_to_cores.end(); it++) {
          if ((*it).second >= cores_per_node) {
            //Remove that many cores from the available_nodes_to_core
            (*it).second -= cores_per_node;
            hosts_assigned.push_back((*it).first);
            resources.insert(std::make_pair((*it).first, cores_per_node));
            if (++host_count >= num_nodes) {
              break;
            }
          }
        }
        if (resources.size() < num_nodes) {
          resources = {};
          std::vector<std::string>::iterator it;
          for (it = hosts_assigned.begin(); it != hosts_assigned.end(); it++) {
            available_nodes_to_cores[*it] += cores_per_node;
          }
        }
      } else if (host_selection_algorithm == "BESTFIT") {
        while (resources.size() < num_nodes) {
          unsigned long target_slack = 0;
          std::string target_host = "";
          unsigned long target_num_cores = 0;

          for (auto h : this->available_nodes_to_cores) {
            const std::string hostname = std::get<0>(h);
            unsigned long num_available_cores = std::get<1>(h);
            if (num_available_cores < cores_per_node) {
              continue;
            }
            unsigned long tentative_target_num_cores = MIN(num_available_cores, cores_per_node);
            unsigned long tentative_target_slack =
                    num_available_cores - tentative_target_num_cores;

            if ((target_host.empty()) ||
                (tentative_target_num_cores > target_num_cores) ||
                ((tentative_target_num_cores == target_num_cores) &&
                 (target_slack > tentative_target_slack))) {
              target_host = hostname;
              target_num_cores = tentative_target_num_cores;
              target_slack = tentative_target_slack;
            }
          }
          if (target_host.empty()) {
            WRENCH_INFO("Didn't find a suitable host");
            resources = {};
            std::vector<std::string>::iterator it;
            for (it = hosts_assigned.begin(); it != hosts_assigned.end(); it++) {
              available_nodes_to_cores[*it] += cores_per_node;
            }
            break;
          }
          this->available_nodes_to_cores[target_host] -= cores_per_node;
          hosts_assigned.push_back(target_host);
          resources.insert(std::make_pair(target_host, cores_per_node));
        }
      } else {
        throw std::invalid_argument(
                "BatchService::scheduleOnHosts(): We don't support " + host_selection_algorithm +
                " as host selection algorithm"
        );
      }

      return resources;
    }

    std::unique_ptr<BatchJob> BatchService::scheduleJob(std::string job_selection_algorithm) {
      if (job_selection_algorithm == "FCFS") {
        std::unique_ptr<BatchJob> batch_job_ptr = std::move(this->pending_jobs.front());
        this->pending_jobs.pop_front();
        return batch_job_ptr;
      }
      return nullptr;
    }

    /**
     * @brief Dispatch one pending job, if possible
     * @return true if a job was dispatched, false otherwise
     */
    bool BatchService::dispatchNextPendingJob() {

      if (this->pending_jobs.empty()) {
        return false;
      }

      //Currently we have only FCFS scheduling
      std::unique_ptr<BatchJob>batch_job_ptr = this->scheduleJob(
              this->getPropertyValueAsString(BatchServiceProperty::JOB_SELECTION_ALGORITHM));

      if (batch_job_ptr == nullptr) {
        throw std::runtime_error(
                "BatchService::dispatchNextPendingJob(): Got no such job in pending queue to dispatch"
        );
      }

      BatchJob* batch_job = batch_job_ptr.get();
      WorkflowJob *next_job = batch_job->getWorkflowJob();


      /* Get the nodes and cores per nodes asked for */
      unsigned long cores_per_node_asked_for = batch_job->getAllocatedCoresPerNode();
      unsigned long num_nodes_asked_for = batch_job->getNumNodes();
      unsigned long time_in_minutes = batch_job->getAllocatedTime();



      //Try to schedule hosts based on FIRSTFIT OR BESTFIT
      std::set<std::pair<std::string, unsigned long>> resources = this->scheduleOnHosts(
              this->getPropertyValueAsString(BatchServiceProperty::HOST_SELECTION_ALGORITHM),
              num_nodes_asked_for, cores_per_node_asked_for);

      if (resources.empty()) {
        return false;
      }


      switch (next_job->getType()) {
        case WorkflowJob::STANDARD: {
          auto job = (StandardJob*) next_job;
          WRENCH_INFO("Creating a StandardJobExecutor on %ld cores for a standard job on %ld nodes",
                      cores_per_node_asked_for, num_nodes_asked_for);
          // Create a standard job executor
          StandardJobExecutor *executor = new StandardJobExecutor(
                  this->simulation,
                  this->mailbox_name,
                  std::get<0>(*resources.begin()),
                  job,
                  resources,
                  this->default_storage_service,
                  {{StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD,
                           this->getPropertyValueAsString(
                                   BatchServiceProperty::THREAD_STARTUP_OVERHEAD)}});
          std::unique_ptr<StandardJobExecutor> executor_uniq_ptr = std::unique_ptr<StandardJobExecutor>(executor);
          this->running_standard_job_executors.insert(std::move(executor_uniq_ptr));
          batch_job->setEndingTimeStamp(S4U_Simulation::getClock() + time_in_minutes * 60);

          this->running_jobs.insert(std::move(batch_job_ptr));

//          this->timeslots.push_back(batch_job->getEndingTimeStamp());
          //remember the allocated resources for the job
          batch_job->setAllocatedResources(resources);

          SimulationMessage* msg =
                  new AlarmJobTimeOutMessage(job, 0);


            std::unique_ptr<Alarm> alarm_ptr = std::unique_ptr<Alarm>(new Alarm(batch_job->getEndingTimeStamp(), this->hostname, this->mailbox_name, msg,
                                                  "batch_standard"));

//            this->sent_alrm_msgs.push_back(msg);
//

          standard_job_alarms.push_back(std::move(alarm_ptr));


          return true;
        }
          break;

        case WorkflowJob::PILOT: {
          auto job = (PilotJob *) next_job;
          WRENCH_INFO("Allocating %ld nodes with %ld cores per node to a pilot job",
                      num_nodes_asked_for, cores_per_node_asked_for);

          std::string host_to_run_on = resources.begin()->first;

          //set the ending timestamp of the batchjob (pilotjob)

          double timeout_timestamp = std::min(job->getDuration(), time_in_minutes * 60 * 1.0);
          batch_job->setEndingTimeStamp(S4U_Simulation::getClock() + timeout_timestamp);

          ComputeService *cs =
                  new MultihostMulticoreComputeService(host_to_run_on,
                                                       true, false,
                                                       resources,
                                                       this->default_storage_service
                  );
          cs->setSimulation(this->simulation);

          // Create and launch a compute service for the pilot job
          job->setComputeService(cs);


          // Put the job in the running queue
          this->running_jobs.insert(std::move(batch_job_ptr));
//          this->timeslots.push_back(batch_job->getEndingTimeStamp());

          //remember the allocated resources for the job
          batch_job->setAllocatedResources(resources);


          // Send the "Pilot job has started" callback
          // Note the getCallbackMailbox instead of the popCallbackMailbox, because
          // there will be another callback upon termination.
          try {
            S4U_Mailbox::dputMessage(job->getCallbackMailbox(),
                                     new ComputeServicePilotJobStartedMessage(job, this,
                                                                              this->getPropertyValueAsDouble(
                                                                                      BatchServiceProperty::PILOT_JOB_STARTED_MESSAGE_PAYLOAD)));
          } catch (std::shared_ptr<NetworkError> cause) {
            throw WorkflowExecutionException(cause);
          }


          SimulationMessage* msg =
                  new AlarmJobTimeOutMessage(job, 0);

            std::unique_ptr<Alarm> alarm_ptr = std::unique_ptr<Alarm>(new Alarm(batch_job->getEndingTimeStamp(), host_to_run_on, this->mailbox_name, msg,
                                                                          "batch_pilot"));

//            this->sent_alrm_msgs.push_back(msg);
          this->pilot_job_alarms.push_back(std::move(alarm_ptr));


          return true;
        }
          break;
      }
    }


    bool BatchService::processNextMessage() {

      // Wait for a message
      std::unique_ptr<SimulationMessage> message = nullptr;

      try {
        message = S4U_Mailbox::getMessage(this->mailbox_name);
      } catch (std::shared_ptr<NetworkError> cause) {
        return true;
      } catch (std::shared_ptr<NetworkTimeout> cause) {
        return true;
      }

      if (message == nullptr) {
        WRENCH_INFO("Got a NULL message... Likely this means we're all done. Aborting!");
        return false;
      }

      WRENCH_INFO("Got a [%s] message", message->getName().c_str());

      if (ServiceStopDaemonMessage *msg = dynamic_cast<ServiceStopDaemonMessage *>(message.get())) {
        this->terminate(false);
        // This is Synchronous;
        try {
          S4U_Mailbox::putMessage(msg->ack_mailbox,
                                  new ServiceDaemonStoppedMessage(this->getPropertyValueAsDouble(
                                          BatchServiceProperty::DAEMON_STOPPED_MESSAGE_PAYLOAD)));

        } catch (std::shared_ptr<NetworkError> cause) {
          return false;
        }
        return false;

      } else if (BatchServiceJobRequestMessage *msg = dynamic_cast<BatchServiceJobRequestMessage *>(message.get())) {
        WRENCH_INFO("Asked to run a batch job using batchservice with jobid %ld", msg->job->getJobID());
        if (msg->job->getWorkflowJob()->getType() == WorkflowJob::STANDARD) {
          if (not this->supports_standard_jobs) {
            try {
              S4U_Mailbox::dputMessage(msg->answer_mailbox,
                                       new ComputeServiceSubmitStandardJobAnswerMessage(
                                               (StandardJob *) msg->job->getWorkflowJob(),
                                               this,
                                               false,
                                               std::shared_ptr<FailureCause>(
                                                       new JobTypeNotSupported(
                                                               msg->job->getWorkflowJob(),
                                                               this)),
                                               this->getPropertyValueAsDouble(
                                                       BatchServiceProperty::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));
            } catch (std::shared_ptr<NetworkError> cause) {
              return true;
            }
            return true;
          }
        } else if (msg->job->getWorkflowJob()->getType() == WorkflowJob::PILOT) {
          if (not this->supports_pilot_jobs) {
            try {
              S4U_Mailbox::dputMessage(msg->answer_mailbox,
                                       new ComputeServiceSubmitPilotJobAnswerMessage(
                                               (PilotJob *) msg->job->getWorkflowJob(),
                                               this,
                                               false,
                                               std::shared_ptr<FailureCause>(
                                                       new JobTypeNotSupported(
                                                               msg->job->getWorkflowJob(),
                                                               this)),
                                               this->getPropertyValueAsDouble(
                                                       BatchServiceProperty::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
            } catch (std::shared_ptr<NetworkError> cause) {
              return true;
            }
            return true;
          }
        }



        if (msg->job->getWorkflowJob()->getType() == WorkflowJob::STANDARD) {

          try {
            S4U_Mailbox::dputMessage(msg->answer_mailbox,
                                     new ComputeServiceSubmitStandardJobAnswerMessage(
                                             (StandardJob *) msg->job->getWorkflowJob(), this,
                                             true,
                                             nullptr,
                                             this->getPropertyValueAsDouble(
                                                     BatchServiceProperty::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));
          } catch (std::shared_ptr<NetworkError> cause) {
            return true;
          }
        } else if (msg->job->getWorkflowJob()->getType() == WorkflowJob::PILOT) {
          try {
            S4U_Mailbox::dputMessage(msg->answer_mailbox,
                                     new ComputeServiceSubmitPilotJobAnswerMessage(
                                             (PilotJob *) msg->job->getWorkflowJob(), this,
                                             true,
                                             nullptr,
                                             this->getPropertyValueAsDouble(
                                                     BatchServiceProperty::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
          } catch (std::shared_ptr<NetworkError> cause) {
            return true;
          }
        }
          this->pending_jobs.push_back(std::move(msg->job));
        return true;

      } else if (StandardJobExecutorDoneMessage *msg = dynamic_cast<StandardJobExecutorDoneMessage *>(message.get())) {
        processStandardJobCompletion(msg->executor, msg->job);
        return true;

      } else if (StandardJobExecutorFailedMessage *msg = dynamic_cast<StandardJobExecutorFailedMessage *>(message.get())) {
        processStandardJobFailure(msg->executor, msg->job, msg->cause);
        return true;

      } else if (ComputeServicePilotJobExpiredMessage *msg = dynamic_cast<ComputeServicePilotJobExpiredMessage *>(message.get())) {
        processPilotJobCompletion(msg->job);
        return true;
      } else if (auto *msg = dynamic_cast<ComputeServiceTerminatePilotJobRequestMessage *>(message.get())) {
        processPilotJobTerminationRequest(msg->job, msg->answer_mailbox);
        return true;

      } else if (AlarmJobTimeOutMessage *msg = dynamic_cast<AlarmJobTimeOutMessage *>(message.get())) {
        if (msg->job->getType() == WorkflowJob::STANDARD) {
          this->processStandardJobTimeout((StandardJob *) (msg->job));
          this->updateResources((StandardJob *) msg->job);
          this->sendStandardJobCallBackMessage((StandardJob *) msg->job);
          return true;
        } else if (msg->job->getType() == WorkflowJob::PILOT) {
          auto job = (PilotJob *) msg->job;
          ComputeService *cs = job->getComputeService();
          try {
            cs->stop();
          } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error(
                    "BatchService::processNextMessage(): Not able to terminate the pilot job"
            );
          }
          this->processPilotJobCompletion(job);
          return true;
        }else{
          throw std::runtime_error(
                  "BatchService::processNextMessage(): Alarm about unknown job type"
          );
        }
      } else {
        throw std::runtime_error(
                "BatchService::processNextMessage(): Unknown message type: " +
                std::to_string(message->payload));
        return false;
      }

    }

    void BatchService::sendPilotJobCallBackMessage(PilotJob *job) {
        try {
            S4U_Mailbox::dputMessage(job->popCallbackMailbox(),
                                     new ComputeServicePilotJobExpiredMessage(job, this,
                                                                              this->getPropertyValueAsDouble(
                                                                                      BatchServiceProperty::PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> cause) {
            return;
        }
    }

    void BatchService::sendStandardJobCallBackMessage(StandardJob *job) {
        WRENCH_INFO("A standard job executor has failed because of timeout %s", job->getName().c_str());
        try {
            S4U_Mailbox::putMessage(job->popCallbackMailbox(),
                                    new ComputeServiceStandardJobFailedMessage(job, this,
                                                                               std::shared_ptr<FailureCause>(
                                                                                       new ServiceIsDown(this)),
                                                                               this->getPropertyValueAsDouble(
                                                                                       BatchServiceProperty::STANDARD_JOB_FAILED_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> cause) {
            return;
        }
    }

    bool BatchService::foundRunningJobOnTheList(WorkflowJob* job) {
        std::set<std::unique_ptr<BatchJob>>::iterator it;
        for (it = this->running_jobs.begin(); it != this->running_jobs.end(); it++) {
            if ((*it)->getWorkflowJob() == job) {
                // Update the cores count in the available resources
                std::set<std::pair<std::string, unsigned long>> resources = (*it)->getResourcesAllocated();
                this->updateResources(resources);
                this->running_jobs.erase(it);
                return true;
            }
        }
        return false;
    }

    void BatchService::updateResources(std::set<std::pair<std::string, unsigned long>> resources) {
        if (resources.empty()) {
            return;
        }
        std::set<std::pair<std::string, unsigned long>>::iterator resource_it;
        for (resource_it = resources.begin(); resource_it != resources.end(); resource_it++) {
            this->available_nodes_to_cores[(*resource_it).first] += (*resource_it).second;
        }
    }

    void BatchService::updateResources(StandardJob *job) {
        // Remove the job from the running job list
        bool job_on_the_list = this->foundRunningJobOnTheList((WorkflowJob*)job);

        if (!job_on_the_list) {
            throw std::runtime_error(
                    "BatchService::updateResources(): Received a standard job completion, but the job is not in the running job list");
        }
    }


    void BatchService::processPilotJobTimeout(PilotJob *job) {
        BatchService *cs = (BatchService *) job->getComputeService();
        if (cs == nullptr) {
            throw std::runtime_error(
                    "BatchService::processPilotJobTimeout(): can't find compute service associated to pilot job");
        }
        try {
            cs->stop();
        } catch (wrench::WorkflowExecutionException &e) {
            return;
        }
    }

    void BatchService::processStandardJobTimeout(StandardJob *job) {
        std::set<std::unique_ptr<StandardJobExecutor>>::iterator it;

        bool terminated = false;
        for (it = this->running_standard_job_executors.begin(); it != this->running_standard_job_executors.end(); it++) {
            if (((*it).get())->getJob() == job) {
                ((*it).get())->kill();
                PointerUtil::moveUniquePtrFromSetToSet(it, &(this->running_standard_job_executors), &(this->finished_standard_job_executors));
                terminated = true;
                break;
            }
        }
        if (not terminated) {
            throw std::runtime_error(
                    "BatchService::processStandardJobTimeout(): Cannot find standard job executor corresponding to job being timedout");
        }
    }

    /**
     * @brief fail a running standard job
     * @param job: the job
     * @param cause: the failure cause
     */
    void BatchService::failRunningStandardJob(StandardJob *job, std::shared_ptr<FailureCause> cause) {

        WRENCH_INFO("Failing running job %s", job->getName().c_str());

        terminateRunningStandardJob(job);

        // Send back a job failed message (Not that it can be a partial fail)
        WRENCH_INFO("Sending job failure notification to '%s'", job->getCallbackMailbox().c_str());
        // NOTE: This is synchronous so that the process doesn't fall off the end
        this->sendStandardJobCallBackMessage(job);
    }

    /**
    * @brief terminate a running standard job
    * @param job: the job
    */
    void BatchService::terminateRunningStandardJob(StandardJob *job) {

        StandardJobExecutor *executor = nullptr;
        std::set<std::unique_ptr<StandardJobExecutor>>::iterator it;
        for (it = this->running_standard_job_executors.begin();
             it != this->running_standard_job_executors.end(); it++) {
            if (((*it).get())->getJob() == job) {
                executor = (it->get());
            }
        }
        if (executor == nullptr) {
            throw std::runtime_error(
                    "BatchService::terminateRunningStandardJob(): Cannot find standard job executor corresponding to job being terminated");
        }

        // Terminate the executor
        WRENCH_INFO("Terminating a standard job executor");
        executor->kill();

        //TODO: Restore the allocated resources
    }


    /**
   * @brief Declare all current jobs as failed (likely because the daemon is being terminated
   * or has timed out (because it's in fact a pilot job))
   */
    void BatchService::failCurrentStandardJobs(std::shared_ptr<FailureCause> cause) {

        std::set<std::unique_ptr<BatchJob>>::iterator it;
        for(it=this->running_jobs.begin();it!=this->running_jobs.end();it++){
            WorkflowJob* workflow_job = it->get()->getWorkflowJob();
            if (workflow_job->getType() == WorkflowJob::STANDARD) {
                auto job = (StandardJob *) workflow_job;
                this->failRunningStandardJob(job, cause);
            }
        }

        std::deque<std::unique_ptr<BatchJob>>::iterator it1;
        for(it1=this->pending_jobs.begin();it1!=this->pending_jobs.end();it1++){
            WorkflowJob* workflow_job = it1->get()->getWorkflowJob();
            if (workflow_job->getType() == WorkflowJob::STANDARD) {
                auto job = (StandardJob *) workflow_job;
                this->failPendingStandardJob(job, cause);
            }
            it1 = this->pending_jobs.erase(it1);
        }
    }


    /**
 * @brief Synchronously terminate a pilot job to the compute service (this function is externally called by User to terminate
     * using JobManager)
 *
 * @param job: a pilot job
 *
 * @throw WorkflowExecutionException
 * @throw std::runtime_error
 */
    void BatchService::terminatePilotJob(PilotJob *job) {

        if (this->state == Service::DOWN) {
            throw WorkflowExecutionException(new ServiceIsDown(this));
        }

        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("terminate_pilot_job");

        // Send a "terminate a pilot job" message to the daemon's mailbox
        try {
            S4U_Mailbox::putMessage(this->mailbox_name,
                                    new ComputeServiceTerminatePilotJobRequestMessage(answer_mailbox, job,
                                                                                      this->getPropertyValueAsDouble(
                                                                                              BatchServiceProperty::TERMINATE_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD)));
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
                    "BatchService::terminatePilotJob(): Received an unexpected [" +
                    message->getName() +
                    "] message!");
        }
    }


    /**
     * @brief Terminate the daemon, dealing with pending/running jobs:: Internal function:: called when the whole BatchService has to terminate
     */
    void BatchService::terminate(bool notify_pilot_job_submitters) {
        this->setStateToDown();

        WRENCH_INFO("Failing current standard jobs");
        this->failCurrentStandardJobs(std::shared_ptr<FailureCause>(new ServiceIsDown(this)));

        //remove standard job alarms
        std::vector<std::unique_ptr<Alarm>>::iterator it;
        for(it=this->standard_job_alarms.begin();it!=this->standard_job_alarms.end();it++){
            if((*it)->isUp()){
                it->reset();
            }
        }

        //remove standard job alarms
        for(it=this->pilot_job_alarms.begin();it!=this->pilot_job_alarms.end();it++){
            if((*it)->isUp()){
                it->reset();
            }
        }

//        this->running_standard_job_executors.clear();
//        std::cout<< "Clearing finished standard jobs "<< this->finished_standard_job_executors.size()<<"\n";
//        this->finished_standard_job_executors.clear();

        if (this->supports_pilot_jobs) {
            std::set<std::unique_ptr<BatchJob>>::iterator it;
            for (it=this->running_jobs.begin();it!=this->running_jobs.end();it++) {
                if ((*it)->getWorkflowJob()->getType() == WorkflowJob::PILOT) {
                    auto p_job = (PilotJob *) ((*it)->getWorkflowJob());
                    auto cs = (BatchService *) p_job->getComputeService();
                    if (cs == nullptr) {
                        throw std::runtime_error(
                                "BatchService::terminate(): can't find compute service associated to pilot job");
                    }
                    try {
                        cs->stop();
                    } catch (wrench::WorkflowExecutionException &e) {
                        return;
                    }
                }
            }
        }


        //clear the jobs queue
        this->pending_jobs.clear();
        this->running_jobs.clear();
        //clear the queues of executors
        this->running_standard_job_executors.clear();
        this->finished_standard_job_executors.clear();
    }


    /**
     * @brief Process a pilot job completion
     *
     * @param job: the pilot job
     */
    void BatchService::processPilotJobCompletion(PilotJob *job) {
      // Remove the job from the running job list
      bool job_on_the_list = this->foundRunningJobOnTheList((WorkflowJob*)job);
      if (!job_on_the_list) {
        throw std::runtime_error(
                "BatchService::processPilotJobCompletion():  Pilot job completion message recevied but no such pilot jobs found in queue"
        );
      }

      // Forward the notification
      this->sendPilotJobCallBackMessage(job);
    }


    /**
     * @brief Process a pilot job termination request
     *
     * @param job: the job to terminate
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     */
    void BatchService::processPilotJobTerminationRequest(PilotJob *job, std::string answer_mailbox) {

      std::deque<std::unique_ptr<BatchJob>>::iterator it;
      for(it=this->pending_jobs.begin();it!=this->pending_jobs.end();it++){
        if(it->get()->getWorkflowJob()==job){
          this->pending_jobs.erase(it);
          ComputeServiceTerminatePilotJobAnswerMessage *answer_message = new ComputeServiceTerminatePilotJobAnswerMessage(
                  job, this, true, nullptr,
                  this->getPropertyValueAsDouble(
                          BatchServiceProperty::TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD));
          try {
            S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
          } catch (std::shared_ptr<NetworkError> &cause) {
            return;
          }
          return;
        }
      }


      std::set<std::unique_ptr<BatchJob>>::iterator it1;
      for (it1 = this->running_jobs.begin(); it1 != this->running_jobs.end();) {
        if ((*it1)->getWorkflowJob() == job) {
          this->processPilotJobTimeout((PilotJob *) (*it1)->getWorkflowJob());
          // Update the cores count in the available resources
          std::set<std::pair<std::string, unsigned long>> resources = (*it1)->getResourcesAllocated();
            this->updateResources(resources);
          ComputeServiceTerminatePilotJobAnswerMessage *answer_message = new ComputeServiceTerminatePilotJobAnswerMessage(
                  job, this, true, nullptr,
                  this->getPropertyValueAsDouble(
                          BatchServiceProperty::TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD));
          try {
            S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
          } catch (std::shared_ptr<NetworkError> &cause) {
            return;
          }
          this->running_jobs.erase(it1);
          return;
        } else {
          ++it1;
        }
      }

      // If we got here, we're in trouble
      WRENCH_INFO("Trying to terminate a pilot job that's neither pending nor running!");
      ComputeServiceTerminatePilotJobAnswerMessage *answer_message = new ComputeServiceTerminatePilotJobAnswerMessage(
              job, this, false, std::shared_ptr<FailureCause>(new JobCannotBeTerminated(job)),
              this->getPropertyValueAsDouble(
                      BatchServiceProperty::TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD));
      try {
        S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
      } catch (std::shared_ptr<NetworkError> &cause) {
        return;
      }
    }

    /**
     * @brief Process a standard job completion:: Internal Function for BatchService
     * @param executor: the standard job executor
     * @param job: the job
     *
     * @throw std::runtime_error
     */
    void BatchService::processStandardJobCompletion(StandardJobExecutor *executor, StandardJob *job) {
        bool executor_on_the_list = false;
        std::set<std::unique_ptr<StandardJobExecutor>>::iterator it;
        for(it=this->running_standard_job_executors.begin();it!=this->running_standard_job_executors.end();it++){
            if((*it).get()==executor){
                PointerUtil::moveUniquePtrFromSetToSet(it, &(this->running_standard_job_executors), &(this->finished_standard_job_executors));
                executor_on_the_list = true;
                break;
            }
        }

        if(not executor_on_the_list){
            throw std::runtime_error(
                    "BatchService::processStandardJobCompletion(): Received a standard job completion, but the executor is not in the executor list");
        }
      // Remove the job from the running job list
      bool job_on_the_list = this->foundRunningJobOnTheList((WorkflowJob*)job);

      if (!job_on_the_list) {
        throw std::runtime_error(
                "BatchService::processStandardJobCompletion(): Received a standard job completion, but the job is not in the running job list");
      }


      WRENCH_INFO("A standard job executor has completed job %s", job->getName().c_str());

      // Send the callback to the originator
      try {
        S4U_Mailbox::dputMessage(job->popCallbackMailbox(),
                                 new ComputeServiceStandardJobDoneMessage(job, this,
                                                                          this->getPropertyValueAsDouble(
                                                                                  BatchServiceProperty::STANDARD_JOB_DONE_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> cause) {
        return;
      }
    }

    /**
     * @brief Process a work failure
     * @param worker_thread: the worker thread that did the work
     * @param work: the work
     * @param cause: the cause of the failure
     */
    void BatchService::processStandardJobFailure(StandardJobExecutor *executor,
                                                 StandardJob *job,
                                                 std::shared_ptr<FailureCause> cause) {
        bool executor_on_the_list = false;
        std::set<std::unique_ptr<StandardJobExecutor>>::iterator it;
        for(it=this->running_standard_job_executors.begin();it!=this->running_standard_job_executors.end();it++){
            if((*it).get()==executor){
                PointerUtil::moveUniquePtrFromSetToSet(it, &(this->running_standard_job_executors), &(this->finished_standard_job_executors));
                executor_on_the_list = true;
                break;
            }
        }

        if(not executor_on_the_list){
            throw std::runtime_error(
                    "BatchService::processStandardJobFailure(): Received a standard job failure, but the executor is not in the executor list");
        }

      // Remove the job from the running job list
      bool job_on_the_list = this->foundRunningJobOnTheList((WorkflowJob*)job);
      if (!job_on_the_list) {
        throw std::runtime_error(
                "BatchService::processStandardJobFailure(): Received a standard job completion, but the job is not in the running job list");
      }

      WRENCH_INFO("A standard job executor has failed to perform job %s", job->getName().c_str());

      // Fail the job
      this->failPendingStandardJob(job, move(cause));

    }


    /**
     * @brief fail a pending standard job
     * @param job: the job
     * @param cause: the failure cause
     */
    void BatchService::failPendingStandardJob(StandardJob *job, std::shared_ptr<FailureCause> cause) {
      // Send back a job failed message
      WRENCH_INFO("Sending job failure notification to '%s'", job->getCallbackMailbox().c_str());
      // NOTE: This is synchronous so that the process doesn't fall off the end
      this->sendStandardJobCallBackMessage(job);
    }

    unsigned long BatchService::generateUniqueJobId() {
      static unsigned long jobid = 1;
      return jobid++;
    }


}
