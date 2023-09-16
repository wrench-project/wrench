/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/ServiceMessagePayload.h>
#include <iostream>
#include <unordered_map>

namespace wrench {
    /**
     * @brief message payload count
     */
    WRENCH_MESSAGEPAYLOAD_TYPE WRENCH_MESSAGEPAYLOAD_COUNT = 0;
    std::map<std::string, WRENCH_MESSAGEPAYLOAD_TYPE> ServiceMessagePayload::stringToPayloadMap = {};
    std::map<WRENCH_MESSAGEPAYLOAD_TYPE, std::string> ServiceMessagePayload::payloadToString = {};

    SET_MESSAGEPAYLOAD_NAME(ServiceMessagePayload, STOP_DAEMON_MESSAGE_PAYLOAD);
    SET_MESSAGEPAYLOAD_NAME(ServiceMessagePayload, DAEMON_STOPPED_MESSAGE_PAYLOAD);

    /**
     * @brief add new message to payload map.  DO NOT CALL THIS FUNCTION DIRECTLY, use SET_MESSAGEPAYLOAD_NAME and DECLARE_MESSAGEPAYLOAD_NAME
     * @param classname: The class to add the message too
     * @param message_payload: the name of the message payload to add
     * @return a wrench message payload type
     */
    WRENCH_MESSAGEPAYLOAD_TYPE ServiceMessagePayload::addMessagePayload(std::string classname, std::string message_payload) {
        ++WRENCH_MESSAGEPAYLOAD_COUNT;
        stringToPayloadMap[classname + "::" + message_payload] = WRENCH_MESSAGEPAYLOAD_COUNT;
        payloadToString[WRENCH_MESSAGEPAYLOAD_COUNT] = classname + "::" + message_payload;
        //        std::cout<<"["<<WRENCH_MESSAGEPAYLOAD_COUNT<<"] " << classname+"::"+messagePayload<<std::endl;
        return WRENCH_MESSAGEPAYLOAD_COUNT;
    }

    /**
     * @brief translate a string key to a message payload id
     * @param message_payload: the name of the message payload to get in classname::messagePayload form (Note: the classname must be the parent class that defines the property)
     * @return a wrench message payload type
     */
    WRENCH_MESSAGEPAYLOAD_TYPE ServiceMessagePayload::translateString(std::string message_payload) {
        WRENCH_MESSAGEPAYLOAD_TYPE to_return;
        try {
            to_return = stringToPayloadMap.at(message_payload);
        } catch (std::out_of_range &e) {
            throw std::runtime_error("Unknown message payload specification " + message_payload +
                                     ". Perhaps you need to use the superclass name?");
        }
        return to_return;
    }

    /**
     * @brief translate a message payload ID to a string key
     * @param message_payload: the ID of the message payload
     * @return a wrench message payload type, as a string
     */
    std::string ServiceMessagePayload::translatePayloadType(WRENCH_MESSAGEPAYLOAD_TYPE message_payload) {
        return payloadToString.at(message_payload);
    }

}// namespace wrench
