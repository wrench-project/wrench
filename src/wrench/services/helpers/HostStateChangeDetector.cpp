/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundatioff, either versioff 3 of the License, or
 * (at your optioff) any later versioff.
 */

#include "wrench/services/helpers/HostStateChangeDetector.h"
#include "wrench/services/helpers/HostStateChangeDetectorMessage.h"

#include <wrench/simulation/Simulation.h>
#include <wrench-dev.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(host_state_change_detector, "Log category for HostStateChangeDetector");

wrench::HostStateChangeDetector::HostStateChangeDetector(std::string host_on_which_to_run,
                                                         std::vector<std::string> hosts_to_monitor,
                                                         bool notify_when_turned_on, bool notify_when_turned_off,
                                                         std::string mailbox_to_notify) :
        Service(host_on_which_to_run, "host_state_change_detector", "host_state_change_detector") {
    this->hosts_to_monitor = hosts_to_monitor;
    this->notify_when_turned_on = notify_when_turned_on;
    this->notify_when_turned_off = notify_when_turned_off;
    this->mailbox_to_notify = mailbox_to_notify;

    // Connect my member method to the on_state_change signal from SimGrid
    std::function<void(simgrid::s4u::Host &h)> host_back_on_function = std::bind(&HostStateChangeDetector::hostChangeCallback, this, std::placeholders::_1);
    simgrid::s4u::Host::on_state_change.connect(host_back_on_function);
}

void wrench::HostStateChangeDetector::hostChangeCallback(simgrid::s4u::Host &h) {
    // If this is not a host I care about, whatever
    if (std::find(this->hosts_to_monitor.begin(), this->hosts_to_monitor.end(), h.get_name()) == this->hosts_to_monitor.end()) {
        return;
    }
    if (h.is_on()) {
        this->hosts_that_have_recently_turned_on.push_back(h.get_name());
    } else {
        this->hosts_that_have_recently_turned_off.push_back(h.get_name());
    }

}


int wrench::HostStateChangeDetector::main() {

    WRENCH_INFO("Starting");
    while(true) {
        Simulation::sleep(1.0);
        if (this->notify_when_turned_on) {
            while (not this->hosts_that_have_recently_turned_on.empty()) {
                std::string hostname = this->hosts_that_have_recently_turned_on.at(0);
                this->hosts_that_have_recently_turned_on.erase(this->hosts_that_have_recently_turned_on.begin());
                WRENCH_INFO("Notifying mailbox '%s' that host '%s' has turned on", this->mailbox_to_notify.c_str(),
                            hostname.c_str());
                auto msg = new HostHasTurnedOnMessage(hostname);
                try {
                    S4U_Mailbox::dputMessage(this->mailbox_to_notify, msg);
                } catch (std::shared_ptr<NetworkError> &e) {
                    WRENCH_INFO("Network error '%s' while notying mailbox of a host turning on... ignoring",
                                e->toString().c_str());
                }
            }
        }

        if (this->notify_when_turned_off) {
            while (not this->hosts_that_have_recently_turned_off.empty()) {
                std::string hostname = this->hosts_that_have_recently_turned_off.at(0);
                this->hosts_that_have_recently_turned_off.erase(this->hosts_that_have_recently_turned_off.begin());
                WRENCH_INFO("Notifying mailbox '%s' that host '%s' has turned off", this->mailbox_to_notify.c_str(),
                            hostname.c_str());
                auto msg = new HostHasTurnedOffMessage(hostname);
                try {
                    S4U_Mailbox::dputMessage(this->mailbox_to_notify, msg);
                } catch (std::shared_ptr<NetworkError> &e) {
                    WRENCH_INFO("Network error '%s' while notying mailbox of a host turning off... ignoring",
                                e->toString().c_str());
                }
            }
        }
    }
    return 0;
}
