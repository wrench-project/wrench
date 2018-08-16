/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include "wrench/exceptions/WorkflowExecutionException.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/services/compute/batch/BatchService.h"
#include "wrench/services/compute/batch/BatchServiceMessage.h"
#include "wrench/services/compute/batch/BatchServiceProperty.h"
#include "wrench/services/compute/batch/BatschedNetworkListener.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"

#ifdef ENABLE_BATSCHED

#include <nlohmann/json.hpp>
#include <zmq.hpp>
#include <zmq.h>

#include <unistd.h>

#endif

XBT_LOG_NEW_DEFAULT_CATEGORY(batch_network_listener_service, "Log category for Batch Network Listener Service");

class context_t;
namespace wrench {

    #ifdef ENABLE_BATSCHED

    /**
    * @brief Constructor
    * @param hostname: the hostname on which to start the service
    * @param batch_service: the BatchService that this service reports to
    * @param batch_service_mailbox: the name of the mailbox of the BatchService
    * @param sched_port the port for sending messages to Batsched
    * @param data_to_send: the data to send (as a JSON string)
    * @param property_list: property list ({} means "use all defaults")
    */
    BatschedNetworkListener::BatschedNetworkListener(std::string hostname, BatchService *batch_service,
                                                     std::string batch_service_mailbox,
                                                     std::string sched_port,
                                                     std::string data_to_send,
                                                     std::map<std::string, std::string> property_list) :
            BatschedNetworkListener(hostname, batch_service, batch_service_mailbox,
                                    sched_port, data_to_send, property_list, "") {
    }


    /**
    * @brief Constructor
    * @param hostname: the hostname on which to start the service
    * @param batch_service: the batch service
    * @param batch_service_mailbox: the name of the mailbox of the batch_service
    * @param sched_port the port to send messages to Batsched
    * @param data_to_send: data to send
    * @param property_list: property list ({} means "use all defaults")
    * @param suffix the suffix to append
    */
    BatschedNetworkListener::BatschedNetworkListener(
            std::string hostname, BatchService *batch_service, std::string batch_service_mailbox,
            std::string sched_port,
            std::string data_to_send, std::map<std::string, std::string> property_list,
            std::string suffix = "") :
            Service(hostname, "batch_network_listener" + suffix, "batch_network_listener" + suffix) {

      // Start the daemon on the same host
      this->sched_port = sched_port;
      this->data_to_send = data_to_send;
      this->batch_service = batch_service;
      this->batch_service_mailbox = batch_service_mailbox;
      // Set default and specified properties
      this->setProperties(this->default_property_values, property_list);

    }

    /**
     * @brief: main routine of the BatschedNetworkListener service
     * @return
     */
    int BatschedNetworkListener::main() {

      #ifdef ENABLE_BATSCHED
      TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_CYAN);

      WRENCH_INFO("Starting");

      /** Main loop **/
      if (this->data_to_send.empty()) {
        throw std::runtime_error(
                "BatschedNetworkListener::BatschedNetworkListener():Network sending process has no data to send"
        );
      }
      this->send_receive();

      WRENCH_INFO("Batch Network Listener Service on host %s terminating!", S4U_Simulation::getHostName().c_str());
      return 0;
      #else
      throw std::runtime_error("BatschedNetworkListener::main(): This should not be called unless ENABLE_BATSCHED is set to on!");
      #endif
    }

    /**
     * @brief Send an "execute" message to the batch service
     * @param answer_mailbox: mailbox on which ack will be received
     * @param execute_job_reply_data: message to send
     */
    void BatschedNetworkListener::sendExecuteMessageToBatchService(std::string answer_mailbox,
                                                                   std::string execute_job_reply_data) {
      try {
        S4U_Mailbox::putMessage(this->batch_service_mailbox,
                                new BatchExecuteJobFromBatSchedMessage(answer_mailbox, execute_job_reply_data, 0));
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }
    }

    /**
     * @brief Send a "query answer" message to the batch service
     * @param estimated_waiting_time: batch queue wait time estimate
     */
    void BatschedNetworkListener::sendQueryAnswerMessageToBatchService(double estimated_waiting_time) {
      try {
        S4U_Mailbox::putMessage(this->batch_service_mailbox,
                                new BatchQueryAnswerMessage(estimated_waiting_time,0));
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }
    }

    /**
     * @brief Method to interact with Batsched
     */
    void BatschedNetworkListener::send_receive() {

      zmq::context_t context(1);
      zmq::socket_t socket(context, ZMQ_REQ);
      socket.connect("tcp://localhost:" + this->sched_port);

      zmq::message_t request(strlen(this->data_to_send.c_str()));
      memcpy(request.data(), this->data_to_send.c_str(), strlen(this->data_to_send.c_str()));
      socket.send(request);

      //  Get the reply.
      zmq::message_t reply;

      // This "backoff" approach is to detect batsched errors!
      int max_num_trials = 100;
      int trials;
      for (trials=0; trials < max_num_trials; trials++) {
          usleep(100 + 100 * trials);
          int ret = socket.recv(&reply, ZMQ_DONTWAIT);
          if (ret > 0) {
            break;
          }
      }
      if (trials == max_num_trials) {
        throw std::runtime_error("Fatal Batsched Error (like some failed assert in the scheduler, could be transient)!");
      }

      socket.close();

      std::string reply_data;
      reply_data = std::string(static_cast<char *>(reply.data()), reply.size());

      nlohmann::json reply_decisions;
      nlohmann::json decision_events;
      reply_decisions = nlohmann::json::parse(reply_data);
      decision_events = reply_decisions["events"];

      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("batch_network_listener_mailbox");
      for (auto decisions:decision_events) {
        std::string decision_type = decisions["type"];
        double decision_timestamp = decisions["timestamp"];
        double time_to_sleep = S4U_Simulation::getClock() - decision_timestamp;
        nlohmann::json execute_json_data = decisions["data"];
        std::string job_reply_data = execute_json_data.dump();

        if (strcmp(decision_type.c_str(), "EXECUTE_JOB") == 0) {
          if (time_to_sleep > 0) {
            S4U_Simulation::sleep(time_to_sleep);
          }
          sendExecuteMessageToBatchService(answer_mailbox, job_reply_data);
        } else if (strcmp(decision_type.c_str(), "ANSWER") == 0) {
          double estimated_waiting_time = execute_json_data["estimate_waiting_time"]["estimated_waiting_time"];
          sendQueryAnswerMessageToBatchService(estimated_waiting_time);
        }
      }

      double decision_now = reply_decisions["now"];
      double time_to_sleep_again = S4U_Simulation::getClock() - decision_now;
      if (time_to_sleep_again > 0) {
        S4U_Simulation::sleep(time_to_sleep_again);
      }


    }
    #endif

}
