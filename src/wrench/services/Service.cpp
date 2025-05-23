/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <wrench/simulation/SimulationMessage.h>
#include <wrench/services/ServiceMessage.h>
#include <wrench/simgrid_S4U_util/S4U_Daemon.h>
#include <wrench/services/Service.h>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/exceptions/ExecutionException.h>
#include <wrench/services/ServiceMessagePayload.h>
#include <wrench/failure_causes/ServiceIsDown.h>
#include <wrench/failure_causes/ServiceIsSuspended.h>
#include <wrench/failure_causes/NotAllowed.h>
#include <wrench/simgrid_S4U_util/S4U_VirtualMachine.h>
#include <wrench/util/UnitParser.h>

#include <memory>

WRENCH_LOG_CATEGORY(wrench_core_service, "Log category for Service");
namespace std {
    inline std::string to_string(std::string a) { return a; }
}// namespace std

namespace wrench {

    std::set<std::shared_ptr<Service>> Service::servicesSetToAutoRestart;

    /**
     * @brief Destructor
     */
    Service::~Service() {
        //        std::cerr << "IN SERVICE DESTRUCTOR: " << this->getName() << "\n";
        //        WRENCH_INFO("IN SERVICE DESTRUCTOR: %s", this->getName().c_str());
    }

    /**
     * @brief Constructor
     * @param hostname: the name of the host on which the service will run
     * @param process_name_prefix: the prefix for the process name
     */
    Service::Service(const std::string &hostname, const std::string &process_name_prefix) : S4U_Daemon(hostname, process_name_prefix) {
        this->name = process_name_prefix;
    }

    /**
      * @brief Set a property of the Service
      * @param property: the property
      * @param value: the property value
      */
    void Service::setProperty(WRENCH_PROPERTY_TYPE property, const std::string &value) {
        if (this->property_list.find(property) != this->property_list.end()) {
            this->property_list[property] = value;
        } else {
            this->property_list.insert(std::make_pair(property, value));
        }
    }


    /**
    * @brief Set a message payload of the Service
    * @param messagepayload: the message payload (which must be a string representation of a >=0 sg_size_t)
    * @param value: the message payload value
    */
    void Service::setMessagePayload(WRENCH_MESSAGEPAYLOAD_TYPE messagepayload, sg_size_t value) {
        this->messagepayload_list[messagepayload] = value;
    }

    /**
     * @brief Get a property of the Service as a string
     * @param property: the property
     * @return the property value as a string
     *
     */
    std::string Service::getPropertyValueAsString(WRENCH_PROPERTY_TYPE property) {
        if (this->property_list.find(property) == this->property_list.end()) {
            throw std::invalid_argument(
                    "Service::getPropertyValueAsString(): Cannot find value for property " + ServiceProperty::translatePropertyType(property) +
                    " (perhaps a derived service class does not provide a default value?)");
        }
        return this->property_list[property];
    }

    /**
     * @brief Get a property of the Service as a double
     * @param property: the property
     * @return the property value as a double
     *
     */
    double Service::getPropertyValueAsDouble(WRENCH_PROPERTY_TYPE property) {
        double value;
        std::string string_value;
        string_value = this->getPropertyValueAsString(property);

        if (string_value == "infinity") {
            return DBL_MAX;
        }
        if (string_value == "zero") {
            return 0;
        }
        if (sscanf(string_value.c_str(), "%lf", &value) != 1) {
            throw std::invalid_argument(
                    "Service::getPropertyValueAsDouble(): Invalid double property value " + ServiceProperty::translatePropertyType(property) + " " +
                    this->getPropertyValueAsString(property));
        }
        return value;
    }

    /**
     * @brief A helper method to parse a property value with units
     * @param property: the property
     * @param unit_parsing_function: the unit parsing function
     * @return the property value as a double, in the basic unit
     */
    double Service::getPropertyValueWithUnitsAsValue(WRENCH_PROPERTY_TYPE property,
                                                     const std::function<double(std::string &s)> &unit_parsing_function) {
        std::string string_value;
        string_value = this->getPropertyValueAsString(property);

        if (string_value == "infinity") {
            return DBL_MAX;
        }
        if (string_value == "zero") {
            return 0;
        }
        return unit_parsing_function(string_value);
    }

    /**
     * @brief Method to parse a property value that is a time with (optional) units
     * @param property: the property
     * @return the time in second
     */
    double Service::getPropertyValueAsTimeInSecond(WRENCH_PROPERTY_TYPE property) {
        return this->getPropertyValueWithUnitsAsValue(property, UnitParser::parse_time);
    }

    /**
     * @brief Method to parse a property value that is a date size with (optional) units
     * @param property: the property
     * @return the size in byte
     */
    sg_size_t Service::getPropertyValueAsSizeInByte(WRENCH_PROPERTY_TYPE property) {
        sg_size_t value;
        std::string string_value;
        string_value = this->getPropertyValueAsString(property);
        if (string_value == "infinity") {
            return LLONG_MAX;
        }
        if (string_value == "zero") {
            return 0;
        }
        value = static_cast<sg_size_t>(this->getPropertyValueWithUnitsAsValue(property, UnitParser::parse_size));

        return value;
    }

