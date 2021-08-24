/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_HTCONDORCENTRALMANAGERSERVICEMESSAGEPAYLOAD_H
#define WRENCH_HTCONDORCENTRALMANAGERSERVICEMESSAGEPAYLOAD_H

#include "wrench/services/compute/ComputeServiceMessagePayload.h"

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief Configurable message payloads for an HTCondor Central Manager service
     */
    class HTCondorCentralManagerServiceMessagePayload : public ComputeServiceMessagePayload {
    public:
        /** @brief The number of bytes in the control message sent by the daemon to state that the negotiator has been completed **/
        DECLARE_MESSAGEPAYLOAD_NAME(HTCONDOR_NEGOTIATOR_DONE_MESSAGE_PAYLOAD);
    };

    /***********************/
    /** \endcond INTERNAL  */
    /***********************/
}

#endif //WRENCH_HTCONDORCENTRALMANAGERSERVICEMESSAGEPAYLOAD_H
