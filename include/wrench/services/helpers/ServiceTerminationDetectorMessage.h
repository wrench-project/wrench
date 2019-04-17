/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_FAILUREDETECTORMESSAGE_H
#define WRENCH_FAILUREDETECTORMESSAGE_H


#include <wrench/simulation/SimulationMessage.h>
#include <wrench-dev.h>

namespace wrench {


    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief Top-level class for messages received/sent by a ServiceTerminationDetector
     */
    class ServiceTerminationDetectorMessage : public SimulationMessage {
    protected:
        explicit ServiceTerminationDetectorMessage(std::string name);
    };

    /**
     * @brief A message sent by the ServiceTerminationDetector to notify some listener that the 
     *        monitored service has crashed
     */
    class ServiceHasCrashedMessage : public ServiceTerminationDetectorMessage {
    public:
        explicit ServiceHasCrashedMessage(Service *service);

        /** @brief The service that has crashed */
        Service *service;
    };

    /**
     * @brief A message sent by the ServiceTerminationDetector to notify some listener that the 
     *        monitored service has terminated
     */
    class ServiceHasTerminatedMessage : public ServiceTerminationDetectorMessage {
    public:
        explicit ServiceHasTerminatedMessage(Service *service);

        /** @brief The service that has terminated */
        Service *service;
    };

    /***********************/
    /** \endcond           */
    /***********************/

};

#endif //WRENCH_FAILUREDETECTORMESSAGE_H
