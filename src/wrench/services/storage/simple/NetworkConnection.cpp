/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>
#include <wrench/workflow/execution_events/FailureCause.h>
#include <wrench-dev.h>
#include <services/storage/StorageServiceMessage.h>
#include "wrench/services/storage/simple/NetworkConnection.h"
#include <xbt/ex.hpp>


XBT_LOG_NEW_DEFAULT_CATEGORY(simple_storage_service_data_connection, "Log category for Data Connection");

namespace wrench {

    constexpr unsigned char NetworkConnection::INCOMING_DATA;
    constexpr unsigned char NetworkConnection::OUTGOING_DATA;
    constexpr unsigned char NetworkConnection::INCOMING_CONTROL;

    NetworkConnection::NetworkConnection(int type, WorkflowFile *file, std::string mailbox, std::string ack_mailbox) {
      this->type = type;
      this->file = file;
      this->mailbox = mailbox;
      this->ack_mailbox = ack_mailbox;

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
                    this->file->getId().c_str());
          try {
            this->comm = S4U_Mailbox::igetMessage(mailbox);
          } catch (std::shared_ptr<NetworkError> &cause) {
            WRENCH_INFO("NetworkConnection::start(): got a NetworkError... giving up");
            return false;
          }
          break;
        case NetworkConnection::OUTGOING_DATA:
        WRENCH_INFO("Asynchronously sending file %s to mailbox_name %s...",
                    this->file->getId().c_str(), this->mailbox.c_str() );
          try {
            this->comm = S4U_Mailbox::iputMessage(this->mailbox, new
                    StorageServiceFileContentMessage(this->file));
          } catch (std::shared_ptr<NetworkError> &cause) {
            WRENCH_INFO("NetworkConnection::start(): got a NetworkError... giving up");
            return false;
          }
          break;
        case NetworkConnection::INCOMING_CONTROL:
        WRENCH_INFO("Asynchronously receiving a control message...");
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
     * @brief Determine whether the connection has failed
     * @return true if failed
     */
    bool NetworkConnection::hasFailed() {
      try {
        this->comm->comm_ptr->test();
      } catch (xbt_ex &e) {
        if (e.category == network_error) {
          if (this->type == NetworkConnection::OUTGOING_DATA) {
            this->failure_cause = std::shared_ptr<FailureCause>(new NetworkError(NetworkError::SENDING, this->mailbox));
          } else {
            this->failure_cause = std::shared_ptr<FailureCause>(new NetworkError(NetworkError::RECEIVING, this->mailbox));
          }
          return true;
        }
      }
      return false;
    }

    /**
     * @brief Retrieve the message for a communication
     * @return  the message, or nullptr if the connection has failed
     */
    std::unique_ptr<SimulationMessage> NetworkConnection::getMessage() {
      if (this->type == NetworkConnection::OUTGOING_DATA) {
        throw std::runtime_error("NetworkConnection::getMessage(): Cannot be called on an outgoing connection");
      }
      if (this->hasFailed()) {
        return nullptr;
      }
      std::unique_ptr<SimulationMessage> message;
      try {
        message = this->comm->wait();
      } catch (std::shared_ptr<NetworkError> &cause) {
        return nullptr;
      }
      return std::move(message);
    }

};