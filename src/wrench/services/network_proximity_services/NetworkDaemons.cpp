//
// Created by suraj on 8/9/17.
//


#include <logging/TerminalOutput.h>
#include "NetworkDaemons.h"
#include <simgrid_S4U_util/S4U_Simulation.h>
#include <simulation/SimulationMessage.h>
#include <simgrid_S4U_util/S4U_Mailbox.h>
#include "NetworkProximityMessage.h"


XBT_LOG_NEW_DEFAULT_CATEGORY(network_daemons_service, "Log category for Network Daemons Service");

namespace wrench {
    /**
     * @brief Constructor
     * @param hostname: the hostname on which to start the service
     */
    NetworkDaemons::NetworkDaemons(std::string hostname,
                                   std::string network_proximity_service_mailbox,
                                   int message_size=1,double measurement_period=1000,
                                   int noise=100):
            NetworkDaemons(hostname,network_proximity_service_mailbox,
            message_size,measurement_period, noise,"") {
    }


    /**
     * @brief Constructor
     * @param hostname: the hostname on which to start the service
     * @param suffix: suffix to append to the service name and mailbox
     */
    NetworkDaemons::NetworkDaemons(
            std::string hostname,
            std::string network_proximity_service_mailbox,
            int message_size=1,double measurement_period=1000,
            int noise=100, std::string suffix="") :
            Service("network_daemons" + suffix, "network_daemons" + suffix) {

        // Start the daemon on the same host
        this->hostname = std::move(hostname);
        this->message_size = message_size;
        this->measurement_period = measurement_period;
        this->noise = noise;
        this->next_mailbox_to_send = "";
        this->next_host_to_send = "";
        this->network_proximity_service_mailbox = std::move(network_proximity_service_mailbox);
        // Set default properties
        for (auto p : this->default_property_values) {
            this->setProperty(p.first, p.second);
        }
        this->setProperty("NETWORK_PROXIMITY_TRANSFER_MESSAGE_PAYLOAD",std::to_string(message_size));
        try {
            this->start(this->hostname);
        } catch (std::invalid_argument e) {
            throw e;
        }
    }

    int NetworkDaemons::main() {

        TerminalOutput::setThisProcessLoggingColor(WRENCH_LOGGING_COLOR_MAGENTA);

        WRENCH_INFO("Network Daemons Service starting on host %s!", S4U_Simulation::getHostName().c_str());

        double time_for_next_measurement = S4U_Simulation::getClock()+measurement_period+
                                            (rand()%((this->noise)-(-this->noise) + 1) + (this->noise));

        S4U_Mailbox::dputMessage(this->network_proximity_service_mailbox,
                                 new NextContactDaemonRequestMessage(this->mailbox_name,
                                                                     this->getPropertyValueAsDouble(
                                                                             NetworkQueryServiceProperty::NETWORK_DAEMON_CONTACT_REQUEST_PAYLOAD)));


        bool life = true;
        /** Main loop **/
        while (life) {
            double countdown = time_for_next_measurement-S4U_Simulation::getClock();
            if (countdown>0){
                life = this->processNextMessage(countdown);
            }else{
                if((next_host_to_send!="") || (next_mailbox_to_send!="")) {

                    char msg_to_send[this->message_size];

                    double start_time = S4U_Simulation::getClock();

                    try {
                        S4U_Mailbox::putMessage(this->next_mailbox_to_send,
                                                new NetworkProximityTransferMessage(msg_to_send,
                                                                                    this->getPropertyValueAsDouble(
                                                                                            NetworkQueryServiceProperty::NETWORK_PROXIMITY_TRANSFER_MESSAGE_PAYLOAD)));


                    } catch (std::shared_ptr<NetworkError> cause) {
                        return true;
                    }

                    double end_time = S4U_Simulation::getClock();

                    double proximityValue = end_time - start_time;


                    std::pair<std::string, std::string> hosts;
                    hosts = std::make_pair(S4U_Simulation::getHostName(), this->next_host_to_send);

                    S4U_Mailbox::dputMessage(this->network_proximity_service_mailbox,
                                             new NetworkProximityComputeAnswerMessage(hosts, proximityValue,
                                                                                      this->getPropertyValueAsDouble(
                                                                                              NetworkQueryServiceProperty::NETWORK_DAEMON_COMPUTE_ANSWER_PAYLOAD)));
                    next_host_to_send = "";
                    next_mailbox_to_send = "";

                    time_for_next_measurement = S4U_Simulation::getClock()+measurement_period+
                                                (rand()%((this->noise)-(-this->noise) + 1) + (this->noise));
                }

            }

        }

        WRENCH_INFO("Network Daemons Service on host %s terminated!", S4U_Simulation::getHostName().c_str());
        return 0;
    }

    bool NetworkDaemons::processNextMessage(double timeout) {

        // Wait for a message
        std::unique_ptr<SimulationMessage> message = nullptr;

        try {
            message = S4U_Mailbox::getMessage(this->mailbox_name,timeout);
        } catch (std::shared_ptr<NetworkTimeout> cause) {
            return true;
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
            } catch (std::shared_ptr<NetworkError> cause) {
                return false;
            }
            return false;

        }  else if (NextContactDaemonAnswerMessage *msg = dynamic_cast<NextContactDaemonAnswerMessage *>(message.get())) {

            this->next_host_to_send = msg->next_host_to_send;
            this->next_mailbox_to_send = msg->next_mailbox_to_send;

            return true;

        }else if (NetworkProximityTransferMessage *msg = dynamic_cast<NetworkProximityTransferMessage *>(message.get())) {

            WRENCH_INFO("NetworkProximityTransferMessage: Got a [%s] message", message->getName().c_str());
            return true;

        } else {
            throw std::runtime_error(
                    "NetworkProximityService::waitForNextMessage(): Unknown message type: " + std::to_string(message->payload));
        }
    }

    std::string NetworkDaemons::getHostname() {
        return this->hostname;
    }

}
