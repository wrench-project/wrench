//
// Created by suraj on 9/16/17.
//

#include "BatchServiceMessage.h"

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param name: the message name
     * @param payload: the message size in bytes
     */
    BatchServiceMessage::BatchServiceMessage(std::string name, double payload) :
            ComputeServiceMessage("BatchServiceMessage::" + name, payload) {
    }

    /**
     * @brief Constructor
     * @param answer: the workflow job
     * @param cs: the compute service
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    BatchServiceJobRequestMessage::BatchServiceJobRequestMessage(std::string answer_mailbox,
                                                                 BatchJob* job,double payload)
            : BatchServiceMessage("SUBMIT_BATCH_JOB_REQUEST", payload) {
        if (job == nullptr) {
            throw std::invalid_argument("BatchServiceJobRequestMessage::BatchServiceJobRequestMessage(): Invalid arguments");
        }
        this->job = job;
        this->answer_mailbox = answer_mailbox;
    }
}