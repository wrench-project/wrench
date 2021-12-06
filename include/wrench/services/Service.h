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

#include "wrench/simgrid_S4U_util/S4U_Daemon.h"

namespace wrench {

    class FailureCause;

    /**
     * @brief A service that can be added to the simulation and that can be used by a WMS
     *        when executing a workflow
     */
    class Service : public S4U_Daemon {

    public:



        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        void start(std::shared_ptr<Service> this_service, bool daemonize, bool auto_restart);
        virtual void stop();
        void suspend();
        void resume();

        std::string getHostname();

        bool isUp();

        std::string getPropertyValueAsString(std::string);
        double getPropertyValueAsDouble(std::string);
        unsigned long getPropertyValueAsUnsignedLong(std::string);
        bool getPropertyValueAsBoolean(std::string);

        void assertServiceIsUp();

        double getNetworkTimeoutValue();
        void setNetworkTimeoutValue(double value);

        /***********************/
        /** \endcond           */
        /***********************/

        /***********************/
        /** \cond INTERNAL     */
        /***********************/

        double getMessagePayloadValue(std::string);

        void setStateToDown();

        static void increaseNumCompletedServicesCount();
        static void clearTrackedServices();
        static void cleanupTrackedServices();

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

        ~Service();

        Service(std::string hostname, std::string process_name_prefix, std::string mailbox_name_prefix);

        // Property stuff
        void setProperty(std::string, std::string);

        void setProperties(std::map<std::string, std::string> default_property_values,
                           std::map<std::string, std::string> overriden_property_values);

        // MessagePayload stuff
        void setMessagePayload(std::string, double);

        void setMessagePayloads(std::map<std::string, double> default_messagepayload_values,
                                std::map<std::string, double> overriden_messagepayload_values);


        void serviceSanityCheck();

        /** @brief The service's property list */
        std::map<std::string, std::string> property_list;

        /** @brief The service's messagepayload list */
        std::map<std::string, double> messagepayload_list;

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
        template <class T>
        std::shared_ptr<T> getSharedPtr() {
            if (Service::service_shared_ptr_map.find(this) == Service::service_shared_ptr_map.end()) {
                throw std::runtime_error("Service::getSharedPtr(): master shared_ptr to service not found! This should happen only "
                                         "if the service has not been started, in which case this method shouldn't have been called");
            }
            auto shared_ptr = std::dynamic_pointer_cast<T>(Service::service_shared_ptr_map[this]);
            if (not shared_ptr) {
                throw std::runtime_error("Service::getSharedPtr(): Invalid provided template");
            }
            return shared_ptr;
        }

        /**
         * @brief Method to retrieve the shared_ptr to a service based on the service's name (not efficient)
         * @tparam T: the class of the service (the base class is Service)
         * @param name: the service's name
         * @return a shared_ptr (or nullptr if not found)
         */
        template <class T>
        static std::shared_ptr<T> getServiceByName(std::string name) {
            for (auto const &s : Service::service_shared_ptr_map) {
                if (s.first->getName() == name) {
                    return std::dynamic_pointer_cast<T>(s.second);
                }
            }
            return nullptr;
        }


        /** @brief A boolean that indicates if the service is in the middle of shutting down **/
        bool shutting_down = false;
        /** @brief A useful map of services to their shared_ptr **/
        static std::unordered_map<Service *, std::shared_ptr<Service>> service_shared_ptr_map;

    private:


        static unsigned long num_terminated_services;


        /***********************/
        /** \endcond           */
        /***********************/

    };
};


#endif //WRENCH_SERVICE_H
