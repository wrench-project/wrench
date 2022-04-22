/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/ServiceMessagePayload.h>
//#include <iostream>

namespace wrench {
    /**
     * @brief message payload count
     */
    WRENCH_MESSAGEPAYLOAD_TYPE WRENCH_MESSAGEPAYLOAD_COUNT = 0;
    std::map<std::string,WRENCH_MESSAGEPAYLOAD_TYPE> ServiceMessagePayload::stringToPayloadMap;
    SET_MESSAGEPAYLOAD_NAME(ServiceMessagePayload, STOP_DAEMON_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ServiceMessagePayload, DAEMON_STOPPED_MESSAGE_PAYLOAD);

    /**
     * @brief add new message to payload map.  DO NOT CALL THIS FUNCTION DIRECTLY, use SET_MESSAGEPAYLOAD_NAME and DECLARE_MESSAGEPAYLOAD_NAME
     * @param classname: The class to add the message too
     * @param messagePayload: the name of the message payload to add
     */
    WRENCH_MESSAGEPAYLOAD_TYPE ServiceMessagePayload::addMessagePayload(std::string classname,std::string messagePayload){
         ++WRENCH_MESSAGEPAYLOAD_COUNT;
         stringToPayloadMap[classname+"::"+messagePayload]=WRENCH_MESSAGEPAYLOAD_COUNT;
         //std::cout<<classname+"::"+messagePayload<<std::endl;
         return WRENCH_MESSAGEPAYLOAD_COUNT;
    }
    /**
     * @brief translate a string key to a message payload id
     * @param messagePayload: the name of the message payload to get in classname::messagePayload form (Note: the classname must be the parent class that defines the property)
     */
    WRENCH_MESSAGEPAYLOAD_TYPE ServiceMessagePayload::translateString(std::string messagePayload){
        return stringToPayloadMap[messagePayload];
    }

};// namespace wrench
