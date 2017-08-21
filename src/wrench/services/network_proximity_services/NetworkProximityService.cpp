//
// Created by suraj on 8/6/17.
//

#include <logging/TerminalOutput.h>
#include "NetworkProximityService.h"
#include <simgrid_S4U_util/S4U_Simulation.h>
#include <simulation/SimulationMessage.h>
#include <simgrid_S4U_util/S4U_Mailbox.h>
#include <services/ServiceMessage.h>
#include "NetworkProximityMessage.h"
#include "NetworkDaemons.h"

#include <exceptions/WorkflowExecutionException.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(network_proximity_service, "Log category for Network Proximity Service");

namespace wrench {

    /**
     * @brief Constructor
     * @param hostname: the hostname on which to start the service
     * @param hosts_in_network: the hosts running in the network
     * @param plist: a property list ({} means "use all defaults")
     */
    NetworkProximityService::NetworkProximityService(std::string hostname,
                                                     std::vector<std::string> hosts_in_network,
                                                     int message_size, double measurement_period, int noise,
                                                     std::map<std::string, std::string> plist):
    NetworkProximityService(hostname, hosts_in_network, message_size, measurement_period, noise, plist,"") {

    }

    /**
     * @brief Constructor
     * @param hostname: the hostname on which to start the service
     * @param hosts_in_network: the hosts running in the network
     * @param plist: a property list ({} means "use all defaults")
     * @param suffix: suffix to append to the service name and mailbox
     */
    NetworkProximityService::NetworkProximityService(
            std::string hostname,
            std::vector<std::string> hosts_in_network,
            int message_size, double measurement_period, int noise,
            std::map<std::string, std::string> plist,
            std::string suffix) :
            Service("network_proximity_service" + suffix, "network_proximity_service" + suffix) {

        this->hosts_in_network = std::move(hosts_in_network);

        // Set default properties
        for (auto p : this->default_property_values) {
            this->setProperty(p.first, p.second);
        }

        // Set specified properties
        for (auto p : plist) {
            this->setProperty(p.first, p.second);
        }

        //Start the network daemons
        std::vector<std::string>::iterator it;
        for (it=this->hosts_in_network.begin();it!=this->hosts_in_network.end();it++){
            this->network_daemons.push_back(new NetworkDaemons(*it,this->mailbox_name, message_size,measurement_period,noise));
        }

        // Start the daemon on the same host
        try {
            this->start(hostname);
        } catch (std::invalid_argument e) {
            throw e;
        }

    }


    /**
     * @brief Lookup an entry for a file
     * @param file: the file to lookup
     * @return The storage services that hold a copy of the file
     *
     * @throw WorkflowExecutionException
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    double NetworkProximityService::query(std::pair<std::string, std::string> hosts) {

        WRENCH_INFO("IN query()");

        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("network_query_entry");

        try {
            S4U_Mailbox::putMessage(this->mailbox_name, new NetworkProximityLookupRequestMessage(answer_mailbox, hosts,
                                                                                                 this->getPropertyValueAsDouble(
                                                                                                         NetworkQueryServiceProperty::NETWORK_DB_LOOKUP_MESSAGE_PAYLOAD)));
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
     * Internal method to add an entry to the database
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
                S4U_Mailbox::putMessage(msg->ack_mailbox,
                                        new ServiceDaemonStoppedMessage(this->getPropertyValueAsDouble(
                                                NetworkQueryServiceProperty::DAEMON_STOPPED_MESSAGE_PAYLOAD)));

                //Stop the network daemons
                std::vector<NetworkDaemons*>::iterator it;
                for (it=this->network_daemons.begin();it!=this->network_daemons.end();it++){
                    (*it)->stop();
                }
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
                                                                                         NetworkQueryServiceProperty::NETWORK_DB_LOOKUP_MESSAGE_PAYLOAD)));
            }
            catch (std::shared_ptr<NetworkError> cause) {
                return true;
            }
            return true;

        } else if (NetworkProximityComputeAnswerMessage *msg = dynamic_cast<NetworkProximityComputeAnswerMessage *>(message.get())) {
            try {
                this->addEntryToDatabase(msg->hosts,msg->proximityValue);
            }
            catch (std::shared_ptr<NetworkError> cause) {
                return true;
            }
            return true;

        }else if (NextContactDaemonRequestMessage *msg = dynamic_cast<NextContactDaemonRequestMessage *>(message.get())) {


            int randNum = (rand()%(this->hosts_in_network.size()));

            S4U_Mailbox::dputMessage(msg->answer_mailbox,
                                     new NextContactDaemonAnswerMessage(network_daemons.at(randNum)->getHostname(),network_daemons.at(randNum)->mailbox_name,
                                                                        this->getPropertyValueAsDouble(
                                                                                NetworkQueryServiceProperty::NETWORK_DAEMON_CONTACT_ANSWER_PAYLOAD)));
            return true;

        }  else {
            throw std::runtime_error(
                    "NetworkProximityService::waitForNextMessage(): Unknown message type: " + std::to_string(message->payload));
        }
    }
}