/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <wrench/logging/TerminalOutput.h>
#include <wrench/services/network_proximity/NetworkProximitySenderDaemon.h>
#include <wrench/failure_causes/NetworkError.h>
#include <wrench/simgrid_S4U_util/S4U_Simulation.h>
#include <wrench/simulation/SimulationMessage.h>
#include <wrench/simgrid_S4U_util/S4U_CommPort.h>
#include <random>
#include "wrench/services/network_proximity/NetworkProximityMessage.h"
#include "wrench/exceptions/ExecutionException.h"


WRENCH_LOG_CATEGORY(wrench_core_network_sender_daemon_service, "Log category for Network Proximity Sender Daemon Service");

namespace wrench {

    //    /**
    //     * @brief Constructor
    //     * @param simulation: a pointer to the simulation object
    //     * @param hostname: the hostname on which to start the service
    //     * @param network_proximity_service_commport the commport of the network proximity service
    //     * @param message_size the size of the message to be sent between network daemons to compute proximity
    //     * @param measurement_period the time-difference between two message transfer to compute proximity
    //     * @param noise the noise to add to compute the time-difference
    //     * @param noise_seed seed for the noise RNG
    //     * @param messagepayload_list: a message payload list
    //     */
    //    NetworkProximitySenderDaemon::NetworkProximitySenderDaemon(Simulation *simulation,
    //                                                   std::string hostname,
    //                                                   S4U_CommPort *network_proximity_service_commport,
    //                                                   double message_size, double measurement_period,
    //                                                   double noise,
    //                                                   int noise_seed,
    //                                                   WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list) : NetworkProximitySenderDaemon(simulation, std::move(hostname), network_proximity_service_commport,
    //                                                                                                                                       message_size, measurement_period, noise, noise_seed, messagepayload_list, "") {
    //    }


    /**
     * @brief Constructor
     * @param simulation: a pointer to the simulation object
     * @param hostname: the hostname on which to start the service
     * @param network_proximity_service_commport: the commport of the network proximity service
     * @param message_size: the size of the message to be sent between network daemons to compute proximity
     * @param measurement_period: the time-difference between two message transfer to compute proximity
     * @param noise: the maximum magnitude of random noises added to the measurement_period at each iteration,
     *               so as to avoid idiosyncratic behaviors that occur with perfect synchrony
     * @param noise_seed seed for the noise RNG
     * @param messagepayload_list: a message payload list
     */

    NetworkProximitySenderDaemon::NetworkProximitySenderDaemon(
            Simulation *simulation,
            std::string hostname,
            S4U_CommPort *network_proximity_service_commport,
            double message_size, double measurement_period,
            double noise,
            int noise_seed,
            WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list) : Service(std::move(hostname), "network_proximity_sender_daemon") {
        // Set the message payloads
        setMessagePayloads(messagepayload_list, {});

        this->simulation = simulation;
        this->message_size = message_size;
        this->measurement_period = measurement_period;
        this->max_noise = noise;
        // TODO: This is weirdly all the same...
        this->rng.seed(noise_seed);

        this->next_commport_to_send = nullptr;
        this->next_daemon_to_send = nullptr;
        this->next_host_to_send = "";
        this->network_proximity_service_commport = network_proximity_service_commport;
    }

    /**
     * @brief Cleanup method
     *
     * @param has_returned_from_main: whether main() returned
     * @param return_value: the return value (if main() returned)
     */
    void NetworkProximitySenderDaemon::cleanup(bool has_returned_from_main, int return_value) {
        // Do nothing. It's fine to die, and we'll just autorestart with our previous state
    }


    /**
     * @brief Returns the (noisy) time until a next measurement should be taken
     * @return time until next measurement (in seconds)
     */
    double NetworkProximitySenderDaemon::getTimeUntilNextMeasurement() {
        static std::uniform_real_distribution<double> noise_dist(-max_noise, +max_noise);
        double noise = noise_dist(rng);
        return S4U_Simulation::getClock() + measurement_period + noise;
    }

    int NetworkProximitySenderDaemon::main() {
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_MAGENTA);

        WRENCH_INFO("Network Daemon Service starting on host %s!", S4U_Simulation::getHostName().c_str());

        // In case of restart
        this->commport->reset();
        this->recv_commport->reset();

        double time_for_next_measurement = this->getTimeUntilNextMeasurement();

