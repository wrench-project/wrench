/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/logging/TerminalOutput.h>
#include "wrench/services/network_proximity/NetworkProximityService.h"
#include <wrench/simgrid_S4U_util/S4U_Simulation.h>
#include <simulation/SimulationMessage.h>
#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>
#include <services/ServiceMessage.h>
#include "NetworkProximityMessage.h"
#include "wrench/services/network_proximity/NetworkProximityDaemon.h"

#include <wrench/exceptions/WorkflowExecutionException.h>
#include <random>

#include <boost/algorithm/string.hpp>
#include <functional>

XBT_LOG_NEW_DEFAULT_CATEGORY(network_proximity_service, "Log category for Network Proximity Service");

namespace wrench {

    // TODO: discuss why clear property values?
    /**
     * @brief Destructor
     */

    NetworkProximityService::~NetworkProximityService() {
        this->default_property_values.clear();
    }

    // TODO: discuss removal of other constructor
    /**
     * @brief Constructor
     * @param hostname: the hostname on which to start the service
     * @param hosts_in_network: the hosts running in the network
     * @param message_size: the message size (in bytes) exchanged by monitoring daemon
     * @param measurement_period: the time (in seconds) between measurements at a daemon
     * @param noise: a random noise added to the daemon (TODO: more explanation)
     * @param plist: a property list ({} means "use all defaults")
     */
    NetworkProximityService::NetworkProximityService(std::string hostname,
                                                     std::vector<std::string> hosts_in_network,
                                                     int message_size, double measurement_period, int noise,
                                                     std::map<std::string, std::string> plist):
                                                     Service(hostname, "network_proximity" , "network_proximity") {

        this->hosts_in_network = std::move(hosts_in_network);

        // Set default properties
        for (auto p : this->default_property_values) {
            this->setProperty(p.first, p.second);
        }

        // Set specified properties
        for (auto p : plist) {
            this->setProperty(p.first, p.second);
        }

        // TODO: discuss
        // NEED TO STORE SHARED POINTERS TO THESE NETWORK DAEMONS INSTEAD...
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //        VIVALDI STARTUP STUFF, PROBABLY SHOULD PUT THIS IN A FUNCTION so the ifelse block is smaller        //
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        std::string network_service_type = this->getPropertyValueAsString("NETWORK_PROXIMITY_SERVICE_TYPE");

        if (boost::iequals(network_service_type, "vivaldi")) {

            // set up the coordinate_lookup_table
            for (auto const &host: this->hosts_in_network) {
                this->coordinate_lookup_table.insert(std::make_pair(host, std::make_pair(0.0, 0.0)));
            }

            if (this->hosts_in_network.size() > 1) {
                double communication_percentage = this->getPropertyValueAsDouble("NETWORK_DAEMON_COMMUNICATION_PERCENTAGE");

                /* Each network daemon can communicate with a maximum of (n-1) other daemons.
                 * The size of a network daemon's communication pool will a percentage of (n-1) other daemons.
                 */
                int communication_pool_size = std::ceil(communication_percentage * this->hosts_in_network.size() - 1);

                /* Set up the communication look up table.
                 * Maybe I need to switch this to an unordered_map for faster lookup times. What happens if
                 * load factor gets bad? That won't really be a problem if I use map, but lookup times are
                 * logn..
                 *
                 */
                std::vector<std::string> empty_vector;
                for (auto const &host: this->hosts_in_network) {
                    this->communication_lookup_table.insert(std::make_pair(host, empty_vector));
                }

                // used for some randomness
                std::hash<std::string> hash_func;
                std::default_random_engine e;
                std::uniform_int_distribution<unsigned> u(0, this->hosts_in_network.size() - 1);

                std::list<std::pair<std::string, int>> possible_peers;
                std::list<std::pair<std::string, int>>::const_iterator stop_itr;

                /* For each host that a network daemon will run on, create a linked list of all the OTHER network
                 * daemons and give each node a random number.
                 * Sort that linked list by the random number.
                 * Each network daemon's communication pool will be the first 'communication_pool_size' nodes
                 * from the linked list.
                 *
                 * This whole nested for loop looks like n^2logn, that seems bad. Maybe if I pick
                 * communication_pool_size random numbers, it could be faster, but would have to account for
                 * duplicate random numbers being chosen.
                 */
                for (auto current_host = this->hosts_in_network.cbegin();
                     current_host != this->hosts_in_network.cend(); ++current_host) {
                    possible_peers.clear();
                    e.seed(hash_func(*current_host));
                    for (auto it = this->hosts_in_network.cbegin(); it != this->hosts_in_network.cend(); ++it) {
                        if (current_host != it) {
                            possible_peers.push_front(std::make_pair(*it, u(e)));
                        }
                    }

                    // Sort based on random value assigned to each node. Is this random enough?
                    possible_peers.sort([](const std::pair<std::string, int> &f, const std::pair<std::string, int> &s) {
                        return f.second <= s.second;
                    });

                    // Pick communication_pool_size network daemons
                    stop_itr = possible_peers.cbegin();
                    std::advance(stop_itr, communication_pool_size);

                    auto search = this->communication_lookup_table.find(*current_host);
                    if (search != this->communication_lookup_table.end()) {
                        for (auto vec_itr = possible_peers.cbegin(); vec_itr != stop_itr; ++vec_itr) {
                            (search->second).push_back(vec_itr->first);
                        }
                    }
                }

                // DEBUGING: printing comm lookup table
                WRENCH_INFO("--------NETWORK DAEMON COMMUNICATION POOLS---------");
                for (auto const & pool: this->communication_lookup_table) {
                    WRENCH_INFO("%s network daemon talks to:", pool.first.c_str());
                    for (auto const & peer: pool.second) {
                        WRENCH_INFO("peer: %s", peer.c_str());
                    }
                    WRENCH_INFO("---------------------------------------------------");
                }
            }
        }
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //                                       END VIVALDI STARTUP RELATED STUFF                                    //
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////


        // Create the network daemons
        std::vector<std::string>::iterator it;
        for (it=this->hosts_in_network.begin();it!=this->hosts_in_network.end();it++){
            this->network_daemons.push_back(std::unique_ptr<NetworkProximityDaemon>(new NetworkProximityDaemon(*it,this->mailbox_name, message_size,measurement_period,noise)));
        }
    }