    /**
     * @brief Method to parse a property value that is a bandwidth with (optional) units
     * @param property: the property
     * @return the bandwidth in byte/sec
     */
    double Service::getPropertyValueAsBandwidthInBytePerSecond(WRENCH_PROPERTY_TYPE property) {
        return this->getPropertyValueWithUnitsAsValue(property, UnitParser::parse_bandwidth);
    }

    /**
    * @brief Get a property of the Service as an unsigned long
    * @param property: the property
    * @return the property value as an unsigned long
    *
    */
    unsigned long Service::getPropertyValueAsUnsignedLong(WRENCH_PROPERTY_TYPE property) {
        unsigned long value;
        std::string string_value;
        string_value = this->getPropertyValueAsString(property);
        if (string_value == "infinity") {
            return ULONG_MAX;
        }
        if (string_value == "zero") {
            return 0;
        }
        if (sscanf(string_value.c_str(), "%lu", &value) != 1) {
            throw std::invalid_argument(
                    "Service::getPropertyValueAsUnsignedLong(): Invalid unsigned long property value " + ServiceProperty::translatePropertyType(property) +
                    " " +
                    this->getPropertyValueAsString(property));
        }
        return value;
    }

    /**
     * @brief Get a message payload of the Service as a double
     * @param message_payload: the message payload
     * @return the message payload value as a double
     *
     */
    sg_size_t Service::getMessagePayloadValue(WRENCH_MESSAGEPAYLOAD_TYPE message_payload) {
        if (this->messagepayload_list.find(message_payload) == this->messagepayload_list.end()) {
            try {
                throw std::invalid_argument(
                        "Service::getMessagePayloadValue(): Cannot find value for message_payload " +
                        ServiceMessagePayload::translatePayloadType(message_payload) +
                        " (perhaps a derived service class does not provide a default value?)");
            } catch (std::out_of_range &e) {
                throw std::invalid_argument(
                        "Service::getMessagePayloadValue(): invalid message_payload index");
            }
        }
        return this->messagepayload_list[message_payload];
    }

    /**
     * @brief Get all message payloads and their values of the Service
     * @return the message payload map
     */
    const WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE &Service::getMessagePayloadList() const {
        return this->messagepayload_list;
    }

    /**
     * @brief Get all properties attached to this service
     * @return the property list
     */
    const WRENCH_PROPERTY_COLLECTION_TYPE &Service::getPropertyList() const {
        return this->property_list;
    }

    /**
     * @brief Get a property of the Service as a boolean
     * @param property: the property
     * @return the property value as a boolean
     *
     */
    bool Service::getPropertyValueAsBoolean(WRENCH_PROPERTY_TYPE property) {
        std::string string_value;
        string_value = this->getPropertyValueAsString(property);
        if (string_value == "true" or string_value == "True") {
            return true;
        } else if (string_value == "false" or string_value == "False") {
            return false;
        } else {
            throw std::invalid_argument(
                    "Service::getPropertyValueAsBoolean(): Invalid boolean property value " + ServiceProperty::translatePropertyType(property) + " " +
                    this->getPropertyValueAsString(property));
        }
    }

    /**
     * @brief Start the service
     * @param this_service: a shared pointer to the service
     * @param daemonize: true if the daemon is to be daemonized, false otherwise
     * @param auto_restart: true if the daemon should restart automatically after a reboot or not
     *
     */
    void Service::start(const std::shared_ptr<Service> &this_service, bool daemonize, bool auto_restart) {
        // Setting the state to UP
        this->state = Service::UP;

        // Creating the life saver so that the actor will never see the
        // Service object deleted from under its feet
        this->createLifeSaver(this_service);

        //            // Keep track of the master share_ptr reference to this service
        //            Service::service_shared_ptr_map[this] = this_service;

        // Start the daemon for the service
        this->startDaemon(daemonize, auto_restart);

        if (auto_restart) {
            Service::servicesSetToAutoRestart.insert(this_service);
        }

        //            // Print some information a out the currently tracked daemons
        //            WRENCH_DEBUG("MAP SIZE = %ld    NUM_TERMINATED_SERVICES = %ld",
        //                         Service::service_shared_ptr_map.size(), Service::num_terminated_services);

        //            if ((Service::service_shared_ptr_map.size() > 5000) or
        //                (Service::num_terminated_services > Service::service_shared_ptr_map.size() / 2)) {
        //                Service::cleanupTrackedServices();
        //                Service::num_terminated_services = 0;
        //            }
    }