        try {
            this->network_proximity_service_commport->putMessage(
                    new NextContactDaemonRequestMessage(
                            this->getSharedPtr<NetworkProximitySenderDaemon>(),
                            this->getMessagePayloadValue(
                                    NetworkProximityServiceMessagePayload::NETWORK_DAEMON_CONTACT_REQUEST_PAYLOAD)));
        } catch (ExecutionException &e) {
            // give up right away!
            return 0;
        }


        bool life = true;
        /** Main loop **/
        while (life) {
            S4U_Simulation::computeZeroFlop();

            double countdown = time_for_next_measurement - S4U_Simulation::getClock();
            //            WRENCH_INFO("COUNTDOWN = %lf", countdown);
            if (countdown > 0) {
                life = this->processNextMessage(countdown);
            } else {
                if ((this->next_commport_to_send) &&
                    (S4U_Simulation::isHostOn(this->next_host_to_send)) &&
                    (this->next_daemon_to_send->isUp())) {

                    double start_time = S4U_Simulation::getClock();

                    //                    WRENCH_INFO("Synchronously sending a NetworkProximityTransferMessage  (%lf) to %s",
                    //                                this->message_size, this->next_commport_to_send->get_cname());
                    try {
                        this->next_commport_to_send->putMessage(
                                new NetworkProximityTransferMessage(
                                        this->message_size));
                    } catch (ExecutionException &e) {
                        time_for_next_measurement = this->getTimeUntilNextMeasurement();
                        continue;
                    }

                    double end_time = S4U_Simulation::getClock();
                    double proximityValue = end_time - start_time;

                    std::pair<std::string, std::string> hosts;
                    hosts = std::make_pair(S4U_Simulation::getHostName(), this->next_host_to_send);

                    //                    WRENCH_INFO("Sending proximity compute answer message: %lf", proximityValue);
                    this->network_proximity_service_commport->dputMessage(
                            new NetworkProximityComputeAnswerMessage(hosts, proximityValue,
                                                                     this->getMessagePayloadValue(
                                                                             NetworkProximityServiceMessagePayload::NETWORK_DAEMON_MEASUREMENT_REPORTING_PAYLOAD)));

                    next_host_to_send = "";
                    next_commport_to_send = nullptr;

                    time_for_next_measurement = this->getTimeUntilNextMeasurement();

                } else {
                    time_for_next_measurement = this->getTimeUntilNextMeasurement();

                    try {
                        this->network_proximity_service_commport->putMessage(
                                new NextContactDaemonRequestMessage(
                                        this->getSharedPtr<NetworkProximitySenderDaemon>(),
                                        this->getMessagePayloadValue(
                                                NetworkProximityServiceMessagePayload::NETWORK_DAEMON_CONTACT_REQUEST_PAYLOAD)));
                    } catch (ExecutionException &e) {
                        // Couldn't find out who to talk to next... will try again soon
                    }
                }
            }
        }


        WRENCH_INFO("Network Daemons Service on host %s cleanly terminating!", S4U_Simulation::getHostName().c_str());
        return 0;
    }


    bool NetworkProximitySenderDaemon::processNextMessage(double timeout) {
        // Wait for a message
        std::shared_ptr<SimulationMessage> message = nullptr;

        try {
            message = this->commport->getMessage(timeout);
        } catch (ExecutionException &e) {
            auto cause = std::dynamic_pointer_cast<NetworkError>(e.getCause());
            if (not cause->isTimeout()) {
                WRENCH_INFO("Got a network error... oh well (%s)",
                            cause->toString().c_str());
            }
            return true;
        }

        WRENCH_DEBUG("Got a [%s] message", message->getName().c_str());

        if (auto msg = std::dynamic_pointer_cast<ServiceStopDaemonMessage>(message)) {
            // This is Synchronous
            try {
                msg->ack_commport->putMessage(
                        new ServiceDaemonStoppedMessage(this->getMessagePayloadValue(
                                NetworkProximityServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
            } catch (ExecutionException &e) {
                return false;
            }
            return false;

        } else if (auto msg = std::dynamic_pointer_cast<NextContactDaemonAnswerMessage>(message)) {
            this->next_host_to_send = msg->next_host_to_send;
            this->next_daemon_to_send = msg->next_daemon_to_send;
            this->next_commport_to_send = msg->next_commport_to_send;
            //            WRENCH_INFO("TOLD TO CONTACT %s NEXT", this->next_daemon_to_send->getName().c_str());
            return true;
        } else {
            throw std::runtime_error(
                    "NetworkProximityService::waitForNextMessage(): Unexpected [" + message->getName() + "] message");
        }
    }

}// namespace wrench