    /**
     * @brief Starts the network proximity service sets of daemons and the
     *        proximity service itself
     *
     * @throw std::runtime_error
     */
    void NetworkProximityService::start() {
      try {
        // Start the network daemons
        for (auto it = this->network_daemons.begin(); it != this->network_daemons.end(); it++) {
          (*it)->start();
        }
        this->start_daemon(this->hostname, false);
      } catch (std::runtime_error &e) {
        throw;
      }
    }

    /**
     * @brief Look up the current (x,y) coordinates of a given host
     * @param requested_host: the host whose coordinates are being requested
     * @return The pair (x,y) coordinate values
     *
     * @throw WorkFlowExecutionException
     * @throw std::runtime_error
     */
    std::pair<double, double> NetworkProximityService::getCoordinate(std::string requested_host) {
        WRENCH_INFO("IN query()");

        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("network_get_coordinate_entry");

        try {
            S4U_Mailbox::putMessage(this->mailbox_name, new CoordinateLookupRequestMessage(answer_mailbox, std::move(requested_host),
                                                                                                 this->getPropertyValueAsDouble(
                                                                                                         NetworkProximityServiceProperty::NETWORK_DB_LOOKUP_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> cause) {
            throw WorkflowExecutionException(cause);
        }

        std::unique_ptr<SimulationMessage> message = nullptr;

        try {
            message = S4U_Mailbox::getMessage(answer_mailbox);
        } catch (std::shared_ptr<NetworkError> cause) {
            throw WorkflowExecutionException(cause);
        }

        if (CoordinateLookupAnswerMessage *msg = dynamic_cast<CoordinateLookupAnswerMessage *>(message.get())) {
            return msg->xy_coordinate;
        } else {
            throw std::runtime_error("NetworkProximityService::getCoordinate(): Unexpected [" + message->getName() + "] message");
        }
    }

    /**
     * @brief Look up for the proximity value in database
     * @param hosts: the pair of hosts to look for the proximity value
     * @return The proximity value between the pair of hosts
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    double NetworkProximityService::query(std::pair<std::string, std::string> hosts) {

        WRENCH_INFO("IN query()");

        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("network_query_entry");

        try {
            S4U_Mailbox::putMessage(this->mailbox_name, new NetworkProximityLookupRequestMessage(answer_mailbox, std::move(hosts),
                                                                                                 this->getPropertyValueAsDouble(
                                                                                                         NetworkProximityServiceProperty::NETWORK_DB_LOOKUP_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> cause) {
            throw WorkflowExecutionException(cause);
        }

        std::unique_ptr<SimulationMessage> message = nullptr;

        try {
            message = S4U_Mailbox::getMessage(answer_mailbox);
        } catch (std::shared_ptr<NetworkError> cause) {
            throw WorkflowExecutionException(cause);
        }

        if (NetworkProximityLookupAnswerMessage *msg = dynamic_cast<NetworkProximityLookupAnswerMessage *>(message.get())) {
            return msg->proximityValue;
        } else {
            throw std::runtime_error("NetworkProximityService::query(): Unexpected [" + message->getName() + "] message");
        }
    }

    /**
     * @brief Internal method to add an entry to the database
     * @param pair: a pair of hosts
     * @param proximity_value: proximity value between the pair
     */
    void NetworkProximityService::addEntryToDatabase(std::pair<std::string,std::string> pair_hosts,double proximity_value) {
        if (this->entries.find(pair_hosts) != this->entries.end()) {
            std::pair<std::pair<std::string,std::string>,double> value = std::make_pair(pair_hosts,proximity_value);
            this->entries.insert(value);
        } else {
            this->entries[pair_hosts] = proximity_value;
        }
    }

    /**
     * @brief Main routine of the daemon
     *
     * @return 0 on success, non 0 otherwise
     */
    int NetworkProximityService::main() {

        TerminalOutput::setThisProcessLoggingColor(WRENCH_LOGGING_COLOR_MAGENTA);

        WRENCH_INFO("Network Proximity Service starting on host %s!", S4U_Simulation::getHostName().c_str());

        /** Main loop **/
        while (this->processNextMessage()) {

        }

        WRENCH_INFO("Network Proximity Service on host %s terminated!", S4U_Simulation::getHostName().c_str());
        return 0;
    }

    bool NetworkProximityService::processNextMessage() {

        // Wait for a message
        std::unique_ptr<SimulationMessage> message = nullptr;

        try {
            message = S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<NetworkError> cause) {
            return true;
        }

        if (message == nullptr) {
            WRENCH_INFO("Got a NULL message... Likely this means we're all done. Aborting!");
            return false;
        }



        WRENCH_INFO("Got a [%s] message", message->getName().c_str());

        if (ServiceStopDaemonMessage *msg = dynamic_cast<ServiceStopDaemonMessage *>(message.get())) {
            // This is Synchronous
            try {
                //Stop the network daemons
                std::vector<std::unique_ptr<NetworkProximityDaemon>>::iterator it;
                for (it=this->network_daemons.begin();it!=this->network_daemons.end();it++){
                    if((*it)->isUp()) {
                        (*it)->stop();
                    }
                }
                this->network_daemons.clear();
                this->hosts_in_network.clear();
                S4U_Mailbox::putMessage(msg->ack_mailbox,
                                        new ServiceDaemonStoppedMessage(this->getPropertyValueAsDouble(
                                                NetworkProximityServiceProperty::DAEMON_STOPPED_MESSAGE_PAYLOAD)));

            } catch (std::shared_ptr<NetworkError> cause) {
                return false;
            }
            return false;

        } else if (NetworkProximityLookupRequestMessage *msg = dynamic_cast<NetworkProximityLookupRequestMessage *>(message.get())) {
            double proximityValue=-1.0;

            // TODO: discuss using typedefs
            std::string network_service_type = this->getPropertyValueAsString("NETWORK_PROXIMITY_SERVICE_TYPE");

            // should i use a typedefd struct for coordinates? if so, where would i put the definition?
            if (boost::iequals(network_service_type, "vivaldi")) {
                auto host1 = this->coordinate_lookup_table.find(msg->hosts.first);
                auto host2 = this->coordinate_lookup_table.find(msg->hosts.second);

                if (host1 != this->coordinate_lookup_table.end() && host2 != this->coordinate_lookup_table.end()) {
                    double h1_x, h1_y, h2_x, h2_y;

                    h1_x = host1->second.first;
                    h1_y = host1->second.second;

                    h2_x = host2->second.first;
                    h2_y = host2->second.second;

                    double x, y;
                    x = h1_x - h2_x;
                    y = h1_y - h2_y;

                    proximityValue = std::sqrt(std::pow(x, 2) + std::pow(y, 2));
                }
            }
            else {
                if (this->entries.find(msg->hosts) != this->entries.end()) {
                    proximityValue = this->entries[msg->hosts];
                    //this->addEntryToDatabase(msg->hosts,proximityValue);
                }
            }

            try {
                //NetworkProximityComputeAnswerMessage *proximity_msg = dynamic_cast<NetworkProximityComputeAnswerMessage *>(message.get());
                S4U_Mailbox::dputMessage(msg->answer_mailbox,
                                         new NetworkProximityLookupAnswerMessage(msg->hosts,proximityValue,
                                                                                 this->getPropertyValueAsDouble(
                                                                                         NetworkProximityServiceProperty::NETWORK_DB_LOOKUP_MESSAGE_PAYLOAD)));
            }
            catch (std::shared_ptr<NetworkError> cause) {
                return true;
            }
            return true;

        } else if (NetworkProximityComputeAnswerMessage *msg = dynamic_cast<NetworkProximityComputeAnswerMessage *>(message.get())) {
            try {
                WRENCH_INFO("NetworkProximityService::processNextMessage()::Adding proximity value between %s and %s into the database",msg->hosts.first.c_str(),msg->hosts.second.c_str());
                this->addEntryToDatabase(msg->hosts,msg->proximityValue);

                // TODO: if this is Vivaldi, update coordinates of hosts.first and hosts.second

            }
            catch (std::shared_ptr<NetworkError> cause) {
                return true;
            }
            return true;

        }else if (NextContactDaemonRequestMessage *msg = dynamic_cast<NextContactDaemonRequestMessage *>(message.get())) {
            int random_number;
            std::string next_host_to_send;
            std::string next_mail_box_to_send;
            // TODO: if this is All2All, keep Suraj's implementation, if not then use vivaldi scheme
            std::string network_service_type = this->getPropertyValueAsString("NETWORK_PROXIMITY_SERVICE_TYPE");

            if (boost::iequals(network_service_type, "a")) {
                std::hash<std::string> hash_func;
                std::default_random_engine e;
               // std::uniform_int_distribution<unsigned> u(0, this->communication_lookup_table.find(msg->hostname)->second.size() - 1);
                // random_number = u(e);
            }
            else {
                std::random_device rdev;
                std::mt19937 rgen(rdev());
                std::uniform_int_distribution<int> idist(0,this->hosts_in_network.size()-1);
                random_number = idist(rgen);
            }

//            unsigned long randNum = (std::rand()%(this->hosts_in_network.size()));

            S4U_Mailbox::dputMessage(msg->answer_mailbox,
                                     new NextContactDaemonAnswerMessage(network_daemons.at(random_number)->getHostname(),network_daemons.at(random_number)->mailbox_name,
                                                                        this->getPropertyValueAsDouble(
                                                                                NetworkProximityServiceProperty::NETWORK_DAEMON_CONTACT_ANSWER_PAYLOAD)));
            return true;
        }

        // TODO: discuss
        else if (CoordinateLookupRequestMessage *msg = dynamic_cast<CoordinateLookupRequestMessage *> (message.get())) {
            std::string requested_host = msg->requested_host;
            auto const coordinate_itr = this->coordinate_lookup_table.find(requested_host);
            if (coordinate_itr != this->coordinate_lookup_table.cend()) {
                try {
                    S4U_Mailbox::dputMessage(msg->answer_mailbox,
                    new CoordinateLookupAnswerMessage(requested_host, coordinate_itr->second,
                    this->getPropertyValueAsDouble(NetworkProximityServiceProperty::NETWORK_DAEMON_CONTACT_ANSWER_PAYLOAD)));
                }
                catch (std::shared_ptr<NetworkError> cause) {
                    return true;
                }
            }
        }

        else {
            throw std::runtime_error(
                    "NetworkProximityService::processNextMessage(): Unknown message type: " + std::to_string(message->payload));
        }
    }
}