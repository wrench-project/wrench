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

#include <wrench/services/Service.h>
#include "BatchServiceProperty.h"

namespace wrench {


    class BatchService;

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief A helper service that handles all interaction with Batsched
     */
    class BatschedNetworkListener: public Service {

    public:

        BatschedNetworkListener(std::string hostname, BatchService *batch_service, std::string batch_service_mailbox, std::string sched_port,
                             std::string data_to_send,std::map<std::string, std::string> property_list = {});


    private:
        std::map<std::string, std::string> default_property_values = {
                };

        std::map<std::string, std::string> default_messagepayload_values = {
                };

        BatschedNetworkListener(std::string, BatchService *batch_service, std::string batch_service_mailbox, std::string sched_port,
                             std::string data_to_send, std::map<std::string, std::string> property_list, std::string suffix);


        int main() override;
        std::string self_port;
        std::string sched_port;
        std::string data_to_send;
        std::string reply_received;
        BatchService *batch_service;
        std::string batch_service_mailbox;

        void send_receive();

        void sendExecuteMessageToBatchService(std::string answer_mailbox, std::string execute_job_reply_data);
        void sendQueryAnswerMessageToBatchService(double estimated_waiting_time);

    };

    /***********************/
    /** \endcond           */
    /***********************/
}


#endif //WRENCH_BATSCHEDNETWORKLISTENER_H
