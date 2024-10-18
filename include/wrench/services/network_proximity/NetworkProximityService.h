/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_NETWORKPROXIMITYSERVICE_H
#define WRENCH_NETWORKPROXIMITYSERVICE_H

#include <cfloat>
#include <complex>
#include <random>
#include "wrench/services/Service.h"
#include "wrench/services/network_proximity/NetworkProximityServiceProperty.h"
#include "wrench/services/network_proximity/NetworkProximitySenderDaemon.h"
#include "wrench/services/network_proximity/NetworkProximityReceiverDaemon.h"
#include "wrench/simgrid_S4U_util/S4U_CommPort.h"

namespace wrench {

    /**
     * @brief A network proximity service that continuously estimates inter-host latencies
     *        and can be queried for such estimates
     */
    class NetworkProximityService : public Service {

    private:
        WRENCH_PROPERTY_COLLECTION_TYPE default_property_values = {
                {NetworkProximityServiceProperty::LOOKUP_OVERHEAD, "0s"},
                {NetworkProximityServiceProperty::NETWORK_PROXIMITY_SERVICE_TYPE, "ALLTOALL"},
                {NetworkProximityServiceProperty::NETWORK_PROXIMITY_MESSAGE_SIZE, "1024"},
                {NetworkProximityServiceProperty::NETWORK_PROXIMITY_MEASUREMENT_PERIOD, "60s"},
                {NetworkProximityServiceProperty::NETWORK_PROXIMITY_MEASUREMENT_PERIOD_MAX_NOISE, "20"},
                {NetworkProximityServiceProperty::NETWORK_PROXIMITY_MEASUREMENT_PERIOD_NOISE_SEED, "0"},
                {NetworkProximityServiceProperty::NETWORK_DAEMON_COMMUNICATION_COVERAGE, "1.0"},
                {NetworkProximityServiceProperty::NETWORK_PROXIMITY_PEER_LOOKUP_SEED, "1"}};

        WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE default_messagepayload_values = {
                {NetworkProximityServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {NetworkProximityServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {NetworkProximityServiceMessagePayload::NETWORK_DAEMON_MEASUREMENT_REPORTING_PAYLOAD, S4U_CommPort::default_control_message_size},
                {NetworkProximityServiceMessagePayload::NETWORK_DB_LOOKUP_REQUEST_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {NetworkProximityServiceMessagePayload::NETWORK_DB_LOOKUP_ANSWER_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {NetworkProximityServiceMessagePayload::NETWORK_DAEMON_CONTACT_REQUEST_PAYLOAD, S4U_CommPort::default_control_message_size},
                {NetworkProximityServiceMessagePayload::NETWORK_DAEMON_CONTACT_ANSWER_PAYLOAD, S4U_CommPort::default_control_message_size},
        };

    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/

        /**
         * @brief A convenient constant that is returned as a latency between two hosts
         *        when no latency estimates are available for this pair of hosts.
         */
        static constexpr double NOT_AVAILABLE = DBL_MAX;

        ~NetworkProximityService() override;

        /***********************/
        /** \endcond           */
        /***********************/

        NetworkProximityService(const std::string& db_hostname,
                                std::vector<std::string> hosts_in_network,
                                const WRENCH_PROPERTY_COLLECTION_TYPE& property_list = {},
                                const WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE& messagepayload_list = {});

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        std::vector<std::string> getHostnameList();

        std::pair<double, double> getHostPairDistance(std::pair<std::string, std::string> hosts);

        std::pair<std::pair<double, double>, double> getHostCoordinate(const std::string&);

        std::string getNetworkProximityServiceType();

        /***********************/
        /** \endcond           */
        /***********************/

    protected:
        void cleanup(bool has_returned_from_main, int return_value) override;

    private:
        friend class Simulation;

        std::vector<std::shared_ptr<NetworkProximitySenderDaemon>> network_sender_daemons;
        std::vector<std::shared_ptr<NetworkProximityReceiverDaemon>> network_receiver_daemons;
        std::vector<std::string> hosts_in_network;

        std::default_random_engine master_rng;

        int main() override;

        bool processNextMessage();

        void addEntryToDatabase(const std::pair<std::string, std::string> &pair_hosts, double proximity_value);

        std::map<std::pair<std::string, std::string>, std::pair<double, double>> entries;

        std::map<std::string, std::pair<std::complex<double>, double>> coordinate_lookup_table;

        std::shared_ptr<NetworkProximityReceiverDaemon>
        getCommunicationPeer(const std::shared_ptr<NetworkProximitySenderDaemon>& sender_daemon);

        void vivaldiUpdate(double proximityValue, const std::string& sender_hostname, const std::string& peer_hostname);

        void validateProperties();
    };
}// namespace wrench

#endif//WRENCH_NETWORKPROXIMITYSERVICE_H
