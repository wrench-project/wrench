/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include "MulticoreComputeServiceMessage.h"

namespace wrench {

    MulticoreComputeServiceMessage::MulticoreComputeServiceMessage(std::string name, double payload) :
            ComputeServiceMessage("ComputeServiceMessage::" + name, payload) {
    }



    /**
     * @brief Constructor
     * @param job: pointer to a WorkflowJob
     * @param cs: points to a ComputeService
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    MulticoreComputeServiceNotEnoughCoresMessage::MulticoreComputeServiceNotEnoughCoresMessage(WorkflowJob *job, ComputeService *cs, double payload)
            : MulticoreComputeServiceMessage("NOT_ENOUGH_CORES", payload) {
      if (job == nullptr) {
        throw std::invalid_argument("Invalid constructor arguments");
      }
      this->job = job;
      this->compute_service = cs;
    }



    /**
    * @brief Constructor
    * @param answer_mailbox: the mailbox to which to send the answer
    * @param payload: message size in bytes
    *
    * @throw std::invalid_argument
    */
    MulticoreComputeServiceNumCoresRequestMessage::MulticoreComputeServiceNumCoresRequestMessage(std::string answer_mailbox, double payload) : MulticoreComputeServiceMessage(
            "NUM_CORES_REQUEST",
            payload) {
      if ((answer_mailbox == "")) {
        throw std::invalid_argument("Invalid constructor arguments");
      }
      this->answer_mailbox = answer_mailbox;
    }

    /**
     * @brief Constructor
     * @param num: number of cores
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    MulticoreComputeServiceNumCoresAnswerMessage::MulticoreComputeServiceNumCoresAnswerMessage(unsigned int num, double payload) : MulticoreComputeServiceMessage(
            "NUM_CORES_ANSWER", payload) {

      this->num_cores = num;
    }

    /**
     * @brief Constructor
     * @param answer_mailbox: the mailbox to which to send the answer
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    MulticoreComputeServiceNumIdleCoresRequestMessage::MulticoreComputeServiceNumIdleCoresRequestMessage(std::string answer_mailbox, double payload)
            : MulticoreComputeServiceMessage("NUM_IDLE_CORES_REQUEST",
                                             payload) {
      if ((answer_mailbox == "")) {
        throw std::invalid_argument("Invalid constructor arguments");
      }
      this->answer_mailbox = answer_mailbox;
    }

    /**
     * @brief Constructor
     * @param num: number of idle cores
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    MulticoreComputeServiceNumIdleCoresAnswerMessage::MulticoreComputeServiceNumIdleCoresAnswerMessage(unsigned int num, double payload) : MulticoreComputeServiceMessage(
            "NUM_IDLE_CORES_ANSWER", payload) {

      this->num_idle_cores = num;
    }

    /**
     * @brief Constructor
     * @param answer_mailbox: the mailbox to which to send the answer
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    MulticoreComputeServiceTTLRequestMessage::MulticoreComputeServiceTTLRequestMessage(std::string answer_mailbox, double payload) : MulticoreComputeServiceMessage("TTL_REQUEST",
                                                                                                                                                                    payload) {
      if ((answer_mailbox == "")) {
        throw std::invalid_argument("Invalid constructor arguments");
      }
      this->answer_mailbox = answer_mailbox;
    }

    /**
     * @brief Constructor
     * @param ttl: time-to-live, in seconds (-1 means infinity)
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    MulticoreComputeServiceTTLAnswerMessage::MulticoreComputeServiceTTLAnswerMessage(double ttl, double payload) : MulticoreComputeServiceMessage("TTL_ANSWER", payload) {

      this->ttl = ttl;
    }

    /**
    * @brief Constructor
    * @param answer_mailbox: the mailbox to which to send the answer
    * @param payload: message size in bytes
    *
    * @throw std::invalid_argument
    */
    MulticoreComputeServiceFlopRateRequestMessage::MulticoreComputeServiceFlopRateRequestMessage(std::string answer_mailbox, double payload) : MulticoreComputeServiceMessage(
            "FLOP_RATE_REQUEST", payload) {
      if ((answer_mailbox == "")) {
        throw std::invalid_argument("Invalid constructor arguments");
      }
      this->answer_mailbox = answer_mailbox;
    }

    /**
     * @brief Constructor
     * @param flop_rate: the flop rate, in flop/sec
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    MulticoreComputeServiceFlopRateAnswerMessage::MulticoreComputeServiceFlopRateAnswerMessage(double flop_rate, double payload) : MulticoreComputeServiceMessage("FLOP_RATE_ANSWER",
                                                                                                                                                                  payload) {
      if ((flop_rate < 0.0)) {
        throw std::invalid_argument("Invalid constructor arguments");
      }
      this->flop_rate = flop_rate;
    }


    /**
     * @brief Constructor
     * @param pre_file_copies: the file copy operations to perform first
     * @param tasks: the tasks to execute in sequence
     * @param file_locations: the locations where tasks should read/write files
     * @param post_file_copies: the file copy operations to perform last
     * @param payload: message size in bytes
     */
    WorkerThreadDoWorkRequestMessage::WorkerThreadDoWorkRequestMessage(
            std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> pre_file_copies,
            std::vector<WorkflowTask *> tasks,
            std::map<WorkflowFile *, StorageService *> file_locations,
            std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> post_file_copies, double payload) :
            MulticoreComputeServiceMessage("WORKER_THREAD_DO_WORK_REQUEST",
                                           payload) {
      this->pre_file_copies = pre_file_copies;
      this->tasks = tasks;
      this->file_locations = file_locations;
      this->post_file_copies = post_file_copies;
    }


    /**
     * @brief Constructor
     * @param worker_thread: the worker thread on which the job was performed
     * @param pre_file_copies: the file copy operations to perform first
     * @param tasks: the tasks to execute in sequence
     * @param file_locations: the locations where tasks should read/write files
     * @param post_file_copies: the file copy operations to perform last
     * @param payload: message size in bytes
     */
    WorkerThreadWorkDoneMessage::WorkerThreadWorkDoneMessage(
            WorkerThread *worker_thread,
            std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> pre_file_copies,
            std::vector<WorkflowTask *> tasks, std::map<WorkflowFile *, StorageService *> file_locations,
            std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> post_file_copies, double payload) :
    MulticoreComputeServiceMessage("WORKER_THREAD_WORK_DONE", payload) {
      this->worker_thread = worker_thread;
      this->pre_file_copies = pre_file_copies;
      this->tasks = tasks;
      this->file_locations = file_locations;
      this->post_file_copies = post_file_copies;

    }

    /**
     * @brief Constructor
     * @param worker_thread: the worker thread on which the job was performed
     * @param pre_file_copies: the file copy operations to perform first
     * @param tasks: the tasks to execute in sequence
     * @param file_locations: the locations where tasks should read/write files
     * @param post_file_copies: the file copy operations to perform last
     * @param cause: the cause of the failure
     * @param payload: message size in bytes
     */
    WorkerThreadWorkFailedMessage::WorkerThreadWorkFailedMessage(
            WorkerThread *worker_thread,
            std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> pre_file_copies,
            std::vector<WorkflowTask *> tasks, std::map<WorkflowFile *, StorageService *> file_locations,
            std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> post_file_copies,
            WorkflowExecutionFailureCause *cause, double payload):
    MulticoreComputeServiceMessage("WORKER_THREAD_WORK_FAILED", payload) {
      this->worker_thread = worker_thread;
      this->pre_file_copies = pre_file_copies;
      this->tasks = tasks;
      this->file_locations = file_locations;
      this->post_file_copies = post_file_copies;
      this->cause = cause;

    }
};