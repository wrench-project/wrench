/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_HADOOPCOMPUTESERVICEMESSAGE_H
#define WRENCH_HADOOPCOMPUTESERVICEMESSAGE_H


#include "wrench/services/compute/ComputeServiceMessage.h"

namespace wrench {

/***********************/
/** \cond INTERNAL     */
/***********************/

    /**
     * @brief Top-level class for messages received/sent by a HadoopComputeService
     */
    class HadoopComputeServiceMessage : public ComputeServiceMessage {
    protected:
        HadoopComputeServiceMessage(std::string name, double payload);
    };

    /**
     * @brief A message sent to a HadoopComputeService to run a MR job
     */
    class HadoopComputeServiceRunMRJobRequestMessage : public HadoopComputeServiceMessage {
    public:
        HadoopComputeServiceRunMRJobRequestMessage(std::string answer_mailbox, double payload);

        /** @brief The mailbox to answer to */
        std::string answer_mailbox;
    };

    /**
     * @brief A message sent by a HadoopComputeService after running a MR job
     */
    class HadoopComputeServiceRunMRJobAnswerMessage : public HadoopComputeServiceMessage {
    public:
        HadoopComputeServiceRunMRJobAnswerMessage(bool success, double payload);

        bool success;
    };

/***********************/
/** \endcond          **/
/***********************/

}


#endif //WRENCH_HADOOPCOMPUTESERVICEMESSAGE_H
