//
// Created by suraj on 8/29/17.
//

#ifndef WRENCH_BATCH_SERVICE_H
#define WRENCH_BATCH_SERVICE_H

#include <services/Service.h>

namespace wrench {
    class BatchService: public Service {

    /**
     * @brief A Batch Service
     */

    private:
        std::map<std::string, std::string> default_property_values =
                {{NetworkQueryServiceProperty::STOP_DAEMON_MESSAGE_PAYLOAD,          "1024"},
                 {NetworkQueryServiceProperty::DAEMON_STOPPED_MESSAGE_PAYLOAD,       "1024"},
                 {NetworkQueryServiceProperty::NETWORK_DB_LOOKUP_MESSAGE_PAYLOAD,    "1024"},
                 {NetworkQueryServiceProperty::NETWORK_DAEMON_CONTACT_ANSWER_PAYLOAD,    "1024"},
                 {NetworkQueryServiceProperty::LOOKUP_OVERHEAD,                      "0.0"},
                };

    public:
        BatchService(std::string db_hostname,
        std::vector<std::string> hosts_in_network,
        int message_size, double measurement_period, int noise,
                std::map<std::string, std::string> = {});

        double query(std::pair<std::string, std::string> hosts);


    private:

        friend class Simulation;

        BatchService(std::string db_hostname,
        std::vector<std::string> hosts_in_network,
        int message_size, double measurement_period,
        int noise,std::map<std::string, std::string>,
                std::string suffix = "");

        int main();

        bool processNextMessage();
    };
}


#endif //WRENCH_BATCH_SERVICE_H
