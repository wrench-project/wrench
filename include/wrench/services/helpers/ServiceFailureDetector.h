/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_FAILUREDETECTOR_H
#define WRENCH_FAILUREDETECTOR_H

#include <wrench/services/Service.h>
#include "wrench/simgrid_S4U_util/S4U_Daemon.h"

namespace wrench {

    class ServiceFailureDetector : public Service {

    public:

        explicit ServiceFailureDetector(std::string host_on_which_to_run, Service *service_to_monitor, std::string mailbox_to_notify);

        /***********************/
        /** \endcond           */
        /***********************/

    private:

        Service *service_to_monitor;
        std::string mailbox_to_notify;
        int main() override;

    };

    /***********************/
    /** \endcond            */
    /***********************/


};




#endif //WRENCH_FAILUREDETECTOR_H