    /**
     * @brief Synchronously stop the service (does nothing if the service is already stopped)
     *
     */
    void Service::stop() {
        // Do nothing if the service is already down
        if ((this->state == Service::DOWN) or (this->shutting_down)) {
            return;
        }
        this->shutting_down = true;// This is to avoid another process calling stop() and being stuck

        WRENCH_DEBUG("Telling the daemon listening on (%s) to terminate", this->commport->get_cname());

        // Send a termination message to the daemon's commport - SYNCHRONOUSLY
        auto ack_commport = S4U_Daemon::getRunningActorRecvCommPort();
        std::unique_ptr<SimulationMessage> message = nullptr;
        try {
            this->commport->putMessage(new ServiceStopDaemonMessage(
                    ack_commport,
                    false,
                    ComputeService::TerminationCause::TERMINATION_NONE,
                    this->getMessagePayloadValue(
                            ServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD)));

            // Wait for the ack
            message = ack_commport->getMessage<ServiceDaemonStoppedMessage>(
                    this->network_timeout,
                    "Service::stop(): Received an");

        } catch (...) {// network error
            this->shutting_down = false;
            throw;
        }

        // Set the service state to down
        this->shutting_down = false;
        this->state = Service::DOWN;
    }

    /**
     * @brief Suspend the service
     */
    void Service::suspend() {
        if (this->state == Service::DOWN) {
            return;// ignore
        }
        if (this->state == Service::SUSPENDED) {
            return;// ignore
        }
        this->state = Service::SUSPENDED;
        this->suspendActor();
    }

    /**
      * @brief Resume the service
      *
      */
    void Service::resume() {
        if (this->state != Service::SUSPENDED) {
            std::string what = "Service cannot be resumed because it is not in the suspended state";
            throw ExecutionException(std::make_shared<NotAllowed>(
                    this->getSharedPtr<Service>(), what));
        }
        this->resumeActor();
        this->state = Service::UP;
    }

    /**
    * @brief Get the name of the host on which the service is / will be running
    * @return the hostname
    */
    std::string Service::getHostname() {
        return this->hostname;
    }

    /**
    * @brief Get the the host on which the service is / will be running
    * @return the hostname
    */
    simgrid::s4u::Host *Service::getHost() {
        return this->host;
    }

    /**
    * @brief Get the physical name of the host on which the service is / will be running
    * @return the physical hostname
    */
    std::string Service::getPhysicalHostname() {
        if (S4U_VirtualMachine::vm_to_pm_map.find(this->hostname) != S4U_VirtualMachine::vm_to_pm_map.end()) {
            return S4U_VirtualMachine::vm_to_pm_map[this->hostname];
        } else {
            return this->hostname;
        }
    }

    /**
    * @brief Returns true if the service is UP, false otherwise
    * @return true or false
    */
    bool Service::isUp() const {
        return (this->state == Service::UP);
    }

    /**
		 * @brief Set the state of the service to DOWN
		 */
    void Service::setStateToDown() {
        this->state = Service::DOWN;
    }

    /**
     * @brief Set default and user-defined properties
     * @param default_property_values: list of default properties
     * @param overridden_property_values: list of overridden properties (override the default)
     */
    void Service::setProperties(const WRENCH_PROPERTY_COLLECTION_TYPE &default_property_values,
                                const WRENCH_PROPERTY_COLLECTION_TYPE &overridden_property_values) {
        // Set default properties
        for (auto const &p: default_property_values) {
            this->setProperty(p.first, p.second);
        }

        // Set specified properties (possible overwriting default ones)
        for (auto const &p: overridden_property_values) {
            this->setProperty(p.first, p.second);
        }
    }

    /**
     * @brief Set default and user-defined message payloads
     * @param default_messagepayload_values: list of default message payloads
     * @param overridden_messagepayload_values: list of overridden message payloads (override the default)
     */
    void Service::setMessagePayloads(const WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE &default_messagepayload_values,
                                     const WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE &overridden_messagepayload_values) {
        // Set default messagepayloads
        for (auto const &p: default_messagepayload_values) {
            this->setMessagePayload(p.first, p.second);
        }

        // Set specified messagepayloads (possible overwriting default ones)
        for (auto const &p: overridden_messagepayload_values) {
            this->setMessagePayload(p.first, p.second);
        }
    }

    /**
     * @brief Check whether the service is properly configured and running
     *
     */
    void Service::serviceSanityCheck() {
        assertServiceIsUp();
    }

    /**
     * @brief Returns the service's network timeout value
     * @return a duration in seconds
     */
    double Service::getNetworkTimeoutValue() const {
        return this->network_timeout;
    }

    /**
     * @brief Sets the service's network timeout value
     * @param value: a duration in seconds (<0 means: never timeout)
     *
     */
    void Service::setNetworkTimeoutValue(double value) {
        this->network_timeout = value;
    }

    /**
     * @brief Throws an exception if the service is not up
     */
    void Service::assertServiceIsUp() {
        if (this->state == Service::DOWN) {
            throw ExecutionException(
                std::make_shared<ServiceIsDown>(this->getSharedPtr<Service>()));
        }
        if (this->state == Service::SUSPENDED) {
            throw ExecutionException(
                std::make_shared<ServiceIsSuspended>(this->getSharedPtr<Service>()));
        }
    }

}// namespace wrench
