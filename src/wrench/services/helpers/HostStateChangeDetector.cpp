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
    simgrid::s4u::VirtualMachine::on_shutdown.disconnect_slots();
    simgrid::s4u::VirtualMachine::on_start.disconnect_slots();
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

    // Connect my member method to the on_state_change signal from SimGrid regarding Hosts
    simgrid::s4u::Host::on_state_change.connect([this](simgrid::s4u::Host const &h) {this->hostChangeCallback(h.get_name(), h.is_on(), "HOST STATE CHANGE");});
    // Connect my member method to the on_state_change signal from SimGrid regarding VMs
    simgrid::s4u::VirtualMachine::on_shutdown.connect([this](simgrid::s4u::VirtualMachine const &h) -> void {this->hostChangeCallback(h.get_name(),false, "VM SHUTDOWN");});
    simgrid::s4u::VirtualMachine::on_start.connect([this](simgrid::s4u::VirtualMachine const &h) {this->hostChangeCallback(h.get_name(), true, "VM START");});
}

void wrench::HostStateChangeDetector::hostChangeCallback(std::string const &name, bool is_on, std::string message) {
//    WRENCH_INFO("************************************");
//    WRENCH_INFO("***** %s : IN CALLBACK: %s : %s ", this->getName().c_str(),message.c_str(), name.c_str());
//    WRENCH_INFO("************************************");

    // If this is not a host I care about, don't do anything
    if (std::find(this->hosts_to_monitor.begin(), this->hosts_to_monitor.end(), name) == this->hosts_to_monitor.end()) {
        return;
    }
    // This seems to be a SimGrid bug: we get the shutdown, but somehow is_on() returns true!
    // So for now we manually turn off the VM, which will then cause another callback
    if (message == "VM SHUTDOWN") {
        simgrid::s4u::VirtualMachine::by_name(name)->turn_off();
        return;
    }
    this->hosts_that_have_recently_changed_state.push_back(std::make_pair(name, is_on));

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
