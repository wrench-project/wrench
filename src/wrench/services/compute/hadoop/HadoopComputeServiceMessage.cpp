/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "./HadoopComputeServiceMessage.h"

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
 * @param job: the MR job
 * @param payload: the message size in bytes
 *
 * @throw std::invalid_argument
 */
    HadoopComputeServiceRunMRJobRequestMessage::HadoopComputeServiceRunMRJobRequestMessage(std::string answer_mailbox,
                                                                                           MRJob *job,
                                                                                           double payload)
            : HadoopComputeServiceMessage("HadoopComputeServiceRunMRJobRequestMessage", payload) {
        this->job = job;
        if (answer_mailbox.empty()) {
            throw std::invalid_argument(
                    "HadoopComputeServiceRunMRJobRequestMessage::HadoopComputeServiceRunMRJobRequestMessage(): Empty answer mailbox");
        }
        this->answer_mailbox = answer_mailbox;
    }


/**
 * @brief Constructor
 * @param success: the job success status
 * @param payload: the message size in bytes
 *
 * @throw std::invalid_argument
 */
    HadoopComputeServiceRunMRJobAnswerMessage::HadoopComputeServiceRunMRJobAnswerMessage(bool success,
                                                                                         double payload)
            : HadoopComputeServiceMessage("HadoopComputeServiceRunMRJobAnswerMessage", payload) {

        this->success = success;
    }

    /**
     *
     * @param success : the job success status
     * @param job : the MR job
     * @param payload : the message size in bytes
     *
     * @throw: std::invalid_argument
     */
    MRJobExecutorNotificationMessage::MRJobExecutorNotificationMessage(bool success,  MRJob *job, double payload)
            : HadoopComputeServiceMessage("MRJobExecutorNotificationMessage", payload) {
        this->success = success;
        this->job = job;
    }

    /**
     *
     * @param job
     * @param data_size
     * @param payload
     */
    HdfsReadDataMessage::HdfsReadDataMessage(double data_size, double payload)
            : HadoopComputeServiceMessage("HdfsReadDataMessage", payload) {
        this->data_size = data_size;
        this->payload = payload;
    };

    /**
     *
     * @param job
     * @param data_size
     * @param payload
     */
    RequestDataFromHdfs::RequestDataFromHdfs(double data_size, double payload)
            : HadoopComputeServiceMessage("RequestDataFromHdfs", payload) {
        this->data_size = data_size;
        this->payload = payload;
    };

    /**
     * 
     * @param data_size
     * @param payload
     */
    HdfsReadComplete::HdfsReadComplete(double data_size, double payload)
            : HadoopComputeServiceMessage("HdfsReadComplete", payload) {
        this->data_size = data_size;
        this->payload = payload;
    }
};
