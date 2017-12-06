//
// Created by Suraj Pandey on 10/17/17.
//

#include <wrench/logging/TerminalOutput.h>
#include <wrench/simgrid_S4U_util/S4U_Simulation.h>
#include <simulation/SimulationMessage.h>
#include <simgrid_S4U_util/S4U_Mailbox.h>
#include <services/ServiceMessage.h>
#include <wrench/services/compute/batch/BatchServiceMessage.h>
#include <wrench/services/compute/batch/BatchNetworkListener.h>
#include "wrench/services/compute/batch/BatchRequestReplyProcess.h"
#include "wrench/services/compute/batch/BatchServiceProperty.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(batch_request_reply_service, "Log category for Batch Request Reply Service");


namespace wrench{


    /**
     * @brief Constructor
     * @param hostname: the hostname on which to start the service
     * @param self_port the port to listen to messages from scheduler
     * @param sched_port the port to send messages to scheduler
     */
    BatchRequestReplyProcess::BatchRequestReplyProcess(std::string hostname,std::string self_port, std::string sched_port):
            BatchRequestReplyProcess(hostname,self_port,
                           sched_port,"") {
    }


    /**
    * @brief Constructor
    * @param hostname: the hostname on which to start the service
    * @param self_port the port to listen to messages from scheduler
    * @param sched_port the port to send messages to scheduler
    * @param suffix the suffix to append
    */

    BatchRequestReplyProcess::BatchRequestReplyProcess(
            std::string hostname,std::string self_port, std::string sched_port, std::string suffix="") :
            Service("batch_request_reply" + suffix, "batch_request_reply" + suffix) {

      // Start the daemon on the same host
      this->hostname = std::move(hostname);
      std::cout<<"Not thrown until here\n";
      //create a network listener here, because there will be only one network listener but many network senders
      std::unique_ptr<BatchNetworkListener> network_listener = std::unique_ptr<BatchNetworkListener>(new BatchNetworkListener(this->hostname, this->mailbox_name, self_port,
                                                                                                                              sched_port,BatchNetworkListener::NETWORK_LISTENER_TYPE::LISTENER,
                                                                                                                              ""));
      try {
        this->start(this->hostname);
      } catch (std::invalid_argument e) {
        throw e;
      }
    }

    int BatchRequestReplyProcess::main() {

      TerminalOutput::setThisProcessLoggingColor(WRENCH_LOGGING_COLOR_CYAN);

      WRENCH_INFO("Batch Request Reply Service starting on host %s!", S4U_Simulation::getHostName().c_str());


      /** Main loop **/
      while (processNextMessage()) {
      }

      WRENCH_INFO("Batch Request Reply Service on host %s terminated!", S4U_Simulation::getHostName().c_str());
      return 0;
    }

    bool BatchRequestReplyProcess::processNextMessage() {
      // Wait for a message
      std::unique_ptr<SimulationMessage> message = nullptr;

      try {
        message = S4U_Mailbox::getMessage(this->mailbox_name);
      } catch (std::shared_ptr<NetworkError> cause) {
        return true;
      } catch (std::shared_ptr<NetworkTimeout> cause) {
        return true;
      }

      if (message == nullptr) {
        WRENCH_INFO("Got a NULL message... Likely this means we're all done. Aborting!");
        return false;
      }

      WRENCH_INFO("Got a [%s] message", message->getName().c_str());

      if (ServiceStopDaemonMessage *msg = dynamic_cast<ServiceStopDaemonMessage *>(message.get())) {

        //TODO::probably I don't have any thing to terminate
        ///the networklisteners I create to listen to the ports will be killed or it doesn't matter???

        // This is Synchronous;
        try {
          S4U_Mailbox::putMessage(msg->ack_mailbox,
                                  new ServiceDaemonStoppedMessage(this->getPropertyValueAsDouble(
                                          //TODO::property value to stop this daemon is used same as that of BatchServiceProperty
                                          BatchServiceProperty::DAEMON_STOPPED_MESSAGE_PAYLOAD)));

        } catch (std::shared_ptr<NetworkError> cause) {
          return false;
        }
        return false;

      }else if (BatchSimulationBeginsToSchedulerMessage *msg = dynamic_cast<BatchSimulationBeginsToSchedulerMessage *>(message.get())) {
        std::unique_ptr<BatchNetworkListener> network_sender = std::unique_ptr<BatchNetworkListener>(new BatchNetworkListener(
                this->hostname, this->mailbox_name, this->self_port, this->sched_port,BatchNetworkListener::NETWORK_LISTENER_TYPE::LISTENER, msg->job_args_to_scheduler));

        return true;

      }else{
        throw std::runtime_error(
                "BatchRequestReplyProcess::BatchRequestReplyProcess():Unknown message type received"
        );
      }

    }

}