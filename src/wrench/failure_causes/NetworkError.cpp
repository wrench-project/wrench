/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/failure_causes/NetworkError.h>

#include <wrench/logging/TerminalOutput.h>
#include <wrench/data_file/DataFile.h>
#include <wrench/job/Job.h>
#include <wrench/services/compute/ComputeService.h>

WRENCH_LOG_CATEGORY(wrench_core_network_error, "Log category for NetworkError");

namespace wrench {


    /**
     * @brief Constructor
     *
     * @param operation_type: NetworkError:OperationType::SENDING or NetworkError::OperationType::RECEIVING or
     *        NetworkError::OperationType::UNKNOWN
     * @param error_type: the error type 
     * @param commport_name: the name of a commport_name (or "" if unknown)
     */
    NetworkError::NetworkError(NetworkError::OperationType operation_type,
                               NetworkError::ErrorType error_type,
                               const std::string &commport_name) {
        if (commport_name.empty()) {
            throw std::invalid_argument("NetworkError::NetworkError(): invalid arguments");
        }
        this->operation_type = operation_type;
        this->error_type = error_type;
        this->commport_name = commport_name;
    }

    /**
     * @brief Returns whether the network error occurred while receiving
     * @return true or false
     */
    bool NetworkError::whileReceiving() {
        return (this->operation_type == NetworkError::RECEIVING);
    }

    /**
     * @brief Returns whether the network error occurred while sending
     * @return true or false
     */
    bool NetworkError::whileSending() {
        return (this->operation_type == NetworkError::SENDING);
    }

    /**
     * @brief Returns whether the network error was a timeout
     * @return true or false
     */
    bool NetworkError::isTimeout() {
        return (this->error_type == NetworkError::TIMEOUT);
    }

    /**
     * @brief Returns the commport_name name on which the error occurred
     * @return the commport_name name
     */
    std::string NetworkError::getMailbox() {
        return this->commport_name;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string NetworkError::toString() {
        std::string operation;
        if (this->whileSending()) {
            operation = "sending to";
        } else {
            operation = "receiving from";
        }
        std::string error;
        if (this->isTimeout()) {
            error = "timeout";
        } else {
            error = "link failure, or communication peer died";
        }
        return "Network error (" + error + ") while " + operation + " commport_name " + this->commport_name;
    }


}// namespace wrench
