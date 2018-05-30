/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <wrench/logging/TerminalOutput.h>
#include "wrench/services/network_proximity/NetworkProximityDaemon.h"
#include <wrench/simgrid_S4U_util/S4U_Simulation.h>
#include <wrench/simulation/SimulationMessage.h>
#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>
#include <random>
#include "NetworkProximityMessage.h"


XBT_LOG_NEW_DEFAULT_CATEGORY(network_daemons_service, "Log category for Network Daemons Service");

namespace wrench {

    /**
     * @brief Constructor
     * @param simulation: a pointer to the simulation object
     * @param hostname: the hostname on which to start the service
     * @param network_proximity_service_mailbox the mailbox of the network proximity service
     * @param message_size the size of the message to be sent between network daemons to compute proximity
     * @param measurement_period the time-difference between two message transfer to compute proximity
     * @param noise the noise to add to compute the time-difference
     */
    NetworkProximityDaemon::NetworkProximityDaemon(Simulation *simulation,
                                                   std::string hostname,
                                                   std::string network_proximity_service_mailbox,
                                                   double message_size = 1, double measurement_period = 1000,
                                                   double noise = 100) :
            NetworkProximityDaemon(simulation, std::move(hostname), std::move(network_proximity_service_mailbox),
                                   message_size, measurement_period, noise, "") {
    }


    /**
     * @brief Constructor
     * @param simulation: a pointer to the simulation object
     * @param hostname: the hostname on which to start the service
     * @param network_proximity_service_mailbox: the mailbox of the network proximity service
     * @param message_size: the size of the message to be sent between network daemons to compute proximity
     * @param measurement_period: the time-difference between two message transfer to compute proximity
     * @param noise: the maximum magnitude of random noises added to the measurement_period at each iteration,
     *               so as to avoid idiosyncratic behaviors that occur with perfect synchrony
     * @param suffix: suffix to append to the service name and mailbox
     */

    NetworkProximityDaemon::NetworkProximityDaemon(
            Simulation *simulation,
            std::string hostname,
            std::string network_proximity_service_mailbox,
            double message_size = 1, double measurement_period = 1000,
            double noise = 100, std::string suffix = "") :
            Service(std::move(hostname), "network_daemons" + suffix, "network_daemons" + suffix) {

      this->simulation = simulation;
      this->message_size = message_size * 0;
      this->measurement_period = measurement_period;
      this->max_noise = noise;
      this->next_mailbox_to_send = "";
      this->next_host_to_send = "";
      this->network_proximity_service_mailbox = std::move(network_proximity_service_mailbox);

      // Set properties
      this->setProperties(this->default_property_values,
                          {});
      this->setMessagePayloads(this->default_messagepayload_values, {});

    }

    /**
     * @brief Returns the (noisy) time until a next measurement should be taken
     * @return time until next measurement (in seconds)
     */
    double NetworkProximityDaemon::getTimeUntilNextMeasurement() {
      // TODO: Probably redo this: (i) pick a random double between -max_noise and + max_noise
      double noise = rand() % ((int) (2 * this->max_noise - 1)) - this->max_noise;
      return S4U_Simulation::getClock() + measurement_period + noise;
    }

