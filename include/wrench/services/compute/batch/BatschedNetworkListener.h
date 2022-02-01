/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_BATSCHEDNETWORKLISTENER_H
#define WRENCH_BATSCHEDNETWORKLISTENER_H

#include "wrench/services/Service.h"
#include "BatchComputeServiceProperty.h"

namespace wrench {


    class BatchComputeService;

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

#ifdef ENABLE_BATSCHED
    /**
     * @brief A helper service that handles all interaction with Batsched
     */
    class BatschedNetworkListener: public Service {

    public:

        BatschedNetworkListener(std::string hostname, std::shared_ptr<BatchComputeService> batch_service,
                                std::string batch_service_mailbox, std::string sched_port,
                                std::string data_to_send,WRENCH_PROPERTY_COLLECTION_TYPE property_list = {});
    private:
        WRENCH_PROPERTY_COLLECTION_TYPE default_property_values = {
                };

WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE  default_messagepayload_values = {
                };

        BatschedNetworkListener(std::string, std::shared_ptr<BatchComputeService> batch_service, std::string batch_service_mailbox, std::string sched_port,
                             std::string data_to_send, WRENCH_PROPERTY_COLLECTION_TYPE property_list, std::string suffix);


        int main() override;
        std::string self_port;
        std::string sched_port;
        std::string data_to_send;
        std::string reply_received;
        std::shared_ptr<BatchComputeService> batch_service;
        std::string batch_service_mailbox;



        void sendExecuteMessageToBatchComputeService(std::string answer_mailbox, std::string execute_job_reply_data);
        void sendQueryAnswerMessageToBatchComputeService(double estimated_waiting_time);
        void send_receive();
    };

#else // ENABLE_BATSCHED
    /**
     * @brief An empty class to make Doxygen happy when Batsched is not enabled
     */
    class BatschedNetworkListener {

    };
#endif

    /***********************/
    /** \endcond           */
    /***********************/
}


#endif //WRENCH_BATSCHEDNETWORKLISTENER_H
