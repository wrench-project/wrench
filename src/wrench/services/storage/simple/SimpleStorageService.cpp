/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <climits>
#include <services/storage/storage_helpers/DataCommunicationThreadMessage.h>

#include "wrench/services/storage/simple/SimpleStorageService.h"
#include "wrench/services/ServiceMessage.h"
#include "services/storage/StorageServiceMessage.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
#include "wrench/workflow/WorkflowFile.h"
#include "wrench/exceptions/WorkflowExecutionException.h"
#include "wrench/services/storage/simple/NetworkConnectionManager.h"
#include "wrench/simulation/SimulationTimestampTypes.h"


WRENCH_LOG_NEW_DEFAULT_CATEGORY(simple_storage_service, "Log category for Simple Storage Service");


namespace wrench {

    /**
    * @brief Generate a unique number
    *
    * @return a unique number
    */
    unsigned long SimpleStorageService::getNewUniqueNumber() {
        static unsigned long sequence_number = 0;
        return (sequence_number++);
    }

    /**
     * @brief Destructor
     */
    SimpleStorageService::~SimpleStorageService() {
        this->default_property_values.clear();
    }

    void SimpleStorageService::cleanup(bool has_returned_from_main, int return_value) {
        this->pending_data_communications.clear();
        this->running_data_communications.clear();
        // Do nothing. It's fine to die and we'll just autorestart with our previous state
    }

    /**
     * @brief Public constructor
     *
     * @param hostname: the name of the host on which to start the service
     * @param capacity: the storage capacity in bytes
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     */
    SimpleStorageService::SimpleStorageService(std::string hostname,
                                               double capacity,
                                               std::map<std::string, std::string> property_list,
                                               std::map<std::string, double> messagepayload_list
    ) :
            SimpleStorageService(std::move(hostname), capacity, property_list, messagepayload_list,
                                 "_" + std::to_string(getNewUniqueNumber())) {
        if (this->getPropertyValueAsString("MAX_NUM_CONCURRENT_DATA_CONNECTIONS") == "infinity") {
            this->num_concurrent_connections = ULONG_MAX;
        } else {
            this->num_concurrent_connections = (unsigned long) (this->getPropertyValueAsDouble(
                    "MAX_NUM_CONCURRENT_DATA_CONNECTIONS"));
        }


    }

    /**
     * @brief Private constructor
     *
     * @param hostname: the name of the host on which to start the service
     * @param capacity: the storage capacity in bytes
     * @param property_list: the property list
     * @param suffix: the suffix (for the service name)
     *
     * @throw std::invalid_argument
     */
    SimpleStorageService::SimpleStorageService(
            std::string hostname,
            double capacity,
            std::map<std::string, std::string> property_list,
            std::map<std::string, double> messagepayload_list,
            std::string suffix) :
            StorageService(std::move(hostname), "simple_storage" + suffix, "simple_storage" + suffix, capacity) {

        this->setProperties(this->default_property_values, property_list);
        this->setMessagePayloads(this->default_messagepayload_values, messagepayload_list);
    }

    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int SimpleStorageService::main() {

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_CYAN);

//        // Create a new network connection manager object
//        this->network_connection_manager = std::unique_ptr<NetworkConnectionManager>(
//                new NetworkConnectionManager(this->num_concurrent_connections));

        // number of files staged
        unsigned long num_stored_files = 0;

        for (auto partition : this->stored_files) {
            num_stored_files += partition.second.size();
        }

        WRENCH_INFO("Simple Storage Service %s starting on host %s (capacity: %lf, holding %ld files, listening on %s)",
                    this->getName().c_str(),
                    S4U_Simulation::getHostName().c_str(),
                    this->capacity,
                    num_stored_files,
                    this->mailbox_name.c_str());

        /** Main loop **/
        while (this->processNextMessage()) {
            this->startPendingDataCommunications();
        }

        WRENCH_INFO("Simple Storage Service %s on host %s cleanly terminating!",
                    this->getName().c_str(),
                    S4U_Simulation::getHostName().c_str());

