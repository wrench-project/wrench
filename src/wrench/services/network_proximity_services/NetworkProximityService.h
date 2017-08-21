//
// Created by suraj on 8/6/17.
//

#ifndef WRENCH_NETWORKPROXIMITYSERVICE_H
#define WRENCH_NETWORKPROXIMITYSERVICE_H

#include <services/Service.h>
#include "NetworkQueryServiceProperty.h"
#include "NetworkDaemons.h"

namespace wrench{

    class NetworkProximityService: public Service {

    private:
        std::map<std::string, std::string> default_property_values =
                {{NetworkQueryServiceProperty::STOP_DAEMON_MESSAGE_PAYLOAD,          "1024"},
                 {NetworkQueryServiceProperty::DAEMON_STOPPED_MESSAGE_PAYLOAD,       "1024"},
                 {NetworkQueryServiceProperty::NETWORK_DB_LOOKUP_MESSAGE_PAYLOAD,    "1024"},
                 {NetworkQueryServiceProperty::NETWORK_DAEMON_CONTACT_ANSWER_PAYLOAD,    "1024"},
                 {NetworkQueryServiceProperty::LOOKUP_OVERHEAD,                      "0.0"},
                };

    public:
        NetworkProximityService(std::string db_hostname,
                                std::vector<std::string> hosts_in_network,
                                int message_size, double measurement_period, int noise,
                                std::map<std::string, std::string> = {});

        double query(std::pair<std::string, std::string> hosts);


    private:

        friend class Simulation;

        NetworkProximityService(std::string db_hostname,
                                std::vector<std::string> hosts_in_network,
                                int message_size, double measurement_period,
                                int noise,std::map<std::string, std::string>,
                                std::string suffix = "");

        std::vector<NetworkDaemons*> network_daemons;
        std::vector<std::string> hosts_in_network;

        int main();

        bool processNextMessage();

        void addEntryToDatabase(std::pair<std::string,std::string> pair_hosts,double proximity_value);

        std::map<std::pair<std::string,std::string>,double> entries;

    };
}




#endif //WRENCH_NETWORKPROXIMITYSERVICE_H
