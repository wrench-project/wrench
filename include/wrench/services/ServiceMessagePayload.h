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

#define DECLARE_MESSAGEPAYLOAD_NAME(name) static const std::string name


#define SET_MESSAGEPAYLOAD_NAME(classname, name) const std::string classname::name=#name

namespace wrench {


    /**
     * @brief Configurable message payloads for a Service
     */
    class ServiceMessagePayload {

    public:
        /** @brief The number of bytes in the control message sent to the daemon to terminate it **/
        DECLARE_MESSAGEPAYLOAD_NAME(STOP_DAEMON_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent by the daemon to confirm it has terminated **/
        DECLARE_MESSAGEPAYLOAD_NAME(DAEMON_STOPPED_MESSAGE_PAYLOAD);

    };

};


#endif //WRENCH_SERVICEMESSAGEPAYLOAD_H
