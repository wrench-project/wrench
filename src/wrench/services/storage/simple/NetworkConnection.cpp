/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <simgrid/s4u.hpp>
#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>
#include <wrench/workflow/execution_events/FailureCause.h>
#include <wrench-dev.h>
#include <services/storage/StorageServiceMessage.h>
#include "wrench/services/storage/simple/NetworkConnection.h"
//#include <xbt/ex.hpp>


XBT_LOG_NEW_DEFAULT_CATEGORY(simple_storage_service_data_connection, "Log category for Data Connection");

namespace wrench {


    /**
     * @brief Constructor
     * @param type: the type of connection
     *              - NetworkConnection::INCOMING_DATA
     *              - NetworkConnection::OUTGOING_DATA
     *              - NetworkConnection::INCOMING_CONTROL
     * @param file: the file (for DATA connection only)
     * @param file_partition: the file partition inside the storage service where the file will be stored to/read from
     * @param mailbox: the mailbox: the mailbox for this connection
     * @param ack_mailbox: the mailbox to which an ack should be sent when the connection completes/fails
     * @param start_timestamp: a pointer to a SimulationTimestampFileCopyStart if this connection is part of a file copy
     */
    NetworkConnection::NetworkConnection(int type, WorkflowFile *file, std::string file_partition, std::string mailbox, std::string ack_mailbox, SimulationTimestampFileCopyStart *start_timestamp) {
      this->type = type;
      this->file = file;
      this->mailbox = mailbox;
      this->ack_mailbox = ack_mailbox;
      this->file_partition = file_partition;
      this->start_timestamp = start_timestamp;

      if (this->mailbox.empty()) {
        throw std::invalid_argument("NetworkConnection::NetworkConnection(): empty mailbox_name");
      }
      if (type == NetworkConnection::INCOMING_DATA or this->type == NetworkConnection::OUTGOING_DATA) {
        if (this->file == nullptr) {
          throw std::invalid_argument("NetworkConnection::NetworkConnection(): file cannot be nullptr for data connections");
        }
      }
      if (this->type == NetworkConnection::OUTGOING_DATA or this->type == NetworkConnection::INCOMING_CONTROL) {
        if (not this->ack_mailbox.empty()) {
          throw std::invalid_argument("NetworkConnection::NetworkConnection(): ack_mailbox should not be set for outgoing data or incoming control connection");
        }
      }
      if (this->type == NetworkConnection::INCOMING_CONTROL) {
        if (this->file != nullptr) {
          throw std::invalid_argument("NetworkConnection::NetworkConnection(): file should be nullptr for incoming control connection");
        }
      }
    }

    /**
     * @brief star a network connection
     *
     * @return true if start was successful, false otherwise
     */
    bool NetworkConnection::start() {

      switch (this->type) {
        case NetworkConnection::INCOMING_DATA:
        WRENCH_INFO("Asynchronously receiving file %s...",
                    this->file->getID().c_str());
          try {
            this->comm = S4U_Mailbox::igetMessage(mailbox);
          } catch (std::shared_ptr<NetworkError> &cause) {
            WRENCH_INFO("NetworkConnection::start(): got a NetworkError... giving up");
            return false;
          }
          break;
        case NetworkConnection::OUTGOING_DATA:
        WRENCH_INFO("Asynchronously sending file %s to mailbox_name %s...",
                    this->file->getID().c_str(), this->mailbox.c_str() );
          try {
            this->comm = S4U_Mailbox::iputMessage(this->mailbox, new
                    StorageServiceFileContentMessage(this->file));
          } catch (std::shared_ptr<NetworkError> &cause) {
            WRENCH_INFO("NetworkConnection::start(): got a NetworkError... giving up");
            return false;
          }
          break;
        case NetworkConnection::INCOMING_CONTROL:
        WRENCH_DEBUG("Asynchronously receiving a control message...");
          try {
            this->comm = S4U_Mailbox::igetMessage(this->mailbox);
          } catch (std::shared_ptr<NetworkError> &cause) {
            WRENCH_INFO("NetworkConnection::start(): got a NetworkError... giving up");
            return false;
          }
          break;
        default:
          throw std::runtime_error("NetworkConnection::start(): Invalid connection type");
      }
      return true;
    }

    /**
     * @brief Returns true if the connection has failed, false otherwise
     * @return true or false
     */
    bool NetworkConnection::hasFailed() {
      try {
        this->comm->comm_ptr->test();
      } catch (simgrid::NetworkFailureException &e) {
        if (this->type == NetworkConnection::OUTGOING_DATA) {
          this->failure_cause = std::shared_ptr<NetworkError>(new NetworkError(NetworkError::SENDING, NetworkError::FAILURE, this->mailbox));
        } else {
          this->failure_cause = std::shared_ptr<NetworkError>(new NetworkError(NetworkError::RECEIVING, NetworkError::FAILURE, this->mailbox));
        }
        return true;
      } catch (simgrid::TimeoutError &e) {
        if (this->type == NetworkConnection::OUTGOING_DATA) {
          this->failure_cause = std::shared_ptr<NetworkError>(new NetworkError(NetworkError::SENDING, NetworkError::TIMEOUT, this->mailbox));
        } else {
          this->failure_cause = std::shared_ptr<NetworkError>(new NetworkError(NetworkError::RECEIVING, NetworkError::TIMEOUT, this->mailbox));
        }
        return true;
      } catch (std::exception &e) {
        // This is likely paranoid
        if (this->type == NetworkConnection::OUTGOING_DATA) {
          this->failure_cause = std::shared_ptr<NetworkError>(new NetworkError(NetworkError::SENDING, NetworkError::FAILURE, this->mailbox));
        } else {
          this->failure_cause = std::shared_ptr<NetworkError>(new NetworkError(NetworkError::RECEIVING, NetworkError::FAILURE, this->mailbox));
        }
        return true;
      }
      return false;
    }

    /**
     * @brief Retrieve the message for a communication
     * @return the message, or nullptr if the connection has failed
     */
    std::shared_ptr<SimulationMessage> NetworkConnection::getMessage() {

      WRENCH_DEBUG("Getting the message from connection");
      if (this->type == NetworkConnection::OUTGOING_DATA) {
        throw std::runtime_error("NetworkConnection::getMessage(): Cannot be called on an outgoing connection");
      }
      if (this->hasFailed()) {
        return nullptr;
      }
      std::shared_ptr<SimulationMessage> message;
      try {
        message = this->comm->wait();
      } catch (std::shared_ptr<NetworkError> &cause) {
        return nullptr;
      }
      return message;
    }

};