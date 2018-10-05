/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_HTCONDORCENTRALMANAGERSERVICEMESSAGE_H
#define WRENCH_HTCONDORCENTRALMANAGERSERVICEMESSAGE_H

#include "wrench/services/ServiceMessage.h"
#include "wrench/workflow/job/StandardJob.h"

namespace wrench {

    class HTCondorCentralManagerServiceMessage : public ServiceMessage {
    protected:
        HTCondorCentralManagerServiceMessage(std::string name, double payload);
    };

    class NegotiatorCompletionMessage : public HTCondorCentralManagerServiceMessage {
    public:
        NegotiatorCompletionMessage(std::vector<StandardJob *> scheduled_jobs, double payload);

        std::vector<StandardJob *> scheduled_jobs;
    };
}

#endif //WRENCH_HTCONDORCENTRALMANAGERSERVICEMESSAGE_H
