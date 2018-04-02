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


    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief A helper service that handles all interaction with Batsched
     */
    class BatschedNetworkListener: public Service {

    public:

        /** @brief Enumerated type to specified the type of the BatschedNetworkListener */
        enum NETWORK_LISTENER_TYPE{
            LISTENER,
            SENDER,
            SENDER_RECEIVER
        };

        BatschedNetworkListener(std::string hostname, std::string batch_service_mailbox, std::string sched_port,
                             NETWORK_LISTENER_TYPE MY_TYPE, std::string data_to_send,std::map<std::string, std::string> plist = {});


    private:
        std::map<std::string, std::string> default_property_values =
                {{BatchServiceProperty::STOP_DAEMON_MESSAGE_PAYLOAD,          "1024"},
                 {BatchServiceProperty::DAEMON_STOPPED_MESSAGE_PAYLOAD,       "1024"},
                 {BatchServiceProperty::BATCH_SCHED_READY_PAYLOAD,           "0"},
                 {BatchServiceProperty::BATCH_EXECUTE_JOB_PAYLOAD,           "0"}
                };

        BatschedNetworkListener(std::string, std::string batch_service_mailbox, std::string sched_port,
                             NETWORK_LISTENER_TYPE MY_TYPE, std::string data_to_send, std::map<std::string, std::string> plist, std::string suffix);


        int main() override;
        std::string self_port;
        std::string sched_port;
        std::string data_to_send;
        std::string reply_received;
        std::string batch_service_mailbox;

        NETWORK_LISTENER_TYPE MY_LISTENER_TYPE;

        void send_receive();

        void sendExecuteMessageToBatchService(std::string answer_mailbox, std::string execute_job_reply_data);
        void sendQueryAnswerMessageToBatchService(double estimated_waiting_time);

    };

    /***********************/
    /** \endcond           */
    /***********************/
}


#endif //WRENCH_BATSCHEDNETWORKLISTENER_H
