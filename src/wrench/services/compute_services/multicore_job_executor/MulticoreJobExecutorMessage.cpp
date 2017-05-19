/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include "MulticoreJobExecutorMessage.h"

namespace wrench {

    MulticoreJobExecutorMessage::MulticoreJobExecutorMessage(std::string name, double payload) :
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
    MulticoreJobExecutorNotEnoughCoresMessage::MulticoreJobExecutorNotEnoughCoresMessage(WorkflowJob *job, ComputeService *cs, double payload)
            : MulticoreJobExecutorMessage("NOT_ENOUGH_CORES", payload) {
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
    MulticoreJobExecutorNumCoresRequestMessage::MulticoreJobExecutorNumCoresRequestMessage(std::string answer_mailbox, double payload) : MulticoreJobExecutorMessage(
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
    MulticoreJobExecutorNumCoresAnswerMessage::MulticoreJobExecutorNumCoresAnswerMessage(unsigned int num, double payload) : MulticoreJobExecutorMessage(
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
    MulticoreJobExecutorNumIdleCoresRequestMessage::MulticoreJobExecutorNumIdleCoresRequestMessage(std::string answer_mailbox, double payload)
            : MulticoreJobExecutorMessage("NUM_IDLE_CORES_REQUEST",
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
    MulticoreJobExecutorNumIdleCoresAnswerMessage::MulticoreJobExecutorNumIdleCoresAnswerMessage(unsigned int num, double payload) : MulticoreJobExecutorMessage(
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
    MulticoreJobExecutorTTLRequestMessage::MulticoreJobExecutorTTLRequestMessage(std::string answer_mailbox, double payload) : MulticoreJobExecutorMessage("TTL_REQUEST",
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
    MulticoreJobExecutorTTLAnswerMessage::MulticoreJobExecutorTTLAnswerMessage(double ttl, double payload) : MulticoreJobExecutorMessage("TTL_ANSWER", payload) {

      this->ttl = ttl;
    }

    /**
    * @brief Constructor
    * @param answer_mailbox: the mailbox to which to send the answer
    * @param payload: message size in bytes
    *
    * @throw std::invalid_argument
    */
    MulticoreJobExecutorFlopRateRequestMessage::MulticoreJobExecutorFlopRateRequestMessage(std::string answer_mailbox, double payload) : MulticoreJobExecutorMessage(
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
    MulticoreJobExecutorFlopRateAnswerMessage::MulticoreJobExecutorFlopRateAnswerMessage(double flop_rate, double payload) : MulticoreJobExecutorMessage("FLOP_RATE_ANSWER",
                                                                                                       payload) {
      if ((flop_rate < 0.0)) {
        throw std::invalid_argument("Invalid constructor arguments");
      }
      this->flop_rate = flop_rate;
    }
};