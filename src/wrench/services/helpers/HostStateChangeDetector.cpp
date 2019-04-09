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
#include <wrench/services/helpers/HostStateChangeDetector.h>


XBT_LOG_NEW_DEFAULT_CATEGORY(host_state_change_detector, "Log category for HostStateChangeDetector");


void wrench::HostStateChangeDetector::cleanup(bool has_terminated_cleanly, int return_value) {
    // Unregister the callbacks! (otherwise we'd get a segfault after destruction of this service)
    simgrid::s4u::Host::on_state_change.disconnect_slots();
}



/**
 * @brief Constructor
 * @param host_on_which_to_run: hosts on which this service runs
 * @param hosts_to_monitor: the list of hosts to monitor
 * @param notify_when_turned_on: whether to send a notifications when hosts turn on
 * @param notify_when_turned_off: whether to send a notifications when hosts turn off
 * @param mailbox_to_notify: the mailbox to notify
 *
 */
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
    simgrid::s4u::Host::on_state_change.connect([this](simgrid::s4u::Host const &h) {this->hostChangeCallback(h);});
}

void wrench::HostStateChangeDetector::hostChangeCallback(simgrid::s4u::Host const &h) {
    // If this is not a host I care about, don't do anything
    if (std::find(this->hosts_to_monitor.begin(), this->hosts_to_monitor.end(), h.get_name()) == this->hosts_to_monitor.end()) {
        return;
    }
    this->hosts_that_have_recently_changed_state.push_back(std::make_pair(h.get_name(), h.is_on()));
//    if (h.is_on()) {
//        this->hosts_that_have_recently_turned_on.push_back(h.get_name());
//    } else {
//        this->hosts_that_have_recently_turned_off.push_back(h.get_name());
//    }

}


int wrench::HostStateChangeDetector::main() {

    WRENCH_INFO("Starting");
    while(true) {
        Simulation::sleep(1.0); // 1 second resolution

        while (not this->hosts_that_have_recently_changed_state.empty()) {
            auto host_info = this->hosts_that_have_recently_changed_state.at(0);
            std::string hostname = std::get<0>(host_info);
            bool new_state_is_on = std::get<1>(host_info);
            bool new_state_is_off  = not new_state_is_on;
            this->hosts_that_have_recently_changed_state.erase(this->hosts_that_have_recently_changed_state.begin());

            HostStateChangeDetectorMessage *msg;

            if (this->notify_when_turned_on && new_state_is_on) {
                msg = new HostHasTurnedOnMessage(hostname);
            } else if (this->notify_when_turned_off && new_state_is_off) {
                msg = new HostHasTurnedOffMessage(hostname);
            } else {
                continue;
            }

            WRENCH_INFO("Notifying mailbox '%s' that host '%s' has changed state", this->mailbox_to_notify.c_str(),
                        hostname.c_str());
            try {
                S4U_Mailbox::dputMessage(this->mailbox_to_notify, msg);
            } catch (std::shared_ptr<NetworkError> &e) {
                WRENCH_INFO("Network error '%s' while notifying mailbox of a host state change ... ignoring",
                            e->toString().c_str());
            }

        }

//        // Hosts that turned on
//        if (this->notify_when_turned_on) {
//            while (not this->hosts_that_have_recently_turned_on.empty()) {
//                std::string hostname = this->hosts_that_have_recently_turned_on.at(0);
//                this->hosts_that_have_recently_turned_on.erase(this->hosts_that_have_recently_turned_on.begin());
//                WRENCH_INFO("Notifying mailbox '%s' that host '%s' has turned on", this->mailbox_to_notify.c_str(),
//                            hostname.c_str());
//                auto msg = new HostHasTurnedOnMessage(hostname);
//                try {
//                    S4U_Mailbox::dputMessage(this->mailbox_to_notify, msg);
//                } catch (std::shared_ptr<NetworkError> &e) {
//                    WRENCH_INFO("Network error '%s' while notying mailbox of a host turning on... ignoring",
//                                e->toString().c_str());
//                }
//            }
//        }
//
//        // Hosts that turned off
//        if (this->notify_when_turned_off) {
//            while (not this->hosts_that_have_recently_turned_off.empty()) {
//                std::string hostname = this->hosts_that_have_recently_turned_off.at(0);
//                this->hosts_that_have_recently_turned_off.erase(this->hosts_that_have_recently_turned_off.begin());
//                WRENCH_INFO("Notifying mailbox '%s' that host '%s' has turned off", this->mailbox_to_notify.c_str(),
//                            hostname.c_str());
//                auto msg = new HostHasTurnedOffMessage(hostname);
//                try {
//                    S4U_Mailbox::dputMessage(this->mailbox_to_notify, msg);
//                } catch (std::shared_ptr<NetworkError> &e) {
//                    WRENCH_INFO("Network error '%s' while notying mailbox of a host turning off... ignoring",
//                                e->toString().c_str());
//                }
//            }
//        }
    }
    return 0;
}

/**
 * @brief Kill the service
 */
void wrench::HostStateChangeDetector::kill() {
    WRENCH_INFO("KILLING %s", this->getName().c_str());
    this->killActor();
}
