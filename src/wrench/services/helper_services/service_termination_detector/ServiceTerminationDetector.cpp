/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/helper_services/service_termination_detector/ServiceTerminationDetector.h>
#include <wrench/services/helper_services/service_termination_detector/ServiceTerminationDetectorMessage.h>

#include <wrench/simulation/Simulation.h>
#include <wrench-dev.h>

#include <utility>

WRENCH_LOG_CATEGORY(wrench_core_failure_detector, "Log category for ServiceTerminationDetector");

/**
 * @brief Constructor
 * @param host_on_which_to_run: the service's host
 * @param service_to_monitor: which service to monitor
 * @param mailbox_to_notify: which mailbox to notify
 * @param notify_on_crash: whether to send a crash notification (in case of non-clean termination)
 * @param notify_on_termination: whether to send a termination notification (in case of clean termination)
 */
wrench::ServiceTerminationDetector::ServiceTerminationDetector(std::string host_on_which_to_run,
                                                               std::shared_ptr<Service> service_to_monitor,
                                                               simgrid::s4u::Mailbox *mailbox_to_notify,
                                                               bool notify_on_crash,
                                                               bool notify_on_termination) : Service(std::move(host_on_which_to_run), "service_termination_detector_for_" + service_to_monitor->getName()) {

    this->service_to_monitor = std::move(service_to_monitor);
    this->mailbox_to_notify = mailbox_to_notify;
    this->notify_on_crash = notify_on_crash;
    this->notify_on_termination = notify_on_termination;
}

/**
 * @brief main method
 */
int wrench::ServiceTerminationDetector::main() {

    WRENCH_INFO("Starting");
    std::pair<bool, int> return_values_from_join;
    return_values_from_join = this->service_to_monitor->join();
    bool service_has_returned_from_main = std::get<0>(return_values_from_join);
    int return_value_from_main = std::get<1>(return_values_from_join);

    if (this->notify_on_crash and (not service_has_returned_from_main)) {
        // Failure detected!
        WRENCH_INFO("Detected crash of service %s (notifying mailbox %s)", this->service_to_monitor->getName().c_str(),
                    this->mailbox_to_notify->get_cname());
        S4U_Mailbox::putMessage(this->mailbox_to_notify, new ServiceHasCrashedMessage(this->service_to_monitor));
    }
    if (this->notify_on_termination and (service_has_returned_from_main)) {
        // Failure detected!
        WRENCH_INFO("Detected termination of service %s (notifying mailbox %s)",
                    this->service_to_monitor->getName().c_str(), this->mailbox_to_notify->get_cname());
        S4U_Mailbox::putMessage(this->mailbox_to_notify,
                                new ServiceHasTerminatedMessage(this->service_to_monitor, return_value_from_main));
    }

    this->service_to_monitor = nullptr;// released, so that it can be freed in case refount = 0
    return 0;
}
