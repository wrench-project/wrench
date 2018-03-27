/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/services/ServiceProperty.h"


namespace wrench {

    /** @brief The number of bytes in the control message sent to the daemon to terminate it **/
    SET_PROPERTY_NAME(ServiceProperty, STOP_DAEMON_MESSAGE_PAYLOAD);

    /** @brief The number of bytes in the control message sent by the daemon to confirm it has terminate **/
    SET_PROPERTY_NAME(ServiceProperty, DAEMON_STOPPED_MESSAGE_PAYLOAD);

};


