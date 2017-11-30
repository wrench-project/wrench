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

    /**
     * @brief Constructor
     *
     * @param name: the message name
     * @param payload: the message size in bytes
     */
    MulticoreComputeServiceMessage::MulticoreComputeServiceMessage(std::string name, double payload) :
            ComputeServiceMessage("MulticoreComputeServiceMessage::" + name, payload) {
    }

    /**
     * @brief Constructor
     * @param job: the workflow job
     * @param cs: the compute service
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    MulticoreComputeServiceNotEnoughCoresMessage::MulticoreComputeServiceNotEnoughCoresMessage(WorkflowJob *job,
                                                                                               ComputeService *cs,
                                                                                               double payload)
            : MulticoreComputeServiceMessage("NOT_ENOUGH_CORES", payload), compute_service(cs) {
      if ((job == nullptr) || (cs == nullptr)) {
        throw std::invalid_argument("MulticoreComputeServiceNotEnoughCoresMessage::MulticoreComputeServiceNotEnoughCoresMessage(): Invalid arguments");
      }
      this->job = job;
    }

    /**
     * @brief Constructor
     * @param answer_mailbox: the mailbox to which to send the answer
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    MulticoreComputeServiceTTLRequestMessage::MulticoreComputeServiceTTLRequestMessage(std::string answer_mailbox,
                                                                                       double payload)
            : MulticoreComputeServiceMessage("TTL_REQUEST", payload) {
      if (answer_mailbox == "") {
        throw std::invalid_argument("MulticoreComputeServiceTTLRequestMessage::MulticoreComputeServiceTTLRequestMessage(): Invalid arguments");
      }
      this->answer_mailbox = answer_mailbox;
    }

    /**
     * @brief Constructor
     * @param ttl: time-to-live, in seconds (-1 means infinity)
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    MulticoreComputeServiceTTLAnswerMessage::MulticoreComputeServiceTTLAnswerMessage(double ttl, double payload)
            :  MulticoreComputeServiceMessage("TTL_ANSWER", payload), ttl(ttl) {}

    /**
    * @brief Constructor
    * @param answer_mailbox: the mailbox to which to send the answer
    * @param payload: the message size in bytes
    *
    * @throw std::invalid_argument
    */
    MulticoreComputeServiceFlopRateRequestMessage::MulticoreComputeServiceFlopRateRequestMessage(
            std::string answer_mailbox, double payload) : MulticoreComputeServiceMessage(
            "FLOP_RATE_REQUEST", payload) {
      if (answer_mailbox == "") {
        throw std::invalid_argument("MulticoreComputeServiceFlopRateRequestMessage::MulticoreComputeServiceFlopRateRequestMessage(): Invalid arguments");
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
    MulticoreComputeServiceFlopRateAnswerMessage::MulticoreComputeServiceFlopRateAnswerMessage(double flop_rate,
                                                                                               double payload)
            : MulticoreComputeServiceMessage("FLOP_RATE_ANSWER", payload) {
      if ((flop_rate < 0.0)) {
        throw std::invalid_argument("MulticoreComputeServiceFlopRateAnswerMessage::MulticoreComputeServiceFlopRateAnswerMessage(): Invalid arguments");
      }
      this->flop_rate = flop_rate;
    }



};
