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
#include <wrench/simulation/SimulationMessage.h>
#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>
#include <wrench/services/ServiceMessage.h>
#include "NetworkProximityMessage.h"
#include "wrench/services/network_proximity/NetworkProximityDaemon.h"

#include <wrench/exceptions/WorkflowExecutionException.h>
#include <random>

XBT_LOG_NEW_DEFAULT_CATEGORY(network_proximity_service, "Log category for Network Proximity Service");

namespace wrench {


    /**
     * @brief Destructor
     */

    NetworkProximityService::~NetworkProximityService() {
        this->default_property_values.clear();
    }

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
    NetworkProximityService(std::move(hostname), std::move(hosts_in_network), message_size, measurement_period, noise, std::move(plist),"") {

    }

    /**
     * @brief Constructor
     * @param hostname: the hostname on which to start the service
     * @param hosts_in_network: the hosts running in the network
     * @param message_size: TODO
     * @param measurement_period: TODO
     * @param noise: TODO
     * @param plist: a property list ({} means "use all defaults")
     * @param suffix: suffix to append to the service name and mailbox
     */
    NetworkProximityService::NetworkProximityService(
            std::string hostname,
            std::vector<std::string> hosts_in_network,
            int message_size, double measurement_period, int noise,
            std::map<std::string, std::string> plist,
            std::string suffix) :
            Service(hostname, "network_proximity" + suffix, "network_proximity" + suffix) {

        this->hosts_in_network = std::move(hosts_in_network);

        // Set default properties
        for (auto p : this->default_property_values) {
            this->setProperty(p.first, p.second);
        }

        // Set specified properties
        for (auto p : plist) {
            this->setProperty(p.first, p.second);
        }

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
            if (this->entries.find(msg->hosts) != this->entries.end()) {
                proximityValue = this->entries[msg->hosts];
                //this->addEntryToDatabase(msg->hosts,proximityValue);

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
            }
            catch (std::shared_ptr<NetworkError> cause) {
                return true;
            }
            return true;

        }else if (NextContactDaemonRequestMessage *msg = dynamic_cast<NextContactDaemonRequestMessage *>(message.get())) {


            std::random_device rdev;
            std::mt19937 rgen(rdev());
            std::uniform_int_distribution<int> idist(0,this->hosts_in_network.size()-1);
            int random_number = idist(rgen);

//            unsigned long randNum = (std::rand()%(this->hosts_in_network.size()));

            S4U_Mailbox::dputMessage(msg->answer_mailbox,
                                     new NextContactDaemonAnswerMessage(network_daemons.at(random_number)->getHostname(),network_daemons.at(random_number)->mailbox_name,
                                                                        this->getPropertyValueAsDouble(
                                                                                NetworkProximityServiceProperty::NETWORK_DAEMON_CONTACT_ANSWER_PAYLOAD)));
            return true;

        }  else {
            throw std::runtime_error(
                    "NetworkProximityService::processNextMessage(): Unknown message type: " + std::to_string(message->payload));
        }
    }
}