    int NetworkProximityDaemon::main() {

      TerminalOutput::setThisProcessLoggingColor(TerminalOutput::Color::COLOR_MAGENTA);

      WRENCH_INFO("Network Daemons Service starting on host %s!", S4U_Simulation::getHostName().c_str());

      double time_for_next_measurement = this->getTimeUntilNextMeasurement();

      S4U_Mailbox::dputMessage(this->network_proximity_service_mailbox,
                               new NextContactDaemonRequestMessage(this,
                                                                   this->getMessagePayloadValueAsDouble(
                                                                           NetworkProximityServiceMessagePayload::NETWORK_DAEMON_CONTACT_REQUEST_PAYLOAD)));


      bool life = true;
      /** Main loop **/
      while (life) {
        double countdown = time_for_next_measurement - S4U_Simulation::getClock();
        if (countdown > 0) {
          life = this->processNextMessage(countdown);
        } else {
          if ((not next_host_to_send.empty()) || (not next_mailbox_to_send.empty())) {

            double start_time = S4U_Simulation::getClock();

            try {
              WRENCH_INFO("Sending a NetworkProximityTransferMessage from %s to %s", this->mailbox_name.c_str(),
                          this->next_mailbox_to_send.c_str());
              S4U_Mailbox::putMessage(this->next_mailbox_to_send,
                                      new NetworkProximityTransferMessage(
                                              this->message_size));


            } catch (std::shared_ptr<NetworkError> &cause) {
              time_for_next_measurement = this->getTimeUntilNextMeasurement();
              continue;
            }

            double end_time = S4U_Simulation::getClock();

            double proximityValue = end_time - start_time;


            std::pair<std::string, std::string> hosts;
            hosts = std::make_pair(S4U_Simulation::getHostName(), this->next_host_to_send);

            S4U_Mailbox::dputMessage(this->network_proximity_service_mailbox,
                                     new NetworkProximityComputeAnswerMessage(hosts, proximityValue,
                                                                              this->getMessagePayloadValueAsDouble(
                                                                                      NetworkProximityServiceMessagePayload::NETWORK_DAEMON_MEASUREMENT_REPORTING_PAYLOAD)));
            next_host_to_send = "";
            next_mailbox_to_send = "";

            time_for_next_measurement = this->getTimeUntilNextMeasurement();

          } else {
            time_for_next_measurement = this->getTimeUntilNextMeasurement();


            S4U_Mailbox::dputMessage(this->network_proximity_service_mailbox,
                                     new NextContactDaemonRequestMessage(this,
                                                                         this->getMessagePayloadValueAsDouble(
                                                                                 NetworkProximityServiceMessagePayload::NETWORK_DAEMON_CONTACT_REQUEST_PAYLOAD)));
          }

        }

      }

      WRENCH_INFO("Network Daemons Service on host %s terminated!", S4U_Simulation::getHostName().c_str());
      return 0;
    }


    bool NetworkProximityDaemon::processNextMessage(double timeout) {

      // Wait for a message
      std::unique_ptr<SimulationMessage> message = nullptr;

      try {
//            if(timeout<1){
//                std::cout<<"Timeout very less "<<this->mailbox_name<<"\n";
//            }
        message = S4U_Mailbox::getMessage(this->mailbox_name, timeout);
      } catch (std::shared_ptr<NetworkTimeout> &cause) {
        return true;
      } catch (std::shared_ptr<NetworkError> &cause) {
        return true;
      }

      if (message == nullptr) {
        WRENCH_INFO("Got a NULL message... Likely this means we're all done. Aborting!");
        return false;
      }

      WRENCH_INFO("Got a [%s] message", message->getName().c_str());

      if (auto msg = dynamic_cast<ServiceStopDaemonMessage *>(message.get())) {
        // This is Synchronous
        try {
          S4U_Mailbox::putMessage(msg->ack_mailbox,
                                  new ServiceDaemonStoppedMessage(this->getMessagePayloadValueAsDouble(
                                          NetworkProximityServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
          return false;
        }
        return false;

      } else if (auto msg = dynamic_cast<NextContactDaemonAnswerMessage *>(message.get())) {

        this->next_host_to_send = msg->next_host_to_send;
        this->next_mailbox_to_send = msg->next_mailbox_to_send;

        return true;

      } else if (auto msg = dynamic_cast<NetworkProximityTransferMessage *>(message.get())) {

        WRENCH_INFO("NetworkProximityTransferMessage: Got a [%s] message", message->getName().c_str());
        return true;

      } else {
        throw std::runtime_error(
                "NetworkProximityService::waitForNextMessage(): Unknown message type: " +
                std::to_string(message->payload));
      }
    }

}
