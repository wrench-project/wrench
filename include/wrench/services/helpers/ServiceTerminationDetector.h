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

#include "wrench/services/Service.h"
#include "wrench/simgrid_S4U_util/S4U_Daemon.h"

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief A service that immediately detects when some service crashes and then notifies
     *        some other service of the crash
     */
    class ServiceTerminationDetector : public Service {

    public:

        explicit ServiceTerminationDetector(std::string host_on_which_to_run, std::shared_ptr<Service> service_to_monitor, std::string mailbox_to_notify, bool notify_on_crash, bool notify_on_termination);


    private:

        std::shared_ptr<Service> service_to_monitor;
        std::string mailbox_to_notify;
        int main() override;
        bool notify_on_crash;
        bool notify_on_termination;

    };

    /***********************/
    /** \endcond            */
    /***********************/


};




#endif //WRENCH_FAILUREDETECTOR_H
