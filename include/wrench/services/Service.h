/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_SERVICE_H
#define WRENCH_SERVICE_H


#include <string>
#include <map>
#include <memory>

#include "wrench/simgrid_S4U_util/S4U_Daemon.h"
#include "wrench/services/ServiceProperty.h"
#include "wrench/services/ServiceMessagePayload.h"
namespace wrench {
    /**
     * @brief Abstraction of a service property collection type
     */
    typedef std::map<WRENCH_PROPERTY_TYPE, std::string> WRENCH_PROPERTY_COLLECTION_TYPE;
    /**
     * @brief Abstraction of a service message payload collection type
     */
    typedef std::unordered_map<WRENCH_MESSAGEPAYLOAD_TYPE, double> WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE;


    class FailureCause;

    /**
     * @brief A service that can be added to the simulation and that can be used by a WMS
     *        when executing a workflow
     */
    class Service : public S4U_Daemon, public std::enable_shared_from_this<Service> {

    public:
        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        void start(std::shared_ptr<Service> this_service, bool daemonize, bool auto_restart);
        virtual void stop();
        void suspend();
        void resume();

        std::string getHostname();
        std::string getPhysicalHostname();

        bool isUp();

        std::string getPropertyValueAsString(WRENCH_PROPERTY_TYPE);
        double getPropertyValueAsDouble(WRENCH_PROPERTY_TYPE);
        unsigned long getPropertyValueAsUnsignedLong(WRENCH_PROPERTY_TYPE);
        bool getPropertyValueAsBoolean(WRENCH_PROPERTY_TYPE);

        double getPropertyValueAsTimeInSecond(WRENCH_PROPERTY_TYPE);
        double getPropertyValueAsSizeInByte(WRENCH_PROPERTY_TYPE);
        double getPropertyValueAsBandwidthInBytePerSecond(WRENCH_PROPERTY_TYPE);

        const WRENCH_PROPERTY_COLLECTION_TYPE &getPropertyList() const;

        void assertServiceIsUp();

        double getNetworkTimeoutValue();
        void setNetworkTimeoutValue(double value);

        /***********************/
        /** \endcond           */
        /***********************/

        /***********************/
        /** \cond INTERNAL     */
        /***********************/

        double getMessagePayloadValue(WRENCH_MESSAGEPAYLOAD_TYPE);
        const WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE &getMessagePayloadList() const;

        void setStateToDown();

        /***********************/
        /** \endcond           */
        /***********************/

    protected:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/

        /**
         * @brief Assert for the service being up
         * @param s: a service
         */
        static void assertServiceIsUp(std::shared_ptr<Service> s) { s->assertServiceIsUp(); };

        friend class Simulation;

        ~Service() override;

        Service(std::string hostname, std::string process_name_prefix);

        // Property stuff
        void setProperty(WRENCH_PROPERTY_TYPE, const std::string &);

        void setProperties(WRENCH_PROPERTY_COLLECTION_TYPE default_property_values,
                           WRENCH_PROPERTY_COLLECTION_TYPE overriden_property_values);

        // MessagePayload stuff
        void setMessagePayload(WRENCH_MESSAGEPAYLOAD_TYPE, double);

        void setMessagePayloads(WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE default_messagepayload_values,
                                WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE overriden_messagepayload_values);


        void serviceSanityCheck();

        /** @brief The service's property list */
        WRENCH_PROPERTY_COLLECTION_TYPE property_list;

        /** @brief The service's messagepayload list */
        WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list;

        /** @brief The service's name */
        std::string name;

        /** @brief The time (in seconds) after which a service that doesn't send back a reply (control) message causes
         *  a NetworkTimeOut exception. (default: 30 second; if <0 never timeout)
         */
        double network_timeout = 30.0;

        /**
         * @brief Method to retrieve the shared_ptr to a service
         * @tparam T: the class of the service (the base class is Service)
         * @return a shared pointer
         */
        template<class T>
        std::shared_ptr<T> getSharedPtr() {
            return std::dynamic_pointer_cast<T>(this->shared_from_this());
        }


        /** @brief A boolean that indicates if the service is in the middle of shutting down **/
        bool shutting_down = false;

    private:

        double getPropertyValueWithUnitsAsValue(
                WRENCH_PROPERTY_TYPE property,
                const std::function<double(std::string &s)> &unit_parsing_function);

        /***********************/
        /** \endcond           */
        /***********************/
    };
};// namespace wrench


#endif//WRENCH_SERVICE_H