        return 0;
    }


    /**
     * @brief Process a received control message
     *
     * @param comm: the pending communication
     * @return false if the daemon should terminate
     */
    bool SimpleStorageService::processNextMessage() {

        S4U_Simulation::computeZeroFlop();

        // Wait for a message
        std::shared_ptr<SimulationMessage> message = nullptr;

        try {
            message = S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<NetworkError> &cause) {
            WRENCH_INFO("Got a network error while getting some message... ignoring");
            return true; // oh well
        }

        WRENCH_INFO("Got a [%s] message", message->getName().c_str());

        if (auto msg = std::dynamic_pointer_cast<ServiceStopDaemonMessage>(message)) {
            try {
                S4U_Mailbox::putMessage(msg->ack_mailbox,
                                        new ServiceDaemonStoppedMessage(this->getMessagePayloadValue(
                                                SimpleStorageServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
            } catch (std::shared_ptr<NetworkError> &cause) {
                return false;
            }
            return false;

        } else if (auto msg = std::dynamic_pointer_cast<StorageServiceFreeSpaceRequestMessage>(message)) {
            double free_space = this->capacity - this->occupied_space;

            S4U_Mailbox::dputMessage(msg->answer_mailbox,
                                     new StorageServiceFreeSpaceAnswerMessage(free_space,
                                                                              this->getMessagePayloadValue(
                                                                                      SimpleStorageServiceMessagePayload::FREE_SPACE_ANSWER_MESSAGE_PAYLOAD)));
            return true;

        } else if (auto msg = std::dynamic_pointer_cast<StorageServiceFileDeleteRequestMessage>(message)) {

            bool success = true;
            std::shared_ptr<FailureCause> failure_cause = nullptr;
            if (this->stored_files.find(msg->dst_partition) != this->stored_files.end()) {
                std::set<WorkflowFile *> files = this->stored_files[msg->dst_partition];
                if (files.find(msg->file) == files.end()) {
                    success = false;
                    failure_cause = std::shared_ptr<FailureCause>(
                            new FileNotFound(msg->file, this->getSharedPtr<SimpleStorageService>()));
                } else {
                    this->removeFileFromStorage(msg->file, msg->dst_partition);
                }
            } else {
                success = false;
                failure_cause = std::shared_ptr<FailureCause>(
                        new FileNotFound(msg->file, this->getSharedPtr<SimpleStorageService>()));
            }


            S4U_Mailbox::dputMessage(msg->answer_mailbox,
                                     new StorageServiceFileDeleteAnswerMessage(msg->file,
                                                                               this->getSharedPtr<SimpleStorageService>(),
                                                                               success,
                                                                               failure_cause,
                                                                               this->getMessagePayloadValue(
                                                                                       SimpleStorageServiceMessagePayload::FILE_DELETE_ANSWER_MESSAGE_PAYLOAD)));
            return true;

        } else if (auto msg = std::dynamic_pointer_cast<StorageServiceFileLookupRequestMessage>(message)) {

            std::set<WorkflowFile *> files = this->stored_files[msg->dst_partition];

            bool file_found = (files.find(msg->file) != files.end());
            S4U_Mailbox::dputMessage(msg->answer_mailbox,
                                     new StorageServiceFileLookupAnswerMessage(msg->file, file_found,
                                                                               this->getMessagePayloadValue(
                                                                                       SimpleStorageServiceMessagePayload::FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD)));
            return true;

        } else if (auto msg = std::dynamic_pointer_cast<StorageServiceFileWriteRequestMessage>(message)) {

            return processFileWriteRequest(msg->file, msg->dst_partition, msg->answer_mailbox);

        } else if (auto msg = std::dynamic_pointer_cast<StorageServiceFileReadRequestMessage>(message)) {

            return processFileReadRequest(msg->file, msg->src_partition, msg->answer_mailbox,
                                          msg->mailbox_to_receive_the_file_content);

        } else if (auto msg = std::dynamic_pointer_cast<StorageServiceFileCopyRequestMessage>(message)) {

            return processFileCopyRequest(msg->file, msg->src, msg->src_partition, msg->dst_partition,
                                          msg->answer_mailbox, msg->start_timestamp);

        } else if (auto msg = std::dynamic_pointer_cast<DataCommunicationThreadNotificationMessage>(message)) {
            return processDataCommunicationThreadNotification(
                    msg->data_communication_thread,
                    msg->file,
                    msg->communication_type,
                    msg->partition,
                    msg->success,
                    msg->failure_cause,
                    msg->answer_mailbox_if_copy,
                    msg->start_time_stamp
            );
        } else {
            throw std::runtime_error("SimpleStorageService::processControlMessage(): Unexpected [" + message->getName() + "] message");
        }
    }

    /**
     * @brief Handle a file write request
     *
     * @param file: the file to write
     * @param dst_partition: the file partition to write the file to
     * @param answer_mailbox: the mailbox to which the reply should be sent
     * @return true if this process should keep running
     */
    bool SimpleStorageService::processFileWriteRequest(WorkflowFile *file, std::string dst_partition,
                                                       std::string answer_mailbox) {

        // If the file is not already there, do a capacity check/update
        // (If the file is already there, then there will just be an overwrite. Note that
        // if the overwrite fails, then the file will disappear, which is expected)
        if (this->stored_files.find(file->getID()) == this->stored_files.end()) {

            // Check the file size and capacity, and reply "no" if not enough space
            if (file->getSize() > (this->capacity - this->occupied_space)) {
                try {
                    S4U_Mailbox::putMessage(answer_mailbox,
                                            new StorageServiceFileWriteAnswerMessage(file,
                                                                                     this->getSharedPtr<SimpleStorageService>(),
                                                                                     false,
                                                                                     std::shared_ptr<FailureCause>(
                                                                                             new StorageServiceNotEnoughSpace(
                                                                                                     file,
                                                                                                     this->getSharedPtr<SimpleStorageService>())),
                                                                                     "none",
                                                                                     this->getMessagePayloadValue(
                                                                                             SimpleStorageServiceMessagePayload::FILE_WRITE_ANSWER_MESSAGE_PAYLOAD)));
                } catch (std::shared_ptr<NetworkError> &cause) {
                    return true;
                }
                return true;
            }
            // Update occupied space, in advance (will have to be decreased later in case of failure)
            this->occupied_space += file->getSize();
        }

        // Generate a mailbox_name name on which to receive the file
        std::string file_reception_mailbox = S4U_Mailbox::generateUniqueMailboxName("file_reception");

        // Reply with a "go ahead, send me the file" message
        S4U_Mailbox::dputMessage(answer_mailbox,
                                 new StorageServiceFileWriteAnswerMessage(file,
                                                                          this->getSharedPtr<SimpleStorageService>(),
                                                                          true,
                                                                          nullptr,
                                                                          file_reception_mailbox,
                                                                          this->getMessagePayloadValue(
                                                                                  SimpleStorageServiceMessagePayload::FILE_WRITE_ANSWER_MESSAGE_PAYLOAD)));
        // Create a DataCommunicationThread
        auto dct = std::shared_ptr<DataCommunicationThread>(
                new DataCommunicationThread(this->hostname, file, dst_partition,
                                            DataCommunicationThread::DataCommunicationType::RECEIVING,
                                            file_reception_mailbox, "", this->mailbox_name));
        dct->simulation = this->simulation;

        // Add it to the Pool of pending data communications
        this->pending_data_communications.push_back(dct);

        return true;
    }


    /**
     * @brief Handle a file read request
     * @param file: the file
     * @param src_partition: the file partition to read the file from
     * @param answer_mailbox: the mailbox to which the answer should be sent
     * @param mailbox_to_receive_the_file_content: the mailbox to which the file will be sent
     * @return
     */
    bool SimpleStorageService::processFileReadRequest(WorkflowFile *file, std::string src_partition,
                                                      std::string answer_mailbox,
                                                      std::string mailbox_to_receive_the_file_content) {



        // Figure out whether this succeeds or not
        bool success = true;
        std::shared_ptr<FailureCause> failure_cause = nullptr;
        if (this->stored_files.find(src_partition) != this->stored_files.end()) {
            std::set<WorkflowFile *> files = this->stored_files[src_partition];
            if (files.find(file) == files.end()) {
                WRENCH_INFO("Received a a read request for a file I don't have (%s)", this->getName().c_str());
                success = false;
                failure_cause = std::shared_ptr<FailureCause>(
                        new FileNotFound(file, this->getSharedPtr<SimpleStorageService>()));
            }
        } else {
            success = false;
            failure_cause = std::shared_ptr<FailureCause>(
                    new FileNotFound(file, this->getSharedPtr<SimpleStorageService>()));
        }

        // Send back the corresponding ack, asynchronously and in a "fire and forget" fashion
        S4U_Mailbox::dputMessage(answer_mailbox,
                                 new StorageServiceFileReadAnswerMessage(file,
                                                                         this->getSharedPtr<SimpleStorageService>(),
                                                                         success, failure_cause,
                                                                         this->getMessagePayloadValue(
                                                                                 SimpleStorageServiceMessagePayload::FILE_READ_ANSWER_MESSAGE_PAYLOAD)));
        // If success, then follow up with sending the file (ASYNCHRONOUSLY!)
        if (success) {
            // Create a DataCommunicationThread
            auto dct = std::shared_ptr<DataCommunicationThread>(
                    new DataCommunicationThread(this->hostname, file, src_partition,
                                                DataCommunicationThread::DataCommunicationType::SENDING,
                                                mailbox_to_receive_the_file_content,  "", this->mailbox_name));
            dct->simulation = this->simulation;

            // Add it to the Pool of pending data communications
            this->pending_data_communications.push_front(dct);
        }

        return true;
    }

    /**
     * @brief Handle a file copy request
     * @param file: the file
     * @param src: the storage service that holds the file
     * @param src_partition: the file partition from where the file will be copied
     * @param dst_partition: the file partition to where the file will be copied
     * @param answer_mailbox: the mailbox to which the answer should be sent
     * @return
     */
    bool
    SimpleStorageService::processFileCopyRequest(WorkflowFile *file, std::shared_ptr<StorageService> src,
                                                 std::string src_partition, std::string dst_partition,
                                                 std::string answer_mailbox,
                                                 SimulationTimestampFileCopyStart *start_timestamp) {

        // Do a capacity check/update
        // If the file is already there, then there will just be an overwrite. Note that
        // if the overwrite fails, then the file will disappear, just like in the real world.
        if (file->getSize() > this->capacity - this->occupied_space) {
            WRENCH_INFO("Cannot perform file copy due to lack of space");

            this->simulation->getOutput().addTimestamp<SimulationTimestampFileCopyFailure>(
                    new SimulationTimestampFileCopyFailure(start_timestamp));

            try {
                S4U_Mailbox::putMessage(answer_mailbox,
                                        new StorageServiceFileCopyAnswerMessage(file,
                                                                                this->getSharedPtr<StorageService>(),
                                                                                dst_partition, nullptr, false,
                                                                                false,
                                                                                std::shared_ptr<FailureCause>(
                                                                                        new StorageServiceNotEnoughSpace(
                                                                                                file,
                                                                                                this->getSharedPtr<SimpleStorageService>())),
                                                                                this->getMessagePayloadValue(
                                                                                        SimpleStorageServiceMessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD)));


            } catch (std::shared_ptr<NetworkError> &cause) {
                return true;
            }
            return true;
        }
        this->occupied_space += file->getSize();

        WRENCH_INFO("Asynchronously copying file %s from storage service %s",
                    file->getID().c_str(),
                    src->getName().c_str());

        // Create a unique mailbox_name on which to receive the file
        std::string file_reception_mailbox = S4U_Mailbox::generateUniqueMailboxName("file_reception");

        // Initiate an ASYNCHRONOUS file read from the source
        try {
            if (src.get() == this) {
                //if the src and the destination are the same in a copy, then we try to simulate a
                // particular time based on the SELF_CONNECTION_DELAY property value
                // TODO: When S4U has a disk thingy, we'll have to revise this to simulate I/O normally
                wrench::S4U_Simulation::sleep(
                        this->getPropertyValueAsDouble(SimpleStorageServiceProperty::SELF_CONNECTION_DELAY));
                //Also, we don't have to simulate the read, because right now it just takes 0 time to read
                //But, below we send/receive INCOMING DATA/OUTGOING DATA, and update my stored files map

            } else {
                src->initiateFileRead(file_reception_mailbox, file, src_partition);
            }
        } catch (WorkflowExecutionException &e) {
            try {
                S4U_Mailbox::putMessage(answer_mailbox,
                                        new StorageServiceFileCopyAnswerMessage(file,
                                                                                this->getSharedPtr<StorageService>(),
                                                                                dst_partition, nullptr, false,
                                                                                false, e.getCause(),
                                                                                this->getMessagePayloadValue(
                                                                                        SimpleStorageServiceMessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD)));

                this->simulation->getOutput().addTimestamp<SimulationTimestampFileCopyFailure>(
                        new SimulationTimestampFileCopyFailure(start_timestamp));

            } catch (std::shared_ptr<NetworkError> &cause) {
                return true;
            }
            return true;
        }

        if (src.get() == this) {
            // create a (bogus) communication thread since technically we are sending data (to oneself)
            auto dct = std::shared_ptr<DataCommunicationThread>(
                    new DataCommunicationThread(this->hostname, file, src_partition,
                                                DataCommunicationThread::DataCommunicationType::SENDING,
                                                file_reception_mailbox,  "", this->mailbox_name));
            dct->simulation = this->simulation;

            // Add it to the Pool of pending data communications
            this->pending_data_communications.push_front(dct);
        }

        auto dct = std::shared_ptr<DataCommunicationThread>(
                new DataCommunicationThread(this->hostname, file, dst_partition,
                                            DataCommunicationThread::DataCommunicationType::RECEIVING,
                                            file_reception_mailbox,  answer_mailbox, this->mailbox_name, start_timestamp));
        dct->simulation = this->simulation;

        // Add it to the Pool of pending data communications
        this->pending_data_communications.push_front(dct);

        return true;
    }

    /**
     * @brief Start pending communications if any and if possible
     */
    void SimpleStorageService::startPendingDataCommunications() {
        while ((not this->pending_data_communications.empty()) and
               (this->running_data_communications.size() < this->num_concurrent_connections)) {
            // Start a communications!
            auto dct = this->pending_data_communications.at(0);
            this->pending_data_communications.pop_front();
            this->running_data_communications.insert(dct);
            dct->start(dct, true, false); // Daemonize, non-auto-restart
        }
    }


    /**
     * @brief Process a notification received from a data communication thread
     * @param dct: the data communication thread
     * @param file: the file
     * @param communication_type: the communication type
     * @param partition: the partition
     * @param success: whether the communications succeeded or not
     * @param failure_cause: the failure cause (nullptr if success)
     * @param answer_mailbox_if_copy: the mailbox to send a copy notification ("" if not a copy)
     * @param start_timestamp: a start file copy time stamp
     * @return false if the daemon should terminate
     */
    bool SimpleStorageService::processDataCommunicationThreadNotification(std::shared_ptr<DataCommunicationThread> dct,
                                                                          WorkflowFile *file,
                                                                          DataCommunicationThread::DataCommunicationType communication_type,
                                                                          std::string partition, bool success,
                                                                          std::shared_ptr<FailureCause> failure_cause,
                                                                          std::string answer_mailbox_if_copy,
                                                                          SimulationTimestampFileCopyStart *start_timestamp) {

        // Remove the dct from the list of running dct
        if (this->running_data_communications.find(dct) == this->running_data_communications.end()) {
            WRENCH_INFO("Got a notification from a non-existing Data Communication Thread. Perhaps this is from a former life... ignoring");
        }
        this->running_data_communications.erase(dct);

        WRENCH_INFO("SUCCESS = %d", success);

        if (communication_type == DataCommunicationThread::DataCommunicationType::RECEIVING) {
            if (success) {
                //Add the file to my storage (this will not add a duplicate in case of an overwrite, because it's a set)
                this->stored_files[partition].insert(file);
                // Deal with time stamps
                if (start_timestamp != nullptr) {
                    this->simulation->getOutput().addTimestamp<SimulationTimestampFileCopyCompletion>(
                            new SimulationTimestampFileCopyCompletion(start_timestamp));
                }
            } else {
                // Process the failure, meaning, just re-decrease the occupied space
                this->occupied_space -= file->getSize();
                // And if this was an overwrite, now we lost the file!!!
                this->stored_files[partition].erase(file);
            }

            // Send back the corresponding ack if this was a copy
            if (not answer_mailbox_if_copy.empty()) {
                WRENCH_INFO(
                        "Sending back an ack since this was a file copy and some client is waiting for me to say something");
                S4U_Mailbox::dputMessage(answer_mailbox_if_copy,
                                         new StorageServiceFileCopyAnswerMessage(file,
                                                                                 this->getSharedPtr<SimpleStorageService>(),
                                                                                 partition,
                                                                                 nullptr,
                                                                                 false,
                                                                                 success,
                                                                                 failure_cause,
                                                                                 this->getMessagePayloadValue(
                                                                                         SimpleStorageServiceMessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD)));
            }

        } else if (communication_type == DataCommunicationThread::DataCommunicationType::SENDING) {

            // Nothing to do
        }

        return true;
    }



//    bool SimpleStorageService::processIncomingDataConnection(std::unique_ptr<NetworkConnection> connection) {
//
//        // Get the message
//        std::shared_ptr<SimulationMessage> message = connection->getMessage();
//
//        if (message == nullptr) {
//            WRENCH_INFO("SimpleStorageService::processDataConnection(): Communication failure when receiving file '%s",
//                        connection->file->getID().c_str());
//            // Process the failure, meaning, just re-decrease the occupied space
//            this->occupied_space -= connection->file->getSize();
//            // And if this was an overwrite, now we lost the file!!!
//            this->stored_files[connection->file_partition].erase(connection->file);
//
//            // If there is a start timestamp for the connection, then this was a file copy... so we have to
//            // record an end timestamp for the filure. Otherwise, it's a file read or write, and this is
//            // considered part of a task execution, so there will just be a task failure timestamp
//            if (connection->start_timestamp) {
//                this->simulation->getOutput().addTimestamp<SimulationTimestampFileCopyFailure>(
//                        new SimulationTimestampFileCopyFailure(connection->start_timestamp));
//            }
//
//            S4U_Mailbox::dputMessage(connection->ack_mailbox,
//                                     new StorageServiceFileCopyAnswerMessage(connection->file,
//                                                                             this->getSharedPtr<SimpleStorageService>(),
//                                                                             connection->file_partition, nullptr,
//                                                                             false,
//                                                                             false, connection->failure_cause,
//                                                                             this->getMessagePayloadValue(
//                                                                                     SimpleStorageServiceMessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD)));
//            return true;
//        }
//
//        WRENCH_DEBUG("Got a [%s] message", message->getName().c_str());
//
//        if (auto msg = std::dynamic_pointer_cast<StorageServiceFileContentMessage>(message)) {
//
//            if (msg->file != connection->file) {
//                throw std::runtime_error(
//                        "SimpleStorageService::processDataConnection(): Mismatch between received file and expected file... a bug in SimpleStorageService");
//            }
//
//            // Add the file to my storage (this will not add a duplicate in case of an overwrite, because it's a set)
//            this->stored_files[connection->file_partition].insert(connection->file);
//            if (connection->start_timestamp != nullptr) {
//                this->simulation->getOutput().addTimestamp<SimulationTimestampFileCopyCompletion>(
//                        new SimulationTimestampFileCopyCompletion(
//                                connection->start_timestamp
//                        ));
//
//            }
//
//            // Send back the corresponding ack?
//            if (not connection->ack_mailbox.empty()) {
//                WRENCH_INFO(
//                        "Sending back an ack since this was a file copy and some client is waiting for me to say something");
//                S4U_Mailbox::dputMessage(connection->ack_mailbox,
//                                         new StorageServiceFileCopyAnswerMessage(connection->file,
//                                                                                 this->getSharedPtr<SimpleStorageService>(),
//                                                                                 connection->file_partition,
//                                                                                 nullptr,
//                                                                                 false,
//                                                                                 true,
//                                                                                 nullptr,
//                                                                                 this->getMessagePayloadValue(
//                                                                                         SimpleStorageServiceMessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD)));
//
//            }
//            return true;
//
//        } else {
//            throw std::runtime_error(
//                    "SimpleStorageService::processControlMessage(): Unexpected [" + message->getName() + "] message");
//        }
//
//    }
//
//    bool SimpleStorageService::processOutgoingDataConnection(std::unique_ptr<NetworkConnection> connection) {
//        // Nothing to do
//        return true;
//    }

};
