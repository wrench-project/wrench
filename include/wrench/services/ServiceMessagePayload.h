/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_SERVICEMESSAGEPAYLOAD_H
#define WRENCH_SERVICEMESSAGEPAYLOAD_H

#include <string>
#include <map>

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/
    /**
     * @brief Message Payload Type
     */
    typedef int WRENCH_MESSAGEPAYLOAD_TYPE;
    /**
     * @brief Message Payload Count
     */
    extern WRENCH_MESSAGEPAYLOAD_TYPE WRENCH_MESSAGEPAYLOAD_COUNT;

    /***********************/
    /** \endcond           */
    /***********************/

}// namespace wrench

#define DECLARE_MESSAGEPAYLOAD_NAME(name) static const wrench::WRENCH_MESSAGEPAYLOAD_TYPE name

#define SET_MESSAGEPAYLOAD_NAME(classname, name) const wrench::WRENCH_MESSAGEPAYLOAD_TYPE classname::name = classname::addMessagePayload(#classname,#name)
//#name
//++WRENCH_MESSAGEPAYLOAD_COUNT
namespace wrench {

    /**
     * @brief Configurable message payloads for a Service
     */
    class ServiceMessagePayload {
         static std::map<std::string,WRENCH_MESSAGEPAYLOAD_TYPE> stringToPayloadMap;
         static std::map<WRENCH_MESSAGEPAYLOAD_TYPE,std::string> payloadToString;
    public:

        static WRENCH_MESSAGEPAYLOAD_TYPE addMessagePayload(std::string classname,std::string messagePayload);
        static WRENCH_MESSAGEPAYLOAD_TYPE translateString(std::string messagePayload);
        static std::string translatePayloadType(WRENCH_MESSAGEPAYLOAD_TYPE messagePayload);

        /** @brief The number of bytes in the control message sent to the daemon to terminate it **/
        DECLARE_MESSAGEPAYLOAD_NAME(STOP_DAEMON_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent by the daemon to confirm it has terminated **/
        DECLARE_MESSAGEPAYLOAD_NAME(DAEMON_STOPPED_MESSAGE_PAYLOAD);
    };

};// namespace wrench


#endif//WRENCH_SERVICEMESSAGEPAYLOAD_H
