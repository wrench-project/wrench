/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <json.hpp>
#include <boost/algorithm/string.hpp>
#include <wrench/util/TraceFileLoader.h>

#include "services/compute/standard_job_executor/StandardJobExecutorMessage.h"
#include "wrench/exceptions/WorkflowExecutionException.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/services/ServiceMessage.h"
#include "wrench/services/compute/ComputeServiceMessage.h"
#include "wrench/services/compute/batch/BatchService.h"
#include "wrench/services/compute/batch/BatchServiceMessage.h"
#include "wrench/services/compute/multihost_multicore/MultihostMulticoreComputeService.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
#include "wrench/simulation/Simulation.h"
#include "wrench/util/MessageManager.h"
#include "wrench/util/PointerUtil.h"
#include "wrench/workflow/job/PilotJob.h"
#include "WorkloadTraceFileReplayer.h"

#ifdef ENABLE_BATSCHED

#include <zmq.hpp>
#include <zmq.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#endif

XBT_LOG_NEW_DEFAULT_CATEGORY(batch_service, "Log category for Batch Service");

namespace wrench {

    BatchService::~BatchService() {
      MessageManager::cleanUpMessages(this->mailbox_name);
    }


    std::map<std::string, double>
    BatchService::getQueueWaitingTimeEstimate(std::set<std::tuple<std::string, unsigned int, double>> set_of_jobs) {

#ifdef ENABLE_BATSCHED
      return getQueueWaitingTimeEstimateFromBatsched(set_of_jobs);
#else
      throw WorkflowExecutionException(std::shared_ptr<FunctionalityNotAvailable>(new FunctionalityNotAvailable(this)));
#endif
    }

    /**
     * @brief Constructor
     * @param hostname: the hostname on which to start the service
     * @param supports_standard_jobs: true if the compute service should support standard jobs
     * @param supports_pilot_jobs: true if the compute service should support pilot jobs
     * @param compute_hosts: the hosts running in the network
     * @param default_storage_service: the default storage service (or nullptr)
     * @param plist: a property list ({} means "use all defaults")
     */
    BatchService::BatchService(std::string &hostname,
                               bool supports_standard_jobs,
                               bool supports_pilot_jobs,
                               std::vector<std::string> compute_hosts,
                               StorageService *default_storage_service,
                               std::map<std::string, std::string> plist) :
            BatchService(hostname, supports_standard_jobs,
                         supports_pilot_jobs, std::move(compute_hosts),
                         default_storage_service, ComputeService::ALL_CORES, std::move(plist), "") {

    }


    /**
     * @brief Constructor
     * @param hostname: the hostname on which to start the service
     * @param supports_standard_jobs: true if the compute service should support standard jobs
     * @param supports_pilot_jobs: true if the compute service should support pilot jobs
     * @param compute_hosts: the hosts running in the network
     * @param default_storage_service: the default storage service (or nullptr)
     * @param cores_per_host: number of cores used per host
     *              - ComputeService::ALL_CORES to use all cores
     * @param plist: a property list ({} means "use all defaults")
     * @param suffix: suffix to append to the service name and mailbox
     *
     * @throw std::invalid_argument
     */
    BatchService::BatchService(std::string hostname,
                               bool supports_standard_jobs,
                               bool supports_pilot_jobs,
                               std::vector<std::string> compute_hosts,
                               StorageService *default_storage_service,
                               unsigned long cores_per_host,
                               std::map<std::string, std::string> plist, std::string suffix) :
            ComputeService(hostname,
                           "batch" + suffix,
                           "batch" + suffix,
                           supports_standard_jobs,
                           supports_pilot_jobs,
                           default_storage_service) {

      // Set default and specified properties
      this->setProperties(this->default_property_values, std::move(plist));

      if (cores_per_host == 0) {
        throw std::invalid_argument("BatchService::BatchService(): compute hosts should have at least one core");
      }

      if (compute_hosts.empty()) {
        throw std::invalid_argument("BatchService::BatchService(): at least one compute hosts must be provided");
      }

      // Check Platform homogeneity
      double num_cores = Simulation::getHostNumCores(*(compute_hosts.begin()));
      double speed = Simulation::getHostFlopRate(*(compute_hosts.begin()));
      double ram = Simulation::getHostMemoryCapacity(*(compute_hosts.begin()));

      for (auto const &h : compute_hosts) {
        // Compute speed
        if (fabs(speed - Simulation::getHostFlopRate(h)) > DBL_EPSILON) {
          throw std::invalid_argument(
                  "BatchService::BatchService(): Compute hosts for a batch service need to be homogeneous (different flop rates detected)");
        }
        // RAM
        if (fabs(ram - Simulation::getHostMemoryCapacity(h)) > DBL_EPSILON) {

          throw std::invalid_argument(
                  "BatchService::BatchService(): Compute hosts for a batch service need to be homogeneous (different RAM capacities detected)");
        }
        // Num cores
        if (cores_per_host == ComputeService::ALL_CORES) {
          if (Simulation::getHostNumCores(h) != num_cores) {
            throw std::invalid_argument(
                    "BatchService::BatchService(): Compute hosts for a batch service need to be homogeneous (different numbers of cores detected");
          }
        } else {
          if (Simulation::getHostNumCores(h) < cores_per_host) {
            throw std::invalid_argument(
                    "BatchService::BatchService(): Compute host has less thatn the specified number of cores (" +
                    std::to_string(cores_per_host) + "'");
          }
        }
      }

      //create a map for host to cores
      int i = 0;
      for (auto h : compute_hosts) {
        // TODO: Why do we have the "not this->supports_pilot_jobs" below?
        if (cores_per_host < ComputeService::ALL_CORES && not this->supports_pilot_jobs) {
          this->nodes_to_cores_map.insert({h, cores_per_host});
          this->available_nodes_to_cores.insert({h, cores_per_host});
        } else {
          this->nodes_to_cores_map.insert({h, S4U_Simulation::getNumCores(h)});
          this->available_nodes_to_cores.insert({h, S4U_Simulation::getNumCores(h)});
        }
        this->host_id_to_names[i++] = h;
      }

      this->num_cores_per_node = this->nodes_to_cores_map.begin()->second;
      this->total_num_of_nodes = compute_hosts.size();

      // Check that the workload file is valid
      std::string workload_file = this->getPropertyValueAsString(BatchServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE);
      if (not workload_file.empty()) {
        try {
          this->workload_trace = TraceFileLoader::loadFromTraceFile(workload_file, 0);
        } catch (std::exception &e) {
          throw;
        }
        for (auto const &j : this->workload_trace) {
          std::string id = std::get<0>(j);
          double submit_time = std::get<1>(j);
          double flops = std::get<2>(j);
          double requested_flops = std::get<3>(j);
          double requested_ram = std::get<4>(j);
          unsigned int num_nodes = std::get<5>(j);

          if (num_nodes > this->total_num_of_nodes) {
            throw std::invalid_argument("Workload trace file contains a job that requires too many compute nodes");
          }
          if (requested_ram > ram) {
            throw std::invalid_argument("Workload trace file contains a job that requires too much ram per compute nodes");
          }
        }
      }

#ifdef ENABLE_BATSCHED
      this->startBatsched();
#endif
      
    }


