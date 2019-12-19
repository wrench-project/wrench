/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "HadoopComputeServiceMessage.h"

namespace wrench {


    /**
     * @brief Constructor
     *
     * @param name: the message name
     * @param payload: the message size in bytes
     */
    HadoopComputeServiceMessage::HadoopComputeServiceMessage(std::string name, double payload) :
            ComputeServiceMessage("HadoopComputeServiceMessage::" + name, payload) {
    }

/**
 * @brief Constructor
 * @param answer_mailbox: the mailbox to reply to
 * @param batsched_decision_reply: the decision reply from Batsched
 * @param payload: the message size in bytes
 *
 * @throw std::invalid_argument
 */
    HadoopComputeServiceRunMRJobRequestMessage::HadoopComputeServiceRunMRJobRequestMessage(std::string answer_mailbox,
                                                                                           double payload)
            : HadoopComputeServiceMessage("HadoopComputeServiceRunMRJobRequestMessage", payload) {
        if (answer_mailbox.empty()) {
            throw std::invalid_argument(
                    "HadoopComputeServiceRunMRJobRequestMessage::HadoopComputeServiceRunMRJobRequestMessage(): Empty answer mailbox");
        }
        this->answer_mailbox = answer_mailbox;
    }


/**
 * @brief Constructor
 * @param payload: the message size in bytes
 *
 * @throw std::invalid_argument
 */
    HadoopComputeServiceRunMRJobAnswerMessage::HadoopComputeServiceRunMRJobAnswerMessage(bool success,
                                                                                         double payload)
            : HadoopComputeServiceMessage("HadoopComputeServiceRunMRJobAnswerMessage", payload) {

        this->success = success;
    }


};