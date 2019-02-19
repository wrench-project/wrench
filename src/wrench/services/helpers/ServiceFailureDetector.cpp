/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/services/helpers/ServiceFailureDetector.h"
#include "wrench/services/helpers/ServiceFailureDetectorMessage.h"

#include <wrench/simulation/Simulation.h>
#include <wrench-dev.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(failure_detector, "Log category for ServiceFailureDetector");

wrench::ServiceFailureDetector::ServiceFailureDetector(std::string host_on_which_to_run,
                                         Service *service_to_monitor,
                                         std::string mailbox_to_notify) :
        Service(host_on_which_to_run, "failure_detector_" + service_to_monitor->getName(), "failure_detector" + service_to_monitor->getName()){
    this->service_to_monitor = service_to_monitor;
    this->mailbox_to_notify = mailbox_to_notify;

}

int wrench::ServiceFailureDetector::main() {

    WRENCH_INFO("Starting");
    bool terminated_cleanly = this->service_to_monitor->join();
    if (not terminated_cleanly) {
        // Failure detected!
        WRENCH_INFO("Detected failure of service %s", this->service_to_monitor->getName().c_str());
        S4U_Mailbox::dputMessage(this->mailbox_to_notify, new ServiceHasFailedMessage(this->service_to_monitor));
    }
    return 0;
}