    /**
     * @brief Synchronously submit a standard job to the batch service
     *
     * @param job: a standard job
     * @param batch_job_args: batch-specific arguments
     *      - "-N": number of hosts
     *      - "-c": number of cores on each host
     *      - "-t": duration (in seconds)
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     * @throw std::invalid_argument
     *
     */
    void BatchService::submitStandardJob(StandardJob *job, std::map<std::string, std::string> &batch_job_args) {

      if (this->state == Service::DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      //check that the number of requested hosts is valid
      unsigned long num_hosts = 0;
      auto it = batch_job_args.find("-N");
      if (it != batch_job_args.end()) {
        if (sscanf((*it).second.c_str(), "%lu", &num_hosts) != 1) {
          throw std::invalid_argument(
                  "BatchService::submitStandardJob(): Invalid -N value '" + (*it).second + "'");
        }
      } else {
        throw std::invalid_argument(
                "BatchService::submitStandardJob(): Batch Service requires -N (number of hosts) to be specified "
        );
      }

      //check that the number of cores per hosts is valid
      unsigned long num_cores_per_host = 0;
      it = batch_job_args.find("-c");
      if (it != batch_job_args.end()) {
        if (sscanf((*it).second.c_str(), "%lu", &num_cores_per_host) != 1) {
          throw std::invalid_argument(
                  "BatchService::submitStandardJob(): Invalid -c value '" + (*it).second + "'");
        }
      } else {
        throw std::invalid_argument(
                "BatchService::submitStandardJob(): Batch Service requires -c (number of cores per host) to be specified "
        );
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
                "BatchService::submitStandardJob(): Batch Service requires -t (duration in seconds) to be specified "
        );
      }

      // Create a Batch Job
      unsigned long jobid = this->generateUniqueJobId();
      auto *batch_job = new BatchJob(job, jobid, time_asked_for,
                                     num_hosts, num_cores_per_host, -1, S4U_Simulation::getClock());

      // Send a "run a batch job" message to the daemon's mailbox_name
      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("batch_standard_job_mailbox");
      try {
        S4U_Mailbox::putMessage(this->mailbox_name,
                                new BatchServiceJobRequestMessage(answer_mailbox, batch_job,
                                                                  this->getPropertyValueAsDouble(
                                                                          BatchServiceProperty::SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD)));
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

      if (auto msg = dynamic_cast<ComputeServiceSubmitStandardJobAnswerMessage *>(message.get())) {
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
     * @param batch_job_args: batch-specific arguments
     *      - "-N": number of hosts
     *      - "-c": number of cores on each host
     *      - "-t": duration (in seconds)
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    void BatchService::submitPilotJob(PilotJob *job, std::map<std::string, std::string> &batch_job_args) {

      if (this->state == Service::DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      // Check the -N argument
      unsigned long nodes_asked_for = 0;
      std::map<std::string, std::string>::iterator it;
      it = batch_job_args.find("-N");
      if (it != batch_job_args.end()) {
        if (sscanf((*it).second.c_str(), "%lu", &nodes_asked_for) != 1) {
          throw std::runtime_error(
                  "BatchService::submitPilotJob(): Invalid -N value '" + (*it).second + "'");
        }
      } else {
        throw std::invalid_argument(
                "BatchService::submitPilotJob(): Batch Service requires -N (number of hosts) to be specified "
        );
      }

      // Check the -c argument
      unsigned long num_cores_per_hosts = 0;
      it = batch_job_args.find("-c");
      if (it != batch_job_args.end()) {
        if (sscanf((*it).second.c_str(), "%lu", &num_cores_per_hosts) != 1) {
          throw std::runtime_error(
                  "BatchService::submitPilotJob(): Invalid -c value '" + (*it).second + "'");
        }
      } else {
        throw std::invalid_argument(
                "BatchService::submitPilotJob(): Batch Service requires -c (number of cores per host) to be specified "
        );
      }


      // Check the -t argument
      unsigned long time_asked_for = 0;
      it = batch_job_args.find("-t");
      if (it != batch_job_args.end()) {
        if (sscanf((*it).second.c_str(), "%lu", &time_asked_for) != 1) {
          throw std::invalid_argument(
                  "BatchService::submitStandardJob(): Invalid -t value '" + (*it).second + "'");
        }
      } else {
        throw std::invalid_argument(
                "BatchService::submitStandardJob(): Batch Service requires -t (duration in seconds) to be specified "
        );
      }

      //Create a Batch Job
      unsigned long jobid = this->generateUniqueJobId();
      auto *batch_job = new BatchJob(job, jobid, time_asked_for,
                                     nodes_asked_for, num_cores_per_hosts, -1, S4U_Simulation::getClock());

      //  send a "run a batch job" message to the daemon's mailbox_name
      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("batch_pilot_job_mailbox");
      try {
        S4U_Mailbox::putMessage(this->mailbox_name,
                                new BatchServiceJobRequestMessage(answer_mailbox, batch_job,
                                                                  this->getPropertyValueAsDouble(
                                                                          BatchServiceProperty::SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD)));
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

      if (auto msg = dynamic_cast<ComputeServiceSubmitPilotJobAnswerMessage *>(message.get())) {
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
     * @brief Terminate a standard job submitted to the compute service
     * @param job: the job
     */
    void BatchService::terminateStandardJob(StandardJob *job) {
      throw std::runtime_error("BatchService::terminateStandardJob(): Not implemented in WRENCH yet!");
    }



    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int BatchService::main() {

      TerminalOutput::setThisProcessLoggingColor(WRENCH_LOGGING_COLOR_MAGENTA);

      WRENCH_INFO("Batch Service starting");

#ifdef ENABLE_BATSCHED
      // Start the Batsched Network Listener
      try {
        this->startBatschedNetworkListener();
      } catch (std::runtime_error &e) {
        throw;
      }
#endif

      // Start the workload trace replayer if needed
      if (not this->workload_trace.empty()) {
        try {
          startBackgroundWorkloadProcess();
        } catch (std::runtime_error &e) {
          throw;
        }
      }

      /** Main loop **/
      bool keep_going = true;
      while (keep_going) {
        keep_going = processNextMessage();

        // Process running jobs
        // TODO: Why do we have to check this? Do we set alarms to do this and we just react to them?
        // TODO: For now, commented out, perhaps useful? We'll see when we add more tests
//        std::set<std::unique_ptr<BatchJob>>::iterator it;
//        for (it = this->running_jobs.begin(); it != this->running_jobs.end();) {
//          if ((*it)->getEndingTimeStamp() <= S4U_Simulation::getClock()) {
//            if ((*it)->getWorkflowJob()->getType() == WorkflowJob::STANDARD) {
//              this->processStandardJobTimeout(
//                      (StandardJob *) (*it)->getWorkflowJob());
//              this->freeUpResources((*it)->getResourcesAllocated());
//              it = this->running_jobs.erase(it);
//            } else if ((*it)->getWorkflowJob()->getType() == WorkflowJob::PILOT) {
//            } else {
//              throw std::runtime_error(
//                      "BatchService::main(): Running a job of type other than Standard and Pilot jobs - Fatal error"
//              );
//            }
//          } else {
//            ++it;
//          }
//        }

#ifdef ENABLE_BATSCHED
        if (keep_going && is_bat_sched_ready) {
          this->sendAllQueuedJobsToBatsched();
        }
#else
        if (keep_going) {
          while (this->scheduleOneQueuedJob());
        }
#endif
      }

      this->clean_exit = true;
      return 0;
    }


    /**
     * @brief Send back notification that a pilot job has expired
     * @param job
     */
    void BatchService::sendPilotJobExpirationNotification(PilotJob *job) {
      try {
        S4U_Mailbox::dputMessage(job->popCallbackMailbox(),
                                 new ComputeServicePilotJobExpiredMessage(job, this,
                                                                          this->getPropertyValueAsDouble(
                                                                                  BatchServiceProperty::PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        return; // ignore
      }
    }

    /**
     * @brief Send back notification that a standard job has failed
     * @param job
     */
    void BatchService::sendStandardJobFailureNotification(StandardJob *job, std::string job_id) {
      WRENCH_INFO("A standard job executor has failed because of timeout %s", job->getName().c_str());

#ifdef ENABLE_BATSCHED
      this->notifyJobEventsToBatSched(job_id, "TIMEOUT", "COMPLETED_FAILED", "");
#endif

      try {
        S4U_Mailbox::putMessage(job->popCallbackMailbox(),
                                new ComputeServiceStandardJobFailedMessage(job, this,
                                                                           std::shared_ptr<FailureCause>(
                                                                                   new ServiceIsDown(this)),
                                                                           this->getPropertyValueAsDouble(
                                                                                   BatchServiceProperty::STANDARD_JOB_FAILED_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        return; // ignore
      }

    }

    /**
     * @brief Increase resource availabilities based on freed resources
     * @param resources
     */
    void BatchService::freeUpResources(std::set<std::tuple<std::string, unsigned long, double>> resources) {
      for (auto r : resources) {
        this->available_nodes_to_cores[std::get<0>(r)] += std::get<1>(r);
      }
    }


    void BatchService::removeJobFromRunningList(BatchJob *job) {
      if (this->running_jobs.find(job) == this->running_jobs.end()) {
        throw std::runtime_error("BatchService::removeJobFromRunningList(): Cannot find job!");
      }
      this->running_jobs.erase(job);
    }

    void BatchService::freeJobFromJobsList(BatchJob *job) {
      if (job == nullptr) {
        return;
      }

      for (auto const & j: this->all_jobs) {
        if (j->getWorkflowJob() == job->getWorkflowJob()) {
          this->all_jobs.erase(j);
          break;
        }
      }
    }


    void BatchService::processPilotJobTimeout(PilotJob *job) {
      auto *cs = (BatchService *) job->getComputeService();
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

    void BatchService::processStandardJobTimeout(StandardJob *job) {
      std::set<std::shared_ptr<StandardJobExecutor>>::iterator it;

      for (it = this->running_standard_job_executors.begin(); it != this->running_standard_job_executors.end(); it++) {
        if (((*it).get())->getJob() == job) {
          ((*it).get())->kill();
          PointerUtil::moveSharedPtrFromSetToSet(it, &(this->running_standard_job_executors),
                                                 &(this->finished_standard_job_executors));
          break;
        }
      }
    }

    /**
    * @brief terminate a running standard job
    * @param job: the job
    */
    void BatchService::terminateRunningStandardJob(StandardJob *job) {

      StandardJobExecutor *executor = nullptr;
      std::set<std::shared_ptr<StandardJobExecutor>>::iterator it;
      for (it = this->running_standard_job_executors.begin();
           it != this->running_standard_job_executors.end(); it++) {
        if (((*it))->getJob() == job) {
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

      // Do not update the resource availability, because this is done at a higher level

    }

    std::set<std::tuple<std::string, unsigned long, double>>
    BatchService::scheduleOnHosts(std::string host_selection_algorithm,
                                  unsigned long num_nodes,
                                  unsigned long cores_per_node,
                                  double ram_per_node) {

      if (ram_per_node > Simulation::getHostMemoryCapacity(this->available_nodes_to_cores.begin()->first)) {
        throw std::runtime_error("BatchService::scheduleOnHosts(): Asking for too much RAM per host");
      }
      if (num_nodes > this->available_nodes_to_cores.size()) {
        throw std::runtime_error("BatchService::scheduleOnHosts(): Asking for too many hosts");
      }
      if (cores_per_node > Simulation::getHostNumCores(this->available_nodes_to_cores.begin()->first)) {
        throw std::runtime_error("BatchService::scheduleOnHosts(): Asking for too many cores per host");
      }

      std::set<std::tuple<std::string, unsigned long, double>> resources = {};
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
            resources.insert(std::make_tuple((*it).first, cores_per_node, ram_per_node));
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
            std::string hostname = std::get<0>(h);
            unsigned long num_available_cores = std::get<1>(h);
            if (num_available_cores < cores_per_node) {
              continue;
            }
            unsigned long tentative_target_num_cores = MIN(num_available_cores, cores_per_node);
            unsigned long tentative_target_slack =
                    num_available_cores - tentative_target_num_cores;

            if (target_host.empty() ||
                (tentative_target_num_cores > target_num_cores) ||
                ((tentative_target_num_cores == target_num_cores) &&
                 (target_slack > tentative_target_slack))) {
              target_host = hostname;
              target_num_cores = tentative_target_num_cores;
              target_slack = tentative_target_slack;
            }
          }
          if (target_host == "") {
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
          resources.insert(std::make_tuple(target_host, cores_per_node, 0)); // TODO: RAM is set to 0 for now
        }
      } else {
        throw std::invalid_argument(
                "BatchService::scheduleOnHosts(): We don't support " + host_selection_algorithm +
                " as host selection algorithm"
        );
      }

      return resources;
    }

    BatchJob *BatchService::pickJobForScheduling(std::string job_selection_algorithm) {
      if (job_selection_algorithm == "FCFS") {
        BatchJob *batch_job = *this->pending_jobs.begin();
        return batch_job;
      }
      return nullptr;
    }



    /**
     * @brief Schedule one queued job
     * @return true if a job was scheduled, false otherwise
     */
    bool BatchService::scheduleOneQueuedJob() {

      if (this->pending_jobs.empty()) {
        return false;
      }

      BatchJob *batch_job = pickJobForScheduling(this->getPropertyValueAsString(BatchServiceProperty::JOB_SELECTION_ALGORITHM));
      if (batch_job == nullptr) {
        throw std::runtime_error(
                "BatchService::scheduleAllQueuedJobs(): Got no such job in pending queue to dispatch"
        );
      }
      WorkflowJob *workflow_job = batch_job->getWorkflowJob();

      /* Get the nodes and cores per nodes asked for */
      unsigned long cores_per_node_asked_for = batch_job->getAllocatedCoresPerNode();
      unsigned long num_nodes_asked_for = batch_job->getNumNodes();
      unsigned long time_in_minutes = batch_job->getAllocatedTime();

      WRENCH_INFO("Trying to see if I can run job %s", workflow_job->getName().c_str());

      //Try to schedule hosts based on FIRSTFIT OR BESTFIT
      // Asking for the FULL RAM (TODO: Change this?)
      std::set<std::tuple<std::string, unsigned long, double>> resources = this->scheduleOnHosts(
              this->getPropertyValueAsString(BatchServiceProperty::HOST_SELECTION_ALGORITHM),
              num_nodes_asked_for, cores_per_node_asked_for, ComputeService::ALL_CORES);


      if (resources.empty()) {
        WRENCH_INFO("Can't run job %s", workflow_job->getName().c_str());
        return false;
      }

      WRENCH_INFO("RUNNING job %s", workflow_job->getName().c_str());

      // Remove it from the pending list
      for (auto it = this->pending_jobs.begin(); it != this->pending_jobs.end(); it++) {
        if ((*it) == batch_job) {
          this->pending_jobs.erase(it);
          break;
        }
      }

      this->running_jobs.insert(batch_job);

      startJob(resources, workflow_job, batch_job, num_nodes_asked_for, time_in_minutes,
               cores_per_node_asked_for);
      return true;

    }

    /**
    * @brief Declare all current jobs as failed (likely because the daemon is being terminated
    * or has timed out (because it's in fact a pilot job))
    */
    void BatchService::failCurrentStandardJobs(std::shared_ptr<FailureCause> cause) {
      for (auto const & j : this->running_jobs) {
        WorkflowJob *workflow_job = j->getWorkflowJob();
        if (workflow_job->getType() == WorkflowJob::STANDARD) {
          auto *job = (StandardJob *) workflow_job;
          terminateRunningStandardJob(job);
          this->sendStandardJobFailureNotification(job, std::to_string(j->getJobID()));
          this->running_jobs.erase(j);
          this->freeJobFromJobsList(j);
        }
      }

      for (auto it1 = this->pending_jobs.begin(); it1 != this->pending_jobs.end(); it1++) {
        WorkflowJob *workflow_job = (*it1)->getWorkflowJob();
        if (workflow_job->getType() == WorkflowJob::STANDARD) {
          auto *job = (StandardJob *) workflow_job;
          this->sendStandardJobFailureNotification(job, std::to_string((*it1)->getJobID()));
          this->pending_jobs.erase(it1);
          this->freeJobFromJobsList(*it1);
        }
      }

      for (auto it2 = this->waiting_jobs.begin(); it2 != this->waiting_jobs.end(); it2++) {
        WorkflowJob *workflow_job = (*it2)->getWorkflowJob();
        if (workflow_job->getType() == WorkflowJob::STANDARD) {
          auto *job = (StandardJob *) workflow_job;
          this->sendStandardJobFailureNotification(job, std::to_string((*it2)->getJobID()));
          this->waiting_jobs.erase(it2);
          this->freeJobFromJobsList(*it2);
        }
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
    void BatchService::terminatePilotJob(PilotJob *job) {

      if (this->state == Service::DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("terminate_pilot_job");

      // Send a "terminate a pilot job" message to the daemon's mailbox_name
      try {
        S4U_Mailbox::putMessage(this->mailbox_name,
                                new ComputeServiceTerminatePilotJobRequestMessage(answer_mailbox, job,
                                                                                  this->getPropertyValueAsDouble(
                                                                                          BatchServiceProperty::TERMINATE_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD)));
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

      if (auto msg = dynamic_cast<ComputeServiceTerminatePilotJobAnswerMessage *>(message.get())) {
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
 * @brief Terminate the daemon, dealing with pending/running jobs
 */
    void BatchService::cleanup() {

      if (this->clean_exit) {
        this->setStateToDown();
        WRENCH_INFO("Failing current standard jobs");
        this->failCurrentStandardJobs(std::shared_ptr<FailureCause>(new ServiceIsDown(this)));
      }

#ifdef ENABLE_BATSCHED
      this->stopBatsched();
#endif

      if (this->clean_exit) {
        if (this->supports_pilot_jobs) {
          for (auto &job : this->running_jobs) {
            if ((job)->getWorkflowJob()->getType() == WorkflowJob::PILOT) {
              PilotJob *p_job = (PilotJob *) ((job)->getWorkflowJob());
              BatchService *cs = (BatchService *) p_job->getComputeService();
              if (cs == nullptr) {
                throw std::runtime_error(
                        "BatchService::terminate(): can't find compute service associated to pilot job");
              }
              try {
                cs->stop();
              } catch (wrench::WorkflowExecutionException &e) {
                // ignore
              }
            }
          }
        }
      }

    }


    bool BatchService::processNextMessage() {

      // Wait for a message
      std::unique_ptr<SimulationMessage> message = nullptr;

      try {
        message = S4U_Mailbox::getMessage(this->mailbox_name);
      } catch (std::shared_ptr<NetworkError> &cause) {
        return true;
      } catch (std::shared_ptr<NetworkTimeout> &cause) {
        return true;
      }

      if (message == nullptr) {
        WRENCH_INFO("Got a NULL message... Likely this means we're all done. Aborting!");
        return false;
      }

      WRENCH_INFO("Got a [%s] message", message->getName().c_str());


      if (auto msg = dynamic_cast<ServiceStopDaemonMessage *>(message.get())) {
        this->cleanup();
        // This is Synchronous;
        try {
          S4U_Mailbox::putMessage(msg->ack_mailbox,
                                  new ServiceDaemonStoppedMessage(this->getPropertyValueAsDouble(
                                          BatchServiceProperty::DAEMON_STOPPED_MESSAGE_PAYLOAD)));

        } catch (std::shared_ptr<NetworkError> &cause) {
          return false;
        }
        return false;

      } else if (auto msg = dynamic_cast<ComputeServiceResourceInformationRequestMessage *>(message.get())) {
        processGetResourceInformation(msg->answer_mailbox);
        return true;

      } else if (auto msg = dynamic_cast<BatchServiceJobRequestMessage *>(message.get())) {
        processJobSubmission(msg->job, msg->answer_mailbox);
        return true;

      } else if (auto msg = dynamic_cast<StandardJobExecutorDoneMessage *>(message.get())) {
        processStandardJobCompletion(msg->executor, msg->job);
        return true;

      } else if (auto msg = dynamic_cast<StandardJobExecutorFailedMessage *>(message.get())) {
        processStandardJobFailure(msg->executor, msg->job, msg->cause);
        return true;

      } else if (auto msg = dynamic_cast<ComputeServicePilotJobExpiredMessage *>(message.get())) {
        processPilotJobCompletion(msg->job);
        return true;
      } else if (auto msg = dynamic_cast<ComputeServiceTerminatePilotJobRequestMessage *>(message.get())) {
        processPilotJobTerminationRequest(msg->job, msg->answer_mailbox);
        return true;

      } else if (auto msg = dynamic_cast<AlarmJobTimeOutMessage *>(message.get())) {
        if (msg->job->getWorkflowJob()->getType() == WorkflowJob::STANDARD) {
          this->processStandardJobTimeout((StandardJob *)(msg->job->getWorkflowJob()));
          this->removeJobFromRunningList(msg->job);
          this->freeUpResources(msg->job->getResourcesAllocated());
          this->sendStandardJobFailureNotification((StandardJob *) msg->job->getWorkflowJob(), std::to_string(msg->job->getJobID()));
          return true;
        } else if (msg->job->getWorkflowJob()->getType() == WorkflowJob::PILOT) {
          auto *pilot_job = (PilotJob *) msg->job->getWorkflowJob();
          ComputeService *cs = pilot_job->getComputeService();
          try {
            cs->stop();
          } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error(
                    "BatchService::processNextMessage(): Not able to terminate the pilot job"
            );
          }
          this->processPilotJobCompletion(pilot_job);
          return true;
        } else {
          throw std::runtime_error(
                  "BatchService::processNextMessage(): Alarm about unknown job type"
          );
        }
#ifdef ENABLE_BATSCHED
      } else if (auto msg = dynamic_cast<BatchSchedReadyMessage *>(message.get())) {
        is_bat_sched_ready = true;
        return true;

      } else if (auto msg = dynamic_cast<BatchExecuteJobFromBatSchedMessage *>(message.get())) {
        processExecuteJobFromBatSched(msg->batsched_decision_reply);
        return true;
      } else if (auto msg = dynamic_cast<AlarmNotifyBatschedMessage *>(message.get())) {
        //first forward this notification to the batsched
        this->notifyJobEventsToBatSched(msg->job_id, "SUCCESS", "COMPLETED_SUCCESSFULLY", "");
        return true;
#endif
      } else {
        throw std::runtime_error(
                "BatchService::processNextMessage(): Unknown message type: " +
                std::to_string(message->payload));
        return false;
      }

    }


    /**
     * @brief Process a job submission
     *
     * @param job: the batch job object
     * @param answer_mailbox: the mailbox to which answer messages should be sent
     */
    void BatchService::processJobSubmission(BatchJob *job, std::string answer_mailbox) {

      WRENCH_INFO("Asked to run a batch job with id %ld", job->getJobID());

      // Check whether the job type is supported
      if ((job->getWorkflowJob()->getType() == WorkflowJob::STANDARD) and (not this->supports_standard_jobs)) {
        try {
          S4U_Mailbox::dputMessage(answer_mailbox,
                                   new ComputeServiceSubmitStandardJobAnswerMessage(
                                           (StandardJob *) job->getWorkflowJob(),
                                           this,
                                           false,
                                           std::shared_ptr<FailureCause>(
                                                   new JobTypeNotSupported(
                                                           job->getWorkflowJob(),
                                                           this)),
                                           this->getPropertyValueAsDouble(
                                                   BatchServiceProperty::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
          return;
        }
        return;
      } else if ((job->getWorkflowJob()->getType() == WorkflowJob::PILOT) and (not this->supports_pilot_jobs)) {
        try {
          S4U_Mailbox::dputMessage(answer_mailbox,
                                   new ComputeServiceSubmitPilotJobAnswerMessage(
                                           (PilotJob *) job->getWorkflowJob(),
                                           this,
                                           false,
                                           std::shared_ptr<FailureCause>(
                                                   new JobTypeNotSupported(
                                                           job->getWorkflowJob(),
                                                           this)),
                                           this->getPropertyValueAsDouble(
                                                   BatchServiceProperty::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
          return;
        }
        return;
      }

      // Check that the job can be admitted in terms of resources:
      //      - number of nodes,
      //      - number of cores per host
      //      - RAM
      unsigned long requested_hosts = job->getNumNodes();
      unsigned long requested_num_cores_per_host = job->getAllocatedCoresPerNode();

      if (job->getWorkflowJob()->getType() == WorkflowJob::STANDARD) {

        double required_ram_per_host = job->getMemoryRequirement();

        if ((requested_hosts > this->available_nodes_to_cores.size()) or
            (requested_num_cores_per_host >
             Simulation::getHostNumCores(this->available_nodes_to_cores.begin()->first)) or
            (required_ram_per_host >
             Simulation::getHostMemoryCapacity(this->available_nodes_to_cores.begin()->first))) {

          {
            try {
              S4U_Mailbox::dputMessage(answer_mailbox,
                                       new ComputeServiceSubmitStandardJobAnswerMessage(
                                               (StandardJob *) job->getWorkflowJob(),
                                               this,
                                               false,
                                               std::shared_ptr<FailureCause>(
                                                       new NotEnoughComputeResources(
                                                               job->getWorkflowJob(),
                                                               this)),
                                               this->getPropertyValueAsDouble(
                                                       BatchServiceProperty::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));
            } catch (std::shared_ptr<NetworkError> &cause) {}
            return;
          }
        }
      }

      if ((requested_hosts > this->available_nodes_to_cores.size()) or
          (requested_num_cores_per_host >
           Simulation::getHostNumCores(this->available_nodes_to_cores.begin()->first))) {
        try {
          S4U_Mailbox::dputMessage(answer_mailbox,
                                   new ComputeServiceSubmitPilotJobAnswerMessage(
                                           (PilotJob *) job->getWorkflowJob(),
                                           this,
                                           false,
                                           std::shared_ptr<FailureCause>(
                                                   new NotEnoughComputeResources(
                                                           job->getWorkflowJob(),
                                                           this)),
                                           this->getPropertyValueAsDouble(
                                                   BatchServiceProperty::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {}
        return;
      }


      if (job->getWorkflowJob()->getType() == WorkflowJob::STANDARD) {

        try {
          S4U_Mailbox::dputMessage(answer_mailbox,
                                   new ComputeServiceSubmitStandardJobAnswerMessage(
                                           (StandardJob *) job->getWorkflowJob(), this,
                                           true,
                                           nullptr,
                                           this->getPropertyValueAsDouble(
                                                   BatchServiceProperty::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
          return;
        }
      } else if (job->getWorkflowJob()->getType() == WorkflowJob::PILOT) {
        try {
          S4U_Mailbox::dputMessage(answer_mailbox,
                                   new ComputeServiceSubmitPilotJobAnswerMessage(
                                           (PilotJob *) job->getWorkflowJob(), this,
                                           true,
                                           nullptr,
                                           this->getPropertyValueAsDouble(
                                                   BatchServiceProperty::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
          return;
        }
      }
      this->all_jobs.insert(std::unique_ptr<BatchJob>(job));
      this->pending_jobs.push_back(job);
    }

    /**
     * @brief Process a pilot job completion
     *
     * @param job: the pilot job
     */
    void BatchService::processPilotJobCompletion(PilotJob *job) {

      // Remove the job from the running job list
      BatchJob *batch_job = nullptr;
      for (auto it = this->running_jobs.begin(); it != this->running_jobs.end();) {
        if ((*it)->getWorkflowJob() == job) {
          batch_job = (*it);
          break;
        }
      }

      if (batch_job == nullptr) {
        throw std::runtime_error(
                "BatchService::processPilotJobCompletion():  Pilot job completion message recevied but no such pilot jobs found in queue"
        );
      }

      this->removeJobFromRunningList(batch_job);
      this->freeUpResources(batch_job->getResourcesAllocated());
      if (this->pilot_job_alarms[job->getName()] != nullptr) {
        this->pilot_job_alarms[job->getName()]->kill();
      }

      //first forward this notification to the batsched
#ifdef ENABLE_BATSCHED
      this->notifyJobEventsToBatSched(std::to_string(batch_job->getJobID()), "SUCCESS", "COMPLETED_SUCCESSFULLY", "");
#endif

      // Forward the notification
      try {
        this->sendPilotJobExpirationNotification(job);
      } catch (std::runtime_error &e) {
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
    void BatchService::processPilotJobTerminationRequest(PilotJob *job, std::string answer_mailbox) {

      std::string job_id;
      for (auto it = this->pending_jobs.begin(); it != this->pending_jobs.end(); it++) {
        if ((*it)->getWorkflowJob() == job) {
          job_id = std::to_string((*it)->getJobID());
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
          this->freeJobFromJobsList(*it);
          //first forward this notification to the batsched
#ifdef ENABLE_BATSCHED
          this->notifyJobEventsToBatSched(job_id, "TIMEOUT", "NOT_SUBMITTED", "");
#endif
          return;
        }
      }

      for (auto it2 = this->waiting_jobs.begin(); it2 != this->waiting_jobs.end(); it2++) {
        if ((*it2)->getWorkflowJob() == job) {
          job_id = std::to_string((*it2)->getJobID());
          this->waiting_jobs.erase(it2);
          ComputeServiceTerminatePilotJobAnswerMessage *answer_message = new ComputeServiceTerminatePilotJobAnswerMessage(
                  job, this, true, nullptr,
                  this->getPropertyValueAsDouble(
                          BatchServiceProperty::TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD));
          try {
            S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
          } catch (std::shared_ptr<NetworkError> &cause) {
            return;
          }
          this->freeJobFromJobsList(*it2);
          //first forward this notification to the batsched
#ifdef ENABLE_BATSCHED
          this->notifyJobEventsToBatSched(job_id, "TIMEOUT", "NOT_SUBMITTED", "");
#endif
          return;
        }
      }

      for (auto it1 = this->running_jobs.begin(); it1 != this->running_jobs.end();) {
        if ((*it1)->getWorkflowJob() == job) {
          job_id = std::to_string((*it1)->getJobID());
          this->processPilotJobTimeout((PilotJob *) (*it1)->getWorkflowJob());
          // Update the cores count in the available resources
          std::set<std::tuple<std::string, unsigned long, double>> resources = (*it1)->getResourcesAllocated();
          for (auto r : resources) {
            this->available_nodes_to_cores[std::get<0>(r)] += std::get<1>(r);
          }
          ComputeServiceTerminatePilotJobAnswerMessage *answer_message = new ComputeServiceTerminatePilotJobAnswerMessage(
                  job, this, true, nullptr,
                  this->getPropertyValueAsDouble(
                          BatchServiceProperty::TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD));
          try {
            S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
          } catch (std::shared_ptr<NetworkError> &cause) {
            return;
          }
          //this is the list of raw pointers
          this->running_jobs.erase(it1);

          //this is the list of unique pointers
          this->freeJobFromJobsList(*it1);
          //first forward this notification to the batsched
#ifdef ENABLE_BATSCHED
          this->notifyJobEventsToBatSched(job_id, "TIMEOUT", "COMPLETED_FAILED", "");
#endif
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
     * @brief Process a standard job completion
     * @param executor: the standard job executor
     * @param job: the job
     *
     * @throw std::runtime_error
     */
    void BatchService::processStandardJobCompletion(StandardJobExecutor *executor, StandardJob *job) {
      bool executor_on_the_list = false;
      std::set<std::shared_ptr<StandardJobExecutor>>::iterator it;
      for (it = this->running_standard_job_executors.begin(); it != this->running_standard_job_executors.end(); it++) {
        if ((*it).get() == executor) {
          PointerUtil::moveSharedPtrFromSetToSet(it, &(this->running_standard_job_executors),
                                                 &(this->finished_standard_job_executors));
          executor_on_the_list = true;
          this->standard_job_alarms[job->getName()]->kill();
          break;
        }
      }

      if (not executor_on_the_list) {
        throw std::runtime_error(
                "BatchService::processStandardJobCompletion(): Received a standard job completion, but the executor is not in the executor list");
      }

      // Look for the corresponding batch job
      BatchJob *batch_job = nullptr;
      for (auto const & rj : this->running_jobs) {
        if (rj->getWorkflowJob() == job) {
          batch_job = rj;
          break;
        }
      }

      if (batch_job == nullptr) {
        throw std::runtime_error(
                "BatchService::processStandardJobCompletion(): Received a standard job completion, but the job is not in the running job list");
      }

      // Free up resources (by finding the corresponding BatchJob)
      this->freeUpResources(batch_job->getResourcesAllocated());

      // Remove the job from the running job list
      this->removeJobFromRunningList(batch_job);


      WRENCH_INFO("A standard job executor has completed job %s", job->getName().c_str());

      //first forward this notification to the batsched
#ifdef ENABLE_BATSCHED
      this->notifyJobEventsToBatSched(std::to_string(batch_job->getJobID()), "SUCCESS", "COMPLETED_SUCCESSFULLY", "");
#endif

      // Send the callback to the originator
      try {
        S4U_Mailbox::dputMessage(job->popCallbackMailbox(),
                                 new ComputeServiceStandardJobDoneMessage(job, this,
                                                                          this->getPropertyValueAsDouble(
                                                                                  BatchServiceProperty::STANDARD_JOB_DONE_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        return;
      }

      //Free the job from the global (pending, running, waiting) job list, (doing this at the end of this method to make sure
      // this job is not used anymore anywhere)
      this->freeJobFromJobsList(batch_job);

      return;
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
      std::set<std::shared_ptr<StandardJobExecutor>>::iterator it;
      for (it = this->running_standard_job_executors.begin(); it != this->running_standard_job_executors.end(); it++) {
        if ((*it).get() == executor) {
          PointerUtil::moveSharedPtrFromSetToSet(it, &(this->running_standard_job_executors),
                                                 &(this->finished_standard_job_executors));
          executor_on_the_list = true;
          this->standard_job_alarms[job->getName()]->kill();
          break;
        }
      }

      if (not executor_on_the_list) {
        throw std::runtime_error(
                "BatchService::processStandardJobCompletion(): Received a standard job completion, but the executor is not in the executor list");
      }

      // Free up resources (by finding the corresponding BatchJob)
      BatchJob *batch_job = nullptr;
      for (auto const & rj : this->running_jobs) {
        if (rj->getWorkflowJob() == job) {
          batch_job = rj;
        }
      }

      if (batch_job == nullptr) {
        throw std::runtime_error(
                "BatchService::processStandardJobFailure(): Received a standard job completion, but the job is not in the running job list");
      }

      this->freeUpResources(batch_job->getResourcesAllocated());
      this->removeJobFromRunningList(batch_job);


      WRENCH_INFO("A standard job executor has failed to perform job %s", job->getName().c_str());

      //first forward this notification to the batsched
#ifdef ENABLE_BATSCHED
      this->notifyJobEventsToBatSched(std::to_string(batch_job->getJobID()), "TIMEOUT", "COMPLETED_FAILED", "");
#endif

      this->sendStandardJobFailureNotification(job, std::to_string((batch_job->getJobID())));
      //Free the job from the global (pending, running, waiting) job list, (doing this at the end of this method to make sure
      // this job is not used anymore anywhere)
      this->freeJobFromJobsList(batch_job);

    }


    unsigned long BatchService::generateUniqueJobId() {
      static unsigned long jobid = 1;
      return jobid++;
    }




    void
    BatchService::startJob(std::set<std::tuple<std::string, unsigned long, double>> resources,
                           WorkflowJob *workflow_job,
                           BatchJob *batch_job, unsigned long num_nodes_allocated,
                           unsigned long time_in_minutes,
                           unsigned long cores_per_node_asked_for) {
      switch (workflow_job->getType()) {
        case WorkflowJob::STANDARD: {
          auto job = (StandardJob *) workflow_job;
          WRENCH_INFO("Creating a StandardJobExecutor on %ld cores for a standard job on %ld nodes",
                      cores_per_node_asked_for, num_nodes_allocated);
          // Create a standard job executor
          std::shared_ptr<StandardJobExecutor> executor = std::shared_ptr<StandardJobExecutor>(
                  new StandardJobExecutor(
                          this->simulation,
                          this->mailbox_name,
                          std::get<0>(*resources.begin()),
                          (StandardJob *) workflow_job,
                          resources,
                          this->default_storage_service,
                          {{StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD,
                                   this->getPropertyValueAsString(
                                           BatchServiceProperty::THREAD_STARTUP_OVERHEAD)}}));
          executor->start(executor, true);

          this->running_standard_job_executors.insert(executor);
          batch_job->setEndingTimeStamp(S4U_Simulation::getClock() + time_in_minutes * 60);
//          this->running_jobs.insert(std::move(batch_job_ptr));
          this->timeslots.push_back(batch_job->getEndingTimeStamp());
          //remember the allocated resources for the job
          batch_job->setAllocatedResources(resources);

          SimulationMessage *msg =
                  new AlarmJobTimeOutMessage(batch_job, 0);

          std::shared_ptr<Alarm> alarm_ptr = Alarm::createAndStartAlarm(this->simulation,
                                                                        batch_job->getEndingTimeStamp(), this->hostname,
                                                                        this->mailbox_name, msg,
                                                                        "batch_standard");
          standard_job_alarms[job->getName()] = alarm_ptr;


          return;
        }
          break;

        case WorkflowJob::PILOT: {
          PilotJob *job = (PilotJob *) workflow_job;
          WRENCH_INFO("Allocating %ld nodes with %ld cores per node to a pilot job",
                      num_nodes_allocated, cores_per_node_asked_for);

          std::vector<std::string> nodes_for_pilot_job = {};
          for (auto r : resources) {
            nodes_for_pilot_job.push_back(std::get<0>(r));
          }
          std::string host_to_run_on = nodes_for_pilot_job[0];

          //set the ending timestamp of the batchjob (pilotjob)

          double timeout_timestamp = std::min(job->getDuration(), time_in_minutes * 60 * 1.0);
          batch_job->setEndingTimeStamp(S4U_Simulation::getClock() + timeout_timestamp);

          // Create and launch a compute service for the pilot job
          std::shared_ptr<ComputeService> cs = std::shared_ptr<ComputeService>(
                  new MultihostMulticoreComputeService(host_to_run_on,
                                                       true, false,
                                                       resources,
                                                       this->default_storage_service
                  ));
          cs->setSimulation(this->simulation);
          job->setComputeService(cs);

          try {
            cs->start(cs, true);
          } catch (std::runtime_error &e) {
            throw;
          }

          // Put the job in the running queue
//          this->running_jobs.insert(std::move(batch_job_ptr));
          this->timeslots.push_back(batch_job->getEndingTimeStamp());

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
          } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
          }

          SimulationMessage *msg =
                  new AlarmJobTimeOutMessage(batch_job, 0);
          std::shared_ptr<Alarm> alarm_ptr = Alarm::createAndStartAlarm(this->simulation,
                                                                        batch_job->getEndingTimeStamp(), host_to_run_on,
                                                                        this->mailbox_name, msg,
                                                                        "batch_pilot");

          this->pilot_job_alarms[job->getName()] = alarm_ptr;

          // Push my own mailbox_name onto the pilot job!
//          job->pushCallbackMailbox(this->mailbox_name);

          return;
        }
          break;
      }
    }




    /**
    * @brief Process a "get resource description message"
    * @param answer_mailbox: the mailbox to which the description message should be sent
    */
    void BatchService::processGetResourceInformation(const std::string &answer_mailbox) {
      // Build a dictionary
      std::map<std::string, std::vector<double>> dict;

      // Num hosts
      std::vector<double> num_hosts;
      num_hosts.push_back((double) (this->nodes_to_cores_map.size()));
      dict.insert(std::make_pair("num_hosts", num_hosts));

      // Num cores per hosts
      std::vector<double> num_cores;
      for (auto h : this->nodes_to_cores_map) {
        num_cores.push_back((double) (h.second));
      }
      dict.insert(std::make_pair("num_cores", num_cores));

      // Num idle cores per hosts
      std::vector<double> num_idle_cores;
      for (auto h : this->available_nodes_to_cores) {
        num_idle_cores.push_back((double) (h.second));
      }
      dict.insert(std::make_pair("num_idle_cores", num_idle_cores));

      // Flop rate per host
      std::vector<double> flop_rates;
      for (auto h : this->nodes_to_cores_map) {
        flop_rates.push_back(S4U_Simulation::getFlopRate(h.first));
      }
      dict.insert(std::make_pair("flop_rates", flop_rates));

      // RAM capacity per host
      std::vector<double> ram_capacities;
      for (auto h : this->nodes_to_cores_map) {
        ram_capacities.push_back(S4U_Simulation::getHostMemoryCapacity(h.first));
      }
      dict.insert(std::make_pair("ram_capacities", ram_capacities));

      // RAM availability per host  (0 if something is running, full otherwise)
      std::vector<double> ram_availabilities;
      for (auto h : this->available_nodes_to_cores) {
        if (h.second < S4U_Simulation::getHostMemoryCapacity(h.first)) {
          ram_availabilities.push_back(0.0);
        } else {
          ram_availabilities.push_back(S4U_Simulation::getHostMemoryCapacity(h.first));
        }
      }


      std::vector<double> ttl;
      ttl.push_back(ComputeService::ALL_RAM);
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
     * @brief Start the process that will replay the background load
     *
     * @throw std::runtime_error
     */
    void BatchService::startBackgroundWorkloadProcess() {
      if (this->workload_trace.empty()) {
        throw std::runtime_error("BatchService::startBackgroundWorkloadProcess(): no workload trace file specified");
      }

      // Create the trace replayer process
      this->workload_trace_replayer = std::shared_ptr<WorkloadTraceFileReplayer>(
              new WorkloadTraceFileReplayer(this->simulation, S4U_Simulation::getHostName(), this,
                                            this->num_cores_per_node, this->workload_trace)
      );
      try {
        this->workload_trace_replayer->setSimulation(this->simulation);
        this->workload_trace_replayer->start(this->workload_trace_replayer, true);
      } catch (std::runtime_error &e) {
        throw;
      }
      return;
    }


    /********************************************************************************************/
    /** BATSCHED INTERFACE METHODS BELOW                                                        */
    /********************************************************************************************/

#ifdef ENABLE_BATSCHED

    /**
     * @brief: Start a batsched process
     *           - exit code 1: unsupported algorithm
     *           - exit code 2: unsupported queuing option
     *           - exit code 3: execvp error
     */
    void BatchService::startBatsched() {

      this->batsched_port = 28000 + S4U_Mailbox::generateUniqueSequenceNumber();
      this->pid = getpid();


      int top_pid = fork();
      if (top_pid == 0) { // Child process that will exec batsched

        std::string algorithm = this->getPropertyValueAsString(BatchServiceProperty::BATCH_SCHEDULING_ALGORITHM);
        bool is_supported = this->scheduling_algorithms.find(algorithm) != this->scheduling_algorithms.end();
        if (not is_supported) {
          exit(1);
        }

        std::string queue_ordering = this->getPropertyValueAsString(
                BatchServiceProperty::BATCH_QUEUE_ORDERING_ALGORITHM);
        bool is_queue_ordering_available =
                this->queue_ordering_options.find(queue_ordering) != this->queue_ordering_options.end();
        if (not is_queue_ordering_available) {
          std::cerr << "The queue ordering option " + queue_ordering + " is not supported by the batch service" << "\n";
          exit(2);
        }

        std::string rjms_delay = this->getPropertyValueAsString(BatchServiceProperty::BATCH_RJMS_DELAY);
        std::string socket_endpoint = "tcp://*:" + std::to_string(this->batsched_port);
        const char *args[] = {"batsched", "-v", algorithm.c_str(), "-o", queue_ordering.c_str(), "-s",
                              socket_endpoint.c_str(), "--rjms_delay", rjms_delay.c_str(), NULL};
        if (execvp(args[0], (char **) args) == -1) {
          exit(3);
        }


      } else if (top_pid > 0) {
        // parent process
        sleep(1); // Wait one second to let batsched the time to start (this is pretty ugly)
        int exit_code = waitpid(top_pid, NULL, WNOHANG);
        switch (exit_code) {
          case 0: {
            int tether[2]; // this is a local variable, only defined in this scope
            if (pipe(tether) != 0) {  // the pipe however is opened during the whole duration of both processes
              throw std::runtime_error("startBatsched(): pipe failed.");
            }
            //now fork a process that sleeps until its parent is dead
            int nested_pid = fork();

            if (nested_pid > 0) {
              //I am the parent, whose child fork exec'd batsched
            } else if (nested_pid == 0) {
              char foo;
              close(tether[1]); // closing write end
              read(tether[0], &foo, 1); // blocking read which returns when the parent dies

              //check if the child that forked batsched is still running
              if (getpgid(top_pid)) {
                kill(top_pid, SIGKILL); //kill the other child that fork exec'd batsched
              }
              //my parent has died so, I will kill myself instead of exiting and becoming a zombie
              kill(getpid(), SIGKILL);
              //exit(is_sent); //if exit myself and become a zombie :D

            }
          }
            return;
          case 1:
            throw std::runtime_error(
                    "startBatsched(): Scheduling algorithm " +
                    this->getPropertyValueAsString(BatchServiceProperty::BATCH_SCHEDULING_ALGORITHM) +
                    " not supported by the batch service");
          case 2:
            throw std::runtime_error(
                    "startBatsched(): Queuing option " +
                    this->getPropertyValueAsString(BatchServiceProperty::BATCH_QUEUE_ORDERING_ALGORITHM) +
                    "not supported by the batch service");
          case 3:
            throw std::runtime_error(
                    "startBatsched(): Cannot start the batsched process");
          default:
            throw std::runtime_error(
                    "startBatsched(): Unknown fatal error");
        }

      } else {
        // fork failed
        throw std::runtime_error(
                "Error while fork-exec of batsched"
        );
      }
    }


    /**
     * @brief: Stop the batsched process
     */
    void BatchService::stopBatsched() {
       // Stop Batsched
      zmq::context_t context(1);
      zmq::socket_t socket(context, ZMQ_REQ);
      socket.connect("tcp://localhost:" + std::to_string(this->batsched_port));


      nlohmann::json simulation_ends_msg;
      simulation_ends_msg["now"] = S4U_Simulation::getClock();
      simulation_ends_msg["events"][0]["timestamp"] = S4U_Simulation::getClock();
      simulation_ends_msg["events"][0]["type"] = "SIMULATION_ENDS";
      simulation_ends_msg["events"][0]["data"] = {};
      std::string data_to_send = simulation_ends_msg.dump();

      zmq::message_t request(strlen(data_to_send.c_str()));
      memcpy(request.data(), data_to_send.c_str(), strlen(data_to_send.c_str()));
      socket.send(request);

      //  Get the reply.
      zmq::message_t reply;
      socket.recv(&reply);

      // TODO: Is this below useful?
      std::string reply_data;
      reply_data = std::string(static_cast<char *>(reply.data()), reply.size());

    }


    std::map<std::string, double>
    BatchService::getQueueWaitingTimeEstimateFromBatsched(std::set<std::tuple<std::string, unsigned int, double>> set_of_jobs) {
          nlohmann::json batch_submission_data;
      batch_submission_data["now"] = S4U_Simulation::getClock();

      int idx = 0;
      batch_submission_data["events"] = nlohmann::json::array();
      for (auto job : set_of_jobs) {
        batch_submission_data["events"][idx]["timestamp"] = S4U_Simulation::getClock();
        batch_submission_data["events"][idx]["type"] = "QUERY";
        batch_submission_data["events"][idx]["data"]["requests"]["estimate_waiting_time"]["job_id"] = std::get<0>(job);
        batch_submission_data["events"][idx]["data"]["requests"]["estimate_waiting_time"]["job"]["id"] = std::get<0>(
                job);
        batch_submission_data["events"][idx]["data"]["requests"]["estimate_waiting_time"]["job"]["res"] = std::get<1>(
                job);
        batch_submission_data["events"][idx++]["data"]["requests"]["estimate_waiting_time"]["job"]["walltime"] = std::get<2>(
                job);
      }


      std::string batchsched_query_mailbox = S4U_Mailbox::generateUniqueMailboxName("batchsched_query_mailbox");
      std::string data = batch_submission_data.dump();
      std::shared_ptr<BatchNetworkListener> network_listener =
              std::unique_ptr<BatchNetworkListener>(new BatchNetworkListener(this->hostname, batchsched_query_mailbox,
                                                                             std::to_string(this->batsched_port),
                                                                             BatchNetworkListener::NETWORK_LISTENER_TYPE::SENDER_RECEIVER,
                                                                             data));
      network_listener->setSimulation(this->simulation);
      network_listener->start(network_listener, true);
      network_listeners.push_back(std::move(network_listener));
      this->is_bat_sched_ready = false;


      std::map<std::string, double> jobs_estimated_waiting_time = {};
      for (auto job : set_of_jobs) {
        // Get the answer
        std::unique_ptr<SimulationMessage> message = nullptr;
        try {
          message = S4U_Mailbox::getMessage(batchsched_query_mailbox);
        } catch (std::shared_ptr<NetworkError> &cause) {
          throw WorkflowExecutionException(cause);
        }

        if (auto *msg = dynamic_cast<BatchQueryAnswerMessage *>(message.get())) {
          jobs_estimated_waiting_time[std::get<0>(job)] = msg->estimated_waiting_time;
        } else {
          throw std::runtime_error(
                  "BatchService::getQueueWaitingTimeEstimate(): Received an unexpected [" + message->getName() +
                  "] message!");
        }
      }
      return jobs_estimated_waiting_time;
    }

      /**
     * @brief Notify a job even to BATSCHED (BATSCHED ONLY)
     * @param job_id
     * @param status
     * @param job_state
     * @param kill_reason
     */
    void BatchService::notifyJobEventsToBatSched(std::string job_id, std::string status, std::string job_state,
                                                 std::string kill_reason) {
      nlohmann::json batch_submission_data;
      batch_submission_data["now"] = S4U_Simulation::getClock();
      batch_submission_data["events"][0]["timestamp"] = S4U_Simulation::getClock();
      batch_submission_data["events"][0]["type"] = "JOB_COMPLETED";
      batch_submission_data["events"][0]["data"]["job_id"] = job_id;
      batch_submission_data["events"][0]["data"]["status"] = status;
      batch_submission_data["events"][0]["data"]["job_state"] = job_state;
      batch_submission_data["events"][0]["data"]["kill_reason"] = kill_reason;

      std::string data = batch_submission_data.dump();
      std::shared_ptr<BatchNetworkListener> network_listener =
              std::shared_ptr<BatchNetworkListener>(new BatchNetworkListener(this->hostname, this->mailbox_name,
                                                                             std::to_string(this->batsched_port),
                                                                             BatchNetworkListener::NETWORK_LISTENER_TYPE::SENDER_RECEIVER,
                                                                             data));
      network_listener->setSimulation(this->simulation);
      network_listener->start(network_listener, true);
      network_listeners.push_back(network_listener);
    }

    /**
     * @brief Start a network listener process (for BATSCHED only)
     */
    void BatchService::startBatschedNetworkListener() {

      nlohmann::json compute_resources_map;
      compute_resources_map["now"] = S4U_Simulation::getClock();
      compute_resources_map["events"][0]["timestamp"] = S4U_Simulation::getClock();
      compute_resources_map["events"][0]["type"] = "SIMULATION_BEGINS";
      compute_resources_map["events"][0]["data"]["nb_resources"] = this->nodes_to_cores_map.size();
      compute_resources_map["events"][0]["data"]["allow_time_sharing"] = false;
      compute_resources_map["events"][0]["data"]["config"]["redis"]["enabled"] = false;
      std::map<std::string, unsigned long>::iterator it;
      int count = 0;
      for (it = this->nodes_to_cores_map.begin(); it != this->nodes_to_cores_map.end(); it++) {
        compute_resources_map["events"][0]["data"]["resources_data"][count]["id"] = std::to_string(count);
        compute_resources_map["events"][0]["data"]["resources_data"][count]["name"] = it->first;
        compute_resources_map["events"][0]["data"]["resources_data"][count]["core"] = it->second;
        compute_resources_map["events"][0]["data"]["resources_data"][count++]["state"] = "idle";
      }
      std::string data = compute_resources_map.dump();


      try {
        std::shared_ptr<BatchNetworkListener> network_listener =
                std::shared_ptr<BatchNetworkListener>(new BatchNetworkListener(this->hostname, this->mailbox_name,
                                                                               std::to_string(this->batsched_port),
                                                                               BatchNetworkListener::NETWORK_LISTENER_TYPE::SENDER_RECEIVER,
                                                                               data));
        network_listener->setSimulation(this->simulation);
        network_listener->start(network_listener, true);
        this->network_listeners.push_back(std::move(network_listener));
      } catch (std::runtime_error &e) {
        throw;
      }
    }

   void BatchService::sendAllQueuedJobsToBatsched() {
      if (this->pending_jobs.empty()) {
        return;
      }

      // Send ALL Queued jobs to batsched, and move them all to the WAITING queue
      // The WAITING queue is: those jobs that I need to hear from Batsched about

      nlohmann::json batch_submission_data;
      batch_submission_data["now"] = S4U_Simulation::getClock();
      batch_submission_data["events"] = nlohmann::json::array();
      size_t i;
      std::deque<BatchJob *>::iterator it;
      for (i = 0, it = this->pending_jobs.begin(); i < this->pending_jobs.size(); i++, it++) {

        BatchJob *batch_job = *it;

        /* Get the nodes and cores per nodes asked for */
        unsigned long cores_per_node_asked_for = batch_job->getAllocatedCoresPerNode();
        unsigned long num_nodes_asked_for = batch_job->getNumNodes();
        unsigned long time_in_minutes = batch_job->getAllocatedTime();

        batch_submission_data["events"][i]["timestamp"] = batch_job->getAppearedTimeStamp();
        batch_submission_data["events"][i]["type"] = "JOB_SUBMITTED";
        batch_submission_data["events"][i]["data"]["job_id"] = std::to_string(batch_job->getJobID());
        batch_submission_data["events"][i]["data"]["job"]["id"] = std::to_string(batch_job->getJobID());
        batch_submission_data["events"][i]["data"]["job"]["res"] = num_nodes_asked_for;
        batch_submission_data["events"][i]["data"]["job"]["core"] = cores_per_node_asked_for;
        batch_submission_data["events"][i]["data"]["job"]["walltime"] = time_in_minutes * 60;
        this->pending_jobs.erase(it);
        this->waiting_jobs.insert(*it);
//        PointerUtil::moveUniquePtrFromDequeToSet(it, &(this->pending_jobs),
//                                                 &(this->waiting_jobs));

      }
      std::string data = batch_submission_data.dump();
      std::shared_ptr<BatchNetworkListener> network_listener =
              std::unique_ptr<BatchNetworkListener>(new BatchNetworkListener(this->hostname, this->mailbox_name,
                                                                             std::to_string(this->batsched_port),
                                                                             BatchNetworkListener::NETWORK_LISTENER_TYPE::SENDER_RECEIVER,
                                                                             data));
      network_listener->setSimulation(this->simulation);
      network_listener->start(network_listener, true);
      network_listeners.push_back(std::move(network_listener));
      this->is_bat_sched_ready = false;

    }




    void BatchService::processExecuteJobFromBatSched(std::string bat_sched_reply) {
      nlohmann::json execute_events = nlohmann::json::parse(bat_sched_reply);
      WorkflowJob *workflow_job = nullptr;
      BatchJob *batch_job = nullptr;
      for (auto it1 = this->waiting_jobs.begin(); it1 != this->waiting_jobs.end(); it1++) {
        if (std::to_string((*it1)->getJobID()) == execute_events["job_id"]) {
          batch_job = (*it1);
          workflow_job = batch_job->getWorkflowJob();
          this->waiting_jobs.erase(batch_job);
          this->running_jobs.insert(batch_job);
          break;
        }
      }
      if (workflow_job == nullptr) {
        throw std::runtime_error(
                "BatchService::processExecuteJobFromBatSched(): Job received from batsched that does not belong to the list of jobs batchservice has"
        );
      }

      /* Get the nodes and cores per nodes asked for */

      std::string nodes_allocated_by_batsched = execute_events["alloc"];
      std::vector<std::string> allocations;
      boost::split(allocations, nodes_allocated_by_batsched, boost::is_any_of(" "));
      std::vector<unsigned long> node_resources;
      for (auto alloc:allocations) {
        std::vector<std::string> each_allocations;
        boost::split(each_allocations, alloc, boost::is_any_of("-"));
        if (each_allocations.size() < 2) {
          std::string start_node = each_allocations[0];
          std::string::size_type sz;
          int start = std::stoi(start_node, &sz);
          node_resources.push_back(start);
        } else {
          std::string start_node = each_allocations[0];
          std::string end_node = each_allocations[1];
          std::string::size_type sz;
          unsigned long start = std::stoi(start_node, &sz);
          unsigned long end = std::stoi(end_node, &sz);
          for (unsigned long i = start; i <= end; i++) {
            node_resources.push_back(i);
          }
        }
      }

      unsigned long num_nodes_allocated = node_resources.size();
      unsigned long time_in_minutes = batch_job->getAllocatedTime();
      unsigned long cores_per_node_asked_for = batch_job->getAllocatedCoresPerNode();

      std::set<std::tuple<std::string, unsigned long, double>> resources = {};
      std::vector<std::string> hosts_assigned = {};
      std::map<std::string, unsigned long>::iterator it;

      for (auto node:node_resources) {
        this->available_nodes_to_cores[this->host_id_to_names[node]] -= cores_per_node_asked_for;
        resources.insert(std::make_tuple(this->host_id_to_names[node], cores_per_node_asked_for,
                                         0)); // TODO: Is setting RAM to 0 ok here?
      }

      startJob(resources, workflow_job, batch_job, num_nodes_allocated, time_in_minutes,
               cores_per_node_asked_for);



//        switch (workflow_job->getType()) {
//            case WorkflowJob::STANDARD: {
//                auto job = (StandardJob*) workflow_job;
//                WRENCH_INFO("Creating a StandardJobExecutor on %ld cores for a standard job on %ld nodes",
//                            cores_per_node_asked_for, num_nodes_allocated);
//                // Create a standard job executor
//                std::cout<<"Executor arguments "<<this->mailbox_name<<" "<<std::get<0>(*resources.begin())<<"\n";
//                StandardJobExecutor *executor = new StandardJobExecutor(
//                        this->simulation,
//                        this->mailbox_name,
//                        std::get<0>(*resources.begin()),
//                        (StandardJob *) workflow_job,
//                        resources,
//                        this->default_storage_service,
//                        {{StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD,
//                                 this->getPropertyValueAsString(
//                                         BatchServiceProperty::THREAD_STARTUP_OVERHEAD)}});
//                this->running_standard_job_executors.insert(std::unique_ptr<StandardJobExecutor>(executor));
//                batch_job->setEndingTimeStamp(S4U_Simulation::getClock() + time_in_minutes * 60);
////          this->running_jobs.insert(std::move(batch_job_ptr));
//                this->timeslots.push_back(batch_job->getEndingTimeStamp());
//                //remember the allocated resources for the job
//                batch_job->setAllocatedResources(resources);
//
//                SimulationMessage* msg =
//                        new AlarmJobTimeOutMessage(job, 0);
//
//                std::unique_ptr<Alarm> alarm_ptr = std::unique_ptr<Alarm>(new Alarm(batch_job->getEndingTimeStamp(), this->hostname, this->mailbox_name, msg,
//                                                                                    "batch_standard"));
//
//                standard_job_alarms.push_back(
//                        std::move(alarm_ptr));
//
//
//                return;
//            }
//                break;
//
//            case WorkflowJob::PILOT: {
//                PilotJob *job = (PilotJob *) workflow_job;
//                WRENCH_INFO("Allocating %ld nodes with %ld cores per node to a pilot job",
//                            num_nodes_allocated, cores_per_node_asked_for);
//
//                std::string host_to_run_on = resources.begin()->first;
//                std::vector<std::string> nodes_for_pilot_job = {};
//                for (auto it = resources.begin(); it != resources.end(); it++) {
//                    nodes_for_pilot_job.push_back(it->first);
//                }
//
//                //set the ending timestamp of the batchjob (pilotjob)
//
//                double timeout_timestamp = std::min(job->getDuration(), time_in_minutes * 60 * 1.0);
//                batch_job->setEndingTimeStamp(S4U_Simulation::getClock() + timeout_timestamp);
//
//                ComputeService *cs =
//                        new MultihostMulticoreComputeService(host_to_run_on,
//                                                             true, false,
//                                                             resources,
//                                                             this->default_storage_service
//                        );
//                cs->setSimulation(this->simulation);
//
//                // Create and launch a compute service for the pilot job
//                job->setComputeService(cs);
//
//
//                // Put the job in the running queue
////          this->running_jobs.insert(std::move(batch_job_ptr));
//                this->timeslots.push_back(batch_job->getEndingTimeStamp());
//
//                //remember the allocated resources for the job
//                batch_job->setAllocatedResources(resources);
//
//
//                // Send the "Pilot job has started" callback
//                // Note the getCallbackMailbox instead of the popCallbackMailbox, because
//                // there will be another callback upon termination.
//                try {
//                    S4U_Mailbox::dputMessage(job->getCallbackMailbox(),
//                                             new ComputeServicePilotJobStartedMessage(job, this,
//                                                                                      this->getPropertyValueAsDouble(
//                                                                                              BatchServiceProperty::PILOT_JOB_STARTED_MESSAGE_PAYLOAD)));
//                } catch (std::shared_ptr<NetworkError> &cause) {
//                    throw WorkflowExecutionException(cause);
//                }
//
//                SimulationMessage* msg =
//                        new AlarmJobTimeOutMessage(job, 0);
//                std::unique_ptr<Alarm> alarm_ptr = std::unique_ptr<Alarm>(new Alarm(batch_job->getEndingTimeStamp(), host_to_run_on, this->mailbox_name, msg,
//                                                                                    "batch_pilot"));
//
//                this->pilot_job_alarms.push_back(
//                        std::move(alarm_ptr));
//
//                // Push my own mailbox_name onto the pilot job!
////          job->pushCallbackMailbox(this->mailbox_name);
//
//                return;
//            }
//                break;
//        }

    }


};


#endif  // ENABLE_BATSCHED

};

