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
#include "wrench/services/compute/hadoop/MRJob.h"

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
        HadoopComputeServiceRunMRJobRequestMessage(std::string answer_mailbox, MRJob *job, double payload);

        /** @brief The mailbox to answer to */
        std::string answer_mailbox;
        /** @brief The MR job */
        MRJob *job;
    };

    /**
     * @brief A message sent by a HadoopComputeService after running a MR job
     */
    class HadoopComputeServiceRunMRJobAnswerMessage : public HadoopComputeServiceMessage {
    public:
        HadoopComputeServiceRunMRJobAnswerMessage(bool success, double payload);

        bool success;
    };

    class MRJobExecutorNotificationMessage : public HadoopComputeServiceMessage {
    public:
        MRJobExecutorNotificationMessage(bool success, MRJob *job, double payload);

        bool success;
        MRJob *job;
    };

/***********************/
/** \endcond          **/
/***********************/

}


#endif //WRENCH_HADOOPCOMPUTESERVICEMESSAGE_H
