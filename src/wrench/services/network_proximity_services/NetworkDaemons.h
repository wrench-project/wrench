//
// Created by suraj on 8/9/17.
//

#ifndef WRENCH_NETWORKDAEMONS_H
#define WRENCH_NETWORKDAEMONS_H

#include <services/Service.h>
#include "NetworkQueryServiceProperty.h"

namespace wrench {

    class NetworkDaemons: public Service {
    public:

        NetworkDaemons(std::string hostname,
                       std::string network_proximity_service_mailbox,
        int message_size,double measurement_period,
        int noise);

        std::string getHostname();

    private:
        std::map<std::string, std::string> default_property_values =
                {{NetworkQueryServiceProperty::STOP_DAEMON_MESSAGE_PAYLOAD,          "1024"},
                 {NetworkQueryServiceProperty::DAEMON_STOPPED_MESSAGE_PAYLOAD,       "1024"},
                 {NetworkQueryServiceProperty::NETWORK_DAEMON_CONTACT_REQUEST_PAYLOAD,    "1024"},
                 {NetworkQueryServiceProperty::NETWORK_DAEMON_CONTACT_ANSWER_PAYLOAD,    "1024"},
                 {NetworkQueryServiceProperty::NETWORK_PROXIMITY_TRANSFER_MESSAGE_PAYLOAD,    "1024"},
                 {NetworkQueryServiceProperty::NETWORK_DAEMON_COMPUTE_ANSWER_PAYLOAD,    "1024"},
                 {NetworkQueryServiceProperty::LOOKUP_OVERHEAD,                      "0.0"},
                };



    private:

        friend class Simulation;

        NetworkDaemons(std::string hostname,
                       std::string network_proximity_service_mailbox,
                       int message_size,double measurement_period,
                       int noise, std::string suffix);


        std::string hostname;
        int message_size;
        double measurement_period;
        int noise;
        std::string suffix;
        std::string next_mailbox_to_send;
        std::string next_host_to_send;
        std::string network_proximity_service_mailbox;

        int main();


        bool processNextMessage(double timeout);
    };
}


#endif //WRENCH_NETWORKDAEMONS_H
